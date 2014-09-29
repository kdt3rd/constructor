//
// Copyright (c) 2014 Kimball Thurston
//
// Permission is hereby granted, free of charge, to any person obtaining
// a copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included
// in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
// IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
// CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
// TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE
// OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
//

#include "LuaExtensions.h"
#include "LuaEngine.h"
#include "LuaValue.h"
#include "ScopeGuard.h"

#include "FileUtil.h"
#include "OSUtil.h"
#include "StrUtil.h"

#include "Directory.h"
#include "Scope.h"
#include "Compile.h"

#include <map>
#include <mutex>
#include <iostream>


////////////////////////////////////////


namespace 
{

inline constexpr const char *buildFileName( void )
{
	return "construct";
}


////////////////////////////////////////


static int
luaFindExecutable( lua_State *L )
{
	if ( lua_gettop( L ) != 1 )
		throw std::runtime_error( "Expecting executable name to find_exe" );
	if ( ! lua_isstring( L, 1 ) )
		throw std::runtime_error( "Expecting string argument to find_exe" );
	size_t len = 0;
	const char *ename = lua_tolstring( L, 1, &len );
	if ( ename )
	{
		std::string name( ename, len );
		std::string ret;
		if ( File::findExecutable( ret, name ) )
			lua_pushlstring( L, ret.c_str(), ret.size() );
		else
			lua_pushnil( L );
	}
	else
		lua_pushnil( L );
	return 1;
}

static const std::string &
pathVarLookup( const std::string &name )
{
	const VariableSet &vars = Scope::current().getVars();
	auto i = vars.find( name );
	if ( i != vars.end() )
		return i->second.value();

	if ( name == "PATH" )
	{
		static std::string curPath;
		const std::vector<std::string> &path = File::getPath();
		curPath.clear();
		for ( const std::string &p: path )
		{
			if ( ! curPath.empty() )
				curPath.push_back( ':' );
			curPath.append( p );
		}
		return curPath;
	}
	return OS::getenv( name );
}

static void
luaSetPath( std::string p )
{
	String::substituteVariables( p, false, &pathVarLookup );
	
	// we will always use unix-style path separators for
	// simplicity
	File::setPathOverride( String::split( p, ':' ) );
}

static int
luaFindFile( lua_State *L )
{
	int N = lua_gettop( L );
	if ( N < 1 || N > 2 )
		throw std::runtime_error( "Expecting 1 or 2 arguments - name(s) [and path(s)] - to file.find" );
	std::vector<std::string> names;
	if ( lua_isstring( L, 1 ) )
		names.emplace_back( Lua::Parm<std::string>::get( L, N, 1 ) );
	else
		names = std::move( Lua::Parm< std::vector<std::string> >::get( L, N, 1 ) );

	std::string filepath;
	bool ret = false;

	if ( N == 2 )
	{
		std::vector<std::string> paths;
		if ( lua_isstring( L, 2 ) )
			paths.emplace_back( Lua::Parm<std::string>::get( L, 2, 2 ) );
		else
			paths = std::move( Lua::Parm< std::vector<std::string> >::get( L, 2, 2 ) );

		ret = File::find( filepath, names, paths );
	}
	else
		ret = File::find( filepath, names );

	if ( ret )
		lua_pushlstring( L, filepath.c_str(), filepath.size() );
	else
		lua_pushnil( L );

	return 1;
}

static int
luaJoinPath( lua_State *L )
{
	int N = lua_gettop( L );
	if ( N < 2 )
		throw std::runtime_error( "Expecting at least 2 arguments to file.path.join" );

	Directory d( Lua::Parm<std::string>::get( L, N, 1 ) );
	for ( int i = 2; i <= N; ++i )
		d.cd( Lua::Parm<std::string>::get( L, N, i ) );

	std::string ret = d.fullpath();
	lua_pushlstring( L, ret.c_str(), ret.size() );
	return 1;
}

static std::vector<std::string> theSubDirFiles;

static int
SubDir( lua_State *L )
{
	int N = lua_gettop( L );
	if ( N != 1 || ! lua_isstring( L, 1 ) )
	{
		luaL_error( L, "SubDir expects at directory name string as an argument" );
		return 1;
	}
	int ret = 0;

	std::string file = lua_tolstring( L, 1, NULL );
	std::shared_ptr<Directory> curDir = Directory::current();
	std::string tmp;
	if ( curDir->exists( tmp, file ) )
	{
		curDir = Directory::pushd( file );
		ON_EXIT{ Directory::popd(); };

		std::string nextFile;
		if ( curDir->exists( nextFile, buildFileName() ) )
		{
			ret = Lua::Engine::singleton().runFile( nextFile.c_str() );
			theSubDirFiles.push_back( nextFile );
		}
		else
			throw std::runtime_error( "Unable to find a '" + std::string( buildFileName() ) + "' in " + curDir->fullpath() );
	}
	else
		throw std::runtime_error( "Sub Directory '" + file + "' does not exist in " + curDir->fullpath() );

	return ret;
}

std::string
SourceDir( void )
{
	return Directory::current()->fullpath();
}

std::string
RelSourceDir( void )
{
	return Directory::current()->relpath();
}

std::string
SourceFile( const std::string &fn )
{
	return Directory::current()->makefilename( fn );
}



////////////////////////////////////////


void
luaAddTool( const Lua::Value &v )
{
	Scope::current().addTool( Tool::parse( v ) );
}

void
luaAddToolSet( const Lua::Value &v )
{
	std::string name;
	std::vector<std::string> tools;
	const Lua::Table &t = v.asTable();
	for ( auto &i: t )
	{
		if ( i.first.type == Lua::KeyType::INDEX )
			continue;

		const std::string &k = i.first.tag;
		if ( k == "name" )
			name = i.second.asString();
		else if ( k == "tools" )
			tools = i.second.toStringList();
		else
			throw std::runtime_error( "Unknown tag '" + k + "' in AddToolSet" );
	}
	if ( name.empty() )
		throw std::runtime_error( "Missing name in AddToolSet" );
	if ( tools.empty() )
		throw std::runtime_error( "Need list of tools in AddToolSet" );

	Scope::current().addToolSet( name, tools );
}

void
luaAddToolOption( const std::string &t, const std::string &g,
				  const std::string &name,
				  const std::vector<std::string> &cmd )
{
	bool found = false;
	for ( auto &tool: Scope::current().getTools() )
	{
		if ( tool->getName() == t )
		{
			found = true;
			tool->addOption( g, name, cmd );
		}
	}

	if ( ! found )
		throw std::runtime_error( "Unable to find tool '" + t + "' in current scope" );
}

int
luaEnableLangs( lua_State *L )
{
	int N = lua_gettop( L );
	for ( int i = 1; i <= N; ++i )
	{
		if ( lua_isstring( L, i ) )
		{
			size_t len = 0;
			const char *s = lua_tolstring( L, i, &len );
			std::string curLang( s, len );
			for ( auto &t: Scope::current().getTools() )
				t->enableLanguage( curLang );
		}
		else
		{
			std::cout << "WARNING: ignoring non-string argument to EnableLanguages" << std::endl;
		}
	}
	return 0;
}

int
luaSetOption( lua_State *L )
{
	int N = lua_gettop( L );
	if ( N != 2 )
	{
		luaL_error( L, "SetOption expects 2 arguments - a name and a value" );
		return 1;
		
	}

	std::string nm = Lua::Parm<std::string>::get( L, N, 1 );
	std::string val = Lua::Parm<std::string>::get( L, N, 2 );
	auto &vars = Scope::current().getVars();
	auto ne = vars.emplace( std::make_pair( nm, Variable( nm ) ) );
	ne.first->second.reset( std::move( val ) );

	return 0;
}

int
luaSubProj( lua_State *L )
{
	Scope::pushScope( Scope::current().newSubScope( false ) );
	ON_EXIT{ Scope::popScope(); };
	int ret = SubDir( L );
//	Scope::current().setNameDir( Directory::last() );
	return ret;
}

static void
recurseAndAdd( std::shared_ptr<CompileSet> &ret,
			   lua_State *L, int i )
{
	if ( lua_isnil( L, i ) )
		return;
	if ( lua_isstring( L, i ) )
	{
		size_t len = 0;
		const char *n = lua_tolstring( L, i, &len );
		if ( n )
			ret->addItem( std::string( n, len ) );
		else
			throw std::runtime_error( "String argument, but unable to extract string" );
	}
	else if ( lua_isuserdata( L, i ) )
		ret->addItem( Item::extract( L, i ) );
	else if ( lua_istable( L, i ) )
	{
		lua_pushnil( L );
		while ( lua_next( L, i ) )
		{
			recurseAndAdd( ret, L, lua_gettop( L ) );
			lua_pop( L, 1 );
		}
	}
	else
		throw std::runtime_error( "Unhandled argument type to Compile" );
}

int
compile( lua_State *L )
{
	int N = lua_gettop( L );
	if ( N == 0 )
	{
		lua_pushnil( L );
		return 1;
	}

	std::shared_ptr<CompileSet> ret = std::make_shared<CompileSet>();
	for ( int i = 1; i <= N; ++i )
		recurseAndAdd( ret, L, i );

	Scope::current().addItem( ret );
	Item::push( L, ret );
	return 1;
}

int
executable( lua_State *L )
{
	int N = lua_gettop( L );
	if ( N < 2 )
		throw std::runtime_error( "Executable expects a name as the first argument and at least 1 link object" );
	std::string ename = Lua::Parm<std::string>::get( L, N, 1 );

	std::shared_ptr<CompileSet> ret = std::make_shared<Executable>( std::move( ename ) );
	for ( int i = 2; i <= N; ++i )
		recurseAndAdd( ret, L, i );

	Scope::current().addItem( ret );
	Item::push( L, ret );
	return 1;
}

int
library( lua_State *L )
{
	int N = lua_gettop( L );
	if ( N < 2 )
		throw std::runtime_error( "Library expects a name as the first argument and at least 1 link object" );
	std::string ename = Lua::Parm<std::string>::get( L, N, 1 );

	std::shared_ptr<CompileSet> ret = std::make_shared<Library>( std::move( ename ) );
	for ( int i = 2; i <= N; ++i )
		recurseAndAdd( ret, L, i );

	Scope::current().addItem( ret );
	Item::push( L, ret );
	return 1;
}

} // empty namespace


////////////////////////////////////////


namespace Lua
{


////////////////////////////////////////


void
registerExtensions( void )
{
	Engine &eng = Engine::singleton();

	eng.registerFunction( "SubDir", &SubDir );
	eng.registerFunction( "SourceDir", &SourceDir );
	eng.registerFunction( "RelativeSourceDir", &RelSourceDir );
	eng.registerFunction( "SourceFile", &SourceFile );
	eng.registerFunction( "SubProject", &luaSubProj );

	eng.registerFunction( "AddTool", &luaAddTool );
	eng.registerFunction( "AddToolOption", &luaAddToolOption );
	eng.registerFunction( "AddToolSet", &luaAddToolSet );
	eng.registerFunction( "EnableLanguages", &luaEnableLangs );
	eng.registerFunction( "SetOption", &luaSetOption );

	eng.registerFunction( "Compile", &compile );
	eng.registerFunction( "Executable", &executable );
	eng.registerFunction( "Library", &library );

	{
		eng.pushLibrary( "file" );
		ON_EXIT{ eng.popLibrary(); };

		eng.registerFunction( "compare", &File::compare );
		eng.registerFunction( "diff", &File::diff );
		eng.registerFunction( "exists", &File::exists );
		eng.registerFunction( "find", &luaFindFile );
		eng.registerFunction( "set_exe_path", &luaSetPath );
		eng.registerFunction( "find_exe", &luaFindExecutable );

		eng.pushSubLibrary( "path" );
		ON_EXIT{ eng.popLibrary(); };
		eng.registerFunction( "join", &luaJoinPath );
		std::string path_sep;
		path_sep.push_back( File::pathSeparator() );
		lua_pushliteral( eng.state(), "sep" );
		lua_pushlstring( eng.state(), path_sep.c_str(), path_sep.size() );
		lua_rawset( eng.state(), -3 );
	}
	{
		eng.pushLibrary( "sys" );
		ON_EXIT{ eng.popLibrary(); };

		eng.registerFunction( "is64bit", &OS::is64bit );
		eng.registerFunction( "machine", &OS::machine );
		eng.registerFunction( "release", &OS::release );
		eng.registerFunction( "version", &OS::version );
		eng.registerFunction( "system", &OS::system );
		eng.registerFunction( "node", &OS::node );
	}
	
}


////////////////////////////////////////


void
startParsing( const std::string &dir )
{
	std::shared_ptr<Directory> curDir = Directory::current();
	bool needPop = false;
	if ( ! dir.empty() )
	{
		needPop = true;
		curDir = Directory::pushd( dir );
	}
	ON_EXIT{ if ( needPop ) Directory::popd(); };

	std::string firstFile;
	if ( curDir->exists( firstFile, buildFileName() ) )
	{
		theSubDirFiles.push_back( firstFile );
		Lua::Engine::singleton().runFile( firstFile.c_str() );
	}
	else
	{
		std::stringstream msg;
		msg << "Unable to find " << buildFileName() << " in " << curDir->fullpath();
		throw std::runtime_error( msg.str() );
	}
}


////////////////////////////////////////


const std::vector<std::string> &
visitedFiles( void )
{
	return theSubDirFiles;
}


////////////////////////////////////////


} // Lua




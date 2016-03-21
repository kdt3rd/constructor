//
// Copyright (c) 2016 Kimball Thurston
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

#include "LuaFileExt.h"
#include "LuaEngine.h"
#include "LuaValue.h"
#include "ScopeGuard.h"

#include "Directory.h"
#include "FileUtil.h"
#include "StrUtil.h"
#include "Debug.h"
#include "Scope.h"
#include "Compile.h"
#include "OSUtil.h"
#include "LuaItemExt.h"

#include <vector>


////////////////////////////////////////


namespace
{


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
		DEBUG( "luaFindExecutable " << name );
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
		// hrm, no transform so just use the system
		return i->second.value( OS::system() );

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
	
	DEBUG( "luaSetPath " << p );
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
		names = Lua::Parm< std::vector<std::string> >::get( L, N, 1 );

	std::string filepath;
	bool ret = false;

	DEBUG( "luaFindFile " << names[0] );
	if ( N == 2 )
	{
		std::vector<std::string> paths;
		if ( lua_isstring( L, 2 ) )
			paths.emplace_back( Lua::Parm<std::string>::get( L, 2, 2 ) );
		else
			paths = Lua::Parm< std::vector<std::string> >::get( L, 2, 2 );

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


////////////////////////////////////////


// searches the current directory, matching a pattern. This can be
// a pattern with sub-directories in it, always in unix notation,
// and shell globbing rules more than just a standard posix regex,
// so a pattern of {foo,bar}/*.h would search for *.h in sub-dirs
// foo and bar.
static void globDir( std::shared_ptr<CompileSet> &ret,
					 const std::string &pattern )
{
	auto curD = Directory::current();

	// trigger constructor to re-run if one of
	// the searched directories changes
	const std::string &fp = curD->fullpath();

	// NB: NOT using the pathSeparator here
	/// @todo { Document the file glob and separator for subdirs }
	std::string::size_type pSep = pattern.find_first_of( '/' );

	if ( pSep != std::string::npos )
	{
		std::string curMatch = pattern.substr( 0, pSep );
		std::string subMatch = pattern.substr( pSep + 1 );

		std::string pat = File::globToRegex( curMatch );
		if ( pat != curMatch )
			Lua::Engine::singleton().addVisitedFile( fp );

		std::vector<std::string> subs = File::globRegex( fp, pat );
		for ( const std::string &s: subs )
		{
			Directory::pushd( s );
			ON_EXIT{ Directory::popd(); };
			globDir( ret, subMatch );
		}
	}
	else
	{
		Lua::Engine::singleton().addVisitedFile( fp );

		// can't use the utility string-based addItem since
		// we might be recursed to a sub-dir of the directory the
		// compile set is set to in which case the utility addItem
		// fails because it doesn't exist
		for ( std::string &f: File::glob( fp, pattern ) )
			ret->addItem( std::make_shared<Item>( std::move( f ) ) );
	}
}

static int
luaGlobFiles( lua_State *L )
{
	if ( lua_gettop( L ) != 1 || ! lua_isstring( L, 1 ) )
		throw std::runtime_error( "Expecting a single pattern string argument to file.glob" );

	DEBUG( "luaGlobFiles " << Lua::Parm<std::string>::get( L, 1, 1 ) );
	std::shared_ptr<CompileSet> ret = std::make_shared<CompileSet>();
	globDir( ret, Lua::Parm<std::string>::get( L, 1, 1 ) );

	Scope::current().addItem( ret );
	Lua::pushItem( L, ret );
	return 1;
}


////////////////////////////////////////


static int
luaJoinPath( lua_State *L )
{
	int N = lua_gettop( L );
	if ( N < 2 )
		throw std::runtime_error( "Expecting at least 2 arguments to file.path.join" );

	DEBUG( "luaJoinPath" );
	Directory d( Lua::Parm<std::string>::get( L, N, 1 ) );
	for ( int i = 2; i <= N; ++i )
		d.cd( Lua::Parm<std::string>::get( L, N, i ) );

	std::string ret = d.fullpath();
	lua_pushlstring( L, ret.c_str(), ret.size() );
	return 1;
}

static std::string
luaSourceDir( void )
{
	DEBUG( "luaSourceDir" );
	return Directory::current()->fullpath();
}

static std::string
luaSourceFile( const std::string &fn )
{
	DEBUG( "luaSourceFile " << fn );
	return Directory::current()->makefilename( fn );
}

}


////////////////////////////////////////


namespace Lua
{


////////////////////////////////////////


void
registerFileExt( void )
{
	Engine &eng = Engine::singleton();

	eng.pushLibrary( "file" );
	ON_EXIT{ eng.popLibrary(); };

	eng.registerFunction( "basename", &File::basename );
	eng.registerFunction( "extension", &File::extension );
	eng.registerFunction( "replace_extension", &File::replaceExtension );
	eng.registerFunction( "compare", &File::compare );
	eng.registerFunction( "diff", &File::diff );
	eng.registerFunction( "exists", &File::exists );
	eng.registerFunction( "find", &luaFindFile );
	eng.registerFunction( "find_exe", &luaFindExecutable );
	eng.registerFunction( "set_exe_path", &luaSetPath );
	eng.registerFunction( "glob", &luaGlobFiles );

	{
		eng.pushSubLibrary( "path" );
		ON_EXIT{ eng.popLibrary(); };
		eng.registerFunction( "join", &luaJoinPath );

		eng.registerFunction( "current", &luaSourceDir );
		eng.registerFunction( "file_path", &luaSourceFile );

		std::string path_sep;
		path_sep.push_back( File::pathSeparator() );
		lua_pushliteral( eng.state(), "sep" );
		lua_pushlstring( eng.state(), path_sep.c_str(), path_sep.size() );
		lua_rawset( eng.state(), -3 );
	}
}


////////////////////////////////////////


} // src




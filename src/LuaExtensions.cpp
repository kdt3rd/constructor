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
#include "Configuration.h"
#include "PackageConfig.h"

#include <map>
#include <mutex>
#include <iostream>


////////////////////////////////////////


namespace 
{

static std::vector<std::string> theToolModulePath;
static std::vector<std::string> theSubDirFiles;


////////////////////////////////////////


inline constexpr const char *buildFileName( void )
{
	return "construct";
}


////////////////////////////////////////


static ItemPtr
extractItem( lua_State *L, int i )
{
	void *ud = luaL_checkudata( L, i, "Constructor.Item" );
	if ( ! ud )
		throw std::runtime_error( "User Data item is not a constructor item" );
	return *( reinterpret_cast<ItemPtr *>( ud ) );
}



////////////////////////////////////////


static void
pushItem( lua_State *L, ItemPtr i )
{
	if ( ! i )
	{
		lua_pushnil( L );
		return;
	}

	void *sptr = lua_newuserdata( L, sizeof(ItemPtr) );
	if ( ! sptr )
		throw std::runtime_error( "Unable to create item userdata" );

	luaL_setmetatable( L, "Constructor.Item" );
	new (sptr) ItemPtr( std::move( i ) );
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


////////////////////////////////////////


static int
luaAddToolModulePath( lua_State *L )
{
	int N = lua_gettop( L );

	for ( int i = 1; i <= N; ++i )
	{
		if ( lua_isnil( L, i ) )
			continue;

		if ( lua_isstring( L, i ) )
		{
			size_t len = 0;
			const char *s = lua_tolstring( L, i, &len );
			std::string curP( s, len );
			theToolModulePath.emplace_back( std::move( curP ) );
		}
		else
		{
			std::cout << "WARNING: ignoring non-string argument in tool module path" << std::endl;
		}
	}

	return 0;
}


////////////////////////////////////////


static int
luaSetToolModulePath( lua_State *L )
{
	theToolModulePath.clear();
	luaAddToolModulePath( L );
	return 0;
}


////////////////////////////////////////


int
luaLoadToolModule( lua_State *L )
{
	int N = lua_gettop( L );

	for ( int i = 1; i <= N; ++i )
	{
		if ( lua_isnil( L, i ) )
			continue;

		if ( lua_isstring( L, i ) )
		{
			size_t len = 0;
			const char *s = lua_tolstring( L, i, &len );
			std::string curP( s, len );
			std::string luaFile;
			bool found = false;
			if ( theToolModulePath.empty() )
			{
				Directory curD;
				std::vector<std::string> tmpPath = { curD.fullpath() };
				found = File::find( luaFile, curP + ".construct", tmpPath );
			}
			else
				found = File::find( luaFile, curP + ".construct", theToolModulePath );

			if ( found )
			{
				Lua::Engine::singleton().runFile( luaFile.c_str() );
				theSubDirFiles.push_back( luaFile );
			}
			else
			{
				throw std::runtime_error( "Unable to find constructor module '" + curP + "' in tool module path" );
			}
		}
		else
		{
			std::cout << "WARNING: ignoring non-string argument in loading tool module path" << std::endl;
		}
	}

	return 0;
}

static int
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

static int
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

static int
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
		ret->addItem( extractItem( L, i ) );
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

static int
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
	pushItem( L, ret );
	return 1;
}

static int
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
	pushItem( L, ret );
	return 1;
}

static int
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
	pushItem( L, ret );
	return 1;
}

static int
include( lua_State *L )
{
	auto &vars = Scope::current().getVars();
	auto incPath = vars.find( "includes" );
	if ( incPath == vars.end() )
		incPath = vars.emplace( std::make_pair( "includes", Variable( "includes" ) ) ).first;

	Variable &v = incPath->second;

	int N = lua_gettop( L );
	for ( int i = 1; i <= N; ++i )
	{
		std::string iname = Lua::Parm<std::string>::get( L, N, 1 );

		if ( File::isAbsolute( iname.c_str() ) )
			v.add( std::move( iname ) );
		else
			v.add( Directory::current()->makefilename( iname ) );
	}

	return 0;
}

static int
itemGetName( lua_State *L )
{
	if ( lua_gettop( L ) != 1 )
		throw std::runtime_error( "Item:name() expects only one argument - self" );
	ItemPtr p = extractItem( L, 1 );
	const std::string &n = p->getName();
	lua_pushlstring( L, n.c_str(), n.size() );
	return 1;
}

static int
itemAddDependency( lua_State *L )
{
	if ( lua_gettop( L ) != 3 )
		throw std::runtime_error( "Item:addDependency() expects 3 arguments - self, dependencyType, dependency" );
	ItemPtr p = extractItem( L, 1 );
	size_t len = 0;
	const char *dtp = lua_tolstring( L, 2, &len );
	if ( dtp )
	{
		std::string dt( dtp, len );
		ItemPtr d = extractItem( L, 3 );
		if ( dt == "explicit" )
			p->addDependency( DependencyType::EXPLICIT, d );
		else if ( dt == "implicit" )
			p->addDependency( DependencyType::IMPLICIT, d );
		else if ( dt == "order" )
			p->addDependency( DependencyType::ORDER, d );
		else if ( dt == "chain" )
			p->addDependency( DependencyType::CHAIN, d );
		else
			throw std::runtime_error( "Invalid dependency type: expect explicit, implicit, order, or chain" );
	}
	else
		throw std::runtime_error( "Unable to extract string in addDependency" );
	return 0;
}

static int
itemHasDependency( lua_State *L )
{
	if ( lua_gettop( L ) != 2 )
		throw std::runtime_error( "Item:addDependency() expects 2 arguments - self and dependency check" );
	ItemPtr p = extractItem( L, 1 );
	ItemPtr d = extractItem( L, 2 );
	bool hd = p->hasDependency( d );
	lua_pushboolean( L, hd ? 1 : 0 );
	return 1;
}

static int
itemVariables( lua_State *L )
{
	if ( lua_gettop( L ) != 1 )
		throw std::runtime_error( "Item:variables() expects 1 argument - self" );
	ItemPtr p = extractItem( L, 1 );
	Lua::Value ret;
	Lua::Table &t = ret.initTable();
	for ( auto &v: p->getVariables() )
		t[v.first].initString( std::move( v.second.value() ) );

	ret.push( L );
	return 1;
}

static int
itemClearVariable( lua_State *L )
{
	if ( lua_gettop( L ) != 2 )
		throw std::runtime_error( "Item:clearVariable() expects 2 arguments - self, variable name" );
	ItemPtr p = extractItem( L, 1 );
	std::string nm = Lua::Parm<std::string>::get( L, 2, 2 );

	VariableSet &vars = p->getVariables();
	auto x = vars.find( nm );
	if ( x != vars.end() )
		vars.erase( x );
	return 0;
}

static int
itemSetVariable( lua_State *L )
{
	int N = lua_gettop( L );
	if ( N < 3 )
		throw std::runtime_error( "Item:setVariable() expects 3 or 4 arguments - self, variable name, variable value, bool env check (nil)" );
	ItemPtr p = extractItem( L, 1 );
	std::string nm = Lua::Parm<std::string>::get( L, N, 2 );
	Lua::Value v;
	v.load( L, 3 );
	bool envCheck = false;
	if ( N == 4 )
		envCheck = Lua::Parm<bool>::get( L, N, 4 );

	VariableSet &vars = p->getVariables();
	auto x = vars.find( nm );
	if ( v.type() == LUA_TNIL )
	{
		if ( x != vars.end() )
			vars.erase( x );
		return 0;
	}

	if ( x == vars.end() )
		x = vars.emplace( std::make_pair( nm, Variable( nm, envCheck ) ) ).first;

	if ( v.type() == LUA_TTABLE )
		x->second.reset( std::move( v.toStringList() ) );
	else if ( v.type() == LUA_TSTRING )
		x->second.reset( v.asString() );
	else
		throw std::runtime_error( "Item:setVariable() - unhandled variable value type, expect nil, table or string" );

	return 0;
}

static int
itemAddToVariable( lua_State *L )
{
	if ( lua_gettop( L ) != 3 )
		throw std::runtime_error( "Item:addToVariable() expects 3 arguments - self, variable name, variable value to add" );
	ItemPtr p = extractItem( L, 1 );
	std::string nm = Lua::Parm<std::string>::get( L, 3, 2 );
	Lua::Value v;
	v.load( L, 3 );
	if ( v.type() == LUA_TNIL )
		return 0;

	VariableSet &vars = p->getVariables();
	auto x = vars.find( nm );
	// should this be an error?
	if ( x == vars.end() )
		x = vars.emplace( std::make_pair( nm, Variable( nm ) ) ).first;

	if ( v.type() == LUA_TTABLE )
		x->second.add( v.toStringList() );
	else if ( v.type() == LUA_TSTRING )
		x->second.add( v.asString() );
	else
		throw std::runtime_error( "Item:addToVariable() - unhandled variable value type, expect nil, table or string" );
	return 0;
}

static int
itemInheritVariable( lua_State *L )
{
	if ( lua_gettop( L ) != 3 )
		throw std::runtime_error( "Item:inheritVariable() expects 3 arguments - self, variable name, boolean" );
	ItemPtr p = extractItem( L, 1 );
	std::string nm = Lua::Parm<std::string>::get( L, 3, 2 );
	bool v = Lua::Parm<bool>::get( L, 3, 3 );
	VariableSet &vars = p->getVariables();
	auto x = vars.find( nm );
	// should this be an error?
	if ( x == vars.end() )
		x = vars.emplace( std::make_pair( nm, Variable( nm ) ) ).first;

	x->second.inherit( v );
	return 0;
}

static int
itemForceTool( lua_State *L )
{
	if ( lua_gettop( L ) != 3 )
		throw std::runtime_error( "Item:forceTool() expects 3 arguments - self, extension, tool name" );
	ItemPtr p = extractItem( L, 1 );
	p->forceTool( Lua::Parm<std::string>::get( L, 3, 2 ),
				  Lua::Parm<std::string>::get( L, 3, 3 ) );

	return 0;
}

static int
itemOverrideToolSetting( lua_State *L )
{
	if ( lua_gettop( L ) != 3 )
		throw std::runtime_error( "Item:overrideToolSetting() expects 3 arguments - self, tool setting name, setting value" );
	ItemPtr p = extractItem( L, 1 );
	p->overrideToolSetting( Lua::Parm<std::string>::get( L, 3, 2 ),
							Lua::Parm<std::string>::get( L, 3, 3 ) );

	return 0;
}


////////////////////////////////////////


static int
itemCreate( lua_State *L )
{
	if ( lua_gettop( L ) != 1 || ! lua_isstring( L, 1 ) )
		throw std::runtime_error( "Item.new expects a single argument - a name" );

	size_t len = 0;
	const char *p = lua_tolstring( L, 1, &len );
	if ( p )
	{
		ItemPtr np = std::make_shared<Item>( std::string( p, len ) );
		pushItem( L, np );
	}
	else
		throw std::runtime_error( "Unable to extract item name" );

	return 1;
}

static const struct luaL_Reg item_m[] =
{
	{ "__tostring", itemGetName },
	{ "name", itemGetName },
	{ "addDependency", itemAddDependency },
	{ "depends", itemHasDependency },
	{ "variables", itemVariables },
	{ "clearVariable", itemClearVariable },
	{ "setVariable", itemSetVariable },
	{ "addToVariable", itemAddToVariable },
	{ "inheritVariable", itemInheritVariable },
	{ "forceTool", itemForceTool },
	{ "overrideToolSetting", itemOverrideToolSetting },
	{ nullptr, nullptr }
};

static const struct luaL_Reg item_class_f[] =
{
	{ "new", itemCreate },
	{ nullptr, nullptr }
};

int
defineConfiguration( lua_State *L )
{
	if ( lua_gettop( L ) != 1 || ! lua_istable( L, 1 ) )
		throw std::runtime_error( "Expected 1 argument - a table - to BuildConfiguration" );
	Lua::Value v( L, 1 );
	Configuration::defined().emplace_back( Configuration( v.extractTable() ) );

	return 0;
}

int
setDefaultConfig( lua_State *L )
{
	if ( lua_gettop( L ) != 1 || ! lua_isstring( L, 1 ) )
		throw std::runtime_error( "Expected 1 argument - a string - to DefaultConfiguration" );

	const char *s = lua_tolstring( L, 1, NULL );
	bool found = false;
	for ( const Configuration &c: Configuration::defined() )
	{
		if ( c.name() == s )
		{
			found = true;
			break;
		}
	}
	if ( ! found )
		throw std::runtime_error( "Configuration '" + std::string( s ) + "' not defined yet, please call BuildConfiguration first" );
	Configuration::setDefault( s );

	return 0;
}


////////////////////////////////////////




static bool
packageExists( const std::string &name, const std::string &reqVersion )
{
	if ( PackageConfig::find( name, reqVersion ) )
		return true;
	return false;
}

static int
packageFind( lua_State *L )
{
	int N = lua_gettop( L );
	std::string name = Lua::Parm<std::string>::get( L, N, 1 );
	std::string ver = Lua::Parm<std::string>::get( L, N, 2 );

	pushItem( L, PackageConfig::find( name, ver ) );
	return 1;
}

static int
packageFindRequired( lua_State *L )
{
	int N = lua_gettop( L );
	std::string name = Lua::Parm<std::string>::get( L, N, 1 );
	std::string ver = Lua::Parm<std::string>::get( L, N, 2 );

	auto p = PackageConfig::find( name, ver );
	if ( ! p )
	{
		if ( ! ver.empty() )
			throw std::runtime_error( "ERROR: Package '" + name + "' and required version " + ver + " not found" );
		else
			throw std::runtime_error( "ERROR: Package '" + name + "' not found" );
	}

	pushItem( L, p );
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
	eng.registerFunction( "AddToolSet", &luaAddToolSet );
	eng.registerFunction( "AddToolOption", &luaAddToolOption );
	eng.registerFunction( "SetToolModulePath", &luaSetToolModulePath );
	eng.registerFunction( "AddToolModulePath", &luaAddToolModulePath );
	eng.registerFunction( "LoadToolModule", &luaLoadToolModule );
	eng.registerFunction( "EnableLanguages", &luaEnableLangs );
	eng.registerFunction( "SetOption", &luaSetOption );

	eng.registerFunction( "Compile", &compile );
	eng.registerFunction( "Executable", &executable );
	eng.registerFunction( "Library", &library );
	eng.registerFunction( "Include", &include );

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
	
	eng.registerClass( "Item", item_class_f, item_m );

	eng.registerFunction( "BuildConfiguration", &defineConfiguration );
	eng.registerFunction( "DefaultConfiguration", &setDefaultConfig );

	eng.registerFunction( "ExternalLibraryExists", &packageExists );
	eng.registerFunction( "ExternalLibrary", &packageFind );
	eng.registerFunction( "RequiredExternalLibrary", &packageFindRequired );

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




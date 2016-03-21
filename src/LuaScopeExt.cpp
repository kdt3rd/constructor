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

#include "LuaScopeExt.h"

#include "Debug.h"

#include "LuaEngine.h"
#include "LuaValue.h"
#include "ScopeGuard.h"

#include "Scope.h"
#include "Configuration.h"
#include "DefaultTools.h"
#include "Directory.h"
#include "FileUtil.h"


////////////////////////////////////////


namespace
{

static void
recurseAndAddPath( Variable &v, lua_State *L, int i )
{
	if ( lua_isnil( L, i ) )
		return;
	if ( lua_isstring( L, i ) )
	{
		size_t len = 0;
		const char *n = lua_tolstring( L, i, &len );
		if ( n )
		{
			if ( File::isAbsolute( n ) )
				v.add( std::string( n, len ) );
			else
				v.add( Directory::current()->makefilename( n ) );
		}
		else
			throw std::runtime_error( "String argument, but unable to extract string" );
	}
	else if ( lua_istable( L, i ) )
	{
		lua_pushnil( L );
		while ( lua_next( L, i ) )
		{
			recurseAndAddPath( v, L, lua_gettop( L ) );
			lua_pop( L, 1 );
		}
	}
	else
		throw std::runtime_error( "Unhandled argument type to include" );
}

static int
luaInclude( lua_State *L )
{
	Scope &s = Scope::current();
	auto &vars = s.getVars();
	auto incPath = vars.find( "includes" );
	if ( incPath == vars.end() )
		incPath = vars.emplace( std::make_pair( "includes", Variable( "includes" ) ) ).first;

	Variable &v = incPath->second;
	if ( s.getParent() )
		v.inherit( true );
	v.setToolTag( "cc" );

	DEBUG( "luaInclude" );
	int N = lua_gettop( L );
	for ( int i = 1; i <= N; ++i )
		recurseAndAddPath( v, L, i );

	return 0;
}


////////////////////////////////////////


static int
luaAddDefines( lua_State *L )
{
	auto &s = Scope::current();
	auto &vars = s.getVars();
	auto defs = vars.find( "defines" );
	if ( defs == vars.end() )
		defs = vars.emplace( std::make_pair( "defines", Variable( "defines" ) ) ).first;

	Variable &v = defs->second;
	if ( s.getParent() )
		v.inherit( true );
	v.setToolTag( "cc" );

	DEBUG( "luaAddDefines" );
	int N = lua_gettop( L );
	for ( int i = 1; i <= N; ++i )
		v.add( Lua::Parm< std::vector<std::string> >::get( L, N, i ) );

	return 0;
}

static int
luaAddSystemDefines( lua_State *L )
{
	Scope &s = Scope::current();
	auto &vars = s.getVars();
	auto defs = vars.find( "defines" );
	if ( defs == vars.end() )
		defs = vars.emplace( std::make_pair( "defines", Variable( "defines" ) ) ).first;

	Variable &v = defs->second;
	if ( s.getParent() )
		v.inherit( true );
	v.setToolTag( "cc" );

	DEBUG( "luaAddSystemDefines" );
	int N = lua_gettop( L );

	std::vector<std::string> vals = Lua::Parm< std::vector<std::string> >::recursive_get( L, 1, N );
	if ( vals.size() < 2 )
	{
		std::string errmsg = "system_defines expects at least 2 arguments - a string value for the system name, and then defines or sets of defines to add";
		luaL_error( L, errmsg.c_str() );
		return 1;
	}
	std::string name = std::move( vals.front() );
	vals.erase( vals.begin() );

	v.addPerSystem( name, vals );
	
	return 0;
}

static int
storeOption( const std::string &name, std::string &&val )
{
	DEBUG( "storeOption to current scope: name " << name << " value " << val );
	auto &opts = Scope::current().getOptions();
	auto ne = opts.emplace( std::make_pair( name, Variable( name ) ) );
	ne.first->second.reset( std::move( val ) );

	return 0;
}

static int
luaSetOption( lua_State *L )
{
	int N = lua_gettop( L );
	if ( N != 2 )
	{
		luaL_error( L, "set_option expects 2 arguments - a name and a value" );
		return 1;
		
	}

	std::string nm = Lua::Parm<std::string>::get( L, N, 1 );
	std::string val = Lua::Parm<std::string>::get( L, N, 2 );
	DEBUG( "luaSetOption " << nm );
	return storeOption( nm, std::move( val ) );
}


////////////////////////////////////////


static int
luaUseToolset( lua_State *L )
{
	int N = lua_gettop( L );
	if ( N != 1 )
	{
		luaL_error( L, "toolset expects 1 arguments - a toolset name to enable" );
		return 1;
	}

	std::string nm = Lua::Parm<std::string>::get( L, N, 1 );

	Scope &s = Scope::current();
	s.useToolSet( nm );
	return 0;
}


////////////////////////////////////////


static int
luaPresetOption( const std::string &nm, lua_State *L )
{
	int N = lua_gettop( L );
	if ( N != 1 )
	{
		std::string errmsg = nm + " expects 1 arguments - a string value";
		luaL_error( L, errmsg.c_str() );
		return 1;
	}

	std::string val = Lua::Parm<std::string>::get( L, N, 1 );

	return storeOption( nm, std::move( val ) );
}

}


////////////////////////////////////////


namespace Lua
{


////////////////////////////////////////


void
registerScopeExt( void )
{
	Engine &eng = Engine::singleton();

	eng.registerFunction( "set_option", &luaSetOption );
	eng.registerFunction( "defines", &luaAddDefines );
	eng.registerFunction( "includes", &luaInclude );

	eng.registerFunction( "system_defines", &luaAddSystemDefines );
	eng.registerFunction( "toolset", &luaUseToolset );

	const auto &opts = DefaultTools::getOptions();
	for ( auto o = opts.begin(); o != opts.end(); ++o )
	{
		const std::string &oname = (*o);

		Lua::Engine::BoundFunction bf = std::bind( &luaPresetOption, oname, std::placeholders::_1 );
		eng.registerFunction( oname.c_str(), bf );
	}
}


////////////////////////////////////////


} // Lua




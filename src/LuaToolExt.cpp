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

#include "LuaToolExt.h"
#include "LuaEngine.h"
#include "LuaValue.h"
#include "FileUtil.h"
#include "OSUtil.h"
#include "Debug.h"
#include "Directory.h"
#include "Scope.h"
#include "LuaExtensions.h"

#include <stdexcept>
#include <iostream>
#include <iomanip>


////////////////////////////////////////


namespace
{

static std::shared_ptr<Toolset> theCurToolset;
static std::vector<std::string> theToolModulePath;


////////////////////////////////////////


static int
luaAddPool( lua_State *L )
{
	int N = lua_gettop( L );
	if ( N != 2 || ! lua_isstring( L, 1 ) || ! lua_isnumber( L, 2 ) )
		throw std::runtime_error( "AddProcessingPool expects 2 arguments: pool name, number of concurrent jobs" );

	std::string name = Lua::Parm<std::string>::get( L, N, 1 );
	int jobs = Lua::Parm<int>::get( L, N, 2 );

	DEBUG( "luaAddPool " << name );
	Scope::current().addPool( std::make_shared<Pool>( name, jobs ) );
	return 0;
}


////////////////////////////////////////


void
luaAddTool( const Lua::Value &v )
{
	DEBUG( "luaAddTool" );
	std::shared_ptr<Tool> t = Tool::parse( v );
	if ( t->getExecutable().empty() && ! t->getGeneratedExecutable() )
	{
		VERBOSE( "Tool '" << t->getName() << "' has no executable, ignoring" );
	}
	else
	{
		Scope::current().addTool( t );
		if ( theCurToolset )
			theCurToolset->addTool( t );
	}
}

int
luaAddToolSet( lua_State *L )
{
	int N = lua_gettop( L );
	if ( N != 1 )
		throw std::runtime_error( "add_toolset expects one argument: a name" );

	std::string name = Lua::Parm<std::string>::get( L, N, 1 );
	theCurToolset = std::make_shared<Toolset>( std::move( name ) );
	DEBUG( "luaAddToolset " << theCurToolset->getName() );
	Scope::current().addToolSet( theCurToolset );

	return 0;
}

int luaToolSetTag( lua_State *L )
{
	int N = lua_gettop( L );
	if ( N != 1 )
		throw std::runtime_error( "tag expects one argument: a tag" );

	std::string t = Lua::Parm<std::string>::get( L, N, 1 );
	if ( theCurToolset )
		theCurToolset->setTag( std::move( t ) );
	else
		throw std::runtime_error( "Attempt to set a toolset tag without an active toolset" );

	return 0;
}

int luaToolLibPath( lua_State *L )
{
	int N = lua_gettop( L );
	if ( N != 1 )
		throw std::runtime_error( "tag expects one argument: a colon (:) separated path" );

	std::string p = Lua::Parm<std::string>::get( L, N, 1 );
	if ( theCurToolset )
		theCurToolset->addLibSearchPath( p );
	else
		throw std::runtime_error( "Attempt to set a library search path without an active toolset" );

	return 0;
}

int luaToolPkgPath( lua_State *L )
{
	int N = lua_gettop( L );
	if ( N != 1 )
		throw std::runtime_error( "tag expects one argument: a colon (:) separated path" );

	std::string p = Lua::Parm<std::string>::get( L, N, 1 );
	if ( theCurToolset )
		theCurToolset->addPkgSearchPath( p );
	else
		throw std::runtime_error( "Attempt to set a package search path without an active toolset" );

	return 0;
}


////////////////////////////////////////


int luaActiveToolSet( lua_State *L )
{
	int N = lua_gettop( L );
	if ( N != 1 )
		throw std::runtime_error( "toolset_active expects one argument: a name" );

	std::string t = Lua::Parm<std::string>::get( L, N, 1 );
	Lua::clearToolset();
	Scope &s = Scope::current();
	auto ts = s.findToolSet( t );
	if ( ts && !( ts->empty() ) )
		lua_pushboolean( L, 1 );
	else
		lua_pushboolean( L, 0 );

	return 1;
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

	DEBUG( "luaAddToolOption tool " << t );
	if ( ! found )
		throw std::runtime_error( "Unable to find tool '" + t + "' in current scope" );
}

}


////////////////////////////////////////


namespace Lua
{


////////////////////////////////////////


void clearToolset( void )
{
	theCurToolset.reset();
}

void registerToolExt( void )
{
	Engine &eng = Engine::singleton();

	eng.registerFunction( "pool", &luaAddPool );
	eng.registerFunction( "add_tool", &luaAddTool );
	eng.registerFunction( "add_toolset", &luaAddToolSet );
	eng.registerFunction( "tag", &luaToolSetTag );
	eng.registerFunction( "lib_search_path", &luaToolLibPath );
	eng.registerFunction( "pkg_search_path", &luaToolPkgPath );

	eng.registerFunction( "toolset_active", &luaActiveToolSet );

	eng.registerFunction( "tool_option", &luaAddToolOption );
}


////////////////////////////////////////


} // Lua




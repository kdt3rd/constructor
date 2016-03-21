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

#include "LuaConfigExt.h"

#include "Debug.h"

#include "LuaExtensions.h"
#include "LuaEngine.h"
#include "LuaValue.h"
#include "ScopeGuard.h"

#include "Configuration.h"


////////////////////////////////////////


namespace
{

static int
luaBuildConfiguration( lua_State *L )
{
	if ( lua_gettop( L ) != 1 || ! lua_isstring( L, 1 ) )
		throw std::runtime_error( "Expected 1 argument - a string - to configuration" );
	std::string nm = Lua::Parm<std::string>::get( L, 1, 1 );

	// hrm, the constructor will call Scope::current but that could be
	// a previous config which we don't want to inherit from
	Configuration::creatingNewConfig();
	ON_EXIT{ Configuration::finishCreatingNewConfig(); };
	
	DEBUG( "luaBuildConfiguration " << nm );
	Configuration::defined().emplace_back( Configuration( nm ) );

	return 0;
}

static int
luaDefaultConfiguration( lua_State *L )
{
	if ( lua_gettop( L ) != 1 || ! lua_isstring( L, 1 ) )
		throw std::runtime_error( "Expected 1 argument - a string - to default_configuration" );

	const char *s = lua_tolstring( L, 1, NULL );
	DEBUG( "luaDefaultConfiguration " << s );
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
		throw std::runtime_error( "Configuration '" + std::string( s ) + "' not defined yet, please call configuration first" );

	Configuration::setDefault( s );

	return 0;
}

static int luaSetSystem( lua_State *L )
{
	int N = lua_gettop( L );
	if ( N != 1 )
		throw std::runtime_error( "system expects one argument: a system name" );

	std::string s = Lua::Parm<std::string>::get( L, N, 1 );

	if ( Configuration::haveDefault() )
		throw std::runtime_error( "Attempt to set a system after default_configuration and regular build start" );

	if ( Configuration::haveAny() )
	{
		Configuration::last().setSystem( std::move( s ) );
	}
	else
		throw std::runtime_error( "Attempt to set a system override prior to creating a configuration" );
	return 0;
}


static int luaSetSkipOnError( lua_State *L )
{
	int N = lua_gettop( L );
	if ( N != 1 )
		throw std::runtime_error( "system expects one argument: a system name" );

	if ( Configuration::haveDefault() )
		throw std::runtime_error( "Attempt to set skip_on_error default_configuration and regular build start" );

	bool sk = Lua::Parm<bool>::get( L, N, 1 );

	if ( Configuration::haveAny() )
	{
		Configuration::last().setSkipOnError( sk );
	}
	else
		throw std::runtime_error( "Attempt to set skip_on_error prior to creating a configuration" );
	return 0;
}

}


////////////////////////////////////////


namespace Lua
{


////////////////////////////////////////


void
registerConfigExt( void )
{
	Engine &eng = Engine::singleton();

	eng.registerFunction( "configuration", &luaBuildConfiguration );
	eng.registerFunction( "default_configuration", &luaDefaultConfiguration );
	eng.registerFunction( "system", &luaSetSystem );
	eng.registerFunction( "skip_on_error", &luaSetSkipOnError );
}


////////////////////////////////////////


} // src




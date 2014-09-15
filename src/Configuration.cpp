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

#include "Configuration.h"
#include "LuaEngine.h"
#include <iostream>
#include "Directory.h"


////////////////////////////////////////


namespace
{

static std::vector<Configuration> theConfigs;
static std::string theDefaultConfig;

//static void
//updateBinaryPath( const Configuration &c )
//{
//	Directory theDir;
//	if ( theUseConfigDir )
//		theDir.cd( c.name() );
//	Directory::setBinaryRoot( theDir.fullpath() );
//}

int
defineConfiguration( lua_State *L )
{
	if ( lua_gettop( L ) != 1 || ! lua_istable( L, 1 ) )
		throw std::runtime_error( "Expected 1 argument - a table - to BuildConfiguration" );
	Lua::Value v( L, 1 );
	theConfigs.emplace_back( Configuration( v.extractTable() ) );

	return 0;
}

int
setDefaultConfig( lua_State *L )
{
	if ( lua_gettop( L ) != 1 || ! lua_isstring( L, 1 ) )
		throw std::runtime_error( "Expected 1 argument - a string - to DefaultConfiguration" );

	const char *s = lua_tolstring( L, 1, NULL );
	bool found = false;
	for ( const Configuration &c: theConfigs )
	{
		if ( c.name() == s )
		{
			found = true;
			break;
		}
	}
	if ( ! found )
		throw std::runtime_error( "Configuration '" + std::string( s ) + "' not defined yet, please call BuildConfiguration first" );
	theDefaultConfig = s;

	return 0;
}

} // empty namespace


////////////////////////////////////////


Configuration::Configuration( void )
{
}


////////////////////////////////////////


Configuration::Configuration( const Lua::Table &t )
{
	for ( auto i: t )
	{
		if ( i.first.type == Lua::KeyType::INDEX )
			continue;

		if ( i.first.tag == "name" )
		{
			if ( i.second.type() != LUA_TSTRING )
				throw std::runtime_error( "Build configuration requires a name as a string to be provided in configuration table" );
			myName = i.second.asString();
		}
		else
		{
			Variable v( i.first.tag );
			if ( i.second.type() == LUA_TSTRING )
				v.reset( i.second.asString() );
			else
				v.reset( i.second.toStringList() );

			myVariables.emplace( std::make_pair( i.first.tag, std::move( v ) ) );
		}
	}

	if ( myName.empty() )
		throw std::runtime_error( "Build configuration requires a name as a string to be provided in configuration table" );
}


////////////////////////////////////////


Configuration::~Configuration( void )
{
}


////////////////////////////////////////


const Configuration &
Configuration::getDefault( void )
{
	if ( theConfigs.empty() )
		throw std::runtime_error( "No configurations specified, please use BuildConfiguration to define at least one" );

	if ( theDefaultConfig.empty() )
		return theConfigs[0];

	for ( const Configuration &r: theConfigs )
	{
		if ( r.name() == theDefaultConfig )
			return r;
	}
	throw std::logic_error( "Configuration '" + theDefaultConfig + "' not found" );
}


////////////////////////////////////////


std::vector<Configuration> &
Configuration::defined( void )
{
	return theConfigs;
}


////////////////////////////////////////


void
Configuration::registerFunctions( void )
{
	Lua::Engine &eng = Lua::Engine::singleton();
	eng.registerFunction( "BuildConfiguration", &defineConfiguration );
	eng.registerFunction( "DefaultConfiguration", &setDefaultConfig );
}


////////////////////////////////////////





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
#include <iostream>
#include "Directory.h"


////////////////////////////////////////


namespace
{

static std::vector<Configuration> theConfigs;
static std::string theDefaultConfig;

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


void
Configuration::setDefault( std::string c )
{
	theDefaultConfig = std::move( c );
}


////////////////////////////////////////


std::vector<Configuration> &
Configuration::defined( void )
{
	return theConfigs;
}


////////////////////////////////////////





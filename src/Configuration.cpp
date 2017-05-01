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
#include "OSUtil.h"


////////////////////////////////////////


namespace
{

static int theCreating = 0;
struct CurSubProject
{
};	
static int theSubProject = 0;
static std::vector<Configuration> theConfigs;
static std::string theDefaultConfig;

} // empty namespace


////////////////////////////////////////


Configuration::Configuration( void )
		: myPseudoScope( Scope::current().newSubScope( true ) )
{
	Scope::current().removeSubScope( myPseudoScope );
}


////////////////////////////////////////


Configuration::Configuration( const std::string &n )
		: myName( n ),
		  myPseudoScope( Scope::current().newSubScope( true ) )
{
	Scope::current().removeSubScope( myPseudoScope );
	if ( myName.empty() )
		throw std::runtime_error( "Build configuration definition requires a name as a string to be provided" );
}


////////////////////////////////////////


const std::string &
Configuration::getSystem( void ) const
{
	if ( mySystem.empty() )
		return OS::system();
	return mySystem;
}


////////////////////////////////////////


void
Configuration::setSystem( std::string s )
{
	mySystem = std::move( s );
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


bool
Configuration::haveDefault( void )
{
	return ! theDefaultConfig.empty();
}


////////////////////////////////////////


bool
Configuration::haveAny( void )
{
	if ( theCreating > 0 )
		return false;

	return ! theConfigs.empty();
}


////////////////////////////////////////


void
Configuration::checkDefault( void )
{
	if ( ! haveDefault() )
		throw std::logic_error( "Must specify default_configuration prior to specifying targets or recursing tree" );
}


////////////////////////////////////////


void
Configuration::creatingNewConfig( void )
{
	++theCreating;
}
void
Configuration::finishCreatingNewConfig( void )
{
	--theCreating;
}


////////////////////////////////////////


bool
Configuration::inSubProject( void )
{
	return theSubProject > 0;
}


////////////////////////////////////////


void
Configuration::pushSubProject( void )
{
}


////////////////////////////////////////


void Configuration::popSubProject( void )
{
}



////////////////////////////////////////


Configuration &
Configuration::last( void )
{
	if ( theConfigs.empty() )
		throw std::logic_error( "Attempt to access configurations without checking if any are defined" );
	return theConfigs.back();
}



////////////////////////////////////////


std::vector<Configuration> &
Configuration::defined( void )
{
	return theConfigs;
}


////////////////////////////////////////





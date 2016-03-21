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

#include "PackageConfig.h"
#include <iomanip>
#include <fstream>
#include "TransformSet.h"
#include "Debug.h"
#include "Directory.h"
#include "StrUtil.h"


////////////////////////////////////////


PackageConfig::PackageConfig( const std::string &n, const std::string &pf )
		: Item( n ), myPackageFile( pf )
{
	// can't call parse here since we add dependencies that uses
	// shared_from_this, which isn't fully initialized yet
}


////////////////////////////////////////


PackageConfig::~PackageConfig( void )
{
}


////////////////////////////////////////


const std::string &
PackageConfig::getVersion( void ) const
{
	return getAndReturn( "Version" );
}


////////////////////////////////////////


const std::string &
PackageConfig::getPackage( void ) const
{
	return getAndReturn( "Name" );
}


////////////////////////////////////////


const std::string &
PackageConfig::getDescription( void ) const
{
	return getAndReturn( "Description" );
}


////////////////////////////////////////


const std::string &
PackageConfig::getConflicts( void ) const
{
	return getAndReturn( "Conflicts" );
}


////////////////////////////////////////


const std::string &
PackageConfig::getURL( void ) const
{
	return getAndReturn( "URL" );
}


////////////////////////////////////////


const std::string &
PackageConfig::getCFlags( void ) const
{
	return getAndReturn( "CFlags" );
}


////////////////////////////////////////


const std::string &
PackageConfig::getLibs( void ) const
{
	return getAndReturn( "Libs" );
}


////////////////////////////////////////


const std::string &
PackageConfig::getStaticLibs( void ) const
{
	return getAndReturn( "Libs.private" );
}


////////////////////////////////////////


const std::string &
PackageConfig::getRequires( void ) const
{
	return getAndReturn( "Requires" );
}


////////////////////////////////////////


const std::string &
PackageConfig::getStaticRequires( void ) const
{
	return getAndReturn( "Requires.private" );
}


////////////////////////////////////////


std::shared_ptr<BuildItem>
PackageConfig::transform( TransformSet &xform ) const
{
	std::shared_ptr<BuildItem> ret = xform.getTransform( this );
	if ( ret )
		return ret;

	DEBUG( "transform PackageConfig " << getName() );
	if ( myPackageFile.empty() )
		ret = std::make_shared<BuildItem>( getName(), getDir() );
	else
	{
		ret = std::make_shared<BuildItem>( getName(), std::shared_ptr<Directory>() );
		ret->addExternalOutput( myPackageFile );
	}
	ret->addToVariable( "cflags", getCFlags() );
	ret->addToVariable( "cflags", getVariable( "cflags" ) );
	ret->addToVariable( "ldflags", getLibs() );
	ret->addToVariable( "ldflags", getVariable( "ldflags" ) );

	if ( myPackageFile.empty() )
	{
		ret->addToVariable( "libdirs", getVariable( "libdirs" ) );
		ret->addToVariable( "includes", getVariable( "includes" ) );
	}

	xform.recordTransform( this, ret );
	return ret;
}


////////////////////////////////////////


void
PackageConfig::forceTool( const std::string & )
{
	throw std::runtime_error( "Invalid request to force a tool on a Package" );
}


////////////////////////////////////////


void
PackageConfig::forceTool( const std::string &, const std::string & )
{
	throw std::runtime_error( "Invalid request to force a tool on a Package" );
}


////////////////////////////////////////


void
PackageConfig::overrideToolSetting( const std::string &, const std::string & )
{
	throw std::runtime_error( "Invalid request to override a tool setting on a Package" );
}


////////////////////////////////////////


const std::string &
PackageConfig::getAndReturn( const char *tag ) const
{
	auto i = myValues.find( tag );
	if ( i != myValues.end() )
		return i->second;
	return myNilValue;
}


////////////////////////////////////////


void
PackageConfig::parse( void )
{
	std::ifstream infl( myPackageFile );
	std::string curline;
	std::string parseline;
	using std::swap;

	while ( infl )
	{
		parseline.clear();
		bool linecontinue = false;
		do
		{
			std::getline( infl, curline );
			std::string::size_type commentPos = curline.find_first_of( '#' );
			while ( commentPos != std::string::npos )
			{
				if ( commentPos > 0 && curline[commentPos-1] == '\\' )
				{
					commentPos = curline.find_first_of( '#', commentPos + 1 );
					continue;
				}
				curline.erase( commentPos );
				break;
			}

			if ( ! curline.empty() && curline.back() == '\\' )
			{
				curline.pop_back();
				linecontinue = true;
			}
			parseline.append( curline );
		} while ( linecontinue );

		String::strip( parseline );

		if ( parseline.empty() )
			continue;

		extractNameAndValue( parseline );
	}

	// now promote everything to item variables we might care about
	for ( auto &x: myLocalVars )
		setVariable( x.first, x.second );
	setVariable( "version", getVersion() );
	setVariable( "cflags", getCFlags(), true );
	setVariable( "libs", getLibs(), true );
	setVariable( "libs.static", getStaticLibs(), true );
}


////////////////////////////////////////


void
PackageConfig::extractNameAndValue( const std::string &curline )
{
	std::string nm, val;

	std::string::size_type i = 0;
	while ( i < curline.size() )
	{
		if ( std::isalnum( curline[i] ) || curline[i] == '_' || curline[i] == '.' )
		{
			++i;
			continue;
		}
		break;
	}

	if ( i >= curline.size() )
		return;

	if ( i > 0 )
		nm = curline.substr( 0, i );

	while ( i < curline.size() && std::isspace( curline[i] ) )
		++i;

	if ( i >= curline.size() )
		return;

	char separator = curline[i];
	++i;
	while ( i < curline.size() && std::isspace( curline[i] ) )
		++i;

	if ( i < curline.size() )
		val = curline.substr( i );

	String::substituteVariables( val, true, myLocalVars );

	if ( separator == ':' )
	{
		if ( myValues.find( nm ) != myValues.end() )
		{
			std::cerr << "WARNING: Package config file '" << myPackageFile << "' has multiple entries for tag '" << nm << "'" << std::endl;
			return;
		}

		if ( nm == "Name" || nm == "Description" || nm == "URL" )
		{
			myValues[nm] = val;
		}
		else if ( nm == "Version" )
		{
			myValues[nm] = val;
		}
		else if ( nm == "Libs.private" || nm == "Libs" )
		{
			// pkg-config parses these as shell arguments and attempts to collapse them when
			// chained, let's not bother with that, it doesn't seem needed
			myValues[nm] = val;
		}
		else if ( nm == "Requires.private" || nm == "Requires" )
		{
			myValues[nm] = val;
		}
		else if ( nm == "Cflags" || nm == "CFlags" )
		{
			// make sure we only have to look up by the one name later
			myValues["CFlags"] = val;
		}
		else if ( nm == "Conflicts" )
		{
			// do we care about this???
			myValues[nm] = val;
		}
		else
		{
			DEBUG( "WARNING: Ignoring unknown package config tag: '" << nm << "', value: " << val );
			myValues[nm] = val;
		}
	}
	else if ( separator == '=' )
	{
		if ( myLocalVars.find( nm ) != myLocalVars.end() )
		{
			std::cerr << "WARNING: Package config file '" << myPackageFile << "' has multiple entries for variable '" << nm << "'" << std::endl;
			return;
		}
		// do we need to implement the special handling
		// for trying to auto-figure out the prefix?
		// this seems to be done on Windows in the pkg-config
		// source. Let's cross that bridge when we get to it
//		if ( nm == "prefix" )
//		{
//			
//		}
		myLocalVars[nm] = val;
	}
	else
	{
		std::cerr << "WARNING: Ignoring bogus line in pkg config file: " << myPackageFile << ": " << curline << std::endl;
	}
}


////////////////////////////////////////




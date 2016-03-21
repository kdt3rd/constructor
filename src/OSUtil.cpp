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

#include "OSUtil.h"
#include "StrUtil.h"
#include <sys/utsname.h>
#include <system_error>
#include <map>

extern "C"
{

extern char **environ;

}


////////////////////////////////////////


namespace
{

std::string theSystem;
std::string theNode;
std::string theRelease;
std::string theVersion;
std::string theMachine;
#if ( defined(__x86_64__) || defined(WIN64) )
bool theIs64bit = true;
#else
bool theIs64bit = false;
#endif
std::map<std::string, std::string> theEnv;

static bool theInitDone = false;

void
init( void )
{
	if ( theInitDone )
		return;
	theInitDone = true;

	struct utsname un;
	if ( uname( &un ) == 0 )
	{
		theSystem = un.sysname;
		theNode = un.nodename;
		theRelease = un.release;
		theVersion = un.version;
		theMachine = un.machine;
	}
	else
		throw std::system_error( errno, std::system_category(), "Unable to retrieve system information" );

	if ( ! environ )
		throw std::runtime_error( "Unable to retrieve program environment" );
	{
		
	}
	for ( char **p = environ; *p; ++p )
	{
		std::vector<std::string> v = String::split( *p, '=' );
		if ( v.size() == 2 )
			theEnv.emplace( std::make_pair( std::move( v[0] ), std::move( v[1] ) ) );
		else if ( v.size() > 2 )
		{
			std::string nval = std::move( v[1] );
			for ( size_t i = 2; i < v.size(); ++i )
			{
				nval.push_back( '=' );
				nval.append( v[i] );
			}
			theEnv.emplace( std::make_pair( std::move( v[0] ), std::move( nval ) ) );
		}
	}
}

} // empty namespace


////////////////////////////////////////


namespace OS
{


////////////////////////////////////////


const std::string &
system( void )
{
	init();
	return theSystem;
}

const std::string &
node( void )
{
	init();
	return theNode;
}

const std::string &
release( void )
{
	init();
	return theRelease;
}

const std::string &
version( void )
{
	init();
	return theVersion;
}

const std::string &
machine( void )
{
	init();
	return theMachine;
}

bool
is64bit( void )
{
	init();
	return theIs64bit;
}

const std::string &
getenv( const std::string &v )
{
	init();

	auto i = theEnv.find( v );
	if ( i != theEnv.end() )
		return i->second;
	return String::empty();
}

} // namespace OS


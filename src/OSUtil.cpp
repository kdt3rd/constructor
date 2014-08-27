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
#include <sys/utsname.h>
#include <system_error>
#include <mutex>
#include "LuaEngine.h"


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

static std::once_flag theNeedInit;

void
init( void )
{
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
}

} // empty namespace


////////////////////////////////////////


namespace OS
{


////////////////////////////////////////


const std::string &
system( void )
{
	std::call_once( theNeedInit, &init );
	return theSystem;
}

const std::string &
node( void )
{
	std::call_once( theNeedInit, &init );
	return theNode;
}

const std::string &
release( void )
{
	std::call_once( theNeedInit, &init );
	return theRelease;
}

const std::string &
version( void )
{
	std::call_once( theNeedInit, &init );
	return theVersion;
}

const std::string &
machine( void )
{
	std::call_once( theNeedInit, &init );
	return theMachine;
}

bool
is64bit( void )
{
	std::call_once( theNeedInit, &init );
	return theIs64bit;
}


////////////////////////////////////////


void
registerFunctions( void )
{
	Lua::Engine &eng = Lua::Engine::singleton();

}

} // namespace OS


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

#pragma once

#include <iostream>
#include <iomanip>
#include <sstream>
#include <stdexcept>


////////////////////////////////////////



#ifndef NDEBUG
namespace Debug
{
extern bool theDebugEnabled;
inline bool on( void ) { return theDebugEnabled; }
void enable( bool d );
}
#endif

namespace Verbose
{
extern bool theVerboseEnabled;
#ifdef NDEBUG
inline bool on( void ) { return theVerboseEnabled; }
#else
inline bool on( void ) { return theVerboseEnabled || Debug::on(); }
#endif
void enable( bool d );
}

namespace Quiet
{
extern bool theQuietEnabled;
inline bool on( void ) { return theQuietEnabled; }
void enable( bool d );
}

#ifdef NDEBUG
# define DEBUG( x )
#else
# define DEBUG( x ) if ( Debug::on() ) { std::cout << x << std::endl; }
#endif

#define VERBOSE( x ) if ( Verbose::on() ) { std::cout << x << std::endl; }
#define WARNING( x ) if ( ! Quiet::on() ) { std::cout << "WARNING: " << x << std::endl; }
#define ERROR( x ) { std::cout << "ERROR: " << x << std::endl; }

#define PRECONDITION( x, msg )							\
	{													\
		if ( !( x ) )									\
		{												\
			std::stringstream msgBuf;					\
			msgBuf << msg;								\
			throw std::runtime_error( msgBuf.str() );	\
		}												\
	}

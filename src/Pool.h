//
// Copyright (c) 2015 Kimball Thurston
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

#include <string>


////////////////////////////////////////


/// @brief Class Pool provides a simple definition for pooling compile
///        jobs.
///
/// When doing -j N in make, or in things like ninja, sometimes there
/// are tasks that use a lot of resources. This is a way to contain how
/// many are happening at once. Ninja also has a pre-defined pool called
/// console that allows interaction with the console, even in a parallel
/// compile environment (the parallel things in the background keep
/// happening, buffered!)
class Pool
{
public:
	Pool( std::string n, int jobs );
	~Pool( void );

	inline const std::string &getName( void ) const;
	inline int getMaxJobCount( void ) const;
	
private:
	std::string myName;
	int myJobCount;
};


////////////////////////////////////////


inline const std::string &Pool::getName( void ) const { return myName; }

inline int Pool::getMaxJobCount( void ) const { return myJobCount; }






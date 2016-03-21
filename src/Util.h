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

#pragma once

#include <vector>
#include <algorithm>


////////////////////////////////////////


namespace util
{

template <typename T>
void append( std::vector<T> &a, std::vector<T> &&b )
{
	if ( a.empty() )
		a = std::move( b );
	else if ( ! b.empty() )
	{
		a.reserve( a.size() + b.size() );
		std::move( std::begin( b ), std::end( b ), std::back_inserter( a ) );
		b.clear();
	}
}

template <typename T>
void append( std::vector<T> &a, const std::vector<T> &b )
{
	if ( a.empty() )
		a = b;
	else if ( ! b.empty() )
		a.insert( a.end(), b.begin(), b.end() );
}

} // namespace src




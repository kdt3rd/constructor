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

#include <cctype>
#include <string>
#include <vector>
#include <map>
#include <functional>


////////////////////////////////////////


namespace String
{

const std::string &empty( void );

void split_append( std::vector<std::string> &l, const std::string &s, const char sep );
std::vector<std::string> split( const std::string &s, const char sep );
std::vector<std::string> split_space_or_sep( const std::string &s, const char sep );

// splits as the shell might (splitting on spaces, but paying attention to quotes)
std::vector<std::string> shell_split( const std::string &s );

void strip( std::string &s );
// replaces all non-alpha-numeric values w/ _
void sanitize( std::string &s );

int versionCompare( const std::string &a, const std::string &b );

void substituteVariables( std::string &a, bool requireCurly, const std::map<std::string, std::string> &varTable );
void substituteVariables( std::string &a, bool requireCurly,
						  const std::function<const std::string &(const std::string&)> &varLookup );

inline bool
startsWith( const std::string &s, const std::string &prefix )
{
	std::string::size_type p = s.find( prefix );
	return p == 0;
}

	
} // namespace String



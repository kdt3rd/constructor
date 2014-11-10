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

#include "StrUtil.h"
#include <iostream>
#include <stack>


////////////////////////////////////////


namespace String
{


////////////////////////////////////////


const std::string &
empty( void )
{
	static const std::string theEmptyStr;
	return theEmptyStr;
}


////////////////////////////////////////


void
split_append( std::vector<std::string> &l, const std::string &s, const char sep )
{
	std::string::size_type last = s.find_first_not_of( sep, 0 );
	std::string::size_type cur = s.find_first_of( sep, last );
		
	while ( cur != std::string::npos || last < s.size() )
	{
		if ( cur != last )
			l.push_back( s.substr( last, cur - last ) );
		last = s.find_first_not_of( sep, cur );
		cur = s.find_first_of( sep, last );
	}
}


////////////////////////////////////////


std::vector<std::string>
split( const std::string &s, const char sep )
{
	std::vector<std::string> ret;
	split_append( ret, s, sep );
	return std::move( ret );
}


////////////////////////////////////////


std::vector<std::string>
split_space_or_sep( const std::string &s, const char sep )
{
	std::vector<std::string> ret;

	std::string::size_type last = 0;
	while ( last != s.size() && ( s[last] == sep || std::isspace( s[last] ) ) )
		++last;
	std::string::size_type cur = last;
	while ( cur != s.size() && s[cur] != sep && ! std::isspace( s[cur] ) )
		++cur;
		
	while ( cur != s.size() || last < s.size() )
	{
		if ( cur != last )
			ret.push_back( s.substr( last, cur - last ) );
		last = cur;
		while ( last != s.size() && ( s[last] == sep || std::isspace( s[last] ) ) )
			++last;
		cur = last;
		while ( cur != s.size() && s[cur] != sep && ! std::isspace( s[cur] ) )
			++cur;
	}

	return std::move( ret );
}


////////////////////////////////////////


namespace
{
std::string::size_type
advanceToEndQuote( const std::string &s, std::string::size_type quoteStart )
{
	std::string::size_type out = quoteStart;
	std::stack< std::pair<char, bool> > matchQ;
	matchQ.push( std::make_pair( s[quoteStart], false ) );
	bool inEscape = false;
	while ( ! matchQ.empty() )
	{
		++out;
		if ( out >= s.size() )
			throw std::runtime_error( "Unbalanced quotes in string, unable to skip" );

		if ( s[out] == '\\' )
			inEscape = ! inEscape;
		else if ( inEscape )
		{
			if ( s[out] == matchQ.top().first )
			{
				if ( matchQ.top().second )
					matchQ.pop();
				else
					matchQ.push( std::make_pair( s[out], true ) );
			}
			else if ( s[out] == '\'' || s[out] == '"' )
				matchQ.push( std::make_pair( s[out], false ) );
		}
		else if ( s[out] == matchQ.top().first )
		{
			if ( matchQ.top().second )
				throw std::runtime_error( "Unbalanced quotes in string, unable to skip" );
			matchQ.pop();
		}
	}
	if ( out < s.size() )
		++out;

	return out;
}
} // empty namespace

std::vector<std::string>
shell_split( const std::string &s )
{
	std::vector<std::string> ret;

	std::string::size_type last = 0;
	while ( last != s.size() && std::isspace( s[last] ) )
		++last;
	std::string::size_type cur = last;
	while ( cur != s.size() && ! std::isspace( s[cur] ) )
	{
		if ( s[cur] == '\'' || s[cur] == '"' )
			cur = advanceToEndQuote( s, cur );
		else
			++cur;
	}

	while ( cur != s.size() || last < s.size() )
	{
		if ( cur != last )
			ret.push_back( s.substr( last, cur - last ) );
		last = cur;
		while ( last != s.size() && std::isspace( s[last] ) )
			++last;
		cur = last;
		while ( cur != s.size() && ! std::isspace( s[cur] ) )
		{
			if ( s[cur] == '\'' || s[cur] == '"' )
				cur = advanceToEndQuote( s, cur );
			else
				++cur;
		}
	}

	return std::move( ret );
}


////////////////////////////////////////


void
strip( std::string &s )
{
	if ( s.empty() )
		return;

	std::string::size_type start = 0;
	while ( start < s.size() && std::isspace( s[start] ) )
		++start;

	std::string::size_type end = s.size();
	while ( end > start && std::isspace( s[end - 1] ) )
		--end;

	if ( start == end )
	{
		s.clear();
		return;
	}

	if ( end < s.size() )
		s.erase( end );
	if ( start > 0 )
		s.erase( 0, start );
}


////////////////////////////////////////


void
sanitize( std::string &s )
{
	if ( s.empty() )
		return;

	if ( ! std::isalnum( s[0] ) )
		s.insert( s.begin(), '_' );

	for ( auto &i: s )
	{
		if ( ! std::isalnum( i ) )
			i = '_';
	}
}


////////////////////////////////////////


int
versionCompare( const std::string &a, const std::string &b )
{
	if ( a == b )
		return 0;

	std::string::size_type one = 0, two = 0;

	while ( one < a.size() && two < b.size() )
	{
		while ( one < a.size() && ! std::isalnum( a[one] ) )
			++one;
		while ( two < b.size() && ! std::isalnum( b[two] ) )
			++two;

		if ( one == a.size() || two == b.size() )
			break;

		bool isnum = false;
		std::string::size_type p1 = one, p2 = two;
		if ( std::isdigit( a[one] ) )
		{
			while ( p1 < a.size() && std::isdigit( a[p1] ) )
				++p1;
			while ( p2 < b.size() && std::isdigit( b[p2] ) )
				++p2;
			isnum = true;
		}
		else
		{
			while ( p1 < a.size() && std::isalpha( a[p1] ) )
				++p1;
			while ( p2 < b.size() && std::isalpha( b[p2] ) )
				++p2;
		}
		// handle case where current segments are of
		// differing "types" - numeric segments are newer
		// than alpha segments
		if ( two == p2 )
			return isnum ? 1 : -1;

		size_t len1 = p1 - one;
		size_t len2 = p2 - two;
		if ( isnum )
		{
			while ( one < a.size() && a[one] == '0' )
				++one;
			while ( two < b.size() && b[two] == '0' )
				++two;

			// if there's a chunk with more digits, it wins
			len1 = p1 - one;
			len2 = p2 - two;
			if ( len1 > len2 )
				return 1;
			if ( len2 > len1 )
				return -1;
		}

		int segrc = a.compare( one, len1, b, two, len2 );
		// only stop if we aren't equal
		if ( segrc )
			return segrc < 1 ? -1 : 1;
		one = p1;
		two = p2;
	}

	// each segment compared identical, but a & b had
	// different separators
	if ( one == a.size() && two == b.size() )
		return 0;

	// whichever still has characters left determines
	if ( one == a.size() )
		return -1;
	return 1;
}


////////////////////////////////////////


namespace {
const std::string &
mapLookup( const std::string &name, const std::map<std::string, std::string> &varTable )
{
	auto i = varTable.find( name );
	if ( i != varTable.end() )
		return i->second;

	std::cout << "WARNING: Variable '" << name << "' undefined" << std::endl;

	return String::empty();
}
} // empty namespace

void substituteVariables( std::string &val, bool requireCurly,
						  const std::map<std::string, std::string> &varTable )
{
	substituteVariables( val, requireCurly,
						 std::bind( &mapLookup, std::placeholders::_1, std::cref( varTable ) ) );
}


////////////////////////////////////////


void substituteVariables( std::string &val, bool requireCurly,
						  const std::function<const std::string &(const std::string &)> &varLookup )
{
	std::string::size_type i = val.find_first_of( '$' );

	while ( i != std::string::npos )
	{
		std::string::size_type varStart = i;
		++i;
		if ( i == val.size() )
			break;

		if ( val[i] == '$' )
		{
			i = val.find_first_of( '$', i + 1 );
			continue;
		}

		std::string::size_type endVar = std::string::npos;
		if ( val[i] == '{' )
		{
			++i;
			endVar = val.find_first_of( '}', i );
			if ( endVar == std::string::npos )
			{
				std::cerr << "WARNING: Variable marker not terminated in '" << val << "'" << std::endl;
				return;
			}
		}
		else if ( ! requireCurly && ( std::isalpha( val[i] ) || val[i] == '_' ) )
		{
			endVar = i;
			while ( endVar != val.size() && ( std::isalnum( val[endVar] ) || val[endVar] == '_' ) )
				++endVar;
		}

		if ( endVar != std::string::npos )
		{
			std::string varname = val.substr( i, endVar - i );
			const std::string &vv = varLookup( varname );
			val.replace( varStart, endVar - varStart, vv );
			i = endVar;
		}

		i = val.find_first_of( '$', i );
	}
}


////////////////////////////////////////


} // namespace String


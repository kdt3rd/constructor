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

#include "variable.h"
#include <stdexcept>
#include <algorithm>
#include <cstdlib>


////////////////////////////////////////


variable::variable( std::string n, bool checkEnv )
		: myName( std::move( n ) )
{
	if ( checkEnv )
	{
		// technically not thread safe, but are we setting
		// the environment anywhere, or for that matter, using
		// threads?
		const char *eVal = getenv( n.c_str() );
		if ( eVal && eVal[0] != '\0' )
			myValues.push_back( std::string( eVal ) );
	}
}


////////////////////////////////////////


variable::~variable( void )
{
}


////////////////////////////////////////


void
variable::clear( void )
{
	myValues.clear();
}


////////////////////////////////////////


void
variable::add( std::string v )
{
	if ( ! v.empty() )
		myValues.emplace_back( std::move( v ) );
}


////////////////////////////////////////


void
variable::add( std::vector<std::string> v )
{
	if ( myValues.empty() )
	{
		myValues = std::move( v );
	}
	else
	{
        myValues.reserve( myValues.size() + v.size() );
        std::move( std::begin(v), std::end(v), std::back_inserter(myValues) );
        v.clear();
	}
}


////////////////////////////////////////


void
variable::reset( std::string v )
{
	myValues.clear();
	if ( ! v.empty() )
		myValues.emplace_back( std::move( v ) );
}


////////////////////////////////////////


void
variable::reset( std::vector<std::string> v )
{
	myValues = std::move( v );
}


////////////////////////////////////////


std::string
variable::value( const deref_func &deref ) const
{
	std::string ret;
	if ( myInherit )
		ret = deref( *this );

	for ( const auto &i: myValues )
	{
		if ( i.val.empty() )
			continue;
		if ( ! ret.empty() )
			ret.push_back( ' ' );
		if ( i.is_variable )
			ret.append( deref( i.val ) );
		else
			ret.append( i.val );
	}

	return std::move( ret );
}


////////////////////////////////////////


std::string
variable::prepended_value( const std::string &prefix ) const
{
	std::string ret;
	for ( const auto &i: myValues )
	{
		if ( i.empty() )
			continue;

		if ( ! ret.empty() )
			ret.push_back( ' ' );

		if ( i.is_variable )
		{
			ret.append( deref( i.val ) );
		}
		else
		{
			if ( i.find( prefix ) != 0 )
				ret.append( prefix );

			ret.append( i );
		}
	}

	return std::move( ret );
}


////////////////////////////////////////







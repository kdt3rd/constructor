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

#include "Variable.h"
#include <stdexcept>
#include <algorithm>
#include <cstdlib>


////////////////////////////////////////


Variable::Variable( std::string n, bool checkEnv )
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


Variable::Variable( const std::string &n, const std::string &val )
		: myName( n )
{
	myValues.push_back( val );
}


////////////////////////////////////////


void
Variable::setToolTag( std::string tag )
{
	myToolTag = std::move( tag );
}


////////////////////////////////////////


void
Variable::clear( void )
{
	myValues.clear();
	myCachedValue.clear();
}


////////////////////////////////////////


void
Variable::add( std::string v )
{
	if ( ! v.empty() )
		myValues.emplace_back( std::move( v ) );
	myCachedValue.clear();
}


////////////////////////////////////////


void
Variable::add( std::vector<std::string> v )
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
	myCachedValue.clear();
}


////////////////////////////////////////


void
Variable::addIfMissing( const std::string &v )
{
	if ( v.empty() )
		return;

	for ( const std::string &i: myValues )
	{
		if ( i == v )
			return;
	}

	myValues.push_back( v );
	myCachedValue.clear();
}


////////////////////////////////////////


void
Variable::addIfMissing( const std::vector<std::string> &v )
{
	for ( const std::string &i: v )
		addIfMissing( i );
}


////////////////////////////////////////


void
Variable::moveToEnd( const std::string &v )
{
	if ( v.empty() )
		return;

	auto i = myValues.begin();
	while ( true )
	{
		if ( i == myValues.end() )
			break;

		if ( (*i) == v )
		{
			myValues.erase( i );
			i = myValues.begin();
			continue;
		}
		++i;
	}

	myValues.push_back( v );
	myCachedValue.clear();
}


////////////////////////////////////////


void
Variable::moveToEnd( const std::vector<std::string> &v )
{
	for ( const std::string &i: v )
		moveToEnd( i );
}


////////////////////////////////////////


void
Variable::removeDuplicatesKeepLast( void )
{
	// O(n^2) but it's simple
	for ( size_t i = 0; i != myValues.size(); ++i )
	{
		bool kill = false;
		const std::string &cur = myValues[i];
		for ( size_t j = i + 1; j < myValues.size(); ++j )
		{
			if ( myValues[j] == cur )
			{
				kill = true;
				break;
			}
		}

		if ( kill )
		{
			typedef typename std::vector<std::string>::iterator::difference_type dt_t;
			myValues.erase( myValues.begin() + static_cast<dt_t>( i ) );
			--i;
		}
	}
}


////////////////////////////////////////


void
Variable::reset( std::string v )
{
	myValues.clear();
	if ( ! v.empty() )
		myValues.emplace_back( std::move( v ) );
	myCachedValue.clear();
}


////////////////////////////////////////


void
Variable::reset( std::vector<std::string> v )
{
	myValues = std::move( v );
	myCachedValue.clear();
}


////////////////////////////////////////


const std::string &
Variable::value( void ) const
{
	myCachedValue.clear();
	if ( myInherit )
		myCachedValue = "${" + myName + "}";

	for ( const auto &i: myValues )
	{
		if ( i.empty() )
			continue;
		if ( ! myCachedValue.empty() )
			myCachedValue.push_back( ' ' );
		myCachedValue.append( i );
	}

	return myCachedValue;
}


////////////////////////////////////////


std::string
Variable::prepended_value( const std::string &prefix ) const
{
	std::string ret;
	if ( myInherit )
		ret = "${" + myName + "}";

	for ( const auto &i: myValues )
	{
		if ( i.empty() )
			continue;

		if ( ! ret.empty() )
			ret.push_back( ' ' );

		if ( i[0] != '$' && i.find( prefix ) != 0 )
			ret.append( prefix );

		ret.append( i );
	}

	return std::move( ret );
}


////////////////////////////////////////


const Variable &
Variable::nil( void )
{
	static const Variable theNilVal = Variable( std::string() );
	return theNilVal;
}



////////////////////////////////////////







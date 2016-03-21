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
#include "Util.h"
#include "Debug.h"
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


Variable::~Variable( void )
{
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
	mySystemValues.clear();
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
	util::append( myValues, std::move( v ) );
	myCachedValue.clear();
}


////////////////////////////////////////


void
Variable::addPerSystem( const std::string &s, std::string v )
{
	if ( v.empty() )
		return;

	std::vector<std::string> &sE = mySystemValues[s];
	sE.emplace_back( std::move( v ) );
	myCachedValue.clear();
}


////////////////////////////////////////


void
Variable::addPerSystem( const std::string &s, std::vector<std::string> v )
{
	std::vector<std::string> &sE = mySystemValues[s];
	util::append( sE, std::move( v ) );
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
Variable::addIfMissingSystem( const std::string &s, const std::string &v )
{
	if ( v.empty() )
		return;

	std::vector<std::string> &sE = mySystemValues[s];
	for ( const std::string &i: sE )
	{
		if ( i == v )
			return;
	}

	sE.push_back( std::move( v ) );
	myCachedValue.clear();
}


////////////////////////////////////////


void
Variable::addIfMissingSystem( const std::string &s, const std::vector<std::string> &v )
{
	for ( const std::string &i: v )
		addIfMissingSystem( s, i );
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
Variable::removeDuplicates( const std::map<std::string, bool> &prefixDisposition )
{
	typedef typename std::vector<std::string>::iterator::difference_type dt_t;

	// O(n^2) but it's simple
	for ( size_t i = 0; i < myValues.size(); ++i )
	{
		const std::string &cur = myValues[i];
		bool inPref = false;
		bool first = true;
		for ( auto &x: prefixDisposition )
		{
			if ( cur.find( x.first ) == 0 )
			{
				inPref = true;
				first = x.second;
				break;
			}
		}
		// entry doesn't have a desired prefix,
		// skip this one
		if ( ! inPref )
			continue;

		if ( first )
		{
			for ( size_t j = i + 1; j < myValues.size(); ++j )
			{
				if ( myValues[j] == cur )
				{
					myValues.erase( myValues.begin() + static_cast<dt_t>( j ) );
					--j;
				}
			}
		}
		else
		{
			bool kill = false;
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
				myValues.erase( myValues.begin() + static_cast<dt_t>( i ) );
				--i;
			}
		}
	}
}


////////////////////////////////////////


void
Variable::reset( std::string v )
{
	clear();
	if ( ! v.empty() )
		myValues.emplace_back( std::move( v ) );
}


////////////////////////////////////////


void
Variable::reset( std::vector<std::string> v )
{
	clear();
	myValues = std::move( v );
}


////////////////////////////////////////


void
Variable::merge( const Variable &other )
{
	addIfMissing( other.myValues );
	for ( auto i: other.mySystemValues )
		addIfMissingSystem( i.first, i.second );

	myCachedValue.clear();
	myCachedSystem.clear();
}


////////////////////////////////////////


const std::string &
Variable::value( const std::string &sys ) const
{
	if ( ! myCachedValue.empty() && sys == myCachedSystem )
		return myCachedValue;

	myCachedValue.clear();
	if ( myInherit )
		myCachedValue = "$" + myName;
	
	for ( const auto &i: myValues )
	{
		if ( i.empty() )
			continue;
		if ( ! myCachedValue.empty() )
			myCachedValue.push_back( ' ' );
		myCachedValue.append( i );
	}

	auto x = mySystemValues.find( sys );
	if ( x != mySystemValues.end() )
	{
		for ( const auto &i: x->second )
		{
			if ( i.empty() )
				continue;
			if ( ! myCachedValue.empty() )
				myCachedValue.push_back( ' ' );
			myCachedValue.append( i );
		}
	}
	myCachedSystem = sys;
	return myCachedValue;
}


////////////////////////////////////////


std::string
Variable::prepended_value( const std::string &prefix, const std::string &sys ) const
{
	std::string ret;
	if ( myInherit )
		ret = "$" + myName;
	
	for ( const auto &i: myValues )
	{
		if ( i.empty() )
			continue;

		if ( ! ret.empty() )
			ret.push_back( ' ' );

		if ( i.find( prefix ) != 0 )
			ret.append( prefix );

		ret.append( i );
	}

	auto x = mySystemValues.find( sys );
	if ( x != mySystemValues.end() )
	{
		for ( const auto &i: x->second )
		{
			if ( i.empty() )
				continue;

			if ( ! ret.empty() )
				ret.push_back( ' ' );

			if ( i.find( prefix ) != 0 )
				ret.append( prefix );

			ret.append( i );
		}
	}

	return ret;
}


////////////////////////////////////////


const Variable &
Variable::nil( void )
{
	static const Variable theNilVal = Variable( std::string() );
	return theNilVal;
}


////////////////////////////////////////



void merge( VariableSet &vs, const VariableSet &other )
{
	if ( vs.empty() )
		vs = other;
	else
	{
		for ( auto i: other )
		{
			auto cur = vs.find( i.first );
			if ( cur == vs.end() )
			{
				vs.emplace( std::make_pair( i.first, i.second ) );
			}
			else
				cur->second.merge( i.second );
		}
	}
}


////////////////////////////////////////







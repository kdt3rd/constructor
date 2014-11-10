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

#include "Rule.h"
#include "StrUtil.h"
#include "Debug.h"


////////////////////////////////////////


Rule::Rule( void )
{
}


////////////////////////////////////////


Rule::Rule( std::string tag, std::string desc )
		: myTag( std::move( tag ) ), myDesc( std::move( desc ) )
{
}


////////////////////////////////////////


Rule::Rule( Rule &&r )
		: myTag( std::move( r.myTag ) ), myDesc( std::move( r.myDesc ) ),
		  myDepFile( std::move( r.myDepFile ) ),
		  myDepStyle( std::move( r.myDepStyle ) ),
		  myJobPool( std::move( r.myJobPool ) ),
		  myCommand( std::move( r.myCommand ) ),
		  myVariables( std::move( r.myVariables ) ),
		  myOutputRestat( r.myOutputRestat )
{
}


////////////////////////////////////////


Rule::~Rule( void )
{
}


////////////////////////////////////////


Rule &
Rule::operator=( const Rule &r )
{
	if ( &r != this )
	{
		myTag = r.myTag;
		myDesc = r.myDesc;
		myDepFile = r.myDepFile;
		myDepStyle = r.myDepStyle;
		myJobPool = r.myJobPool;
		myCommand = r.myCommand;
		myVariables = r.myVariables;
		myOutputRestat = r.myOutputRestat;
	}

	return *this;
}


////////////////////////////////////////


Rule &
Rule::operator=( Rule && r )
{
	myTag = std::move( r.myTag );
	myDesc = std::move( r.myDesc );
	myDepFile = std::move( r.myDepFile );
	myDepStyle = std::move( r.myDepStyle );
	myJobPool = std::move( r.myJobPool );
	myCommand = std::move( r.myCommand );
	myVariables = std::move( r.myVariables );
	myOutputRestat = r.myOutputRestat;
	return *this;
}


////////////////////////////////////////


void
Rule::setCommand( const std::string &c )
{
	myCommand = String::shell_split( c );
}


////////////////////////////////////////


void
Rule::setCommand( const std::vector<std::string> &c )
{
	myCommand = c;
}


////////////////////////////////////////


void
Rule::setCommand( std::vector<std::string> &&c )
{
	myCommand = std::move( c );
}


////////////////////////////////////////


void
Rule::addToCommand( const std::vector<std::string> &c )
{
	myCommand.insert( myCommand.end(), c.begin(), c.end() );
}


////////////////////////////////////////


void
Rule::setVariable( const std::string &n, const std::string &v )
{
	myVariables[n] = v;
}


////////////////////////////////////////


std::string
Rule::getCommand( void ) const
{
	std::string ret;
	for ( const std::string &i: myCommand )
	{
		if ( i.empty() )
			continue;

		if ( ! ret.empty() )
			ret.push_back( ' ' );

		ret.append( i );
	}
	return std::move( ret );
}


////////////////////////////////////////


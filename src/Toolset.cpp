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

#include "Toolset.h"
#include "StrUtil.h"
#include "Debug.h"


////////////////////////////////////////


Toolset::Toolset( std::string name )
		: myName( std::move( name ) )
{
}


////////////////////////////////////////


Toolset::~Toolset( void )
{
}


////////////////////////////////////////


void
Toolset::addLibSearchPath( const std::string &p )
{
	String::split_append( myLibPath, p, ':' );
}


////////////////////////////////////////


void
Toolset::addPkgSearchPath( const std::string &p )
{
	String::split_append( myPkgPath, p, ':' );
}


////////////////////////////////////////


void
Toolset::addTool( const std::shared_ptr<Tool> &t )
{
	if ( ! t )
		return;

	VERBOSE( "Adding tool " << t->getName() << " to toolset " << getName() );
	myTools[t->getName()] = t;
}


////////////////////////////////////////


bool
Toolset::hasTool( const std::shared_ptr<Tool> &t ) const
{
	if ( t )
	{
		auto h = myTools.find( t->getName() );
		return h != myTools.end();
	}
	return false;
}


////////////////////////////////////////


std::shared_ptr<Tool>
Toolset::findTool( const std::string &name ) const
{
	std::shared_ptr<Tool> ret;
	auto i = myTools.find( name );
	if ( i != myTools.end() )
		ret = i->second;

	return ret;
}



////////////////////////////////////////







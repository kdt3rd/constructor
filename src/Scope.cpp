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

#include "Scope.h"
#include <stack>
#include <stdexcept>


////////////////////////////////////////


namespace
{
static std::shared_ptr<Scope> theRootScope = std::make_shared<Scope>( std::shared_ptr<Scope>() );
static std::stack< std::shared_ptr<Scope> > theScopes;
} // empty namespace


////////////////////////////////////////


Scope::Scope( std::shared_ptr<Scope> parent )
		: myParent( parent )
{
}


////////////////////////////////////////


Scope::~Scope( void )
{
}


////////////////////////////////////////


std::shared_ptr<Scope>
Scope::newSubScope( bool inherits )
{
	mySubScopes.emplace_back( std::make_shared<Scope>( shared_from_this() ) );
	if ( inherits )
	{
		
	}
	return mySubScopes.back();
}


////////////////////////////////////////


void
Scope::addTool( const std::shared_ptr<Tool> &t )
{
	myTools.push_back( t );
}


////////////////////////////////////////


std::shared_ptr<Tool>
Scope::findTool( const std::string &extension ) const
{
	for ( auto &t: myTools )
		if ( t->handlesExtension( extension ) )
			return t;

	if ( myInheritParentScope )
		return parent()->findTool( extension );
	return std::shared_ptr<Tool>();
}


////////////////////////////////////////


void
Scope::useToolset( const std::string &tset )
{
}


////////////////////////////////////////


void
Scope::addItem( const ItemPtr &i )
{
	myItems.push_back( i );
}


////////////////////////////////////////


Scope &
Scope::root( void )
{
	return *(theRootScope);
}


////////////////////////////////////////


Scope &
Scope::current( void )
{
	if ( theScopes.empty() )
		return *(theRootScope);
	return *(theScopes.top());
}


////////////////////////////////////////


void
Scope::pushScope( const std::shared_ptr<Scope> &scope )
{
	theScopes.push( scope );
}


////////////////////////////////////////


void
Scope::popScope( void )
{
	if ( theScopes.empty() )
		throw std::runtime_error( "unbalanced Scope management -- too many pops for pushes" );
	theScopes.pop();
}


////////////////////////////////////////




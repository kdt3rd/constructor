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
#include "Tool.h"
#include "Debug.h"
#include "ScopeGuard.h"
#include "Configuration.h"
#include "TransformSet.h"
#include "DefaultTools.h"


////////////////////////////////////////


namespace
{

static std::shared_ptr<Scope> theRootScope;
static std::stack< std::shared_ptr<Scope> > theScopes;

} // empty namespace


////////////////////////////////////////


Scope::Scope( std::shared_ptr<Scope> parent )
		: myParent( parent )
{
	addDefaultTools();
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
		mySubScopes.back()->grabScope( *this );

	return mySubScopes.back();
}


////////////////////////////////////////


void
Scope::addTool( const std::shared_ptr<Tool> &t )
{
	bool found = false;
	for ( std::shared_ptr<Tool> &i: myTools )
	{
		if ( i->getTag() == t->getTag() && i->getName() == t->getName() )
		{
			i = t;
			auto ti = myTagMap.find( t->getTag() );
			if ( ti != myTagMap.end() )
			{
				for ( std::shared_ptr<Tool> &v: ti->second )
				{
					if ( v->getName() == t->getName() )
					{
						VERBOSE( "Overriding tool '" << t->getName() << "'..." );
						v = t;
						found = true;
						break;
					}
				}

				if ( found )
					break;
			}
		}
	}

	if ( ! found )
	{
		myTools.push_back( t );
		myTagMap[t->getTag()].push_back( t );
	}
}


////////////////////////////////////////


std::shared_ptr<Tool>
Scope::findTool( const std::string &extension ) const
{
	for ( auto &t: myTools )
		if ( t->handlesExtension( extension ) )
			return t;

	if ( myInheritParentScope )
		return getParent()->findTool( extension );

	return std::shared_ptr<Tool>();
}


////////////////////////////////////////


void
Scope::addToolSet( const std::string &name,
				   const std::vector<std::string> &tools )
{
	if ( myToolSets.find( name ) != myToolSets.end() )
		throw std::runtime_error( "ToolSet '" + name + "' already defined" );

	myToolSets[name] = tools;
}


////////////////////////////////////////


void
Scope::useToolSet( const std::string &tset )
{
	myEnabledToolsets.push_back( tset );
}


////////////////////////////////////////


void
Scope::addItem( const ItemPtr &i )
{
	myItems.push_back( i );
}


////////////////////////////////////////


void
Scope::transform( TransformSet &xform,
				  const Configuration &conf ) const
{
	for ( const std::shared_ptr<Scope> &ss: mySubScopes )
	{
		std::shared_ptr<TransformSet> sx = std::make_shared<TransformSet>( xform.getOutDir() );
		ss->transform( *sx, conf );
		xform.addChildScope( sx );
	}

	for ( auto i: myTagMap )
	{
		if ( i.second.size() == 1 )
			xform.addTool( i.second.front() );
		else
		{
			std::shared_ptr<Tool> found;
			for ( const std::shared_ptr<Tool> &tagTool: i.second )
			{
				for ( const std::string &ts: myEnabledToolsets )
				{
					auto tsi = myToolSets.find( ts );
					if ( tsi != myToolSets.end() )
					{
						const std::vector<std::string> &tlist = tsi->second;
						for ( const std::string &tool: tlist )
						{
							if ( tagTool->getName() == tool )
							{
								if ( found )
									throw std::runtime_error( "Tool '" + tool + "' conflicts with tool '" + found->getName() + "' previously matched to active toolset" );

								found = tagTool;
							}
						}
					}
				}
			}
			if ( found )
				xform.addTool( found );
			else
				throw std::runtime_error( "Unable to find active tool for tool tag '" + i.first + "'" );
		}
	}

	xform.mergeVariables( myVariables );
	xform.mergeOptions( myOptions );
	xform.mergeOptions( conf.getChoices() );

	for ( const ItemPtr &i: myItems )
		i->transform( xform );

	for ( const ItemPtr &i: myItems )
		i->copyDependenciesToBuild( xform );
}


////////////////////////////////////////


void
Scope::grabScope( const Scope &o )
{
	myToolSets = o.myToolSets;
	myTagMap = o.myTagMap;
	myTools = o.myTools;
	myEnabledToolsets = o.myEnabledToolsets;
	myExtensionMap = o.myExtensionMap;

	myVariables = o.myVariables;
}


////////////////////////////////////////


void
Scope::addDefaultTools( void )
{
#ifdef WIN32
	throw std::runtime_error( "Not yet implemented" );
#endif

	DefaultTools::checkAndAddCFamilies( *this );
}


////////////////////////////////////////


Scope &
Scope::root( void )
{
	if ( ! theRootScope )
		theRootScope = std::make_shared<Scope>();

	return *(theRootScope);
}


////////////////////////////////////////


Scope &
Scope::current( void )
{
	if ( theScopes.empty() )
		return root();
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




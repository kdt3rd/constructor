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
#include "Util.h"
#include "Debug.h"
#include "ScopeGuard.h"
#include "Configuration.h"
#include "TransformSet.h"
#include "DefaultTools.h"
#include "Configuration.h"


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
	if ( ! parent )
		addDefaultTools();
}


////////////////////////////////////////


std::shared_ptr<Scope>
Scope::newSubScope( bool inherits )
{
	mySubScopes.emplace_back( std::make_shared<Scope>( shared_from_this() ) );
	if ( inherits )
		mySubScopes.back()->grabScope( *this );
	else
		mySubScopes.back()->addDefaultTools();

	return mySubScopes.back();
}


////////////////////////////////////////


void
Scope::removeSubScope( const std::shared_ptr<Scope> &c )
{
	for ( auto i = mySubScopes.begin(); i != mySubScopes.end(); ++i )
	{
		if ( (*i) == c )
		{
			mySubScopes.erase( i );
			return;
		}
	}
}


////////////////////////////////////////


bool
Scope::checkAdopt( const std::shared_ptr<Scope> &child )
{
	if ( child->myVariables == myVariables &&
		 child->myOptions == myOptions &&
		 child->myToolSets == myToolSets &&
		 child->myEnabledToolsets == myEnabledToolsets &&
		 child->myExtensionMap == myExtensionMap &&
		 child->myPools == myPools )
	{
		if ( child->myTools != myTools )
		{
			// the child added some tools, pull them back
			for ( auto &t: child->myTools )
			{
				bool found = false;
				for ( auto &mt: myTools )
				{
					if ( mt == t )
					{
						found = true;
						break;
					}
				}
				if ( ! found )
					addTool( t );
			}
		}

		util::append( myItems, child->myItems );
		for ( auto i = mySubScopes.begin(); i != mySubScopes.end(); ++i )
		{
			if ( (*i) == child )
			{
				mySubScopes.erase( i );
				break;
			}
		}
		for ( auto i = child->mySubScopes.begin(); i != child->mySubScopes.end(); ++i )
		{
			(*i)->setParent( shared_from_this() );
			mySubScopes.push_back( (*i) );
		}

		return true;
	}

	return false;
}


////////////////////////////////////////


void
Scope::addPool( const std::shared_ptr<Pool> &p )
{
	for ( std::shared_ptr<Pool> &i: myPools )
	{
		if ( i->getName() == p->getName() )
			throw std::runtime_error( "Duplicate pool definition found" );
	}
	myPools.push_back( p );
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
	return std::shared_ptr<Tool>();
}


////////////////////////////////////////


void
Scope::addToolSet( const std::shared_ptr<Toolset> &ts )
{
	if ( ! ts )
		return;

	if ( myToolSets.find( ts->getName() ) != myToolSets.end() )
		throw std::runtime_error( "ToolSet '" + ts->getName() + "' already defined" );

	myToolSets[ts->getName()] = ts;
}


////////////////////////////////////////


void
Scope::useToolSet( const std::string &tset )
{
	auto i = myToolSets.find( tset );
	if ( i == myToolSets.end() )
		throw std::runtime_error( "Unable to find toolset '" + tset + "' definition" );

	std::shared_ptr<Toolset> ts = i->second;

	bool added = false;
	// remove any toolset that conflicts
	for ( auto &i: myEnabledToolsets )
	{
		if ( i == ts )
		{
			// already using this toolset
			added = true;
			break;
		}

		if ( i->getTag() == ts->getTag() )
		{
			VERBOSE( "Replacing toolset '" << i->getName() << "' with '" << tset << "'" );
			added = true;
			i = ts;
		}
	}
	if ( ! added )
		myEnabledToolsets.push_back( ts );
}


////////////////////////////////////////


std::shared_ptr<Toolset>
Scope::findToolSet( const std::string &tset )
{
	auto i = myToolSets.find( tset );
	if ( i == myToolSets.end() )
		return std::shared_ptr<Toolset>();
	return i->second;
}


////////////////////////////////////////


void
Scope::modifyActive( std::vector< std::shared_ptr<Toolset> > &tsets ) const
{
	if ( myEnabledToolsets.empty() )
		return;

	for ( const auto &ts: myEnabledToolsets )
	{
		bool added = false;
		// remove any toolset that conflicts
		for ( auto &i: tsets )
		{
			if ( i->getTag() == ts->getTag() )
			{
				VERBOSE( "Replacing toolset '" << i->getName() << "' with '" << ts->getName() << "' for current configuration" );
				added = true;
				i = ts;
			}
		}
		if ( ! added )
			tsets.push_back( ts );
	}
}


////////////////////////////////////////


void
Scope::addItem( const ItemPtr &i )
{
	// NB: we use a vector here and prevent multiple inserts such that
	// we preserve ordering. if we used a set, it would be ordered
	// randomly based on memory pointer allocated
	for ( auto &m: myItems )
	{
		if ( m == i )
			return;
	}
	myItems.push_back( i );
}


////////////////////////////////////////


void
Scope::removeItem( const ItemPtr &i )
{
	for ( auto m = myItems.begin(); m != myItems.end(); ++m )
	{
		if ( (*m) == i )
		{
			myItems.erase( m );
			return;
		}
	}
}

////////////////////////////////////////


void
Scope::transform( TransformSet &xform,
				  const Configuration &conf ) const
{
	DEBUG( "transform Scope..." );
	for ( const std::shared_ptr<Scope> &ss: mySubScopes )
	{
		std::shared_ptr<TransformSet> sx = std::make_shared<TransformSet>( xform.getOutDir(), conf.getSystem() );
		ss->transform( *sx, conf );
		xform.addChildScope( sx );
	}

	for ( auto &p: myPools )
		xform.addPool( p );

	std::vector< std::shared_ptr<Toolset> > actTset = myEnabledToolsets;
	conf.getPseudoScope().modifyActive( actTset );

	std::vector<std::string> lsearch;
	std::vector<std::string> psearch;
	for ( auto &ts: actTset )
	{
		util::append( lsearch, ts->getLibSearchPath() );
		util::append( psearch, ts->getPkgSearchPath() );
	}

	xform.setLibSearchPath( lsearch );
	xform.setPkgSearchPath( psearch );

	for ( auto &i: myTagMap )
	{
		if ( i.second.size() == 1 )
			xform.addTool( i.second.front() );
		else
		{
			std::shared_ptr<Tool> found;
			for ( const std::shared_ptr<Tool> &tagTool: i.second )
			{
				for ( const std::shared_ptr<Toolset> &ts: actTset )
				{
					VERBOSE( "Checking if " << tagTool->getName() << " is in " << ts->getName() );
					if ( ts->hasTool( tagTool ) )
					{
						VERBOSE( " --> YES" );
						if ( found )
							throw std::runtime_error( "Tool '" + tagTool->getName() + "' conflicts with tool '" + found->getName() + "' previously matched to a different active toolset" );

						found = tagTool;
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
	xform.mergeVariables( conf.getPseudoScope().getVars() );
	xform.mergeOptions( myOptions );
	xform.mergeOptions( conf.getPseudoScope().getOptions() );

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
	myPools = o.myPools;

	myVariables = o.myVariables;
	myOptions = o.myOptions;
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
	// check to see if we need to add it to the
	// pseudo scope in the configuration
	if ( Configuration::haveAny() && ! Configuration::haveDefault() )
		return Configuration::last().getPseudoScope();

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
Scope::popScope( bool adopt )
{
	if ( theScopes.empty() )
		throw std::runtime_error( "unbalanced Scope management -- too many pops for pushes" );
	if ( adopt )
	{
		std::shared_ptr<Scope> t = theScopes.top();
		std::shared_ptr<Scope> p = t->getParent();
		p->checkAdopt( t );
	}

	theScopes.pop();
}


////////////////////////////////////////




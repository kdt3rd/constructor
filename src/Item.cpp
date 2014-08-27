// Item.cpp -*- C++ -*-

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

#include "Item.h"
#include <algorithm>
#include "Scope.h"


////////////////////////////////////////

namespace {

static Item::ID theLastID = 1;
static std::map<std::string, Item *> theItemsByName;
static std::map<Item::ID, Item *> theItemsByID;
static std::map<std::string, std::vector<Item *>> theUnresolvedBackPointers;

static inline void
registerItem( Item *i )
{
	theItemsByName[i->name()] = i;
	theItemsByID[i->id()] = i;
	auto u = theUnresolvedBackPointers.find( i->name() );
	if ( u != theUnresolvedBackPointers.end() )
	{
		for ( auto &i: u->second )
			i->update_dependency( i->name(), i->id() );
	}
}

static inline void
unregisterItem( Item *i )
{
	auto x = theItemsByName.find( i->name() );
	if ( x != theItemsByName.end() )
		theItemsByName.erase( x );

	auto y = theItemsByID.find( i->id() );
	if ( y != theItemsByID.end() )
		theItemsByID.erase( y );
}

static inline Item::ID
findItem( const std::string &name )
{
	auto x = theItemsByName.find( name );
	if ( x != theItemsByName.end() )
		return x->second->id();
	return Item::UNKNOWN;
}

static inline Item *
retrieveItem( Item::ID i )
{
	auto y = theItemsByID.find( i );
	if ( y != theItemsByID.end() )
		return y->second;
	return nullptr;
}

} // empty namespace


////////////////////////////////////////


Item::Item( const std::string &name )
		: myID( theLastID++ ), myName( name ), myScope( Scope::currentScope() )
{
	registerItem( this );
}


////////////////////////////////////////


Item::Item( std::string &&name )
		: myID( theLastID++ ), myName( std::move( name ) ), myScope( Scope::currentScope() )
{
	registerItem( this );
}


////////////////////////////////////////


Item::~Item( void )
{
	unregisterItem( this );
}


////////////////////////////////////////


void
Item::add_dependency( DependencyType dt, ID otherObj )
{
	Item *i = retrieveItem( otherObj );
	if ( i && i->has_dependency( *this ) )
		throw std::runtime_error( "Attempt to create a circular dependency between '" + name() + "' and '" + i->name() + "'" );

	auto cur = myDependencies.find( otherObj );
	if ( cur == myDependencies.end() )
	{
		myDependencies[otherObj] = dt;
	}
	else if ( cur->second > dt )
		cur->second = dt;
}


////////////////////////////////////////


void
Item::add_dependency( DependencyType dt, const std::string &otherObj )
{
	ID o = findItem( otherObj );
	if ( o == UNKNOWN )
	{
		bool found = false;
		for ( auto &x: myUnresolvedDependencies )
		{
			if ( x.second == otherObj )
			{
				found = true;
				if ( dt < x.first )
					x.first = dt;
			}
		}
		if ( ! found )
			myUnresolvedDependencies.push_back( std::make_pair( dt, otherObj ) );
	}
	else
		add_dependency( dt, o );
}


////////////////////////////////////////


bool
Item::has_dependency( const Item &other ) const
{
	if ( myDependencies.find( other.id() ) != myDependencies.end() )
		return true;

	for ( auto &unres: myUnresolvedDependencies )
	{
		if ( unres.second == other.name() )
			return true;
	}

	for ( auto &dep: myDependencies )
	{
		Item *i = retrieveItem( dep.first );
		if ( i )
		{
			if ( i->has_dependency( other ) )
				return true;
		}
	}

	return false;
}


////////////////////////////////////////


std::vector<const Item *>
Item::extract_dependencies( DependencyType dt ) const
{
	std::vector<const Item *> retval;

	if ( dt == DependencyType::CHAIN )
	{
		recurse_chain( retval );

		if ( ! retval.empty() )
		{
			// go through in reverse order and remove any duplicates
			// that follow
			std::reverse( std::begin( retval ), std::end( retval ) );

			size_t i = 0;
			while ( i != retval.size() )
			{
				for ( size_t j = i + 1; j < retval.size(); ++j )
				{
					if ( retval[i] == retval[j] )
					{
						retval.erase( retval.begin() + j );
						j = i;
					}
				}
				++i;
			}
			std::reverse( std::begin( retval ), std::end( retval ) );
		}
	}
	else
	{
		for ( auto &dep: myDependencies )
		{
			if ( dep.second != dt )
				continue;

			const Item *newItem = retrieveItem( dep.first );
			if ( ! newItem )
				throw std::logic_error( "Unknown item ID traversing dependents" );

			retval.push_back( newItem );
		}
	}
	

	return retval;
}


////////////////////////////////////////


void
Item::update_dependency( const std::string &name, ID otherID )
{
	for ( auto x = myUnresolvedDependencies.begin(); x != myUnresolvedDependencies.end(); ++x )
	{
		if ( (*x).second == name )
		{
			add_dependency( (*x).first, otherID );
			myUnresolvedDependencies.erase( x );
			return;
		}
	}
	throw std::runtime_error( "Unable to find unresolved dependency '" + name + "'" );
}


////////////////////////////////////////


void
Item::check_dependencies( void )
{
	for ( auto x = theItemsByID.begin(); x != theItemsByID.end(); ++x )
	{
		if ( x->second->has_unresolved_dependencies() )
			throw std::runtime_error( "Object '" + x->second->name() + "' has unresolved dependencies" );
	}
}


////////////////////////////////////////


void
Item::recurse_chain( std::vector<const Item *> &chain ) const
{
	for ( auto &dep: myDependencies )
	{
		if ( dep.second != DependencyType::CHAIN )
			continue;

		add_chain_dependent( chain, dep.first );
	}
}


////////////////////////////////////////


void
Item::add_chain_dependent( std::vector<const Item *> &chain, ID otherID ) const
{
	const Item *newItem = retrieveItem( otherID );
	if ( ! newItem )
		throw std::logic_error( "Unknown item ID traversing chain dependents" );

	chain.push_back( newItem );
	newItem->recurse_chain( chain );
}


////////////////////////////////////////



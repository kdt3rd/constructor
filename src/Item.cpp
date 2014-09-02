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
#include "LuaEngine.h"


////////////////////////////////////////

namespace {

int itemGetName( lua_State *L )
{
	if ( lua_gettop( L ) != 1 )
		throw std::runtime_error( "Item:name() expects only one argument - self" );
	ItemPtr p = Item::extract( L, 1 );
	const std::string &n = p->name();
	lua_pushlstring( L, n.c_str(), n.size() );
	return 1;
}

int itemGetID( lua_State *L )
{
	if ( lua_gettop( L ) != 1 )
		throw std::runtime_error( "Item:id() expects only one argument - self" );
	ItemPtr p = Item::extract( L, 1 );
	lua_pushunsigned( L, p->id() );
	return 1;
}

int itemAddDependency( lua_State *L )
{
	if ( lua_gettop( L ) != 3 )
		throw std::runtime_error( "Item:addDependency() expects 3 arguments - self, dependencyType, dependency" );
	ItemPtr p = Item::extract( L, 1 );
	size_t len = 0;
	const char *dtp = lua_tolstring( L, 2, &len );
	if ( dtp )
	{
		std::string dt( dtp, len );
		ItemPtr d = Item::extract( L, 3 );
		if ( dt == "explicit" )
			p->addDependency( DependencyType::EXPLICIT, d );
		else if ( dt == "implicit" )
			p->addDependency( DependencyType::IMPLICIT, d );
		else if ( dt == "order" )
			p->addDependency( DependencyType::ORDER, d );
		else if ( dt == "chain" )
			p->addDependency( DependencyType::CHAIN, d );
		else
			throw std::runtime_error( "Invalid dependency type: expect explicit, implicit, order, or chain" );
	}
	else
		throw std::runtime_error( "Unable to extract string in addDependency" );
	return 0;
}

int itemHasDependency( lua_State *L )
{
	if ( lua_gettop( L ) != 2 )
		throw std::runtime_error( "Item:addDependency() expects 2 arguments - self and dependency check" );
	ItemPtr p = Item::extract( L, 1 );
	ItemPtr d = Item::extract( L, 2 );
	bool hd = p->hasDependency( d );
	lua_pushboolean( L, hd ? 1 : 0 );
	return 1;
}

int itemCreate( lua_State *L )
{
	if ( lua_gettop( L ) != 1 || ! lua_isstring( L, 1 ) )
		throw std::runtime_error( "Item.new expects a single argument - a name" );

	size_t len = 0;
	const char *p = lua_tolstring( L, 1, &len );
	if ( p )
	{
		ItemPtr np = std::make_shared<Item>( std::string( p, len ) );
		Item::push( L, np );
	}
	else
		throw std::runtime_error( "Unable to extract item name" );

	return 1;
}

static Item::ID theLastID = 1;

} // empty namespace


////////////////////////////////////////


Item::Item( const std::string &name )
		: myID( theLastID++ ), myName( name )
{
}


////////////////////////////////////////


Item::Item( std::string &&name )
		: myID( theLastID++ ), myName( std::move( name ) )
{
}


////////////////////////////////////////


Item::~Item( void )
{
}


////////////////////////////////////////


void
Item::addDependency( DependencyType dt, ItemPtr otherObj )
{
	if ( ! otherObj )
		return;

	if ( otherObj->hasDependency( shared_from_this() ) )
		throw std::runtime_error( "Attempt to create a circular dependency between '" + name() + "' and '" + otherObj->name() + "'" );

	auto cur = myDependencies.find( otherObj );
	if ( cur == myDependencies.end() )
		myDependencies[otherObj] = dt;
	else if ( cur->second > dt )
		cur->second = dt;
}


////////////////////////////////////////


bool
Item::hasDependency( const ItemPtr &other ) const
{
	if ( myDependencies.find( other ) != myDependencies.end() )
		return true;

	for ( auto &dep: myDependencies )
	{
		if ( dep.first->hasDependency( other ) )
			return true;
	}

	return false;
}


////////////////////////////////////////


std::vector<ItemPtr>
Item::extractDependencies( DependencyType dt ) const
{
	std::vector<ItemPtr> retval;

	if ( dt == DependencyType::CHAIN )
	{
		recurseChain( retval );

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

			retval.push_back( dep.first );
		}
	}
	

	return std::move( retval );
}


////////////////////////////////////////


void
Item::recurseChain( std::vector<ItemPtr> &chain ) const
{
	for ( auto &dep: myDependencies )
	{
		if ( dep.second != DependencyType::CHAIN )
			continue;

		chain.push_back( dep.first );
		dep.first->recurseChain( chain );
	}
}


////////////////////////////////////////


ItemPtr
Item::extract( lua_State *L, int i )
{
	void *ud = luaL_checkudata( L, i, "Constructor.Item" );
	if ( ! ud )
		throw std::runtime_error( "User Data item is not a constructor item" );
	return *( reinterpret_cast<ItemPtr *>( ud ) );
}



////////////////////////////////////////


void
Item::push( lua_State *L, ItemPtr i )
{
	void *sptr = lua_newuserdata( L, sizeof(ItemPtr) );
	if ( ! sptr )
		throw std::runtime_error( "Unable to create item userdata" );

	luaL_setmetatable( L, "Constructor.Item" );
	new (sptr) ItemPtr( std::move( i ) );
}


////////////////////////////////////////


static const struct luaL_Reg item_m[] =
{
	{ "__tostring", itemGetName },
	{ "name", itemGetName },
	{ "id", itemGetID },
	{ "addDependency", itemAddDependency },
	{ "depends", itemHasDependency },
	{ nullptr, nullptr }
};

static const struct luaL_Reg class_f[] =
{
	{ "new", itemCreate },
	{ nullptr, nullptr }
};
	
void
Item::registerFunctions( void )
{
	Lua::Engine &eng = Lua::Engine::singleton();
	eng.registerClass( "Item", class_f, item_m );
}


////////////////////////////////////////



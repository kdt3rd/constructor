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
#include "LuaEngine.h"
#include "StrUtil.h"
#include <iostream>


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

int itemVariables( lua_State *L )
{
	if ( lua_gettop( L ) != 1 )
		throw std::runtime_error( "Item:variables() expects 1 argument - self" );
	ItemPtr p = Item::extract( L, 1 );
	Lua::Value ret;
	Lua::Table &t = ret.initTable();
	for ( auto &v: p->variables() )
		t[v.first].initString( std::move( v.second.value() ) );

	ret.push( L );
	return 1;
}


////////////////////////////////////////


int itemClearVariable( lua_State *L )
{
	if ( lua_gettop( L ) != 2 )
		throw std::runtime_error( "Item:clearVariable() expects 2 arguments - self, variable name" );
	ItemPtr p = Item::extract( L, 1 );
	std::string nm = Lua::Parm<std::string>::get( L, 2, 2 );

	VariableSet &vars = p->variables();
	auto x = vars.find( nm );
	if ( x != vars.end() )
		vars.erase( x );
	return 0;
}

int itemSetVariable( lua_State *L )
{
	int N = lua_gettop( L );
	if ( N < 3 )
		throw std::runtime_error( "Item:setVariable() expects 3 or 4 arguments - self, variable name, variable value, bool env check (nil)" );
	ItemPtr p = Item::extract( L, 1 );
	std::string nm = Lua::Parm<std::string>::get( L, N, 2 );
	Lua::Value v;
	v.load( L, 3 );
	bool envCheck = false;
	if ( N == 4 )
		envCheck = Lua::Parm<bool>::get( L, N, 4 );

	VariableSet &vars = p->variables();
	auto x = vars.find( nm );
	if ( v.type() == LUA_TNIL )
	{
		if ( x != vars.end() )
			vars.erase( x );
		return 0;
	}

	if ( x == vars.end() )
		x = vars.emplace( std::make_pair( nm, Variable( nm, envCheck ) ) ).first;

	if ( v.type() == LUA_TTABLE )
		x->second.reset( std::move( v.toStringList() ) );
	else if ( v.type() == LUA_TSTRING )
		x->second.reset( v.asString() );
	else
		throw std::runtime_error( "Item:setVariable() - unhandled variable value type, expect nil, table or string" );

	return 0;
}

int itemAddToVariable( lua_State *L )
{
	if ( lua_gettop( L ) != 3 )
		throw std::runtime_error( "Item:addToVariable() expects 3 arguments - self, variable name, variable value to add" );
	ItemPtr p = Item::extract( L, 1 );
	std::string nm = Lua::Parm<std::string>::get( L, 3, 2 );
	Lua::Value v;
	v.load( L, 3 );
	if ( v.type() == LUA_TNIL )
		return 0;

	VariableSet &vars = p->variables();
	auto x = vars.find( nm );
	// should this be an error?
	if ( x == vars.end() )
		x = vars.emplace( std::make_pair( nm, Variable( nm ) ) ).first;

	if ( v.type() == LUA_TTABLE )
		x->second.add( v.toStringList() );
	else if ( v.type() == LUA_TSTRING )
		x->second.add( v.asString() );
	else
		throw std::runtime_error( "Item:addToVariable() - unhandled variable value type, expect nil, table or string" );
	return 0;
}

int itemInheritVariable( lua_State *L )
{
	if ( lua_gettop( L ) != 3 )
		throw std::runtime_error( "Item:inheritVariable() expects 3 arguments - self, variable name, boolean" );
	ItemPtr p = Item::extract( L, 1 );
	std::string nm = Lua::Parm<std::string>::get( L, 3, 2 );
	bool v = Lua::Parm<bool>::get( L, 3, 3 );
	VariableSet &vars = p->variables();
	auto x = vars.find( nm );
	// should this be an error?
	if ( x == vars.end() )
		x = vars.emplace( std::make_pair( nm, Variable( nm ) ) ).first;

	x->second.inherit( v );
	return 0;
}


////////////////////////////////////////


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
		: myID( theLastID++ ), myName( name ),
		  myDirectory( Directory::current() )
{
}


////////////////////////////////////////


Item::Item( std::string &&name )
		: myID( theLastID++ ), myName( std::move( name ) ),
		  myDirectory( Directory::current() )
{
}


////////////////////////////////////////


Item::~Item( void )
{
}


////////////////////////////////////////


const std::string &
Item::name( void ) const 
{
	return myName;
}


////////////////////////////////////////


void
Item::transform( std::vector< std::shared_ptr<BuildItem> > &,
				 const TransformSet & ) const
{
}


////////////////////////////////////////


Variable &
Item::variable( const std::string &nm )
{
	auto v = myVariables.emplace( std::make_pair( nm, Variable( nm ) ) );
	return v.first->second;
}


////////////////////////////////////////


void
Item::setVariable( const std::string &nm, const std::string &value,
				   bool doSplit )
{
	if ( doSplit )
		variable( nm ).reset( String::split( value, ' ' ) );
	else
		variable( nm ).reset( value );
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
	if ( ! i )
	{
		lua_pushnil( L );
		return;
	}

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
	{ "variables", itemVariables },
	{ "clearVariable", itemClearVariable },
	{ "setVariable", itemSetVariable },
	{ "addToVariable", itemAddToVariable },
	{ "inheritVariable", itemInheritVariable },
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



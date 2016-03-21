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

#pragma once

#include "Item.h"
#include "LuaEngine.h"
#include "LuaValue.h"
#include "Debug.h"
#include <stdexcept>


////////////////////////////////////////


namespace Lua
{


////////////////////////////////////////


void pushItem( lua_State *L, ItemPtr i );

ItemPtr extractItem( lua_State *L, int i );
ItemPtr extractItem( const Lua::Value &v );

template <typename Set>
inline void
recurseAndAdd( const std::shared_ptr<Set> &ret,
			   lua_State *L, int i )
{
	if ( lua_isnil( L, i ) )
		return;
	if ( lua_isstring( L, i ) )
	{
		size_t len = 0;
		const char *n = lua_tolstring( L, i, &len );
		DEBUG( "recurseAndAdd " << n );
		if ( n )
			ret->addItem( std::string( n, len ) );
		else
			throw std::runtime_error( "String argument, but unable to extract string" );
	}
	else if ( lua_isuserdata( L, i ) )
	{
		DEBUG( "recurseAndAdd existing item " << Lua::extractItem( L, i )->getName() );
		ret->addItem( Lua::extractItem( L, i ) );
	}
	else if ( lua_istable( L, i ) )
	{
		lua_pushnil( L );
		while ( lua_next( L, i ) )
		{
			recurseAndAdd( ret, L, lua_gettop( L ) );
			lua_pop( L, 1 );
		}
	}
	else
		throw std::runtime_error( "Unhandled argument type to Compile" );
}

template <typename Set>
inline void
recurseAndAdd( const std::shared_ptr<Set> &ret,
			   const Lua::Value &curV )
{
	switch ( curV.type() )
	{
		case LUA_TNIL:
			break;
		case LUA_TSTRING:
			DEBUG( "recurseAndAdd2 " << curV.asString() );
			ret->addItem( curV.asString() );
			break;
		case LUA_TTABLE:
			for ( auto &i: curV.asTable() )
				recurseAndAdd( ret, i.second );
			break;
		case LUA_TUSERDATA:
			DEBUG( "recurseAndAdd2 item " << extractItem( curV )->getName() );
			ret->addItem( extractItem( curV ) );
			break;
		default:
			throw std::runtime_error( "Unhandled argument type passed to Compile" );
	}
}

void registerItemExt( void );

} // namespace Lua




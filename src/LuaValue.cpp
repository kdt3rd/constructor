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

#include "LuaValue.h"
#include <stdexcept>
#include <iostream>


////////////////////////////////////////


namespace Lua
{


////////////////////////////////////////


Key::Key( void )
{
}


////////////////////////////////////////


Key::Key( lua_State *L, int idx )
{
	int lt = lua_type( L, idx );
	switch ( lt )
	{
		case LUA_TNUMBER:
			type = KeyType::INDEX;
			index = lua_tounsigned( L, idx );
			break;
		case LUA_TSTRING:
		{
			type = KeyType::STRING;
			size_t len = 0;
			const char *s = lua_tolstring( L, idx, &len );
			new (&tag) String( s, len );
			break;
		}
		default:
			break;
	}
}


////////////////////////////////////////


Key::Key( lua_Unsigned idx )
		: index( idx ), type( KeyType::INDEX )
{
}


////////////////////////////////////////


Key::Key( String t )
		: tag( std::move( t ) ), type( KeyType::STRING )
{
}


////////////////////////////////////////


Key::Key( const char *t )
		: tag( t ), type( KeyType::STRING )
{
}


////////////////////////////////////////


Key::Key( const Key &k )
		: type( k.type )
{
	if ( k.type == KeyType::INDEX )
		index = k.index;
	else
		new (&tag) String( k.tag );
}


////////////////////////////////////////


Key::Key( Key &&k )
		: type( k.type )
{
	if ( k.type == KeyType::INDEX )
		index = k.index;
	else
		new (&tag) String( std::move( k.tag ) );
}


////////////////////////////////////////


Key::~Key( void )
{
	if ( type == KeyType::STRING )
	{
		tag.~String();
	}
}


////////////////////////////////////////


Key &Key::operator=( const Key &k )
{
	if ( this != &k )
	{
		if ( k.type == KeyType::INDEX )
		{
			if ( type == KeyType::STRING )
				tag.~String();
			index = k.index;
		}
		else
		{
			if ( type == KeyType::INDEX )
				new (&tag) String( k.tag );
			else
				tag = k.tag;
		}

		type = k.type;
	}
	return *this;
}


////////////////////////////////////////


Key &Key::operator=( Key &&k )
{
	if ( this != &k )
	{
		if ( k.type == KeyType::INDEX )
		{
			if ( type == KeyType::STRING )
				tag.~String();
			index = k.index;
		}
		else
		{
			if ( type == KeyType::INDEX )
				new (&tag) String( std::move( k.tag ) );
			else
				tag = std::move( k.tag );
		}

		type = k.type;
	}
	return *this;
}


////////////////////////////////////////


bool
Key::operator<( const Key &o ) const
{
	if ( type == o.type )
	{
		if ( type == KeyType::INDEX )
			return index < o.index;
		return tag < o.tag;
	}
	// make indices appear first...
	return ( type == KeyType::INDEX );
}


////////////////////////////////////////


void
Key::push( lua_State *L ) const
{
	if ( type == KeyType::INDEX )
		lua_pushunsigned( L, index );
	else
		lua_pushlstring( L, tag.c_str(), tag.size() );
}


////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////


Value::Value( void )
{
}


////////////////////////////////////////


Value::Value( lua_State *L, int idx )
{
	load( L, idx );
}


////////////////////////////////////////


Value::~Value( void )
{
	destroyData();
}


////////////////////////////////////////


Value::Value( const Value &v )
		: myType( v.myType )
{
	switch ( myType )
	{
		case LUA_TNIL: p = nullptr; break;
		case LUA_TBOOLEAN: b = v.b; break;
		case LUA_TLIGHTUSERDATA: p = v.p; break;
		case LUA_TNUMBER: n = v.n; break;
		case LUA_TSTRING: new (&s) String( v.s ); break;
		case LUA_TTABLE: new (&t) Table( v.t ); break;
		case LUA_TFUNCTION: f = v.f; break;
		case LUA_TUSERDATA: p = v.p; break;
		case LUA_TTHREAD: p = v.p; break;
		default:
			throw std::runtime_error( "Unknown lua type" );
	}
}


////////////////////////////////////////


Value::Value( Value &&v )
		: myType( v.myType )
{
	switch ( myType )
	{
		case LUA_TNIL: p = nullptr; break;
		case LUA_TBOOLEAN: b = v.b; break;
		case LUA_TLIGHTUSERDATA: p = v.p; break;
		case LUA_TNUMBER: n = v.n; break;
		case LUA_TSTRING: new (&s) String( std::move( v.s ) ); break;
		case LUA_TTABLE: new (&t) Table( std::move( v.t ) ); break;
		case LUA_TFUNCTION: f = v.f; break;
		case LUA_TUSERDATA: p = v.p; break;
		case LUA_TTHREAD: p = v.p; break;
		default:
			throw std::runtime_error( "Unknown lua type" );
	}
}


////////////////////////////////////////


Value &
Value::operator=( const Value &v )
{
	if ( this != &v )
	{
		destroyData();
		myType = v.type();

		switch ( myType )
		{
			case LUA_TNIL: p = nullptr; break;
			case LUA_TBOOLEAN: b = v.b; break;
			case LUA_TLIGHTUSERDATA: p = v.p; break;
			case LUA_TNUMBER: n = v.n; break;
			case LUA_TSTRING: new (&s) String( v.s ); break;
			case LUA_TTABLE: new (&t) Table( v.t ); break;
			case LUA_TFUNCTION: f = v.f; break;
			case LUA_TUSERDATA: p = v.p; break;
			case LUA_TTHREAD: p = v.p; break;
			default:
				throw std::runtime_error( "Unknown lua type" );
		}
	}
	return *this;
}


////////////////////////////////////////


Value &
Value::operator=( Value &&v )
{
	if ( this != &v )
	{
		destroyData();
		myType = v.type();

		switch ( myType )
		{
			case LUA_TNIL: p = nullptr; break;
			case LUA_TBOOLEAN: b = v.b; break;
			case LUA_TLIGHTUSERDATA: p = v.p; break;
			case LUA_TNUMBER: n = v.n; break;
			case LUA_TSTRING: new (&s) String( std::move( v.s ) ); break;
			case LUA_TTABLE: new (&t) Table( std::move( v.t ) ); break;
			case LUA_TFUNCTION: f = v.f; break;
			case LUA_TUSERDATA: p = v.p; break;
			case LUA_TTHREAD: p = v.p; break;
			default:
				throw std::runtime_error( "Unknown lua type" );
		}
	}
	return *this;
}


////////////////////////////////////////


void
Value::load( lua_State *L, int idx )
{
	destroyData();
	myType = lua_type( L, idx );
	switch ( myType )
	{
		case LUA_TNIL:
			p = nullptr;
			break;
		case LUA_TBOOLEAN:
			b = lua_toboolean( L, idx ) != 0;
			break;
		case LUA_TNUMBER:
			n = lua_tonumber( L, idx );
			break;
		case LUA_TSTRING:
		{
			size_t len = 0;
			const char *ptr = lua_tolstring( L, idx, &len );
			new (&s) String( ptr, len );
			break;
		}
		case LUA_TTABLE:
		{
			new (&t) Table;
			int tabIdx = lua_absindex( L, idx );
			lua_pushnil( L );
			while ( lua_next( L, tabIdx ) )
			{
				t[Key(L, -2)] = Value( L, -1 );
				lua_pop( L, 1 );
			}
			break;
		}
		case LUA_TFUNCTION:
			if ( lua_iscfunction( L, idx ) )
				f = lua_tocfunction( L, idx );
			else
				throw std::runtime_error( "Support for lua functions in a table not yet implemented" );
			break;
		case LUA_TUSERDATA:
		case LUA_TLIGHTUSERDATA:
			p = lua_touserdata( L, idx );
			break;
		case LUA_TTHREAD:
			p = lua_tothread( L, idx );
			break;
		default:
			throw std::runtime_error( "Unknown lua type" );
	}
}


////////////////////////////////////////


void
Value::push( lua_State *L ) const
{
	switch ( myType )
	{
		case LUA_TNIL:
			lua_pushnil( L );
			break;
		case LUA_TBOOLEAN:
			lua_pushboolean( L, b ? 1 : 0 );
			break;
		case LUA_TNUMBER:
			lua_pushnumber( L, n );
			break;
		case LUA_TSTRING:
			lua_pushlstring( L, s.c_str(), s.size() );
			break;
		case LUA_TTABLE:
		{
			lua_newtable( L );
			for ( const auto &i: t )
			{
				i.first.push( L );
				i.second.push( L );
				lua_rawset( L, -3 );
			}
			break;
		}
		case LUA_TFUNCTION:
			throw std::runtime_error( "Support for storing C functions not yet implemented" );
		case LUA_TUSERDATA:
			throw std::runtime_error( "Unable to store user data" );
		case LUA_TLIGHTUSERDATA:
			lua_pushlightuserdata( L, p );
			break;
		case LUA_TTHREAD:
			throw std::runtime_error( "Support for storing lua thread not yet implemented" );
		default:
			throw std::runtime_error( "Unknown lua type" );
	}
}


////////////////////////////////////////


Table &
Value::initTable( void )
{
	destroyData();
	myType = LUA_TTABLE;
	new (&t) Table;
	return t;
}


////////////////////////////////////////


void
Value::initBool( bool v )
{
	destroyData();
	myType = LUA_TBOOLEAN;
	b = v;
}


////////////////////////////////////////


void
Value::initNil( void )
{
	destroyData();
	myType = LUA_TNIL;
	p = nullptr;
}


////////////////////////////////////////


void
Value::initString( String v )
{
	destroyData();
	myType = LUA_TSTRING;
	new (&s) String( std::move( v ) );
}


////////////////////////////////////////


std::vector<std::string>
Value::toStringList( void ) const
{
	std::vector<std::string> ret;

	if ( type() == LUA_TSTRING )
	{
		ret.push_back( asString() );
		return ret;
	}

	checkType( LUA_TTABLE );

	for ( auto &i: asTable() )
	{
		if ( i.first.type != KeyType::INDEX )
			continue;
		if ( i.second.type() == LUA_TSTRING )
		{
			size_t idx = i.first.index;
			if ( idx == 0 )
				throw std::runtime_error( "Invalid index in string list conversion" );
			--idx;

			if ( idx >= ret.size() )
				ret.resize( idx + 1 );
			ret[idx] = i.second.asString();
		}
	}
	return std::move( ret );
}


////////////////////////////////////////


void
Value::destroyData( void )
{
	if ( myType == LUA_TTABLE )
		t.~Table();
	else if ( myType == LUA_TSTRING )
		s.~String();
}

} // namespace Lua


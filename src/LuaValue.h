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

#pragma once

#include "LuaInclude.h"

#include <string>
#include <map>
#include <vector>
#include <stdexcept>


////////////////////////////////////////


namespace Lua
{

enum class KeyType : intptr_t
{
	INDEX,
	STRING
};

using String = std::string;

struct Key
{
	Key( void );
	Key( lua_State *L, int idx );
	Key( lua_Integer idx );
	Key( String t );
	Key( const char *t );
	Key( const Key &k );
	Key( Key &&k );
	~Key( void );
	Key &operator=( const Key &k );
	Key &operator=( Key &&k );
	bool operator<( const Key &o ) const;

	void push( lua_State *L ) const;

	union
	{
		lua_Integer index = 0;
		String tag;
	};
	KeyType type = KeyType::INDEX;
};

class Value
{
public:
	using Table = std::map<Key, Value>;

	Value( void );
	Value( lua_State *L, int idx );
	~Value( void );
	Value( const Value &v );
	Value( Value &&v );
	Value &operator=( const Value &v );
	Value &operator=( Value &&v );

	void load( lua_State *L, int idx );
	// pushes a new stack item to send to lua
	void push( lua_State *L ) const;

	int type( void ) const { return static_cast<int>( myType ); }
	Table &initTable( void );
	void initBool( bool v );
	void initNil( void );
	void initString( String v );

	bool asBool( void ) const { checkType( LUA_TBOOLEAN ); return b; }
	void *asPointer( void ) const { return p; }
	lua_CFunction asFunction( void ) const { checkType( LUA_TFUNCTION ); return f; }
	double asNumber( void ) const { checkType( LUA_TNUMBER ); return n; }
	int asInteger( void ) const { return static_cast<int>( asNumber() ); }
	size_t asUnsigned( void ) const { return static_cast<size_t>( asNumber() ); }
	const String &asString( void ) const { checkType( LUA_TSTRING ); return s; }
	const Table &asTable( void ) const { checkType( LUA_TTABLE ); return t; }

	Table &&extractTable( void ) { checkType( LUA_TTABLE ); return std::move( t ); }

	// converts number-index table entries that have a string
	// in them to a vector of strings, ignoring all the rest
	std::vector<std::string> toStringList( void ) const;

private:
	inline void checkType( int expType ) const;
	void destroyData( void );

	// declare as intptr_t so alignment works
	intptr_t myType = LUA_TNIL;
	union
	{
		bool b;
		void *p = nullptr;
		lua_CFunction f;
		double n;
		String s;
		Table t;
	};
};

using Table = Value::Table;

inline void Value::checkType( int expType ) const
{
	if ( myType != expType )
		throw std::runtime_error( "Attempt to retrieve value as a type it is not" );
}

} // namespace Lua


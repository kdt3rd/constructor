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

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

#include <string>
#include <vector>
#include <type_traits>


////////////////////////////////////////


namespace Lua
{

template <typename T> struct ParmAdapt;

template <>
struct ParmAdapt<bool>
{
	static inline bool get( lua_State *l, int i )
	{
		return lua_toboolean( l, i ) != 0;
	}

	static inline bool check_and_get( lua_State *l, int i )
	{
		if ( ! lua_isnumber( l, i ) )
			throw std::runtime_error( "Expected a number argument" );
		return get( l, i );
	}

	static inline void put( lua_State *l, bool v )
	{
		lua_pushboolean( l, v ? 1 : 0 );
	}
};

template <>
struct ParmAdapt<int>
{
	static inline int get( lua_State *l, int i )
	{
		return lua_tointeger( l, i );
	}

	static inline int check_and_get( lua_State *l, int i )
	{
		if ( ! lua_isnumber( l, i ) )
			throw std::runtime_error( "Expected a number argument" );
		return get( l, i );
	}

	static inline void put( lua_State *l, int v )
	{
		lua_pushinteger( l, v );
	}
};

template <>
struct ParmAdapt<size_t>
{
	static inline size_t get( lua_State *l, int i )
	{
		return lua_tounsigned( l, i );
	}
	static inline size_t check_and_get( lua_State *l, int i )
	{
		if ( ! lua_isnumber( l, i ) )
			throw std::runtime_error( "Expected a number argument" );
		return get( l, i );
	}

	static inline void put( lua_State *l, size_t v )
	{
		lua_pushunsigned( l, v );
	}
};

template <>
struct ParmAdapt<double>
{
	static inline double get( lua_State *l, int i )
	{
		return lua_tonumber( l, i );
	}
	static inline double check_and_get( lua_State *l, int i )
	{
		if ( ! lua_isnumber( l, i ) )
			throw std::runtime_error( "Expected a number argument" );
		return get( l, i );
	}

	static inline void put( lua_State *l, double v )
	{
		lua_pushnumber( l, v );
	}
};

template <>
struct ParmAdapt<const char *>
{
	static inline const char *get( lua_State *l, int i )
	{
		return lua_tolstring( l, i, NULL );
	}

	static inline const char *check_and_get( lua_State *l, int i )
	{
		if ( ! lua_isstring( l, i ) )
			throw std::runtime_error( "Expected a string argument" );
		return get( l, i );
	}

	static inline void put( lua_State *l, const char *v )
	{
		lua_pushstring( l, v );
	}
};

template <>
struct ParmAdapt<std::string>
{
	static inline std::string get( lua_State *l, int i )
	{
		std::string ret;
		size_t n = 0;
		const char *s = lua_tolstring( l, i, &n );
		if ( s )
			ret.assign( s, n );

		return ret;
	}

	static inline std::string check_and_get( lua_State *l, int i )
	{
		if ( ! lua_isstring( l, i ) )
			throw std::runtime_error( "Expected a string argument" );
		return get( l, i );
	}

	static inline void put( lua_State *l, const std::string &v )
	{
		lua_pushlstring( l, v.c_str(), v.size() );
	}
};

template <>
struct ParmAdapt< std::vector<std::string> >
{
	static inline std::vector<std::string> get( lua_State *l, int i )
	{
		std::vector<std::string> ret;

		lua_pushnil( l );
		while ( lua_next( l, i ) )
		{
			// value at -1, key at -2
			if ( ! lua_isnumber( l, -2 ) )
				throw std::runtime_error( "Expected table key / index to be a number" );
			if ( ! lua_isstring( l, -1 ) )
				throw std::runtime_error( "Expected table element to be a string" );

			size_t idx = ParmAdapt<size_t>::get( l, -2 );
			if ( ret.size() < idx + 1 )
				ret.resize( idx + 1 );
			ret[idx] = std::move( ParmAdapt<std::string>::get( l, -1 ) );
			lua_pop( l, 1 );
		}
		return ret;
	}

	static inline std::vector<std::string> check_and_get( lua_State *l, int i )
	{
		if ( lua_istable( l, i ) )
			return get( l, i );
		else if ( lua_isstring( l, i ) )
		{
			// hrm, let's consider a single string a table
			std::vector<std::string> ret;
			ret.emplace_back( ParmAdapt<std::string>::get( l, i ) );
			return ret;
		}

		throw std::runtime_error( "Expected argument to be a table of strings" );
	}

	static inline void put( lua_State *l, const std::vector<std::string> &v )
	{
		lua_createtable( l, v.size(), 0 );
		for ( size_t i = 0; i != v.size(); ++i )
		{
			ParmAdapt<std::string>::put( l, v[i] );
			lua_rawseti( l, -2, static_cast<int>( i + 1 ) );
		}
	}
};


////////////////////////////////////////////////////////////////////////////////


template <typename T>
struct Parm
{
	typedef typename std::remove_cv< typename std::remove_reference<T>::type >::type type;

	static inline type get( lua_State *l, int N, int i )
	{
		if ( i > N )
			return type();

		return ParmAdapt<type>::check_and_get( l, i );
	}

	static inline void put( lua_State *l, const type &v )
	{
		return ParmAdapt<type>::put( l, v );
	}
};

template <>
struct Parm<const char *>
{
	typedef const char *type;
	static inline type get( lua_State *l, int N, int i )
	{
		if ( i > N )
			return nullptr;

		return ParmAdapt<type>::check_and_get( l, i );
	}
	static inline void put( lua_State *l, const char *v )
	{
		return ParmAdapt<type>::put( l, v );
	}
};

} // namespace Lua

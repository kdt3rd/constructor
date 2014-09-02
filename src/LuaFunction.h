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

#include <functional>
#include <string>
#include <vector>
#include <sstream>

#include "LuaParm.h"

namespace Lua
{

template <int ...> struct unpack_sequence {};
template <int N, int ...S> struct gen_sequence : public gen_sequence<N - 1, N - 1, S...> {};
template <int ...S>
struct gen_sequence<0, S...>
{
	typedef unpack_sequence<S...> type;
};


////////////////////////////////////////////////////////////////////////////////


class FunctionBase
{
public:
	inline FunctionBase( void ) {}
	virtual ~FunctionBase( void ) {}

	virtual int process( lua_State *l ) = 0;
};

////////////////////////////////////////


template <typename R, typename... Args>
class Function : public FunctionBase
{
public:
	inline Function( const char *name, R (*f)(Args...) ) : myName( name ), myFunc( f ) {}
	inline Function( const char *name, std::function<R (Args...)> &&f ) : myName( name ), myFunc( std::move( f ) ) {}
	inline Function( const char *name, const std::function<R (Args...)> &f ) : myName( name ), myFunc( f ) {}
	template <typename LambdaFunc> Function( const char *name, LambdaFunc &&f ) : myName( name ), myFunc( f ) {}
	virtual ~Function( void )
	{}

	virtual int process( lua_State *l )
	{
		R ret = extractArgsAndCall( l, typename gen_sequence<sizeof...(Args)>::type() );
		lua_pop( l, static_cast<int>( sizeof...(Args) ) );
		Parm<R>::put( l, ret );
		return 1;
	}

private:
	template <int ...S>
	R extractArgsAndCall( lua_State *l, const unpack_sequence<S...> & )
	{
		int N = lua_gettop( l );
		return myFunc( Parm<Args>::get( l, N, S + 1 )... );
	}

	std::string myName;
	std::function<R (Args...)> myFunc;
};

////////////////////////////////////////


template <typename... Args>
class Function<void, Args...> : public FunctionBase
{
public:
	inline Function( const char *name, void (*f)(Args...) ) : myName( name ), myFunc( f ) {}
	inline Function( const char *name, std::function<void (Args...)> &&f ) : myName( name ), myFunc( std::move( f ) ) {}
	inline Function( const char *name, const std::function<void (Args...)> &f ) : myName( name ), myFunc( f ) {}
	template <typename LambdaFunc> Function( const char *name, LambdaFunc &&f ) : myName( name ), myFunc( f ) {}
	virtual ~Function( void )
	{}

	virtual int process( lua_State *l )
	{
		extractArgsAndCall( l, typename gen_sequence<sizeof...(Args)>::type() );
		lua_pop( l, static_cast<int>( sizeof...(Args) ) );
		return 0;
	}

private:
	template <int ...S>
	void extractArgsAndCall( lua_State *l, const unpack_sequence<S...> & )
	{
		int N = lua_gettop( l );
		myFunc( Parm<Args>::get( l, N, S + 1 )... );
	}

	std::string myName;
	std::function<void (Args...)> myFunc;
};

} // namespace Lua




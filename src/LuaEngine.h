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

#include "ScopeGuard.h"

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

#include <functional>
#include <utility>
#include <vector>
#include <stack>
#include <memory>
#include "LuaFunction.h"


////////////////////////////////////////


namespace Lua
{


////////////////////////////////////////


class StackCheck
{
public:
	StackCheck( lua_State *l, const char *func );
	~StackCheck( void );

	int returns( int n );
	inline int start( void ) const { return myTop; }
private:
	lua_State *L;
	const char *myFunc;
	int myTop = -1;
};

#define LUA_STACK_CHECK( x ) Lua::StackCheck ANONYMOUS_VAR(luaStackChecker)( x, __PRETTY_FUNCTION__ )
#define LUA_STACK( nm, x ) Lua::StackCheck nm( x, __PRETTY_FUNCTION__ )

///
/// @brief Class LuaEngine provides...
///
class Engine
{
public:
	typedef std::function<int(lua_State *)> BoundFunction;

	void registerFunction( const char *name, int (*funcPtr)( lua_State * ) );
	void registerFunction( const char *name, const BoundFunction &f );
	template <typename R, typename... Args>
	void registerFunction( const char *name, R (*f)(Args...) )
	{
		mCPPFuncs.emplace_back( std::make_shared< Function<R, Args...> >( name, f ) );
		registerFunction( name, std::bind( &FunctionBase::process, mCPPFuncs.back().get(), std::placeholders::_1 ) );
	}
	template <typename R, typename... Args>
	void registerFunction( const char *name, const std::function<R(Args...)> &f )
	{
		mCPPFuncs.emplace_back( std::make_shared< Function<R,Args...> >( name, f ) );
		registerFunction( name, std::bind( &FunctionBase::process, mCPPFuncs.back().get(), std::placeholders::_1 ) );
	}
	template <typename R, typename... Args>
	void registerFunction( const char *name, std::function<R(Args...)> &&f )
	{
		mCPPFuncs.emplace_back( std::make_shared<Function>( name, std::move( f ) ) );
		registerFunction( name, std::bind( &FunctionBase::process, mCPPFuncs.back().get(), std::placeholders::_1 ) );
	}

	void pushLibrary( const char *name );
	void pushSubLibrary( const char *name );
	void popLibrary( void );
	void registerLibrary( const char *name,
						  const struct luaL_Reg *libFuncs );
	void registerClass( const char *name,
						const struct luaL_Reg *classFuncs,
						const struct luaL_Reg *memberFuncs );

	int runFile( const char *filename );

	inline lua_State *state( void ) { return L; }
	const char *getError( void );

	static Engine &singleton( void );

private:
	static int errorCB( lua_State *L );
	static int dispatchFunc( lua_State *L );

	Engine( void );
	~Engine( void );

	lua_State *L;
	int myErrFunc;
	std::vector<BoundFunction> mFuncs;
	std::vector< std::shared_ptr<FunctionBase> > mCPPFuncs;
	std::stack< std::pair<std::string, bool> > myCurLib;
};

} // namespace Lua






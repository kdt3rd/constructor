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

#include "Compile.h"
#include "LuaEngine.h"
#include "Scope.h"


////////////////////////////////////////


namespace
{

void
recurseAndAdd( std::shared_ptr<CompileSet> &ret,
			   lua_State *L, int i )
{
	if ( lua_isnil( L, i ) )
		return;
	if ( lua_isstring( L, i ) )
	{
		size_t len = 0;
		const char *n = lua_tolstring( L, i, &len );
		if ( n )
			ret->addItem( std::string( n, len ) );
		else
			throw std::runtime_error( "String argument, but unable to extract string" );
	}
	else if ( lua_isuserdata( L, i ) )
		ret->addItem( Item::extract( L, i ) );
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

int
compile( lua_State *L )
{
	int N = lua_gettop( L );
	if ( N == 0 )
	{
		lua_pushnil( L );
		return 1;
	}

	std::shared_ptr<CompileSet> ret = std::make_shared<CompileSet>();
	for ( int i = 1; i <= N; ++i )
		recurseAndAdd( ret, L, i );

	Scope::current().addItem( ret );
	Item::push( L, ret );
	return 1;
}

} // empty namespace


////////////////////////////////////////


CompileSet::CompileSet( void )
		: Item( "__compile__" ), myDir( Directory::current() )
{
}


////////////////////////////////////////


CompileSet::~CompileSet( void )
{
}


////////////////////////////////////////


void
CompileSet::addItem( const ItemPtr &i )
{
	myItems.push_back( i );
}


////////////////////////////////////////


void
CompileSet::addItem( const std::string &name )
{
	if ( ! myDir.exists( name ) )
		throw std::runtime_error( "File '" + name + "' does not exist in directory '" + myDir.fullpath() + "'" );

	ItemPtr i = std::make_shared<Item>( name );
	myItems.push_back( i );
}


////////////////////////////////////////


void
CompileSet::registerFunctions( void )
{
	Lua::Engine::singleton().registerFunction( "Compile", &compile );
}


////////////////////////////////////////





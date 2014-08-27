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

#include "LuaEngine.h"
#include <stdexcept>
#include <string>
#include <iostream>
#include <sstream>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <system_error>
#include <errno.h>
#include <vector>

namespace
{

void throwIfError( lua_State *L, int r, const char *file )
{
	if ( r == LUA_OK )
		return;

	const char *errmsg = NULL;

	if ( lua_isstring( L, -1 ) )
		errmsg = lua_tolstring( L, -1, NULL );

	if ( ! errmsg )
		errmsg = "Unknown error";

	std::string msg;
	switch ( r )
	{
		case LUA_ERRFILE:
			msg = std::string( "Unable to open / read file '" ) + file + std::string( "': " ) + errmsg;
			break;
		case LUA_ERRSYNTAX:
			msg = std::string( "Syntax error in file '" ) + file + std::string( "': " ) + errmsg;
			break;
		case LUA_ERRMEM:
			msg = std::string( "Memory error processing file '" ) + file + std::string( "': " ) + errmsg;
			break;
		case LUA_ERRERR:
			msg = std::string( "Error while running message handler processing file '" ) + file + std::string( "': " ) + errmsg;
			break;
		case LUA_ERRRUN:
			msg = std::string( "Error while running message handler processing file '" ) + file + std::string( "': Runtime Error" );
			break;
		case LUA_ERRGCMM:
			msg = std::string( "Error while running message handler processing file '" ) + file + std::string( "': Error while running a __gc metamethod");
			break;
		case LUA_OK:
		default:
			break;
	}
//	lua_pop( L, 1 );
//	luaL_error( L, msg.c_str() );
	throw std::runtime_error( msg );
//	return true;
}

struct FileClose
{
	inline FileClose( int &fd ) : myFD( fd ) {}
	inline ~FileClose( void ) { if ( myFD >= 0 ) { ::close( myFD ); myFD = -1; } }
	int myFD;
};

class LuaFile
{
public:
	LuaFile( const char *filename )
	{
		int fd = ::open( filename, O_RDONLY );
		if ( fd < 0 )
			throw std::system_error( errno, std::system_category(),
									 std::string( "Unable to open file " ) + filename );
		FileClose fc( fd );

		struct stat sb;
		if ( ::fstat( fd, &sb ) == 0 )
		{
			myBuf.resize( sb.st_size, '\0' );
			do
			{
				if ( ::read( fd, myBuf.data(), sb.st_size ) != sb.st_size )
				{
					if ( errno == EINTR )
						continue;

					throw std::system_error( errno, std::system_category(),
											 std::string( "Unable to read contents of " ) + filename );
				}
			} while ( false );
			myLeft = sb.st_size;
		}
		else
			throw std::system_error( errno, std::system_category(),
									 std::string( "Unable to get file info for " ) + filename );
	}
	~LuaFile( void )
	{
	}

	const char *get( size_t *sz ) const
	{
		if ( myLeft == 0 )
		{
			if ( sz ) *sz = 0;
			return NULL;
		}

		if ( sz ) *sz = myBuf.size() ;
		myLeft = 0;
		return myBuf.data();
	}

private:
	std::vector<char> myBuf;
	mutable size_t myLeft = 0;
};

const char *
fileReader( lua_State *, void *ud, size_t *sz )
{
	LuaFile *f = reinterpret_cast<LuaFile *>( ud );
	return f->get( sz );
}

} // empty namespace


////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////


namespace Lua
{


////////////////////////////////////////


StackCheck::StackCheck( lua_State *x, const char *func )
		: L( x ), myFunc( func ), myTop( lua_gettop( x ) )
{
}


////////////////////////////////////////


StackCheck::~StackCheck( void )
{
	if ( myTop != -1 )
	{
		int x = lua_gettop( L );
//		if ( x != myTop )
		if ( x != 0 )
		{
			std::cerr << "ERROR: Function '" << myFunc << "' Lua stack mismatch: incoming: " << myTop << " returning top: " << x << std::endl;
		}
	}
}


////////////////////////////////////////


int
StackCheck::returns( int x )
{
	if ( myTop == -1 )
		return x;
	int t = lua_gettop( L );
	if ( t != x )
	{
		std::stringstream err;
		err << "ERROR: Function '" << myFunc << "' Lua stack mismatch: returning: " << x << " starting state: " << myTop << " stack top: " << t;

//		throw std::runtime_error( err.str() );
	}
	myTop = -1;
	return x;
}


////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////


Engine::Engine( void )
		: L( luaL_newstate() )
{
	luaL_openlibs( L );

//	lua_getglobal( L, "debug" );
//	lua_getfield( L, -1, "traceback" );
	lua_pushcfunction( L, &Engine::errorCB );
	myErrFunc = lua_gettop( L );
}


////////////////////////////////////////


Engine::~Engine( void )
{
	lua_close( L );
}


////////////////////////////////////////


void
Engine::registerFunction( const char *name, int (*funcPtr)( lua_State * ) )
{
	lua_pushcfunction( L, funcPtr );
	lua_setglobal( L, name );
}


////////////////////////////////////////


void
Engine::registerFunction( const char *name, const BoundFunction &f )
{
	int i = mFuncs.size();
	lua_pushinteger( L, i );
	lua_pushcclosure( L, &Engine::dispatchFunc, 1 );
	lua_setglobal( L, name );
	mFuncs.push_back( f );
}


////////////////////////////////////////


int
Engine::runFile( const char *file )
{
	LUA_STACK( stkcheck, L );

	// create a new global environment for the file
	// but let it still look up in the current one;
	lua_newtable( L );
	int envPos = lua_gettop( L );

	LuaFile f( file );
	throwIfError( L, lua_load( L, &fileReader, &f, file, NULL ), file );

	int funcPos = lua_gettop( L );

	// for the metatable
	lua_newtable( L );
	lua_getglobal( L, "_G" );
	lua_setfield( L, -2, "__index" );
	// push the metatable into the new global table
	lua_setmetatable( L, envPos );
	// and set it as the up value for the function
	lua_pushnil( L );
	lua_copy( L, envPos, -1 );
	lua_setupvalue( L, funcPos, 1 );

	throwIfError( L, lua_pcall( L, 0, LUA_MULTRET, myErrFunc ), file );

//	lua_getglobal( L, "_G" );
//	// now troll through the created environment and copy into
//	// the real global table
//	lua_pushnil( L );
//	while ( lua_next( L, envPos ) )
//	{
//		lua_pushvalue( L, -2 );
//        // stack now contains: -1 => key; -2 => value; -3 => key; -4 => global table
//		std::cout << "Considering " << lua_tolstring( L, -1, NULL ) << std::endl;
//		bool copy = false;
//		switch ( lua_type( L, -2 ) )
//		{
//			case LUA_TTABLE: copy = true; break;
//			case LUA_TFUNCTION:
//			case LUA_TLIGHTUSERDATA:
//			case LUA_TNIL:
//			case LUA_TBOOLEAN:
//			case LUA_TTHREAD:
//			case LUA_TNONE:
//			default:
//				break;
//		}
//
//		if ( copy )
//		{
//			std::cout << " -> Copying ..." << std::endl;
//			// need to swap so the settable args are in the right order
//			lua_insert( L, -2 );
//			lua_settable( L, -4 );
//		}
//		else
//			lua_pop( L, 2 );
//	}
//	lua_pop( L, 1 );
	return stkcheck.returns( 1 );
}


////////////////////////////////////////


const char *
Engine::getError( void )
{
	int sp = lua_gettop( L );
	if ( sp > 0 && lua_isstring( L, -1 ) )
		return lua_tolstring( L, -1, NULL );
	return "Unknown LUA Error";
}


////////////////////////////////////////


Engine &
Engine::singleton( void )
{
	static Engine theEng;
	return theEng;
}


////////////////////////////////////////


int
Engine::errorCB( lua_State *L )
{
	int level;
	int arg = 0;
	lua_State *L1 = L;
	if ( lua_isthread( L, 1 ) )
	{
		arg = 1;
		L1 = lua_tothread( L, 1 );
	}
	lua_Debug ar;
	if ( lua_isnumber( L, arg + 2 ) )
	{
		level = lua_tointeger( L, arg + 2 );
		lua_pop( L, 1 );
	}
	else
		level = (L == L1) ? 1 : 0;  /* level 0 may be this own function */
	if ( lua_gettop( L ) == arg )
		lua_pushliteral(L, "");
	else if ( ! lua_isstring( L, arg+1 ) ) // message not a string, bail
		return 1;
	else
		lua_pushliteral(L, "\n");
	lua_pushliteral(L, "stack traceback:");
	while ( lua_getstack( L1, level++, &ar ) )
	{
		lua_pushliteral( L, "\n\t" );
		lua_getinfo( L1, "Snl", &ar );
		lua_pushfstring( L, "%s:", ar.short_src );
		if( ar.currentline > 0 )
			lua_pushfstring( L, "%d:", ar.currentline );
		if( *ar.namewhat != '\0' ) // there is a function name
			lua_pushfstring( L, " in function " LUA_QS, ar.name );
		else
		{
			if( *ar.what == 'm' )  /* main? */
				lua_pushfstring( L, " in main chunk" );
			else if ( *ar.what == 'C' || *ar.what == 't' )
				lua_pushliteral( L, " ?" );  /* C function or tail call */
			else
				lua_pushfstring( L, " in function <%s:%d>",
								 ar.short_src, ar.linedefined );
		}
		lua_concat(L, lua_gettop(L) - arg);
	}
	lua_concat(L, lua_gettop(L) - arg);
	return 1;
}


////////////////////////////////////////


int
Engine::dispatchFunc( lua_State *l )
{
	size_t funcIdx = static_cast<size_t>( lua_tointeger( l, lua_upvalueindex( 1 ) ) );
	Engine &eng = singleton();
	if ( funcIdx < eng.mFuncs.size() )
		return eng.mFuncs[funcIdx]( l );
	throw std::runtime_error( "Invalid function index in upvalue" );
}


////////////////////////////////////////


} // namespace Lua


////////////////////////////////////////





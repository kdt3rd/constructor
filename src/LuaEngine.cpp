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

	const char *errmsg = lua_tolstring( L, -1, NULL );
	if ( ! errmsg )
		errmsg = "Unknown error";

	std::string msg;
	switch ( r )
	{
		case LUA_ERRFILE:
			msg = std::string( "Unable to open / read file '" ) + file + std::string( "':\n\t" ) + errmsg;
			break;
		case LUA_ERRSYNTAX:
			msg = std::string( "Syntax error in file '" ) + file + std::string( "':\n\t" ) + errmsg;
			break;
		case LUA_ERRMEM:
			msg = std::string( "Memory error processing file '" ) + file + std::string( "':\n\t" ) + errmsg;
			break;
		case LUA_ERRERR:
			msg = std::string( "Error while running message handler processing file '" ) + file + std::string( "':\n\t" ) + errmsg;
			break;
		case LUA_ERRRUN:
			msg = std::string( "Runtime Error while processing file '" ) + file + std::string( "':\n\t" ) + errmsg;
			break;
		case LUA_ERRGCMM:
			msg = std::string( "Error while processing file '" ) + file + std::string( "': Error while running a __gc metamethod");
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
			myBuf.resize( static_cast<size_t>( sb.st_size ), '\0' );
			do
			{
				if ( ::read( fd, myBuf.data(), myBuf.size() ) != sb.st_size )
				{
					if ( errno == EINTR )
						continue;

					throw std::system_error( errno, std::system_category(),
											 std::string( "Unable to read contents of " ) + filename );
				}
			} while ( false );
			myLeft = myBuf.size();
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


FunctionBase::FunctionBase( void )
{
}


////////////////////////////////////////


FunctionBase::~FunctionBase( void )
{
}


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
		if ( x != myTop )
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
	if ( t != ( myTop + x ) )
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
	if ( myCurLib.empty() )
	{
		lua_pushcfunction( L, funcPtr );
		lua_setglobal( L, name );
	}
	else
	{
		lua_pushstring( L, name );
		lua_pushcfunction( L, funcPtr );
		lua_rawset( L, -3 );
	}
}


////////////////////////////////////////


void
Engine::registerFunction( const char *name, const BoundFunction &f )
{
	int i = static_cast<int>( mFuncs.size() );
	if ( myCurLib.empty() )
	{
		lua_pushinteger( L, i );
		lua_pushcclosure( L, &Engine::dispatchFunc, 1 );
		lua_setglobal( L, name );
	}
	else
	{
		lua_pushstring( L, name );
		lua_pushinteger( L, i );
		lua_pushcclosure( L, &Engine::dispatchFunc, 1 );
		lua_rawset( L, -3 );
	}
	mFuncs.push_back( f );
}


////////////////////////////////////////


void
Engine::pushLibrary( const char *name )
{
	myCurLib.push( std::make_pair( std::string( name ), false ) );
	lua_createtable( L, 0, 0 );
}


////////////////////////////////////////


void
Engine::pushSubLibrary( const char *name )
{
	myCurLib.push( std::make_pair( std::string( name ), true ) );
	lua_createtable( L, 0, 0 );
}


////////////////////////////////////////


void
Engine::popLibrary( void )
{
	if ( myCurLib.empty() )
		throw std::runtime_error( "unbalanced push / pops" );

	if ( myCurLib.top().second )
		lua_setfield( L, -2, myCurLib.top().first.c_str() );
	else
	{
		luaL_getsubtable( L, LUA_REGISTRYINDEX, "_LOADED" );
		lua_pushvalue( L, -2 ); // copy of library table
		lua_setfield( L, -2, myCurLib.top().first.c_str() );
		lua_pop( L, 1 );
		lua_setglobal( L, myCurLib.top().first.c_str() );
	}

	myCurLib.pop();
}


////////////////////////////////////////


void
Engine::registerLibrary( const char *name,
						 const struct luaL_Reg *libFuncs )
{
	lua_createtable( L, 0, 0 );
	luaL_setfuncs( L, libFuncs, 0 );
	lua_setglobal( L, name );
}


////////////////////////////////////////


void
Engine::registerClass( const char *name,
					   const struct luaL_Reg *classFuncs,
					   const struct luaL_Reg *memberFuncs )
{
	std::string metaName = "Constructor.";
	metaName.append( name );
	luaL_newmetatable( L, metaName.c_str() );
	lua_pushliteral( L, "__index" );
	lua_pushvalue( L, -2 );
	lua_settable( L, -3 );
	luaL_setfuncs( L, memberFuncs, 0 );

	lua_createtable( L, 0, 0 );
	luaL_setfuncs( L, classFuncs, 0 );
	lua_setglobal( L, name );
	lua_pop( L, 1 );
}


////////////////////////////////////////


int
Engine::runFile( const char *file )
{
	if ( ! myCurLib.empty() )
		throw std::runtime_error( "unbalanced push / pops for library definitions" );
	// create a new global environment for the file
	// but let it still look up in the current one;
	lua_newtable( L );
	int envPos = lua_gettop( L );

	std::string tmpname = "@";
	tmpname.append( file );
	LuaFile f( file );
	throwIfError( L, lua_load( L, &fileReader, &f, tmpname.c_str(), NULL ), file );

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
	lua_pop( L, 1 );
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
	return 1;
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
	lua_Debug d;
	lua_getstack(L, 1, &d);
	lua_getinfo(L, "Sln", &d);
	std::string err = lua_tostring(L, -1);
	lua_pop(L, 1);
	std::stringstream msg;
	msg << d.short_src << ":" << d.currentline;

	if (d.name != 0)
	{
		msg << "(" << d.namewhat << " " << d.name << ")";
	}
	msg << " " << err;
//	lua_pushstring(L, msg.str().c_str());
	std::cout << "error: " << err << std::endl;
	return 1;
#if 0
	int T = 1;//1;//lua_gettop( L );
	const char *msg = "<Unable to retrieve error value";
//	const char *tmsg = lua_tostring( L, T );
//	if ( tmsg )
//		msg = tmsg;

	if ( msg )
	{
		std::cout << "errorCB: msg '" << msg << "'" << std::endl;
//		luaL_traceback( L, L, msg, T );
	}
	else if ( ! lua_isnoneornil( L, T ) )
	{
		std::cout << "errorCB: non-none or nil value" << std::endl;
		// there's an error object, try the tostring method
//		if ( ! luaL_callmeta( L, 1, "__tostring" ) )
//			lua_pushliteral( L, "<Unable to find an error message>" );
//		else
//			lua_pushliteral( L, "<No string meta for error message>" );
	}
	else
	{
		std::cout << "errorCB: black / none / nil / pining-for-the-fjords value" << std::endl;
//		lua_pushliteral( L, "<nil error message>" );
	}

//	return 1;
	return 0;
#endif
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





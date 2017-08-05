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
#include "Debug.h"
#include "FileUtil.h"
#include "Directory.h"

#include "LuaToolExt.h"
#include "LuaCompileExt.h"

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
			break;
		default:
			msg = std::string( "Unknown error in '" ) + file + std::string( "':\n\t" ) + errmsg;
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


////////////////////////////////////////


static int
luaAddModulePath( lua_State *L )
{
	int N = lua_gettop( L );

	for ( int i = 1; i <= N; ++i )
	{
		if ( lua_isnil( L, i ) )
			continue;

		if ( lua_isstring( L, i ) )
		{
			size_t len = 0;
			const char *s = lua_tolstring( L, i, &len );
			std::string curP( s, len );

			DEBUG( "luaAddToolModulePath " << curP );
			if ( File::isAbsolute( s ) )
				Lua::Engine::singleton().addModulePath( std::move( curP ) );
			else
				Lua::Engine::singleton().addModulePath( Directory::current()->makefilename( curP ) );
		}
		else
		{
			std::cout << "WARNING: ignoring non-string argument in module path" << std::endl;
		}
	}

	return 0;
}

static int
luaSetModulePath( lua_State *L )
{
	Lua::Engine::singleton().resetModulePath();
	DEBUG( "luaSetToolModulePath" );
	return luaAddModulePath( L );
}

static int
luaLoadModule( lua_State *L )
{
	int N = lua_gettop( L );
	std::string name = Lua::Parm<std::string>::get( L, N, 1 );

	DEBUG( "luaLoadModule " << name );
	if ( ! name.empty() )
		return Lua::Engine::singleton().loadModule( std::move( name ) );

	return 1;
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
//	lua_remove( L, -2 );

//	lua_pushcfunction( L, &Engine::errorCB );
	myErrFunc = 0;//lua_gettop( L );

	// Rather than just change the search path
	// to add ?.construct to the package patterns,
	// we store the module path and it's usage
	// such that if the module definition changes,
	// the file will be regenerated
    lua_getglobal( L, "package" );
    if ( ! lua_istable( L, -1 ) )
		throw std::runtime_error( "Missing package library" );

	lua_pushstring( L, "searchers" );
	lua_gettable( L, -2 );
	if ( ! lua_istable( L, -1 ) )
		throw std::runtime_error( "package library missing searchers table" );
	int tbl_idx = lua_absindex( L, -1 );

	for ( int64_t n = luaL_len( L, tbl_idx ) + 1; n  > 1; --n )
	{
		lua_rawgeti( L, tbl_idx, n - 1 );
		lua_rawseti( L, tbl_idx, n );
	}
	lua_pushcfunction( L, luaLoadModule );
	lua_rawseti( L, tbl_idx, 1 );
	lua_pop( L, 2 );

	registerFunction( "set_module_path", &luaSetModulePath );
	registerFunction( "add_module_path", &luaAddModulePath );
}


////////////////////////////////////////


Engine::~Engine( void )
{
	lua_close( L );
}


////////////////////////////////////////


void
Engine::resetModulePath( void )
{
	myModulePath.clear();
}


////////////////////////////////////////


void
Engine::addModulePath( std::string p )
{
	myModulePath.emplace_back( std::move( p ) );
}


////////////////////////////////////////


int
Engine::loadModule( std::string p )
{
	std::string luaFile;
	bool found = false;
	if ( myModulePath.empty() )
	{
		Directory curD;
		std::vector<std::string> tmpPath = { curD.fullpath() };
		found = File::find( luaFile, p + ".construct", tmpPath );
	}
	else
		found = File::find( luaFile, p + ".construct", myModulePath );

	if ( ! found )
		return 1;

	clearCompileContext();
	clearToolset();

	std::string tmpname = "@";
	tmpname.append( p );
	LuaFile f( luaFile.c_str() );
	throwIfError( L, lua_load( L, &fileReader, &f, tmpname.c_str(), NULL ), p.c_str() );

	// add filename as argument to module
	lua_pushstring( L, luaFile.c_str() );
	return 2;
}


////////////////////////////////////////


void
Engine::registerFunction( const char *name, int (*funcPtr)( lua_State * ) )
{
	BoundFunction f = std::bind( funcPtr, std::placeholders::_1 );
	registerFunction( name, f );
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
	luaL_newmetatable( L, name );
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

	// create the table, this will serve as the return value
	// for the function
	lua_newtable( L );
	int envPos = lua_gettop( L );

	return runFile( file, envPos );
}


////////////////////////////////////////


int
Engine::runFile( const char *file, int envPos )
{
	if ( ! myCurLib.empty() )
		throw std::runtime_error( "unbalanced push / pops for library definitions" );

	std::string fn( file );
	addVisitedFile( fn );
	myFileNames.push( fn );
	ON_EXIT{ myFileNames.pop(); };

	std::string tmpname = "@";
	tmpname.append( file );
	LuaFile f( file );
	throwIfError( L, lua_load( L, &fileReader, &f, tmpname.c_str(), NULL ), file );

	int funcPos = lua_gettop( L );

	// if we're the first function, we'll just modify the global table
	// and not override the upvalue
	if ( ! myFileFuncs.empty() )
	{
		// metatable
		lua_newtable( L );
		lua_getglobal( L, "_G" );
		lua_setfield( L, -2, "__index" );
		lua_setmetatable( L, envPos );

		lua_pushnil( L );
		lua_copy( L, envPos, -1 );
		lua_setupvalue( L, funcPos, 1 );
	}
	{
		myFileFuncs.push( funcPos );
		ON_EXIT{ myFileFuncs.pop(); };

		try
		{
			throwIfError( L, lua_pcall( L, 0, LUA_MULTRET, myErrFunc ), file );
		}
		catch ( ... )
		{
			std::cerr << "Processing file " << myFileNames.top() << ":\n";
			throw;
		}
	}

	if ( myFileFuncs.empty() )
	{
		lua_pop( L, 1 );
		return 0;
	}

	return 1;
}


////////////////////////////////////////


void
Engine::addVisitedFile( const std::string &f )
{
	for ( const auto &i: myVisitedPaths )
	{
		if ( i == f )
			return;
	}
	myVisitedPaths.push_back( f );
	std::sort( myVisitedPaths.begin(), myVisitedPaths.end() );
}


////////////////////////////////////////


const std::vector<std::string> &
Engine::visitedFiles( void )
{
	return myVisitedPaths;
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


void
Engine::copyTable( int tablePos, int destPos )
{
	// now troll through the created environment and copy into
	// the real global table
	lua_pushnil( L );
	while ( lua_next( L, tablePos ) )
	{
		lua_pushvalue( L, -2 );
		// stack now contains: -1 => key; -2 => value; -3 => key; -4 => global table
		std::cout << "Considering " << lua_tolstring( L, -1, NULL ) << std::endl;
		bool copy = true;
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

		if ( copy )
		{
			std::cout << " -> Copying ..." << std::endl;
			// need to swap so the settable args are in the right order
			lua_insert( L, -2 );
			lua_settable( L, destPos );
		}
		else
			lua_pop( L, 2 );
	}
}


////////////////////////////////////////


int
Engine::errorCB( lua_State *L )
{
	Engine &me = singleton();

	int T = lua_gettop( L );
	std::stringstream msgbuf;
	if ( ! me.myFileNames.empty() )
		msgbuf << "In file " << me.myFileNames.top() << ": ";

	if ( T == 1 )
	{
		const char *tmsg = lua_tostring( L, T );
		if ( tmsg )
			msgbuf << tmsg;
		else
			msgbuf << "<non-string error message>";
	}
	else
		msgbuf << "<nil error message>";
	
	std::string tmp = msgbuf.str();
	lua_pushlstring( L, tmp.c_str(), tmp.size() );
	return 1;
}


////////////////////////////////////////


int
Engine::dispatchFunc( lua_State *l )
{
	try
	{
		size_t funcIdx = static_cast<size_t>( lua_tointeger( l, lua_upvalueindex( 1 ) ) );
		Engine &eng = singleton();
		if ( funcIdx < eng.mFuncs.size() )
			return eng.mFuncs[funcIdx]( l );
		throw std::runtime_error( "Invalid function index in upvalue" );
	}
	catch ( std::exception &e )
	{
		return luaL_error( l, "ERROR: %s", e.what() );
	}
}


////////////////////////////////////////


} // namespace Lua


////////////////////////////////////////





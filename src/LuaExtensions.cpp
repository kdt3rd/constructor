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

#include "LuaExtensions.h"
#include "LuaEngine.h"
#include "LuaValue.h"

// these used to be part of this file as one
// giant monolithic file, but have been broken out
// to make them easier to maintain
#include "LuaCodeGenExt.h"
#include "LuaConfigExt.h"
#include "LuaCompileExt.h"
#include "LuaFileExt.h"
#include "LuaItemExt.h"
#include "LuaScopeExt.h"
#include "LuaSysExt.h"
#include "LuaToolExt.h"

#include "ScopeGuard.h"

#include "OSUtil.h"
#include "StrUtil.h"
#include "Debug.h"

#include "Directory.h"
#include "Scope.h"
#include "Compile.h"
#include "Executable.h"
#include "Library.h"
#include "CodeGenerator.h"
#include "CodeFilter.h"
#include "CreateFile.h"
#include "InternalExecutable.h"
#include "Configuration.h"
#include "PackageConfig.h"
#include "Version.h"

#include <map>
#include <mutex>
#include <iostream>

// NB: This is a rather monolithic file. It's unfortunate, but this
// started out as separate files, but was collapsed while searching
// for the correct abstraction points such as separating the program
// from the lua integration and trying to figure out what utility
// functions were necessary to divide something from a "file" function
// or a "directory" function, and the various lua integration point
// utility functions for parsing tables, or recursively populating the
// compile list. Hopefully once things are relatively stable, it can
// be split apart again.


////////////////////////////////////////


namespace 
{

static std::vector<std::string> theSubDirFiles;


////////////////////////////////////////


inline constexpr const char *buildFileName( void )
{
	return "construct";
}

static int
luaCheckVersion( lua_State *L )
{
	int N = lua_gettop( L );
	if ( N != 1 || ! lua_isstring( L, 1 ) )
		lua_pushnil( L );
	else
	{
		std::string ver = Lua::Parm<std::string>::get( L, N, 1 );
		int rc = String::versionCompare( Constructor::version(), ver );
		if ( rc >= 0 )
			lua_pushboolean( L, 1 );
		else
			lua_pushnil( L );
	}
	return 1;
}

static int
luaSubDir( lua_State *L )
{
	Configuration::checkDefault();

	int N = lua_gettop( L );
	if ( N < 1 || ! lua_isstring( L, 1 ) )
		throw std::runtime_error( "subdir expects at directory name string as an argument" );
	if ( N > 2 )
		throw std::runtime_error( "subdir can pass an environment to a subdirectory, but at most 2 arguments expected" );
	else if ( N == 2 )
	{
		if ( ! lua_istable( L, 2 ) )
			throw std::runtime_error( "subdir expects a table as the environment argument" );
	}
	int ret = 0;

	std::string file = Lua::Parm<std::string>::get( L, N, 1 );
	DEBUG( "luaSubDir " << file );
	std::shared_ptr<Directory> curDir = Directory::current();
	std::string tmp;
	if ( curDir->exists( tmp, file ) )
	{
		curDir = Directory::pushd( file );
		ON_EXIT{ Directory::popd(); };

		Lua::clearToolset();
		Lua::clearCompileContext();

		Scope::pushScope( Scope::current().newSubScope( true ) );
		ON_EXIT{ Scope::popScope( true ); };
		std::string nextFile;
		if ( curDir->exists( nextFile, buildFileName() ) )
		{
			if ( N == 2 )
				ret = Lua::Engine::singleton().runFile( nextFile.c_str(), 2 );
			else
				ret = Lua::Engine::singleton().runFile( nextFile.c_str() );
		}
		else
			throw std::runtime_error( "Unable to find a '" + std::string( buildFileName() ) + "' in " + curDir->fullpath() );

		Lua::clearToolset();
		Lua::clearCompileContext();
	}
	else
		throw std::runtime_error( "Sub Directory '" + file + "' does not exist in " + curDir->fullpath() );

	return ret;
}

static int
luaSubProject( lua_State *L )
{
	// not quite the same as subdir, here
	// we want a new, default scope, and so
	// everything can reset
	int N = lua_gettop( L );
	if ( N != 1 || ! lua_isstring( L, 1 ) )
		throw std::runtime_error( "subproject expects at directory name string as an argument" );
	int ret = 0;

	std::string file = Lua::Parm<std::string>::get( L, N, 1 );
	DEBUG( "luaSubDir " << file );
	std::shared_ptr<Directory> curDir = Directory::current();
	std::string tmp;
	if ( curDir->exists( tmp, file ) )
	{
		curDir = Directory::pushd( file );
		ON_EXIT{ Directory::popd(); };

		Lua::clearToolset();
		Lua::clearCompileContext();

		Scope::pushScope( Scope::current().newSubScope( false ) );
		ON_EXIT{ Scope::popScope( false ); };

		std::string nextFile;
		if ( curDir->exists( nextFile, buildFileName() ) )
			ret = Lua::Engine::singleton().runFile( nextFile.c_str() );
		else
			throw std::runtime_error( "Unable to find a '" + std::string( buildFileName() ) + "' in " + curDir->fullpath() );

		Lua::clearToolset();
		Lua::clearCompileContext();
	}
	else
		throw std::runtime_error( "Sub Directory '" + file + "' does not exist in " + curDir->fullpath() );

	return ret;
}

} // empty namespace


////////////////////////////////////////


namespace Lua
{


////////////////////////////////////////


void
registerExtensions( void )
{
	Engine &eng = Engine::singleton();

	eng.registerFunction( "check_version", &luaCheckVersion );

	eng.registerFunction( "subdir", &luaSubDir );
	eng.registerFunction( "subproject", &luaSubProject );

	registerCodeGenExt();
	registerCompileExt();
	registerConfigExt();
	registerFileExt();
	registerItemExt();
	registerScopeExt();
	registerSysExt();
	registerToolExt();
}


////////////////////////////////////////


void
startParsing( const std::string &dir )
{
	std::shared_ptr<Directory> curDir = Directory::current();
	bool needPop = false;
	if ( ! dir.empty() )
	{
		needPop = true;
		curDir = Directory::pushd( dir );
	}
	ON_EXIT{ if ( needPop ) Directory::popd(); };

	std::string firstFile;
	if ( curDir->exists( firstFile, buildFileName() ) )
	{
		Lua::Engine::singleton().runFile( firstFile.c_str() );
	}
	else
	{
		std::stringstream msg;
		msg << "Unable to find " << buildFileName() << " in " << curDir->fullpath();
		throw std::runtime_error( msg.str() );
	}
}


////////////////////////////////////////


} // Lua




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

#include "Directory.h"
#include "FileUtil.h"
#include "ScopeGuard.h"
#include "LuaEngine.h"

#include <unistd.h>
#include <errno.h>
#include <system_error>
#include <sys/stat.h>
#include <sys/param.h>
#include <stdlib.h>
#include <sstream>

namespace 
{

int
SubDir( lua_State *L )
{
	LUA_STACK( stk, L );
	int N = lua_gettop( L );
	if ( N != 1 || ! lua_isstring( L, 1 ) )
	{
		if ( N > 0 )
			lua_pop( L, N );
		luaL_error( L, "SubDir expects at directory name string as an argument" );
		return stk.returns( 1 );
	}
	int ret = 0;

	std::string file = lua_tolstring( L, 1, NULL );
	Directory &curDir = Directory::currentDirectory();
	std::string tmp;
	lua_pop( L, 1 );
	if ( curDir.exists( tmp, file ) )
	{
		curDir.cd( file );
		std::string nextFile;
		if ( curDir.exists( nextFile, File::buildFileName() ) )
			ret = Lua::Engine::singleton().runFile( nextFile.c_str() );
		else
			throw std::runtime_error( "Unable to find a '" + std::string( File::buildFileName() ) + "' in " + curDir.fullpath() );

		curDir.cdUp();
	}
	else
		throw std::runtime_error( "Sub Directory '" + file + "' does not exist in " + curDir.fullpath() );

	return stk.returns( ret );
}

} // empty namespace


////////////////////////////////////////


Directory::Directory( void )
{
	char *cwd = getcwd( NULL, MAXPATHLEN );
	if ( ! cwd )
		throw std::system_error( errno, std::system_category(),
								 "Unable to query current directory" );
	ON_EXIT{ free( cwd ); };

	myRoot = cwd;
	updateFullPath();
}


////////////////////////////////////////


Directory::Directory( std::string root )
{
	reset( std::move( root ) );
}


////////////////////////////////////////


void
Directory::reset( std::string root )
{
	myRoot = std::move( root );
	updateFullPath();
}


////////////////////////////////////////


void
Directory::cd( std::string name )
{
	mySubDirs.emplace_back( std::move( name ) );
	updateFullPath();
}


////////////////////////////////////////


void
Directory::cdUp( void )
{
	if ( mySubDirs.empty() )
		throw std::runtime_error( "Attempt to change directories above root" );
	mySubDirs.pop_back();
	updateFullPath();
}


////////////////////////////////////////


void
Directory::mkpath( void ) const
{
	if ( ! checkRootPath() )
		throw std::runtime_error( "Attempt to create path but root directory does not exist" );
	std::string tmpPath = myRoot;
	for ( const auto &curDir: mySubDirs )
	{
		tmpPath.push_back( File::pathSeparator() );
		tmpPath.append( curDir );

		if ( mkdir( tmpPath.c_str(), 0777 ) != 0 )
		{
			if ( errno == EEXIST )
				continue;

			throw std::system_error( errno, std::system_category(),
				"Unable to create directory '" + tmpPath + "'" );
		}
	}
}


////////////////////////////////////////


const std::string &
Directory::fullpath( void ) const
{
	return myCurFullPath;
}


////////////////////////////////////////


void
Directory::updateFullPath( void )
{
	myCurFullPath = myRoot;
	for ( const auto &curDir: mySubDirs )
	{
		myCurFullPath.push_back( File::pathSeparator() );
		myCurFullPath.append( curDir );
	}
}


////////////////////////////////////////


bool
Directory::find( std::string &concatpath, const std::vector<std::string> &names ) const
{
	for ( const auto &n: names )
	{
		if ( exists( concatpath, n ) )
			return true;
	}

	return false;
}


////////////////////////////////////////


bool
Directory::exists( std::string &concatpath, const char *fn ) const
{
	concatpath = makefilename( fn );

	struct stat sb;
	if ( stat( concatpath.c_str(), &sb ) == 0 )
		return true;

	return false;
}


////////////////////////////////////////


bool
Directory::exists( std::string &concatpath, const std::string &fn ) const
{
	concatpath = makefilename( fn );

	struct stat sb;
	if ( stat( concatpath.c_str(), &sb ) == 0 )
		return true;

	return false;
}


////////////////////////////////////////


std::string
Directory::makefilename( const char *fn ) const
{
	std::string concatpath = fullpath();
	concatpath.push_back( File::pathSeparator() );
	concatpath.append( fn );
	return std::move( concatpath );
}


////////////////////////////////////////


std::string
Directory::makefilename( const std::string &fn ) const
{
	std::string concatpath = fullpath();
	concatpath.push_back( File::pathSeparator() );
	concatpath.append( fn );
	return std::move( concatpath );
}


////////////////////////////////////////


Directory &
Directory::currentDirectory( void )
{
	static Directory theDir;
	return theDir;
}


////////////////////////////////////////


bool
Directory::checkRootPath( void ) const
{
	// we should always check as some process may have created the
	// directory under us (or removed it)... there's still a possibility
	// of a collision with mkpath, etc. but not much we can do about that
	struct stat sb;
	if ( stat( myRoot.c_str(), &sb ) == 0 )
	{
		if ( ! S_ISDIR( sb.st_mode ) )
			throw std::runtime_error( "Path '" + myRoot + "' exists, but is not a directory" );
		return true;
	}
	return false;
}


////////////////////////////////////////


void
Directory::registerFunctions( void )
{
	Lua::Engine &eng = Lua::Engine::singleton();
	eng.registerFunction( "SubDir", &SubDir );
}


////////////////////////////////////////




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

static const std::string theBuildFileName = "build.lua";


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
		if ( curDir.exists( nextFile, theBuildFileName ) )
			ret = Lua::Engine::singleton().runFile( nextFile.c_str() );
		else
			throw std::runtime_error( "Unable to find a '" + theBuildFileName + "' in " + curDir.fullpath() );

		curDir.cdUp();
	}
	else
		throw std::runtime_error( "Sub Directory '" + file + "' does not exist in " + curDir.fullpath() );

	return stk.returns( ret );
}

} // empty namespace


////////////////////////////////////////


namespace File
{


////////////////////////////////////////


void
trimTrailingSeparators( std::string &path )
{
	std::string::size_type end = path.size();
	while ( end > 0 && path[end - 1] == pathSeparator() )
		--end;

	if ( end < path.size() )
		path = path.substr( 0, end );
}


////////////////////////////////////////


bool
isAbsolute( const char *path )
{
	if ( ! path || path[0] == '\0' )
		return false;

#ifdef WIN32
#else
	if ( path[0] == '/' )
		return true;
#endif
	return false;
}


////////////////////////////////////////


bool
exists( const char *fn )
{
	if ( ! fn || fn[0] == '\0' )
		return false;

	bool e = false;
	if ( isAbsolute( fn ) )
	{
		struct stat sb;
		if ( stat( fn, &sb ) == 0 )
			e = true;
	}
	else
	{
		std::string tmp;
		e = Directory::currentDirectory().exists( tmp, fn );
	}

	return e;
}


////////////////////////////////////////


bool
isDirectory( const char *fn )
{
	if ( ! fn || fn[0] == '\0' )
		return false;

	bool e = false;
	if ( isAbsolute( fn ) )
	{
		struct stat sb;
		if ( stat( fn, &sb ) == 0 )
		{
			if ( S_ISDIR( sb.st_mode ) )
				e = true;
		}
	}
	else
	{
		std::string tmp = Directory::currentDirectory().makefilename( fn );
		struct stat sb;
		if ( stat( tmp.c_str(), &sb ) == 0 )
		{
			if ( S_ISDIR( sb.st_mode ) )
				e = true;
		}
	}

	return e;
}


////////////////////////////////////////


bool
find( std::string &filepath, const std::vector<std::string> &names )
{
	return Directory::currentDirectory().find( filepath, names );
}


////////////////////////////////////////


bool
find( std::string &filepath, const std::string &name, const std::vector<std::string> &path )
{
	for ( const auto &p: path )
	{
		try
		{
			Directory d( p );
			if ( d.exists( filepath, name ) )
				return true;
		}
		catch ( ... )
		{
		}
	}
	return false;
}


////////////////////////////////////////


bool
find( std::string &filepath, const std::string &name,
		const std::vector<std::string> &extensions,
		const std::vector<std::string> &path )
{
	for ( const auto &p: path )
	{
		try
		{
			Directory d( p );
			for ( const auto &e: extensions )
			{
				if ( d.exists( filepath, name + e ) )
					return true;
			}
		}
		catch ( ... )
		{
		}
	}
	return false;
}


////////////////////////////////////////


void
startParsing( const std::string &dir )
{
	Directory &curDir = Directory::currentDirectory();
	if ( ! dir.empty() )
		curDir.reset( dir );

	std::string firstFile;
	if ( curDir.exists( firstFile, theBuildFileName ) )
		Lua::Engine::singleton().runFile( firstFile.c_str() );
	else
	{
		std::stringstream msg;
		msg << "Unable to find " << theBuildFileName << " in " << curDir.fullpath();
		throw std::runtime_error( msg.str() );
	}
}


////////////////////////////////////////


void
registerFunctions( void )
{
	Lua::Engine &eng = Lua::Engine::singleton();
	eng.registerFunction( "SubDir", &SubDir );
	eng.registerFunction( "exists", &exists );
}

} // namespace File


////////////////////////////////////////




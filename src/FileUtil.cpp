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
#include "Directory.h"
#include "OSUtil.h"
#include "StrUtil.h"

#include <unistd.h>
#include <errno.h>
#include <system_error>
#include <sys/stat.h>
#include <sys/param.h>
#include <stdlib.h>
#include <sstream>


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


bool
findExectuable( std::string &filepath, const std::string &name )
{
	filepath.clear();
	const char *p = getenv( "PATH" );
	if ( ! p )
		return false;

	std::vector<std::string> paths = String::split( p, OS::pathSeparator() );
	bool ret = find( filepath, name, paths );
#ifdef WIN32
	if ( ! ret )
		ret = find( filepath, name + ".exe", paths );
#endif
	return ret;
}


////////////////////////////////////////


void
startParsing( const std::string &dir )
{
	Directory &curDir = Directory::currentDirectory();
	if ( ! dir.empty() )
		curDir.reset( dir );

	std::string firstFile;
	if ( curDir.exists( firstFile, buildFileName() ) )
		Lua::Engine::singleton().runFile( firstFile.c_str() );
	else
	{
		std::stringstream msg;
		msg << "Unable to find " << buildFileName() << " in " << curDir.fullpath();
		throw std::runtime_error( msg.str() );
	}
}


////////////////////////////////////////


void
registerFunctions( void )
{
	Lua::Engine &eng = Lua::Engine::singleton();
	eng.registerFunction( "exists", &exists );
}

} // namespace File


////////////////////////////////////////




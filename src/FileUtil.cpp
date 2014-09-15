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
#include "Scope.h"

#include <unistd.h>
#include <errno.h>
#include <system_error>
#include <sys/stat.h>
#include <sys/param.h>
#include <stdlib.h>
#include <sstream>
#include <fstream>
#include <fcntl.h>
#include <string.h>
#include <iostream>
#include <mutex>


////////////////////////////////////////


namespace
{

static std::once_flag theNeedPathInit;
static std::vector<std::string> thePath;
static void initPath( void )
{
	thePath = String::split( OS::getenv( "PATH" ), OS::pathSeparator() );
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
		e = Directory::current()->exists( tmp, fn );
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
		std::string tmp = Directory::current()->makefilename( fn );
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


std::string
extension( const std::string &fn )
{
	std::string::size_type p = fn.find_last_of( '.' );
	if ( p == std::string::npos )
		return std::string();

	return fn.substr( p );
}


////////////////////////////////////////


std::string
replaceExtension( const std::string &fn, const std::string &newext )
{
	std::string::size_type p = fn.find_last_of( '.' );
	if ( p == std::string::npos )
		return fn + newext;

	std::string ret = fn.substr( 0, p );
	return ret + newext;
}


////////////////////////////////////////


bool
diff( const char *fn, const std::vector<std::string> &lines )
{
	std::ifstream f( fn );
	if ( ! f )
		return true;
	std::string curline;
	size_t lIdx = 0;
	bool founddiff = false;
	while ( f )
	{
		std::getline( f, curline );

		if ( lIdx >= lines.size() )
		{
			if ( ! curline.empty() )
				founddiff = true;
			break;
		}

		if ( curline != lines[lIdx] )
		{
			founddiff = true;
			break;
		}

		++lIdx;
	}
	if ( lIdx < lines.size() )
		founddiff = true;

	return founddiff;
}


////////////////////////////////////////


bool
compare( const char *pathA, const char *pathB )
{
	if ( ! pathA )
		throw std::runtime_error( "nil path A passed for comparison" );
	if ( ! pathB )
		throw std::runtime_error( "nil path B passed for comparison" );
	if ( strcmp( pathA, pathB ) == 0 )
		return false;

	int fdA = open( pathA, O_RDONLY );
	ON_EXIT{ if ( fdA >= 0 ) close( fdA ); };
	if ( fdA < 0 )
	{
		std::stringstream msg;
		msg << "Unable to open file '" << pathA << "' for comparison";
		throw std::system_error( errno, std::system_category(), msg.str() );
	}

	int fdB = open( pathB, O_RDONLY );
	ON_EXIT{ if ( fdB >= 0 ) close( fdB ); };
	if ( fdB < 0 )
	{
		std::stringstream msg;
		msg << "Unable to open file '" << pathB << "' for comparison";
		throw std::system_error( errno, std::system_category(), msg.str() );
	}

	char aBuf[4096], bBuf[4096];
	ssize_t nA = 0, nB = 0;
	off_t offset = 0;
	do
	{
		nA = pread( fdA, aBuf, 4096, offset );
		if ( nA < 0 )
		{
			if ( errno == EINTR )
				continue;

			std::stringstream msg;
			msg << "Error reading from file '" << pathA << "'";
			throw std::system_error( errno, std::system_category(), msg.str() );
		}
		nB = pread( fdB, bBuf, 4096, offset );
		if ( nB < 0 )
		{
			if ( errno == EINTR )
				continue;

			std::stringstream msg;
			msg << "Error reading from file '" << pathB << "'";
			throw std::system_error( errno, std::system_category(), msg.str() );
		}

		if ( nA != nB )
			break;

		if ( nA == 0 )
			break;

		if ( memcmp( aBuf, bBuf, nA ) != 0 )
			return true;

		offset += nA;
	} while ( true );

	return nA != nB;
}


////////////////////////////////////////


bool
find( std::string &filepath, const std::vector<std::string> &names )
{
	return Directory::current()->find( filepath, names );
}


////////////////////////////////////////


bool
find( std::string &filepath, const std::vector<std::string> &names,
	  const std::vector<std::string> &path )
{
	for ( const auto &p: path )
	{
		try
		{
			if ( isAbsolute( p.c_str() ) )
			{
				Directory d( p );
				if ( d.find( filepath, names ) )
					return true;
			}
			else
			{
				Directory d( *Directory::current() );
				d.cd( p );
				if ( d.find( filepath, names ) )
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


std::map<std::string, std::string>
find( std::vector<std::string> progs,
	  const std::vector<std::string> &path,
	  const std::vector<std::string> &extensions )
{
	std::map<std::string, std::string> ret;
	std::string filepath;
	for ( const auto &p: path )
	{
		if ( progs.empty() )
			break;

		try
		{
			Directory d( p );
			for ( size_t i = 0; i != progs.size(); ++i )
			{
				const std::string &name = progs[i];
				try
				{
					if ( d.exists( filepath, name ) )
					{
						ret[name] = filepath;
						progs.erase( progs.begin() + i );
						--i;
						continue;
					}

					for ( const auto &e: extensions )
					{
						if ( d.exists( filepath, name + e ) )
						{
							ret[name] = filepath;
							progs.erase( progs.begin() + i );
							--i;
							break;
						}
					}
				}
				catch ( ... )
				{
				}
			}
		}
		catch ( ... )
		{
		}
	}
	return std::move( ret );
}


////////////////////////////////////////


void
setPathOverride( const std::vector<std::string> &p )
{
	std::call_once( theNeedPathInit, &initPath );
	thePath = p;
}


////////////////////////////////////////


const std::vector<std::string> &
getPath( void )
{
	std::call_once( theNeedPathInit, &initPath );
	return thePath;
}


////////////////////////////////////////


bool
findExecutable( std::string &filepath, const std::string &name )
{
	std::call_once( theNeedPathInit, &initPath );

	bool ret = find( filepath, name, thePath );
#ifdef WIN32
	if ( ! ret )
		ret = find( filepath, name + ".exe", thePath );
#endif
	return ret;
}


////////////////////////////////////////


std::map<std::string, std::string>
findExecutables( std::vector<std::string> progs )
{
	std::call_once( theNeedPathInit, &initPath );

#ifdef WIN32
	return find( std::move( progs ), thePath, { ".exe" } );
#endif

	return find( std::move( progs ), thePath );
}

} // namespace File


////////////////////////////////////////




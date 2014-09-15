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
#include "StrUtil.h"

#include <unistd.h>
#include <errno.h>
#include <system_error>
#include <sys/stat.h>
#include <sys/param.h>
#include <stdlib.h>
#include <sstream>
#include <iostream>
#include <vector>
#include <stack>
#include <mutex>

namespace 
{

static std::once_flag theNeedCWDInit;

static std::string theCWD;
static std::vector<std::string> theCWDPath;

static void initCWD( void )
{
	char *cwd = getcwd( NULL, MAXPATHLEN );
	if ( ! cwd )
		throw std::system_error( errno, std::system_category(),
								 "Unable to query current directory" );
	ON_EXIT{ free( cwd ); };

	theCWD = cwd;
	theCWDPath = String::split( theCWD, File::pathSeparator() );
}

static std::stack< std::shared_ptr<Directory> > theLiveDirs;

} // empty namespace


////////////////////////////////////////


Directory::Directory( void )
{
	std::call_once( theNeedCWDInit, &initCWD );

	myRoot = theCWD;
	myFullDirs = theCWDPath;
	myCurFullPath = myRoot;
}


////////////////////////////////////////


Directory::Directory( const std::string &root )
		: myRoot( root )
{
	myFullDirs = String::split( myRoot, File::pathSeparator() );
	myCurFullPath = myRoot;
}


////////////////////////////////////////


Directory::Directory( std::string &&root )
		: myRoot( std::move( root ) )
{
	myFullDirs = String::split( myRoot, File::pathSeparator() );
	myCurFullPath = myRoot;
}


////////////////////////////////////////


Directory::Directory( const Directory &d )
		: myRoot( d.myRoot ), mySubDirs( d.mySubDirs ),
		  myFullDirs( d.myFullDirs ),
		  myCurFullPath( d.myCurFullPath )
{
}


////////////////////////////////////////


Directory::Directory( Directory &&d )
		: myRoot( std::move( d.myRoot ) ),
		  mySubDirs( std::move( d.mySubDirs ) ),
		  myFullDirs( std::move( d.myFullDirs ) ),
		  myCurFullPath( std::move( d.myCurFullPath ) )
{
}


////////////////////////////////////////


Directory &
Directory::operator=( const Directory &d )
{
	if ( this != &d )
	{
		myRoot = d.myRoot;
		mySubDirs = d.mySubDirs;
		myFullDirs = d.myFullDirs;
		myCurFullPath = d.myCurFullPath;
	}
	return *this;
}


////////////////////////////////////////


Directory &
Directory::operator=( Directory &&d )
{
	if ( this != &d )
	{
		myRoot = std::move( d.myRoot );
		mySubDirs = std::move( d.mySubDirs );
		myFullDirs = std::move( d.myFullDirs );
		myCurFullPath = std::move( d.myCurFullPath );
	}
	return *this;
}


////////////////////////////////////////


Directory
Directory::reroot( const std::string &newroot ) const
{
#ifdef WIN32
	throw std::runtime_error( "Need to handle UNC paths and / and \\ in split" );
#endif
	Directory ret( newroot );
	ret.mySubDirs = mySubDirs;
	ret.updateFullPath();
	return std::move( ret );
}


////////////////////////////////////////


void
Directory::rematch( const Directory &d )
{
	bool needChange = true;
	if ( d.mySubDirs.size() == mySubDirs.size() )
	{
		needChange = false;
		for ( size_t i = 0; i != mySubDirs.size(); ++i )
		{
			if ( mySubDirs[i] != d.mySubDirs[i] )
			{
				needChange = true;
				break;
			}
		}
	}
	if ( needChange )
	{
		mySubDirs = d.mySubDirs;
		updateFullPath();
	}
}


////////////////////////////////////////


void
Directory::cd( const std::string &name )
{
	std::vector<std::string> dirs = String::split( name, File::pathSeparator() );
	if ( dirs.empty() )
		return;

	for ( auto &d: dirs )
		mySubDirs.emplace_back( std::move( d ) );

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
#ifdef WIN32
	throw std::logic_error( "NYI: Need to handle drive letters" );
#endif
	std::string tmpPath;
	std::vector<std::string> alldirs;
	combinePath( alldirs );
	for ( const auto &curDir: alldirs )
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


std::string
Directory::relpath( void ) const
{
	std::string ret;

	for ( const std::string &p: mySubDirs )
	{
		if ( ! ret.empty() )
			ret.push_back( File::pathSeparator() );
		ret.append( p );
	}
	return std::move( ret );
}


////////////////////////////////////////


void
Directory::updateFullPath( void )
{
	std::vector<std::string> pathElements;
	combinePath( pathElements );

#ifdef WIN32
	throw std::logic_error( "NYI: Need to handle drive letters" );
#endif
	myCurFullPath.clear();
	for ( const std::string &i: pathElements )
	{
		myCurFullPath.push_back( File::pathSeparator() );
		myCurFullPath.append( i );
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


std::string
Directory::relfilename( const std::string &fn ) const
{
	std::string cpath = relpath();
	cpath.push_back( File::pathSeparator() );
	cpath.append( fn );
	return std::move( cpath );
}


////////////////////////////////////////


const std::shared_ptr<Directory> &
Directory::current( void )
{
	if ( theLiveDirs.empty() )
		theLiveDirs.push( std::make_shared<Directory>() );

	return theLiveDirs.top();
}


////////////////////////////////////////


const std::shared_ptr<Directory> &
Directory::pushd( const std::string &d )
{
	std::shared_ptr<Directory> newD = std::make_shared<Directory>( *current() );
	newD->cd( d );
	theLiveDirs.push( newD );
	return theLiveDirs.top();
}


////////////////////////////////////////


const std::shared_ptr<Directory> &
Directory::popd( void )
{
	if ( theLiveDirs.empty() )
		throw std::runtime_error( "Directory pushd / popd mismatch" );
	theLiveDirs.pop();
	if ( theLiveDirs.empty() )
		throw std::runtime_error( "Directory pushd / popd mismatch" );
	return theLiveDirs.top();
}


////////////////////////////////////////


void
Directory::combinePath( std::vector<std::string> &elements ) const
{
	elements = myFullDirs;
	for ( const std::string &p: mySubDirs )
	{
		if ( p == "." )
			continue;

		if ( p == ".." )
		{
			if ( elements.empty() )
				throw std::runtime_error( "Invalid attempt to create relative path above root" );

			elements.pop_back();
		}
		else
			elements.push_back( p );
	}

}


////////////////////////////////////////



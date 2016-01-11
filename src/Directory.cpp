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

#include "ScopeGuard.h"
#include "StrUtil.h"
#include "Debug.h"

#include <unistd.h>
#include <errno.h>
#include <system_error>
#include <sys/stat.h>
#include <sys/param.h>
#include <stdlib.h>
#include <sstream>
#include <iostream>
#include <fstream>
#include <list>
#include <iterator>
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

std::shared_ptr<Directory> theLastDir;
static std::stack< std::shared_ptr<Directory> > theLiveDirs;

} // empty namespace


////////////////////////////////////////


Directory::Directory( void )
{
	std::call_once( theNeedCWDInit, &initCWD );

	myFullDirs = theCWDPath;
	myCurFullPath = theCWD;
}


////////////////////////////////////////


Directory::Directory( const std::string &root )
{
	myFullDirs = String::split( root, File::pathSeparator() );
	myCurFullPath = root;
}


////////////////////////////////////////


Directory::Directory( const Directory &d )
		: mySubDirs( d.mySubDirs ),
		  myFullDirs( d.myFullDirs ),
		  myCurFullPath( d.myCurFullPath )
{
}


////////////////////////////////////////


Directory::Directory( Directory &&d )
		: mySubDirs( std::move( d.mySubDirs ) ),
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
		mySubDirs = std::move( d.mySubDirs );
		myFullDirs = std::move( d.myFullDirs );
		myCurFullPath = std::move( d.myCurFullPath );
	}
	return *this;
}


////////////////////////////////////////


void
Directory::extractDirFromFile( const std::string &fn )
{
	myFullDirs = String::split( fn, File::pathSeparator() );
	if ( ! myFullDirs.empty() )
		myFullDirs.pop_back();
	mySubDirs.clear();
	updateFullPath();
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
	return ret;
}


////////////////////////////////////////


std::shared_ptr<Directory>
Directory::reroot( const std::shared_ptr<Directory> &newroot ) const
{
	std::shared_ptr<Directory> ret = std::make_shared<Directory>( *newroot );
	ret->mySubDirs = mySubDirs;
	ret->updateFullPath();
	return ret;
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
	if ( ! mySubDirs.empty() )
		mySubDirs.pop_back();
	else
	{
		if ( myFullDirs.empty() )
			throw std::runtime_error( "Attempt to change directories above root" );
		myFullDirs.pop_back();
	}
	
	updateFullPath();
}


////////////////////////////////////////


const std::string &
Directory::cur( void ) const
{
	if ( ! mySubDirs.empty() )
		return mySubDirs.back();

	if ( ! myFullDirs.empty() )
		return myFullDirs.back();

	return String::empty();
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
	return ret;
}


////////////////////////////////////////


void
Directory::promoteFull( void )
{
	for ( auto &d: mySubDirs )
		myFullDirs.emplace_back( std::move( d ) );
	mySubDirs.clear();
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
	return concatpath;
}


////////////////////////////////////////


std::string
Directory::makefilename( const std::string &fn ) const
{
	std::string concatpath = fullpath();
	concatpath.push_back( File::pathSeparator() );
	concatpath.append( fn );
	return concatpath;
}


////////////////////////////////////////


std::string
Directory::relfilename( const std::string &fn ) const
{
	std::string cpath = relpath();
	cpath.push_back( File::pathSeparator() );
	cpath.append( fn );
	return cpath;
}


////////////////////////////////////////


std::string
Directory::relativeTo( const Directory &o,
					   const std::string &fn ) const
{
	std::vector<const std::string *> mynonCommonSubdirs;
	mynonCommonSubdirs.reserve( myFullDirs.size() + mySubDirs.size() );
	for ( const std::string &i: myFullDirs )
		mynonCommonSubdirs.push_back( &i );
	for ( const std::string &i: mySubDirs )
		mynonCommonSubdirs.push_back( &i );

	std::vector<const std::string *> relPath;
	relPath.reserve( o.myFullDirs.size() + o.mySubDirs.size() );
	for ( const std::string &i: o.myFullDirs )
		relPath.push_back( &i );
	for ( const std::string &i: o.mySubDirs )
		relPath.push_back( &i );

	size_t subdirI = 0;
	size_t relI = 0;
	while ( true )
	{
		if ( subdirI == mynonCommonSubdirs.size() || relI == relPath.size() )
			break;
		if ( *(mynonCommonSubdirs[subdirI]) != *(relPath[relI]) )
			break;

		++subdirI;
		++relI;
	}

	std::string ret;
	bool notfirst = false;
	while ( relI < relPath.size() )
	{
		if ( notfirst )
			ret.push_back( File::pathSeparator() );
		ret.append( ".." );
		++relI;
		notfirst = true;
	}
	notfirst = false;
	while ( subdirI < mynonCommonSubdirs.size() )
	{
		if ( notfirst )
			ret.push_back( File::pathSeparator() );
		ret.append( *(mynonCommonSubdirs[subdirI]) );
		++subdirI;
		notfirst = true;
	}

	if ( ! fn.empty() )
	{
		if ( ! ret.empty() )
			ret.push_back( File::pathSeparator() );
		ret.append( fn );
	}

	return ret;
}


////////////////////////////////////////


void
Directory::updateIfDifferent( const std::string &name, const std::vector<std::string> &lines )
{
	bool doCreate = true;
	std::string fn;
	if ( exists( fn, name ) )
	{
		std::ifstream inf( fn );
		size_t curIdx = 0;
		std::string curLine;
		do
		{
			if ( std::getline( inf, curLine ) )
			{
				if ( curIdx >= lines.size() )
					break;

				if ( lines[curIdx] != curLine )
				{
					VERBOSE( name << ": line " << (curIdx + 1) << " differs: '" << curLine << "' vs '" << lines[curIdx] << "' - regenerating" );
					break;
				}
				++curIdx;
			}
			else if ( curIdx == lines.size() )
			{
				doCreate = false;
				break;
			}
			else
			{
				VERBOSE( name << ": line count different - regenerating" );
				break;
			}
		} while ( doCreate );
	}
	
	if ( doCreate )
	{
		mkpath();
		fn = makefilename( name );
		VERBOSE( "Creating/updating '" << fn << "'..." );
		std::ofstream outf( fn );
		for ( const std::string &l: lines )
			outf << l << '\n';
	}
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
	theLastDir = current();
	theLiveDirs.push( newD );
	return theLiveDirs.top();
}


////////////////////////////////////////


const std::shared_ptr<Directory> &
Directory::popd( void )
{
	if ( theLiveDirs.empty() )
		throw std::runtime_error( "Directory pushd / popd mismatch" );

	theLastDir = theLiveDirs.top();
	theLiveDirs.pop();

	if ( theLiveDirs.empty() )
		throw std::runtime_error( "Directory pushd / popd mismatch" );

	return theLiveDirs.top();
}


////////////////////////////////////////


const std::shared_ptr<Directory> &
Directory::last( void )
{
	return theLastDir;
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




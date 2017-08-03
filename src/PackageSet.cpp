//
// Copyright (c) 2016 Kimball Thurston
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

#include "PackageSet.h"

#include <dirent.h>
#include <unistd.h>
#include <errno.h>
#include <system_error>
#include <sys/stat.h>
#include <sys/param.h>
#include <stdlib.h>
#include <string>
#include <algorithm>
#include <utility>
#include <map>

#include "StrUtil.h"
#include "FileUtil.h"
#include "Directory.h"
#include "OSUtil.h"
#include "ScopeGuard.h"
#include "Debug.h"


////////////////////////////////////////


void
PackageSet::resetPackageSearchPath( void )
{
	myPkgSearchPath.clear();
	myInit = false;
	myPackageConfigs.clear();
}


////////////////////////////////////////


void
PackageSet::setPackageSearchPath( const std::string &p )
{
	resetPackageSearchPath();
	addPackagePath( p );
}


////////////////////////////////////////


void
PackageSet::addPackagePath( const std::string &p )
{
	// all internal things like this are to be
	// in unix path style
	String::split_append( myPkgSearchPath, p, ':' );
}


////////////////////////////////////////


void
PackageSet::resetLibSearchPath( void )
{
	myLibSearchPath.clear();
}


////////////////////////////////////////


void
PackageSet::setLibSearchPath( const std::string &p )
{
	resetLibSearchPath();
	addLibPath( p );
}


////////////////////////////////////////


void
PackageSet::addLibPath( const std::string &p )
{
	// all internal things like this are to be
	// in unix path style
	String::split_append( myLibSearchPath, p, ':' );
}


////////////////////////////////////////


std::shared_ptr<PackageConfig>
PackageSet::find( const std::string &name,
				  const std::string &reqVersion )
{
	VersionCompare vc = VersionCompare::ANY;
	if ( ! reqVersion.empty() )
	{
		std::string::size_type verPos = reqVersion.find_first_not_of( "<>=!" );
		if ( verPos == std::string::npos )
			throw std::runtime_error( "Invalid version specification, missing version number" );
		std::string vercmp;
		std::string ver;
		if ( verPos > 0 )
		{
			vercmp = reqVersion.substr( 0, verPos );
			ver = reqVersion.substr( verPos );
		}
		else
			ver = reqVersion;

		String::strip( vercmp );
		String::strip( ver );

		if ( vercmp.empty() || vercmp == "=" )
			vc = VersionCompare::EQUAL;
		else if ( vercmp == "!=" )
			vc = VersionCompare::NOT_EQUAL;
		else if ( vercmp == "<" )
			vc = VersionCompare::LESS;
		else if ( vercmp == "<=" )
			vc = VersionCompare::LESS_EQUAL;
		else if ( vercmp == ">" )
			vc = VersionCompare::GREATER;
		else if ( vercmp == ">=" )
			vc = VersionCompare::GREATER_EQUAL;

		return find( name, vc, ver );
	}

	return find( name, vc, reqVersion );
}


////////////////////////////////////////


std::shared_ptr<PackageConfig>
PackageSet::find( const std::string &name,
				  const std::string &reqVersion,
				  const std::vector<std::string> &libPath,
				  const std::vector<std::string> &pkgPath )
{
	std::vector<std::string> tl;
	std::vector<std::string> tp;
	std::map<std::string, std::string> tpc;
	std::map<std::string, std::shared_ptr<PackageConfig> > tppc;

	if ( ! libPath.empty() )
	{
		std::swap( tl, myLibSearchPath );
		myLibSearchPath = libPath;
	}

	bool restorpkg = false;
	if ( ! pkgPath.empty() )
	{
		myInit = false;
		restorpkg = true;
		std::swap( tp, myPkgSearchPath );
		myPkgSearchPath = pkgPath;
		std::swap( tpc, myPackageConfigs );
		std::swap( tppc, myParsedPackageConfigs );
	}

	try
	{
		auto ret = find( name, reqVersion );
		myInit = false;
		if ( ! tl.empty() )
			std::swap( myLibSearchPath, tl );
		if ( restorpkg )
		{
			std::swap( myPkgSearchPath, tp );
			std::swap( myPackageConfigs, tpc );
			std::swap( myParsedPackageConfigs, tppc );
		}
		return ret;
	}
	catch ( ... )
	{
		if ( ! tl.empty() )
			std::swap( myLibSearchPath, tl );
		if ( restorpkg )
		{
			std::swap( myPkgSearchPath, tp );
			std::swap( myPackageConfigs, tpc );
			std::swap( myParsedPackageConfigs, tppc );
		}
		throw;
	}
}



////////////////////////////////////////


std::shared_ptr<PackageConfig>
PackageSet::find( const std::string &name, VersionCompare comp, const std::string &reqVersion )
{
	init();

	std::shared_ptr<PackageConfig> ret;

	auto preparsed = myParsedPackageConfigs.find( name );
	if ( preparsed != myParsedPackageConfigs.end() )
		ret = preparsed->second;
	else
	{
		auto f = myPackageConfigs.find( name );
		if ( f != myPackageConfigs.end() )
		{
			DEBUG( "using pkg-config information for " << name );
			ret = std::make_shared<PackageConfig>( f->first, f->second );
			// we can't call this in the constructor
			ret->parse();
			//std::cout << "found package '" << name << "' for system " << mySystem << std::endl;
			myParsedPackageConfigs[f->first] = ret;
			// pull in any dependent libraries
			extractOtherModules( *ret, ret->getRequires(), true );
		}
		else
		{
			DEBUG( "Searching in OS path for library " << name );
			std::string libpath;
			if ( mySystem == "Darwin" )
			{
				// look for framework
				if ( File::find( libpath, name, {".framework"}, myLibSearchPath ) )
				{
					ret = makeLibraryReference( name, libpath );
					myParsedPackageConfigs[name] = ret;
				}
				// look for framework
				else if ( File::find( libpath, "lib" + name, {".dylib", ".a"}, myLibSearchPath ) )
				{
					ret = makeLibraryReference( name, libpath );
					myParsedPackageConfigs[name] = ret;
				}
				else
				{
					auto p = name.find( "lib" );
					std::string altname = name;
					if ( p != std::string::npos && ( (p + 3) == altname.size() ) )
					{
						altname.erase( p );
						if ( File::find( libpath, "lib" + altname, {".dylib", ".a"}, myLibSearchPath ) )
						{
							ret = makeLibraryReference( altname, libpath );
							myParsedPackageConfigs[name] = ret;
						}
					}
				}
			}
			else if ( mySystem == "Windows" )
			{
				if ( File::find( libpath, name, {".lib", ".a"}, myLibSearchPath ) )
				{
					ret = makeLibraryReference( name, libpath );
					myParsedPackageConfigs[name] = ret;
				}
				else if ( File::find( libpath, "lib" + name, {".dll.a", ".a"}, myLibSearchPath ) )
				{
					// for mingw
					ret = makeLibraryReference( name, libpath );
					myParsedPackageConfigs[name] = ret;
				}
				else
				{
					auto p = name.find( "lib" );
					std::string altname = name;
					if ( p != std::string::npos && ( (p + 3) == altname.size() ) )
					{
						altname.erase( p );
						if ( File::find( libpath, altname, {".lib", ".a"}, myLibSearchPath ) )
						{
							ret = makeLibraryReference( altname, libpath );
							myParsedPackageConfigs[name] = ret;
						}
					}
				}
			}
			else
			{
				if ( File::find( libpath, "lib" + name, {".so", ".a"}, myLibSearchPath ) )
				{
					ret = makeLibraryReference( name, libpath );
					myParsedPackageConfigs[name] = ret;
				}
				else
				{
					auto p = name.find( "lib" );
					std::string altname = name;
					if ( p != std::string::npos && ( (p + 3) == altname.size() ) )
					{
						altname.erase( p );
						if ( File::find( libpath, "lib" + altname, {".so", ".a"}, myLibSearchPath ) )
						{
							ret = makeLibraryReference( altname, libpath );
							myParsedPackageConfigs[name] = ret;
						}
					}
				}
			}
		}
	}

	if ( ret )
	{
		if ( ! reqVersion.empty() )
		{
			DEBUG( "Comparing found version '" << ret->getVersion() << "' to requested version '" << reqVersion << "'" );
		}
		int rc = String::versionCompare( ret->getVersion(), reqVersion );
		bool zap = false;
		switch ( comp )
		{
			case VersionCompare::NOT_EQUAL: zap = ( rc == 0 ); break;
			case VersionCompare::EQUAL: zap = ( rc != 0 ); break;
			case VersionCompare::LESS: zap = ( rc >= 0 ); break;
			case VersionCompare::LESS_EQUAL: zap = ( rc > 0 ); break;
			case VersionCompare::GREATER:zap = ( rc <= 0 ); break;
			case VersionCompare::GREATER_EQUAL: zap = ( rc < 0 ); break;
			case VersionCompare::ANY: break;
		}

		if ( zap )
		{
			WARNING( "Found package '" << name << "' (" << ret->getName() << "), version " << ret->getVersion()
					 << " but failed version check against requested version '" << reqVersion << "'" );
			ret.reset();
		}
	}

	return ret;
}


////////////////////////////////////////


PackageSet &
PackageSet::get( const std::string &sys )
{
	static std::map<std::string, PackageSet> theSets;
	if ( sys.empty() )
		return get( OS::system() );

	auto i = theSets.find( sys );
	if ( i == theSets.end() )
		return theSets.emplace( std::make_pair( sys, PackageSet( sys ) ) ).first->second;

	return i->second;
}


////////////////////////////////////////


PackageSet::PackageSet( const std::string &s )
		: mySystem( s )
{
	if ( mySystem == OS::system() )
	{
		const char *pcPath = getenv( "PKG_CONFIG_PATH" );
		if ( pcPath )
			addPackagePath( std::string( pcPath ) );
		pcPath = getenv( "PKG_CONFIG_LIBDIR" );
		if ( pcPath )
			addPackagePath( std::string( pcPath ) );
		else
		{
			myPkgSearchPath.push_back( "/usr/lib/pkgconfig" );
			myPkgSearchPath.push_back( "/usr/local/lib/pkgconfig" );
		}

#ifdef __APPLE__
		myLibSearchPath.emplace_back( "/System/Library/Frameworks" );
		myLibSearchPath.emplace_back( "/Library/Frameworks" );
#endif
#ifndef WIN32
		myLibSearchPath.emplace_back( "/lib" );
		myLibSearchPath.emplace_back( "/usr/lib" );
		myLibSearchPath.emplace_back( "/usr/local/lib" );
#endif
	}
}


////////////////////////////////////////


void
PackageSet::init( void )
{
	if ( myInit )
		return;
	myInit = true;

	DEBUG( "---------- PackageSet::init --------------" );

	for ( auto &i: myPkgSearchPath )
	{
		// first trim any trailing slashes, win32 opendir doesn't seem to like it
		String::strip( i );
		File::trimTrailingSeparators( i );

		DIR *d = ::opendir( i.c_str() );
		if ( d )
		{
			ON_EXIT{ ::closedir( d ); };
			// glibc deprecates readdir_r in 2.24...
#if defined(__GNU_LIBRARY__) && ( __GLIBC__ > 2 || ( __GLIBC__ == 2 && __GLIBC_MINOR__ >= 24 ) ) 
			while ( true )
			{
				errno = 0;
				struct dirent *cur = ::readdir( d );
				if ( ! cur )
				{
					if ( errno != 0 )
					{
						std::cerr << "WARNING: error reading directory '" << i << "'" << std::endl;
					}
					break;
				}

				std::string cname = cur->d_name;
				std::string::size_type ePC = cname.rfind( ".pc", std::string::npos, 3 );
				if ( ePC != std::string::npos )
				{
					if ( ePC == ( cname.size() - 3 ) )
					{
						std::string name = cname.substr( 0, ePC );
						// if we found the same name earlier, ignore this one
						if ( myPackageConfigs.find( name ) == myPackageConfigs.end() )
						{
							std::string fullpath = i;
							fullpath.push_back( File::pathSeparator() );
							fullpath.append( cname );
							DEBUG( name << ": " << fullpath );
							myPackageConfigs[name] = fullpath;
						}
					}
				}
			}
#else
			std::unique_ptr<uint8_t[]> rdBuf;
			size_t allocSize = 0;
			long name_max = fpathconf( dirfd( d ), _PC_NAME_MAX );
			if ( name_max == -1 )
			{
#if defined(NAME_MAX)
                name_max = (NAME_MAX > 255) ? NAME_MAX : 255;
#else
                name_max = 255;
#endif
			}
			allocSize = sizeof(struct dirent) + static_cast<size_t>( name_max ) + 1;
			rdBuf.reset( new uint8_t[allocSize] );

			struct dirent *curDir = reinterpret_cast<struct dirent *>( rdBuf.get() );
			struct dirent *cur = nullptr;
			while ( readdir_r( d, curDir, &cur ) == 0 )
			{
				if ( ! cur )
					break;

				std::string cname = cur->d_name;
				std::string::size_type ePC = cname.rfind( ".pc", std::string::npos, 3 );
				if ( ePC != std::string::npos )
				{
					if ( ePC == ( cname.size() - 3 ) )
					{
						std::string name = cname.substr( 0, ePC );
						// if we found the same name earlier, ignore this one
						if ( myPackageConfigs.find( name ) == myPackageConfigs.end() )
						{
							std::string fullpath = i;
							fullpath.push_back( File::pathSeparator() );
							fullpath.append( cname );
							myPackageConfigs[name] = fullpath;
						}
					}
				}
			}
#endif
		}
	}
}


////////////////////////////////////////


namespace {
enum class ModNameParseState
{
	LOOKING = 0,
	IN_NAME = 1,
	LOOKING_OP = 2,
	IN_OP = 3,
	LOOKING_VER = 4,
	IN_VERSION = 5
};
}

void
PackageSet::extractOtherModules( PackageConfig &pc, const std::string &val, bool required )
{
	if ( val.empty() )
		return;

	++myParseDepth;
	ON_EXIT{ --myParseDepth; };

	std::string::size_type p = 0, N = val.size();
	std::string::size_type nameStart = std::string::npos;
	std::string::size_type nameEnd = std::string::npos;
	std::string::size_type opStart = std::string::npos;
	std::string::size_type opEnd = std::string::npos;
	std::string::size_type verStart = std::string::npos;
	std::string::size_type verEnd = std::string::npos;

	ModNameParseState curState = ModNameParseState::LOOKING;
	ModNameParseState lastState = ModNameParseState::LOOKING;
	while ( p != N )
	{
		char curC = val[p];
		switch ( curState )
		{
			case ModNameParseState::LOOKING:
				if ( curC != ',' && ! std::isspace( curC ) )
				{
					opStart = opEnd = std::string::npos;
					verStart = verEnd = std::string::npos;
					nameStart = p;
					nameEnd = std::string::npos;
					curState = ModNameParseState::IN_NAME;
				}
				break;
			case ModNameParseState::IN_NAME:
				if ( std::isspace( curC ) )
				{
					nameEnd = p;
					curState = ModNameParseState::LOOKING_OP;
				}
				else if ( curC == ',' )
				{
					nameEnd = p;
					curState = ModNameParseState::LOOKING;
				}

				if ( ( p + 1 ) == N )
				{
					nameEnd = p + 1;
					curState = ModNameParseState::LOOKING;
				}
				break;
			case ModNameParseState::LOOKING_OP:
				if ( curC == '<' || curC == '>' || curC == '=' || curC == '!' )
				{
					opStart = p;
					curState = ModNameParseState::IN_OP;
				}
				else if ( curC == ',' )
					curState = ModNameParseState::LOOKING;
				else if ( ! std::isspace( curC ) )
				{
					curState = ModNameParseState::LOOKING;
					--p;
				}

				if ( ( p + 1 ) == N )
					curState = ModNameParseState::LOOKING;
				break;
			case ModNameParseState::IN_OP:
				if ( !( curC == '<' || curC == '>' || curC == '=' || curC == '!' ) )
				{
					opEnd = p;
					curState = ModNameParseState::LOOKING_VER;
				}

				if ( ( p + 1 ) == N )
				{
					opEnd = p + 1;
					curState = ModNameParseState::LOOKING;
				}
				break;
			case ModNameParseState::LOOKING_VER:
				if ( ! std::isspace( curC ) )
				{
					verStart = p;
					curState = ModNameParseState::IN_VERSION;
				}
				if ( ( p + 1 ) == N )
					curState = ModNameParseState::LOOKING;
				break;
			case ModNameParseState::IN_VERSION:
				if ( curC == ',' || std::isspace( curC ) )
				{
					verEnd = p;
					curState = ModNameParseState::LOOKING;
				}

				if ( ( p + 1 ) == N )
				{
					verEnd = p + 1;
					curState = ModNameParseState::LOOKING;
				}
				break;
		}

		if ( curState == ModNameParseState::LOOKING &&
			 lastState != ModNameParseState::LOOKING )
		{
			if ( nameStart == std::string::npos || nameEnd == std::string::npos )
				throw std::logic_error( "Error in state machine parsing module names" );

			std::string name = val.substr( nameStart, nameEnd - nameStart );
			std::shared_ptr<PackageConfig> cur;
			std::string verC, ver;
			if ( opStart != std::string::npos )
			{
				if ( verStart == std::string::npos || verEnd == std::string::npos )
				{
					std::cerr << "ERROR: mal-formed package module version check specification: found operator but no version to check against" << std::endl;
					cur = find( name );
				}
				else
				{
					ver = val.substr( verStart, verEnd - verStart );
					verC = val.substr( opStart, opEnd - opStart );
					VersionCompare vc = VersionCompare::ANY;
					if ( verC == "=" )
						vc = VersionCompare::EQUAL;
					else if ( verC == "!=" )
						vc = VersionCompare::NOT_EQUAL;
					else if ( verC == "<" )
						vc = VersionCompare::LESS;
					else if ( verC == "<=" )
						vc = VersionCompare::LESS_EQUAL;
					else if ( verC == ">" )
						vc = VersionCompare::GREATER;
					else if ( verC == ">=" )
						vc = VersionCompare::GREATER_EQUAL;
					else
						throw std::runtime_error( "Invalid operator string: " + verC );
					cur = find( name, vc, ver );
				}
			}
			else
				cur = find( name );

			if ( required && ! cur )
			{
				std::stringstream msg;
				msg << "Unable to find required package '" << name << "'";
				if ( ! ver.empty() )
					msg << ", version " << verC << ' '<< ver;
				msg << " - please ensure it is installed or the package config search path is set appropriately";
				throw std::runtime_error( msg.str() );
			}
			if ( cur )
				pc.addDependency( DependencyType::EXPLICIT, cur );
		}
		lastState = curState;
		++p;
	}
}


////////////////////////////////////////


std::shared_ptr<PackageConfig>
PackageSet::makeLibraryReference( const std::string &name,
								  const std::string &path )
{
	std::shared_ptr<PackageConfig> ret = std::make_shared<PackageConfig>( name, std::string() );
	VERBOSE( "Creating external (non- pkg-config) library reference for '" << name << "'..." );

#ifdef WIN32
	throw std::runtime_error( "Not yet implemented for Windows" );
#endif

#ifdef __APPLE__
	if ( path.find( ".framework" ) != std::string::npos )
	{
		// put it as one value so any repeated-value
		// compression doesn't remove the -framework bit
		// but multiple of the same will be cleaned
		std::string incval = "-F " + name;
		std::string varval = "-framework " + name;
		Variable &incs = ret->getVariable( "includes" );
		incs.add( incval );
		Variable &libs = ret->getVariable( "ldflags" );
		libs.add( varval );
		return ret;
	}
#endif

	Directory d( path );
	d.cdUp();

	std::string basepath = d.fullpath();
	
	Variable &libs = ret->getVariable( "ldflags" );
	libs.add( "-l" + name );

	if ( basepath != "/lib" && basepath != "/usr/lib" && basepath != "/usr/local/lib" )
	{
		Variable &libdirs = ret->getVariable( "libdirs" );
		libdirs.add( basepath );
		libdirs.setToolTag( "ld" );

		d.cdUp();
		if ( d.exists( "include" ) )
		{
			d.cd( "include" );

			Variable &cflags = ret->getVariable( "includes" );
			cflags.setToolTag( "cc" );
			cflags.add( d.fullpath() );
		}
	}

	return ret;
}


////////////////////////////////////////



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

#include "PackageConfig.h"

#include <dirent.h>
#include <unistd.h>
#include <errno.h>
#include <system_error>
#include <sys/stat.h>
#include <sys/param.h>
#include <stdlib.h>
#include <string>
#include <map>
#include <mutex>
#include "StrUtil.h"
#include "FileUtil.h"
#include "ScopeGuard.h"
#include "LuaEngine.h"
#include <iostream>
#include <iomanip>
#include <fstream>


////////////////////////////////////////


namespace
{

std::map<std::string, std::string> thePackageConfigs;
std::map<std::string, std::shared_ptr<PackageConfig> > theParsedPackageConfigs;
#ifdef WIN32
static constexpr char theSeparator = ';';
#else
static constexpr char theSeparator = ':';
#endif
static std::once_flag theNeedInit;

static int theParseDepth = 0;

void
init( void )
{
	std::vector<std::string> searchPaths;
	const char *pcPath = getenv( "PKG_CONFIG_PATH" );
	if ( pcPath )
		String::split_append( searchPaths, std::string( pcPath ), theSeparator );
	pcPath = getenv( "PKG_CONFIG_LIBDIR" );
	if ( pcPath )
		String::split_append( searchPaths, std::string( pcPath ), theSeparator );
	else
	{
		searchPaths.push_back( "/usr/lib/pkgconfig" );
		searchPaths.push_back( "/usr/local/lib/pkgconfig" );
	}

	for ( auto &i: searchPaths )
	{
		// first trim any trailing slashes, win32 opendir doesn't seem to like it
		String::strip( i );
		File::trimTrailingSeparators( i );

		DIR *d = ::opendir( i.c_str() );
		if ( d )
		{
			ON_EXIT{ ::closedir( d ); };

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
			allocSize = std::max( sizeof(struct dirent), size_t( offsetof(struct dirent, d_name) + name_max + 1 ) );
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
						if ( thePackageConfigs.find( name ) == thePackageConfigs.end() )
						{
							std::string fullpath = i;
							fullpath.push_back( File::pathSeparator() );
							fullpath.append( cname );
							thePackageConfigs[name] = fullpath;
						}
					}
				}
			}
		}
	}
}

} // empty namespace


////////////////////////////////////////


PackageConfig::PackageConfig( const std::string &n, const std::string &pf )
		: myName( n ), myPackageFile( pf )
{
	parse();
}


////////////////////////////////////////


PackageConfig::~PackageConfig( void )
{
}


////////////////////////////////////////


const std::string &
PackageConfig::version( void ) const
{
	return getAndReturn( "Version" );
}


////////////////////////////////////////


const std::string &
PackageConfig::package( void ) const
{
	return getAndReturn( "Name" );
}


////////////////////////////////////////


const std::string &
PackageConfig::description( void ) const
{
	return getAndReturn( "Description" );
}


////////////////////////////////////////


const std::string &
PackageConfig::conflicts( void ) const
{
	return getAndReturn( "Conflicts" );
}


////////////////////////////////////////


const std::string &
PackageConfig::url( void ) const
{
	return getAndReturn( "URL" );
}


////////////////////////////////////////


const std::string &
PackageConfig::cflags( void ) const
{
	return getAndReturn( "CFlags" );
}


////////////////////////////////////////


const std::string &
PackageConfig::libs( void ) const
{
	return getAndReturn( "Libs" );
}


////////////////////////////////////////


const std::string &
PackageConfig::libsStatic( void ) const
{
	return getAndReturn( "Libs.private" );
}


////////////////////////////////////////


const std::string &
PackageConfig::requires( void ) const
{
	return getAndReturn( "Requires" );
}


////////////////////////////////////////


const std::string &
PackageConfig::requiresStatic( void ) const
{
	return getAndReturn( "Requires.private" );
}


////////////////////////////////////////


const std::vector< std::shared_ptr<PackageConfig> > &
PackageConfig::dependencies( void ) const
{
	return myDepends;
}


////////////////////////////////////////


const std::string &
PackageConfig::getAndReturn( const char *tag ) const
{
	static std::string theNullStr;

	auto i = myValues.find( tag );
	if ( i != myValues.end() )
		return i->second;
	return theNullStr;
}


////////////////////////////////////////


void
PackageConfig::parse( void )
{
	std::ifstream infl( myPackageFile );
	std::string curline;
	std::string parseline;
	using std::swap;

	while ( infl )
	{
		parseline.clear();
		bool linecontinue = false;
		do
		{
			std::getline( infl, curline );
			std::string::size_type commentPos = curline.find_first_of( '#' );
			while ( commentPos != std::string::npos )
			{
				if ( commentPos > 0 && curline[commentPos-1] == '\\' )
				{
					commentPos = curline.find_first_of( '#', commentPos + 1 );
					continue;
				}
				curline.erase( commentPos );
				break;
			}

			if ( ! curline.empty() && curline.back() == '\\' )
			{
				curline.pop_back();
				linecontinue = true;
			}
			parseline.append( curline );
		} while ( linecontinue );

		String::strip( parseline );

		if ( parseline.empty() )
			continue;

		extractNameAndValue( parseline );
	}

	// now load any dependencies. do we care about private requires?
	const std::string &req = requires();
	if ( ! req.empty() )
	{
		++theParseDepth;
		ON_EXIT{ --theParseDepth; };
		myDepends = extractOtherModules( req, true );
	}
}


////////////////////////////////////////


void
PackageConfig::extractNameAndValue( const std::string &curline )
{
	std::string name, val;

	std::string::size_type i = 0;
	while ( i < curline.size() )
	{
		if ( std::isalnum( curline[i] ) || curline[i] == '_' || curline[i] == '.' )
		{
			++i;
			continue;
		}
		break;
	}

	if ( i >= curline.size() )
		return;

	if ( i > 0 )
		name = curline.substr( 0, i );

	while ( i < curline.size() && std::isspace( curline[i] ) )
		++i;

	if ( i >= curline.size() )
		return;

	char separator = curline[i];
	++i;
	while ( i < curline.size() && std::isspace( curline[i] ) )
		++i;

	if ( i < curline.size() )
		val = curline.substr( i );

	String::substituteVariables( val, true, myVariables );

	if ( separator == ':' )
	{
		if ( myValues.find( name ) != myValues.end() )
		{
			std::cerr << "WARNING: Package config file '" << myPackageFile << "' has multiple entries for tag '" << name << "'" << std::endl;
			return;
		}

		if ( name == "Name" || name == "Description" || name == "URL" )
		{
			myValues[name] = val;
		}
		else if ( name == "Version" )
		{
			std::cout << std::setw( theParseDepth * 2 ) << std::setfill( ' ' ) << "" << "Found package '" << myName << "', version " << val << std::endl;
			myValues[name] = val;
		}
		else if ( name == "Libs.private" || name == "Libs" )
		{
			// pkg-config parses these as shell arguments and attempts to collapse them when
			// chained, let's not bother with that, it doesn't seem needed
			myValues[name] = val;
		}
		else if ( name == "Requires.private" || name == "Requires" )
		{
//			myDepends[name] = extractOtherModules( val, true );
			myValues[name] = val;
		}
		else if ( name == "Cflags" || name == "CFlags" )
		{
			// make sure we only have to look up by the one name later
			myValues["CFlags"] = val;
		}
		else if ( name == "Conflicts" )
		{
			// do we care about this???
			//myDepends[name] = extractOtherModules( val, false );
			myValues[name] = val;
		}
		else
			std::cerr << "WARNING: Ignoring unknown package config tag: '" << name << "', value: " << val << std::endl;

	}
	else if ( separator == '=' )
	{
		if ( myVariables.find( name ) != myVariables.end() )
		{
			std::cerr << "WARNING: Package config file '" << myPackageFile << "' has multiple entries for variable '" << name << "'" << std::endl;
			return;
		}
		// do we need to implement the special handling
		// for trying to auto-figure out the prefix?
		// this seems to be done on Windows in the pkg-config
		// source. Let's cross that bridge when we get to it
//		if ( name == "prefix" )
//		{
//			
//		}
		myVariables[name] = val;
	}
	else
	{
		std::cerr << "WARNING: Ignoring bogus line in pkg config file: " << myPackageFile << ": " << curline << std::endl;
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

std::vector< std::shared_ptr<PackageConfig> >
PackageConfig::extractOtherModules( const std::string &val, bool required )
{
	std::vector< std::shared_ptr<PackageConfig> > ret;
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
			default:
				throw std::logic_error( "Unknown module parse state" );
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
					cur = PackageConfig::find( name );
				}
				else
				{
					std::string ver = val.substr( verStart, verEnd - verStart );
					std::string verC = val.substr( opStart, opEnd - opStart );
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
					cur = PackageConfig::find( name, vc, ver );
				}
			}
			else
				cur = PackageConfig::find( name );

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
				ret.emplace_back( std::move( cur ) );
		}
		lastState = curState;
		++p;
	}

	return std::move( ret );
}


////////////////////////////////////////


std::shared_ptr<PackageConfig>
PackageConfig::find( const std::string &name, VersionCompare comp, const std::string &reqVersion )
{
	std::call_once( theNeedInit, &init );

	std::shared_ptr<PackageConfig> ret;

	auto f = thePackageConfigs.find( name );
	if ( f != thePackageConfigs.end() )
	{
		auto preparsed = theParsedPackageConfigs.find( name );
		if ( preparsed != theParsedPackageConfigs.end() )
			ret = preparsed->second;
		else
		{
			ret = std::make_shared<PackageConfig>( f->first, f->second );
			theParsedPackageConfigs[f->first] = ret;
		}
	}

	if ( ret )
	{
		int rc = String::versionCompare( ret->version(), reqVersion );
		bool zap = false;
		switch ( comp )
		{
			case VersionCompare::EQUAL: zap = ( rc != 0 ); break;
			case VersionCompare::LESS: zap = ( rc >= 0 ); break;
			case VersionCompare::LESS_EQUAL: zap = ( rc > 0 ); break;
			case VersionCompare::GREATER:zap = ( rc <= 0 ); break;
			case VersionCompare::GREATER_EQUAL: zap = ( rc < 0 ); break;
			case VersionCompare::ANY: break;
			default: break;
		}

		if ( zap )
		{
			std::cout << "WARNING: Found package '" << name << "' (" << ret->name() << "), version " << ret->version()
					  << " but failed version check against requested version '" << reqVersion << "'" << std::endl;
			ret.reset();
		}
	}
	return ret;
}

static bool
PackageExists( const std::string &name, const std::string &reqVersion )
{
	auto x = PackageConfig::find( name, VersionCompare::ANY, reqVersion );
	if ( x )
		return true;
	return false;
}


////////////////////////////////////////


void
PackageConfig::registerFunctions( void )
{
	Lua::Engine &eng = Lua::Engine::singleton();
	eng.registerFunction( "PackageExists", &PackageExists );
}


////////////////////////////////////////




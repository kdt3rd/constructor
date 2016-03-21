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

#pragma once

#include "PackageConfig.h"


////////////////////////////////////////


enum class VersionCompare
{
	ANY, // default match any version
	EQUAL, // equal to (=)
	NOT_EQUAL, // not equal to (!=)
	LESS, // less than (<)
	LESS_EQUAL, // less than or equal to (<=)
	GREATER, // greater than (>)
	GREATER_EQUAL // greater than or equal to (>=)
};
	
class PackageSet
{
public:
	void resetPackageSearchPath( void );
	void setPackageSearchPath( const std::string &p );
	void addPackagePath( const std::string &p );

	void resetLibSearchPath( void );
	void setLibSearchPath( const std::string &p );
	void addLibPath( const std::string &p );

	std::shared_ptr<PackageConfig> find( const std::string &name,
										 const std::string &reqVersion );
	std::shared_ptr<PackageConfig> find( const std::string &name,
										 const std::string &reqVersion,
										 const std::vector<std::string> &libPath,
										 const std::vector<std::string> &pkgPath );
	std::shared_ptr<PackageConfig> find( const std::string &name,
										 VersionCompare comp = VersionCompare::ANY,
										 const std::string &reqVersion = std::string() );

	static PackageSet &get( const std::string &sys = std::string() );

private:
	PackageSet( const std::string &sys );

	void init( void );

	void extractOtherModules( PackageConfig &pc, const std::string &val, bool required );

	std::shared_ptr<PackageConfig> makeLibraryReference( const std::string &name,
														 const std::string &path );

	std::string mySystem;
	std::vector<std::string> myPkgSearchPath;
	std::vector<std::string> myLibSearchPath;

	std::map<std::string, std::string> myPackageConfigs;
	std::map<std::string, std::shared_ptr<PackageConfig>> myParsedPackageConfigs;
	int myParseDepth = 0;
	bool myInit = false;
};


////////////////////////////////////////




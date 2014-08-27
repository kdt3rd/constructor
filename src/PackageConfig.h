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

#pragma once

#include <memory>
#include <string>
#include <map>
#include <vector>


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
	
class PackageConfig
{
public:
	PackageConfig( const std::string &n, const std::string &pkgfile );
	~PackageConfig( void );

	inline const std::string &name( void ) const { return myName; }
	inline const std::string &filename( void ) const { return myPackageFile; }

	const std::string &version( void ) const;
	const std::string &package( void ) const;
	const std::string &description( void ) const;
	const std::string &conflicts( void ) const;
	const std::string &url( void ) const;
	const std::string &cflags( void ) const;
	const std::string &libs( void ) const;
	const std::string &libsStatic( void ) const;
	const std::string &requires( void ) const;
	const std::string &requiresStatic( void ) const;
	const std::vector< std::shared_ptr<PackageConfig> > &dependencies( void ) const;

	//	inline const std::vector<PackageConfig> &dependsOn( void ) const { return myDepends; }

	static std::shared_ptr<PackageConfig> find( const std::string &name,
												VersionCompare comp = VersionCompare::ANY,
												const std::string &reqVersion = std::string() );
	static void registerFunctions( void );

private:
	const std::string &getAndReturn( const char *tag ) const;

	void parse( void );
	void extractNameAndValue( const std::string &curline );
	std::vector< std::shared_ptr<PackageConfig> > extractOtherModules( const std::string &val, bool required );

	std::string myName;
	std::string myPackageFile;
	std::map<std::string, std::string> myVariables;
	std::map<std::string, std::string> myValues;
	// do we care about anything but direct requires?
	std::vector< std::shared_ptr<PackageConfig> > myDepends;
};


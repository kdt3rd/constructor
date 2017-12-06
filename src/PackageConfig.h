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
#include <iostream>
#include "Item.h"


////////////////////////////////////////


class PackageConfig : public Item
{
public:
	PackageConfig( const std::string &n, const std::string &pkgfile );
	~PackageConfig( void );

	inline const std::string &getFilename( void ) const { return myPackageFile; }

	const std::string &getVersion( void ) const;
	const std::string &getPackage( void ) const;
	const std::string &getDescription( void ) const;
	const std::string &getConflicts( void ) const;
	const std::string &getURL( void ) const;
	const std::string &getCFlags( void ) const;
	const std::string &getLibs( void ) const;
	const std::string &getStaticLibs( void ) const;
	const std::string &getRequires( void ) const;
	const std::string &getStaticRequires( void ) const;

	virtual std::shared_ptr<BuildItem> transform( TransformSet &xform ) const;
	virtual void forceTool( const std::string &t );
	virtual void forceTool( const std::string &ext, const std::string &t );
	virtual void overrideToolSetting( const std::string &s, const std::string &n );

private:
	const std::string &getAndReturn( const char *tag ) const;

	void parse( void );
	void extractNameAndValue( const std::string &curline );
	friend class PackageSet;

	std::string myPackageFile;
	std::map<std::string, std::string> myLocalVars;
	std::map<std::string, std::string> myValues;
	std::string myNilValue;
};

inline std::ostream &operator<<( std::ostream &os, const PackageConfig &pc )
{
	os << "{"
	   << "\n  name: " << pc.getName()
	   << "\n  file: " << pc.getFilename()
	   << "\n  version: " << pc.getVersion()
	   << "\n  package: " << pc.getPackage()
	   << "\n  description: " << pc.getDescription()
	   << "\n  conflicts: " << pc.getConflicts()
	   << "\n  url: " << pc.getURL()
	   << "\n  cflags: " << pc.getCFlags()
	   << "\n  libs: " << pc.getLibs()
	   << "\n  statlibs: " << pc.getStaticLibs()
	   << "\n  requires: " << pc.getRequires()
	   << "\n  statreq: " << pc.getStaticRequires() << "\n}";

	return os;
}

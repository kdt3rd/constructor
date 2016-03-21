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

#include <memory>
#include <string>
#include <vector>
#include <map>

#include "Tool.h"


////////////////////////////////////////


class Toolset
{
public:
	Toolset( std::string name );
	~Toolset( void );

	inline const std::string &getName( void ) const { return myName; }

	inline void setTag( std::string t ) { myTag = std::move( t ); }
	inline const std::string &getTag( void ) const { return myTag; }

	void addLibSearchPath( const std::string &p );
	void addPkgSearchPath( const std::string &p );

	inline bool empty( void ) const { return myTools.empty(); }

	void addTool( const std::shared_ptr<Tool> &t );
	bool hasTool( const std::shared_ptr<Tool> &t ) const;
	std::shared_ptr<Tool> findTool( const std::string &name ) const;

	inline const std::vector<std::string> &getLibSearchPath( void ) const { return myLibPath; }
	inline const std::vector<std::string> &getPkgSearchPath( void ) const { return myPkgPath; }

private:
	std::string myName;
	std::string myTag;
	std::map< std::string, std::shared_ptr<Tool> > myTools;
	std::vector<std::string> myLibPath;
	std::vector<std::string> myPkgPath;
};





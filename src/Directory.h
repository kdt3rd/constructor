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

#include <string>
#include <vector>


////////////////////////////////////////


class Directory
{
public:
	Directory( void );
	Directory( std::string root );

	void reset( std::string root );

	void cd( std::string name );
	void cdUp( void );

	void mkpath( void ) const;
	const std::string &fullpath( void ) const;

	// returns the first name found
	bool find( std::string &concatpath, const std::vector<std::string> &names ) const;
	bool exists( std::string &concatpath, const char *fn ) const;
	bool exists( std::string &concatpath, const std::string &fn ) const;
	std::string makefilename( const char *fn ) const;
	std::string makefilename( const std::string &fn ) const;

	// NB: modifying this will modify global state, but doesn't change
	// any O.S. level current directory, at least currently. if we
	// support running scripts in the future, this may have to change
	static Directory &currentDirectory( void );

	static void registerFunctions( void );

private:
	bool checkRootPath( void ) const;
	void updateFullPath( void );

	std::string myRoot;
	std::vector<std::string> mySubDirs;
	std::string myCurFullPath;
};

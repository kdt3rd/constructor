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
#include <memory>
#include "FileUtil.h"


////////////////////////////////////////


class Directory
{
public:
	Directory( void );
	Directory( const std::string &root );
	Directory( const Directory &d );
	Directory( Directory &&d );
	Directory &operator=( const Directory &d );
	Directory &operator=( Directory &&d );

	void extractDirFromFile( const std::string &fn );

	Directory reroot( const std::string &newroot ) const;
	std::shared_ptr<Directory> reroot( const std::shared_ptr<Directory> &newroot ) const;
	void rematch( const Directory &d );

	void cd( const std::string &name );
	void cdUp( void );

	void mkpath( void ) const;
	// kind of like realpath, but doesn't actually
	// check for existence or resolve any symlinks, but
	// cleans up multiple separators and ./.. paths
	const std::string &fullpath( void ) const;

	// only the path elements that have been cd-ed to
	std::string relpath( void ) const;

	void promoteFull( void );

	// returns the first name found
	bool find( std::string &concatpath, const std::vector<std::string> &names ) const;
	bool exists( std::string &concatpath, const char *fn ) const;
	bool exists( std::string &concatpath, const std::string &fn ) const;
	inline bool exists( const std::string &fn ) const;

	std::string makefilename( const char *fn ) const;
	std::string makefilename( const std::string &fn ) const;
	std::string relfilename( const std::string &fn ) const;

	std::string relativeTo( const Directory &o,
							const std::string &fn = std::string() ) const;

	// NB: modifying this will modify global state, but doesn't change
	// any O.S. level current directory, at least currently. if we
	// support running scripts in the future, this may have to change
	static const std::shared_ptr<Directory> &current( void );
	static const std::shared_ptr<Directory> &pushd( const std::string &subd );
	static const std::shared_ptr<Directory> &popd( void );
	static const std::shared_ptr<Directory> &last( void );

private:
	void combinePath( std::vector<std::string> &fullpath ) const;
	bool checkRootPath( void ) const;
	void updateFullPath( void );

	std::vector<std::string> mySubDirs;
	std::vector<std::string> myFullDirs;
	std::string myCurFullPath;
};


////////////////////////////////////////


inline bool
Directory::exists( const std::string &fn ) const
{
	std::string p;
	return exists( p, fn );
}


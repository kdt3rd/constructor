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
#include <map>


////////////////////////////////////////


namespace File
{

inline constexpr char pathSeparator( void ) 
{
#ifdef WIN32
	return '\\';
#else
	return '/';
#endif
}

void trimTrailingSeparators( std::string &path );
bool isAbsolute( const char *path );
bool exists( const char *path );
bool isDirectory( const char *path );

std::string extension( const std::string &fn );
std::string replaceExtension( const std::string &fn, const std::string &newext );

bool diff( const char *path, const std::vector<std::string> &lines );
bool compare( const char *pathA, const char *pathB );

// looks in the current directory for a file
// returns the first name found
bool find( std::string &filepath, const std::vector<std::string> &names );
// looks along the provided path, returns the first name found
bool find( std::string &filepath, const std::vector<std::string> &names,
		   const std::vector<std::string> &path );

// looks in the specified path for the given filename
// finds the first filepath found
bool find( std::string &filepath, const std::string &name, const std::vector<std::string> &path );
// looks in the specified path for the given filename + extension
// assumes the extension contains any separator (i.e. '.')
// returns the first result found
bool find( std::string &filepath, const std::string &name, const std::vector<std::string> &extensions, const std::vector<std::string> &path );
// looks along given path and returns a map of found
// items. checks for both prog and prog + extension
std::map<std::string, std::string>
find( std::vector<std::string> progs,
	  const std::vector<std::string> &path,
	  const std::vector<std::string> &extensions = std::vector<std::string>() );

void setPathOverride( const std::vector<std::string> &p );
const std::vector<std::string> &getPath( void );
bool findExecutable( std::string &filepath, const std::string &name );
std::map<std::string, std::string> findExecutables( std::vector<std::string> progs );

} // namespace File


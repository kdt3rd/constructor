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
#include <map>
#include <vector>
#include <memory>


////////////////////////////////////////


class Scope;
class Toolset;

class DefaultTools
{
public:
	static void checkAndAddCFamilies( Scope &s );

	/// returns the available default tool options which are then
	/// exposed as functions to modify the current configuration or
	/// top level scope
	static const std::vector<std::string> &getOptions( void );

protected:
#ifdef WIN32
	static std::shared_ptr<Toolset> checkAndAddCl( Scope &s, const std::map<std::string, std::string> &exelist );
#endif
	static std::shared_ptr<Toolset> checkAndAddClang( Scope &s, const std::map<std::string, std::string> &exelist );
	static std::shared_ptr<Toolset> checkAndAddGCC( Scope &s, const std::map<std::string, std::string> &exelist );
	static std::shared_ptr<Toolset> checkAndAddArchiver( Scope &s, const std::map<std::string, std::string> &exelist );
	static void addSelfGenerator( Scope &s );
};






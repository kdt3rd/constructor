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

#include "LuaValue.h"
#include "Variable.h"


////////////////////////////////////////


class Configuration
{
public:
	Configuration( void );
	Configuration( const Lua::Table &t );
	~Configuration( void );

	inline const std::string &name( void ) const;
	inline const VariableSet &vars( void ) const;

	static const Configuration &getDefault( void );
	static std::vector<Configuration> &defined( void );
	static void registerFunctions( void );

private:
	std::string myName;
	VariableSet myVariables;
};


////////////////////////////////////////


inline
const std::string &
Configuration::name( void ) const
{
	return myName;
}


////////////////////////////////////////


inline
const VariableSet &
Configuration::vars( void ) const
{
	return myVariables;
}



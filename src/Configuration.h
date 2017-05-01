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

#include "Scope.h"


////////////////////////////////////////


class Configuration
{
public:
	Configuration( void );
	Configuration( const std::string &n );

	inline const std::string &name( void ) const;
	inline Scope &getPseudoScope( void );
	inline const Scope &getPseudoScope( void ) const;

	const std::string &getSystem( void ) const;
	void setSystem( std::string s );

	inline void setSkipOnError( bool s ) { mySkipOnError = s; }
	inline bool isSkipOnError( void ) const { return mySkipOnError; }
	static const Configuration &getDefault( void );
	static void setDefault( std::string name );
	static bool haveDefault( void );
	static bool haveAny( void );
	static void checkDefault( void );
	static void creatingNewConfig( void );
	static void finishCreatingNewConfig( void );
	static bool inSubProject( void );
	static void pushSubProject( void );
	static void popSubProject( void );
	static Configuration &last( void );
	static std::vector<Configuration> &defined( void );

private:
	std::string myName;
	std::string mySystem;
	std::shared_ptr<Scope> myPseudoScope;
	bool mySkipOnError = false;
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
Scope &
Configuration::getPseudoScope( void )
{
	return *myPseudoScope;
}


////////////////////////////////////////


inline
const Scope &
Configuration::getPseudoScope( void ) const
{
	return *myPseudoScope;
}



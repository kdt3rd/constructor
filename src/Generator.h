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
#include <ostream>
#include "Directory.h"


////////////////////////////////////////


class Configuration;


////////////////////////////////////////


class Generator
{
public:
	Generator( std::string n, std::string d, std::string p );
	virtual ~Generator( void );

	inline const std::string &name( void ) const;
	inline const std::string &description( void ) const;
	inline const std::string &program( void ) const;

	virtual void targetCall( std::ostream &os,
							 const std::string &tname ) = 0;
	virtual void emit( const std::shared_ptr<Directory> &dest,
					   const Configuration &config,
					   int args, const char *argv[] ) = 0;

	static const std::vector< std::shared_ptr<Generator> > &available( void );
	static void registerGenerator( const std::shared_ptr<Generator> &g );

protected:
	std::string myName;
	std::string myDescription;
	std::string myProgram;
};


////////////////////////////////////////


inline const std::string &
Generator::name( void ) const
{
	return myName;
}


////////////////////////////////////////


inline const std::string &
Generator::description( void ) const
{
	return myDescription;
}


////////////////////////////////////////


inline const std::string &
Generator::program( void ) const
{
	return myProgram;
}


////////////////////////////////////////



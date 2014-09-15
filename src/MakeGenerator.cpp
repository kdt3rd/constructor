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

#include "MakeGenerator.h"
#include "FileUtil.h"
#include <fstream>
#include <iostream>
#include <unistd.h>


////////////////////////////////////////


MakeGenerator::MakeGenerator( std::string p )
		: Generator( "make", "Standard GNU makefile system",
					 std::move( p ) )
{
}


////////////////////////////////////////


MakeGenerator::~MakeGenerator( void )
{
}


////////////////////////////////////////


void
MakeGenerator::targetCall( std::ostream &os,
						   const std::string &tname )
{
	os << program();
	long np = sysconf( _SC_NPROCESSORS_ONLN );
	if ( np > 0 )
		os << " -J " << np;

	if ( tname.find_first_of( ' ' ) != std::string::npos )
		os << " \"" << tname << "\"";
	else if ( ! tname.empty() )
		os << ' ' << tname;
}



////////////////////////////////////////


void
MakeGenerator::emit( const std::shared_ptr<Directory> &d,
					 const Configuration &conf,
					 int argc, const char *argv[] )
{
	std::ofstream f( d->makefilename( "Makefile" ) );
	f <<
		".PHONY: all clean\n"
		".ONESHELL:\n"
		".DEFAULT: all\n"
		"\n";
	f << "all:\n\n";
	f << "clean:\n\t@echo \"Cleaning...\"\n";
}


////////////////////////////////////////


void
MakeGenerator::init( void )
{
	std::string makeP;
	if ( File::findExecutable( makeP, "make" ) )
	{
		registerGenerator( std::make_shared<MakeGenerator>( std::move( makeP ) ) );
	}
	else
	{
		std::cout << "WARNING: make not found in path, ignoring Makefile generator" << std::endl;
	}
}


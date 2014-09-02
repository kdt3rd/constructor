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

#include "NinjaGenerator.h"
#include "FileUtil.h"
#include "Configuration.h"
#include <fstream>
#include <iostream>


////////////////////////////////////////


NinjaGenerator::NinjaGenerator( std::string p )
		: Generator( "ninja", "Small, fast build system",
					 std::move( p ) )
{
}


////////////////////////////////////////


NinjaGenerator::~NinjaGenerator( void )
{
}


////////////////////////////////////////


void
NinjaGenerator::targetCall( std::ostream &os,
							const std::string &tname )
{
	os << program();
	if ( tname == "clean" )
		os << " -t " << tname;
	else if ( tname.find_first_of( ' ' ) != std::string::npos )
		os << " \"" << tname << "\"";
	else if ( ! tname.empty() )
		os << ' ' << tname;
}


////////////////////////////////////////


void
NinjaGenerator::emit( const Directory &d,
					  const Configuration &conf,
					  int argc, const char *argv[] )
{
	std::ofstream f( d.makefilename( "build.ninja" ) );
	f << "ninja_required_version = 1.3\n";
	
	f << "build all: phony ";
	
	f << "\n";
	f << "default all\n\n";

	Directory curD;
	f <<
		"rule regen_constructor\n"
		"  command = cd " << curD.fullpath() << " &&";
	for ( int a = 0; a < argc; ++a )
		f << ' ' << argv[a];
	f <<
		"\n  description = Regenerating build files..."
		"\n  generator = 1"
		"\n\n";

	f << "build build.ninja: regen_constructor |";
	for ( const std::string &x: Directory::visited() )
		f << ' ' << x;
	f << "\n\n";
}


////////////////////////////////////////


void
NinjaGenerator::init( void )
{
	std::string ninjaP;
	if ( File::findExecutable( ninjaP, "ninja" ) )
	{
		registerGenerator( std::make_shared<NinjaGenerator>( std::move( ninjaP ) ) );
	}
	else
	{
		std::cout << "WARNING: ninja not found in path, ignoring ninja generator" << std::endl;
	}
}



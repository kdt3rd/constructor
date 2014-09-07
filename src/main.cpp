// main.cpp -*- C++ -*-

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

#include "Item.h"
#include "LuaEngine.h"
#include <stdexcept>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <string.h>
#include "Debug.h"
#include "OSUtil.h"
#include "FileUtil.h"
#include "Directory.h"
#include "Compile.h"
#include "Tool.h"
#include "PackageConfig.h"
#include "Configuration.h"
#include "Generator.h"
#include "NinjaGenerator.h"
#include "MakeGenerator.h"


////////////////////////////////////////


static void
emitGenerators( int es )
{
	std::cout << "Available build generators:\n\n";
	for ( const auto &g: Generator::available() )
		std::cout << "   " << std::setw( 20 ) << std::setfill( ' ' ) << std::left << g->name() << ' ' << g->description() << '\n';
	std::cout << std::endl;
	exit( es );
}


////////////////////////////////////////


static void
usageAndExit( const char *argv0, int es )
{
	std::cout << argv0 << " [-C <configname>] [-G <generator>] [path]\n\n"
		" path              Specifies the root of the source tree for out-of-tree builds\n\n"
		"Options:\n\n"
		" -C|--config       Specifies which configuration to generate\n"
		" -no-config-dir    Disables sub-directory named by configuration\n"
		" -emit-wrapper     Creates a GNU makefile wrapper in source tree for all configurations\n"
		" -G|--generator    Specifies which generator to use\n"
		" --show-generators Displays a list of generators and exits\n"
		" --verbose         Displays messages as the build tree is processed\n"
		" --debug           Displays debugging messages\n"
		" -h|--help|-?      This help message\n"
		"\n";
	emitGenerators( es );
}


////////////////////////////////////////


static void
emitWrapper( Directory &srcDir,
			 const std::shared_ptr<Generator> &generator,
			 bool doConfigDir )
{
	std::string wrapper = srcDir.makefilename( "Makefile" );
	std::ofstream wf( wrapper );

	if ( doConfigDir )
	{
		const std::vector<Configuration> &clist = Configuration::defined();
		wf << ".PHONY: all clean";
		for ( const Configuration &c: clist )
			wf << ' ' << c.name();
		wf << "\n.ONESHELL:\n"
			".SHELLFLAGS := -e\n"
			".DEFAULT: all\n"
			"\n"
			"all: "
		   << Configuration::getDefault().name()
		   << "\n\n";
		for ( const Configuration &c: clist )
		{
			wf << c.name() << ":\n";
			Directory cdir;
			cdir.cd( c.name() );
			wf << "\t@pushd " << cdir.fullpath() << " > /dev/null && ";
			generator->targetCall( wf, std::string() );
			wf << " && popd > /dev/null\n\n";
		}
		wf
			<< "clean:\n\t@echo \"Cleaning...\"\n";
		for ( const Configuration &c: clist )
		{
			Directory cdir;
			cdir.cd( c.name() );
			wf << "\t@pushd " << cdir.fullpath() << " > /dev/null && ";
			generator->targetCall( wf, "clean" );
			wf << " && popd > /dev/null\n";
		}
	}
	else
	{
		Directory &outDir = Directory::binary();
		wf << ".PHONY: all clean";
		wf << "\n.ONESHELL:\n"
			".SHELLFLAGS := -e\n"
			".DEFAULT: all\n"
			"\n"
			"all:\n"
			"\t@pushd " << outDir.fullpath() << " > /dev/null && ";
		generator->targetCall( wf, std::string() );
		wf << " && popd > /dev/null\n\n"
		   <<
			"clean:\n\t@echo \"Cleaning...\"\n"
			"\t@pushd " << outDir.fullpath() << " > /dev/null && ";
		generator->targetCall( wf, "clean" );
		wf << " && popd > /dev/null\n\n";
	}
}


////////////////////////////////////////


int
main( int argc, const char *argv[] )
{
	try
	{
		NinjaGenerator::init();
		MakeGenerator::init();

		std::string config;
		std::string subdir;
		std::shared_ptr<Generator> generator;
		bool doConfigDir = true;
		bool doWrapper = false;

		for ( int i = 1; i < argc; ++i )
		{
			std::string tmp = argv[i];
			bool haveOption = false;
			if ( ! tmp.empty() && tmp[0] == '-' )
			{
				std::string::size_type nChars = 1;
				if ( tmp.size() > 1 && tmp[1] == '-' )
					++nChars;
				haveOption = true;
				tmp.erase( 0, nChars );
			}

			if ( haveOption )
			{
				if ( tmp == "h" || tmp == "help" || tmp == "?" )
					usageAndExit( argv[0], 0 );

				if ( tmp == "show-generators" )
					emitGenerators( 0 );

				if ( tmp == "no-config-dir" )
				{
					doConfigDir = false;
					continue;
				}

				if ( tmp == "emit-wrapper" )
				{
					doWrapper = true;
					continue;
				}

				if ( tmp == "debug" )
				{
					Debug::enable( true );
					continue;
				}

				if ( tmp == "verbose" )
				{
					Verbose::enable( true );
					continue;
				}

				if ( tmp == "G" || tmp == "generator" )
				{
					if ( ( i + 1 ) >= argc )
					{
						std::cerr << "ERROR: Missing argument for generator" << std::endl;
						usageAndExit( argv[0], 1 );
					}
					++i;
					for ( const auto &g: Generator::available() )
					{
						if ( g->name() == argv[i] )
							generator = g;
					}

					if ( ! generator )
					{
						std::cerr << "ERROR: Generator '" << argv[i] << "' not available. Please select an available one." << std::endl;
						emitGenerators( 1 );
					}
					continue;
				}

				if ( tmp == "C" || tmp == "config" )
				{
					if ( ( i + 1 ) >= argc )
					{
						std::cerr << "ERROR: Missing argument for config" << std::endl;
						usageAndExit( argv[0], 1 );
					}
					++i;
					config = argv[i];
					continue;
				}
			}
			else
			{
				if ( ! subdir.empty() )
				{
					std::cerr << "ERROR: Please only specify one path to a source tree" << std::endl;
					usageAndExit( argv[0], 1 );
				}
				subdir = std::move( tmp );
			}
		}

		if ( ! generator )
		{
			const auto &a = Generator::available();
			if ( a.empty() )
				throw std::runtime_error( "ERROR: No generators available, please install relevant tools" );
			generator = a.front();
//			std::cout << "Using default generator: " << generator->name() << std::endl;
		}

		OS::registerFunctions();
		File::registerFunctions();
		Directory::registerFunctions();
		PackageConfig::registerFunctions();
		Tool::registerFunctions();
		Item::registerFunctions();
		CompileSet::registerFunctions();
		Configuration::registerFunctions();

		Configuration::requestedConfig( config, doConfigDir );

		Directory::startParsing( subdir );

		const Configuration &cfg = Configuration::getActive();
		Directory &outDir = Directory::binary();
		outDir.mkpath();

		generator->emit( outDir, cfg, argc, argv );

		if ( doWrapper )
		{
			Directory srcDir;
			srcDir.cd( subdir );
			emitWrapper( srcDir, generator, doConfigDir );

		}
	}
	catch ( std::exception &e )
	{
		std::cerr << "ERROR: " << e.what() << std::endl;
		return -1;
	}
	catch ( ... )
	{
		std::cerr << "Unhandled exception" << std::endl;
		return -1;
	}

	return 0;
}


////////////////////////////////////////






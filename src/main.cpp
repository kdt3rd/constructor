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
#include "LuaExtensions.h"
#include "Directory.h"
#include "Compile.h"
#include "Scope.h"
#include "PackageConfig.h"
#include "Configuration.h"
#include "Generator.h"
#include "NinjaGenerator.h"
#include "MakeGenerator.h"
#include "CodeGenerator.h"
#include "Version.h"


////////////////////////////////////////


[[noreturn]] static void
emitGenerators( int es )
{
	std::cout << "Available build generators:\n\n";
	for ( const auto &g: Generator::available() )
		std::cout << "   " << std::setw( 20 ) << std::setfill( ' ' ) << std::left << g->name() << ' ' << g->description() << '\n';
	std::cout << std::endl;
	exit( es );
}


////////////////////////////////////////


[[noreturn]] static void
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
#ifndef NDEBUG
		" -d|--debug        Displays debugging messages\n"
#endif
		" -q|--quiet        Disables display of warning messages\n"
		" -v|--version      Displays the constructor version number\n"
		" -h|--help|-?      This help message\n"
		"\n"
		"----\n\n"
		"Built in data blob transform:\n"
			  << argv0 << " -embed_binary_cstring <outname> [-comma] [-file_prefix <fn>] [-file_suffix <fn>] [-item_prefix <fn>] [-item_suffix <fn>] [-item_indent <fn>] inputfile1 ...\n"
		" to be used with GenerateSourceDataFile to transform data into binary C strings for embedding in executables\n";
	emitGenerators( es );
}


////////////////////////////////////////


static void
emitWrapper( Directory &srcDir,
			 const std::shared_ptr<Generator> &generator,
			 bool doConfigDir,
			 int argc, const char *argv[] )
{
	std::string wrapper = srcDir.makefilename( "Makefile" );
	std::ofstream wf( wrapper );

	if ( doConfigDir )
	{
		const std::vector<Configuration> &clist = Configuration::defined();
		wf <<
			".SUFFIXES:\n"
			".DEFAULT: all\n"
			".ONESHELL:\n"
			".NOTPARALLEL:\n"
			".SILENT:\n"
			"\n"
			".PHONY: all clean graph config";
		for ( const Configuration &c: clist )
			wf << ' ' << c.name();
		wf <<
			"\n"
			"LIVE_CONFIG := " << Configuration::getDefault().name() << "\n"
			"\n";

		for ( const Configuration &c: clist )
		{
			wf <<
				"ifeq ($(findstring " << c.name() << ",${MAKECMDGOALS})," << c.name() << ")\n"
				"LIVE_CONFIG := " << c.name() << "\nendif\n";
		}

		wf <<
			"\nifeq (\"$(wildcard ${LIVE_CONFIG})\",\"\")\n"
			"NEED_CONFIG := config\n"
			"endif\n";

		wf << "\nTARGETS := $(filter-out all clean graph config";
		for ( const Configuration &c: clist )
			wf << ' ' << c.name();
		wf << ",${MAKECMDGOALS})\n"
			"MAKECMDGOALS :=\n"
			"\n"
			"all: ${LIVE_CONFIG}\n"
			"\n";

		wf << "config:\n"
			"\techo \"Generating Build Files...\"\n"
			"\t" << argv[0];
		for ( int a = 1; a < argc; ++a )
		{
			std::string arg = argv[a];
			if ( arg == "-emit-wrapper" || arg == "--emit-wrapper" )
				continue;
			wf << ' ' << argv[a];
		}
		wf << "\n\n";
		
		for ( const Configuration &c: clist )
		{
			Directory cdir;
			cdir.cd( c.name() );
			wf << c.name() << "/: ${NEED_CONFIG}\n\n";
			wf << c.name() << ": " << c.name() << "/\n";
			wf << "\t@cd " << cdir.fullpath() << "; ";
			generator->targetCall( wf, "${TARGETS}" );
			wf << "\n\n";
		}

		// make it so there is a rule for whatever target is specified
		// that just passes it on to the real live rule
		wf <<
			"\n"
			"${TARGETS}: all ;\n"
			"\n";

		wf
			<< "clean:\n\t@echo \"Cleaning...\"\n";
		for ( const Configuration &c: clist )
		{
			Directory cdir;
			cdir.cd( c.name() );
			wf << "\t@cd " << cdir.fullpath() << "; ";
			generator->targetCall( wf, "clean" );
			wf << '\n';
		}
		wf << "\n";
	}
	else
	{
		Directory outDir;
		wf << 
			".SUFFIXES:\n"
			".ONESHELL:\n"
			".NOTPARALLEL:\n"
			".SILENT:\n"
			".DEFAULT: all\n"
			"\n"
			".PHONY: all clean\n"
			"\n"
			"all:\n"
			"\t@cd " << outDir.fullpath() << "; ";
		generator->targetCall( wf, std::string() );
		wf <<
			"clean:\n\t@echo \"Cleaning...\"\n"
			"\t@cd " << outDir.fullpath() << "; ";
		generator->targetCall( wf, "clean" );
		wf << "\n\n";
	}
}


////////////////////////////////////////


int
main( int argc, const char *argv[] )
{
	try
	{
		File::setArgv0( argv[0] );

		NinjaGenerator::init();
		MakeGenerator::init();

		std::string config;
		std::string subdir;
		std::shared_ptr<Generator> generator;
		bool doConfigDir = true;
		bool doWrapper = false;

		bool generateCode = false;
		std::string generateOutputName;
		bool generateComma = false;
		std::string filePrefix;
		std::string fileSuffix;
		std::string itemPrefix;
		std::string itemSuffix;
		std::string itemIndent;
		std::vector<std::string> inpList;

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

#ifndef NDEBUG
				if ( tmp == "d" || tmp == "debug" )
				{
					Debug::enable( true );
					continue;
				}
#endif
				if ( tmp == "verbose" )
				{
					Verbose::enable( true );
					continue;
				}

				if ( tmp == "v" || tmp == "version" )
				{
					std::cout << "constructor " << Constructor::version() << std::endl;
					return 0;
				}

				if ( tmp == "q" || tmp == "quiet" )
				{
					Quiet::enable( true );
					continue;
				}

				if ( tmp == "embed_binary_cstring" )
				{
					generateCode = true;
					if ( ( i + 1 ) >= argc )
					{
						std::cerr << "ERROR: Missing argument for embed_binary_cstring" << std::endl;
						usageAndExit( argv[0], 1 );
					}
					++i;
					generateOutputName = argv[i];
					continue;
				}

				if ( tmp == "comma" )
				{
					if ( ! generateCode )
					{
						std::cerr << "ERROR: -comma argument only valid when running in embed code mode" << std::endl;
						usageAndExit( argv[0], 1 );
					}
					generateComma = true;
					continue;
				}

				if ( tmp == "file_prefix" || tmp == "file_suffix" ||
					 tmp == "item_prefix" || tmp == "item_suffix" ||
					 tmp == "item_indent" )
				{
					if ( ! generateCode )
					{
						std::cerr << "ERROR: -" << tmp << " argument only valid when running in embed code mode" << std::endl;
						usageAndExit( argv[0], 1 );
					}

					if ( ( i + 1 ) >= argc )
					{
						std::cerr << "ERROR: Missing argument for " << tmp << std::endl;
						usageAndExit( argv[0], 1 );
					}
					++i;
					if ( tmp == "file_prefix" )
						filePrefix = argv[i];
					else if ( tmp == "file_suffix" )
						fileSuffix = argv[i];
					else if ( tmp == "item_prefix" )
						itemPrefix = argv[i];
					else if ( tmp == "item_suffix" )
						itemSuffix = argv[i];
					else if ( tmp == "item_indent" )
						itemIndent = argv[i];
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
			else if ( generateCode )
			{
				inpList.push_back( argv[i] );
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

		if ( generateCode )
		{
			CodeGenerator::emitCode( generateOutputName,
									 inpList,
									 filePrefix, fileSuffix,
									 itemPrefix, itemSuffix,
									 itemIndent, generateComma );
			return 0;
		}

		if ( ! generator )
		{
			const auto &a = Generator::available();
			if ( a.empty() )
				throw std::runtime_error( "ERROR: No generators available, please install relevant tools" );
			generator = a.front();
//			std::cout << "Using default generator: " << generator->name() << std::endl;
		}

		Lua::registerExtensions();
		Lua::startParsing( subdir );

		for ( const Configuration &c: Configuration::defined() )
		{
			if ( config.empty() || c.name() == config )
			{
				std::shared_ptr<Directory> outDir = Directory::current();
				if ( doConfigDir )
				{
					outDir = Directory::pushd( c.name() );
					ON_EXIT{ Directory::popd(); };
					outDir->mkpath();
					generator->emit( outDir, c, argc, argv );
				}
				else
					generator->emit( outDir, c, argc, argv );
			}
		}

		if ( doWrapper )
		{
			Directory srcDir;
			srcDir.cd( subdir );
			emitWrapper( srcDir, generator, doConfigDir, argc, argv );
		}
	}
	catch ( std::exception &e )
	{
		std::cerr << "ERROR: " << e.what() << std::endl;
		return 1;
	}
	catch ( ... )
	{
		std::cerr << "Unhandled exception" << std::endl;
		return -1;
	}

	return 0;
}


////////////////////////////////////////






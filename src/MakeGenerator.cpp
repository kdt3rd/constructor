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
#include "TransformSet.h"
#include "LuaExtensions.h"
#include "Scope.h"
#include "Debug.h"
#include "StrUtil.h"
#include <fstream>
#include <iostream>
#include <vector>
#include <set>
#include <unistd.h>


////////////////////////////////////////


namespace
{

static inline std::string
escape_path( const std::string &fn )
{
	if ( fn.find_first_of( ' ' ) != std::string::npos )
		return "\"" + fn + "\"";
	return fn;
}

static void
addOutputList( std::ostream &os, const std::shared_ptr<BuildItem> &bi, bool addFirstSpace = true )
{
	auto outd = bi->getOutDir();
	bool notFirst = addFirstSpace;
	if ( outd )
	{
		for ( const std::string &bo: bi->getOutputs() )
		{
			if ( notFirst )
				os << ' ';

			os << escape_path( outd->makefilename( bo ) );
			notFirst = true;
		}
	}
	else
	{
		for ( const std::string &bo: bi->getOutputs() )
		{
			if ( notFirst )
				os << ' ';
			os << escape_path( bo );
			notFirst = true;
		}
	}

	if ( ! bi->getTool() )
	{
		std::vector< std::shared_ptr<BuildItem> > deps =
			bi->extractDependencies( DependencyType::EXPLICIT );
		for ( auto &d: deps )
		{
			addOutputList( os, d, notFirst );
			notFirst = true;
		}
	}
}

static void
addOutputDirMake( std::ostream &os, const std::shared_ptr<BuildItem> &bi )
{
	auto outd = bi->getOutDir();

	if ( outd )
	{
		for ( const std::string &bo: bi->getOutputs() )
		{
			std::string fn = outd->makefilename( bo );
			Directory tmp;
			tmp.extractDirFromFile( fn );
			os << " " << escape_path( tmp.fullpath() );
		}
	}
}

static void
emitTargets( std::ostream &os, std::vector<std::string> &defTargs, std::vector<std::string> &depFiles, const TransformSet &x )
{
	std::set< std::shared_ptr<Tool> > toolsInPlay;
	for ( const std::shared_ptr<BuildItem> &bi: x.getBuildItems() )
		toolsInPlay.insert( bi->getTool() );

	std::map<std::shared_ptr<Tool>, Rule> rules;
	for ( const std::shared_ptr<Tool> &t: toolsInPlay )
	{
		if ( t )
		{
			const Rule &r = t->createRule( x, true );
			rules[t] = r;
			os << '\n';
			for ( auto &v: r.getVariables() )
				os << v.first << " := " << v.second << '\n';
		}
	}

	std::set<std::string> outDirs;
	for ( const std::shared_ptr<BuildItem> &bi: x.getBuildItems() )
	{
		auto outd = bi->getOutDir();

		if ( outd )
		{
			for ( const std::string &bo: bi->getOutputs() )
			{
				std::string fn = outd->makefilename( bo );
				Directory tmp;
				tmp.extractDirFromFile( fn );
				if ( outDirs.find( tmp.fullpath() ) == outDirs.end() )
				{
					std::string p = escape_path( tmp.fullpath() );
					os << p << ":\n\t@mkdir -p " << p << '\n';
					outDirs.insert( tmp.fullpath() );
				}
			}
		}
	}
	
	for ( const std::shared_ptr<BuildItem> &bi: x.getBuildItems() )
	{
		auto t = bi->getTool();
		if ( t )
		{
			const Rule &r = rules[t];

			auto outd = bi->getOutDir();
			os << "\n";

			auto &bivars = bi->getVariables();
			for ( auto &bv: bivars )
			{
				std::string outv = bv.second.prepended_value( t->getCommandPrefix( bv.first ) );
				addOutputList( os, bi, false );
				os << ": override " << bv.first << ":=" << outv << '\n';
			}

			const std::string &dFile = r.getDependencyFile();
			if ( ! dFile.empty() )
			{
				if ( bi->getOutputs().size() != 1 )
					throw std::runtime_error( "Sorry Makefile generator does not support dependency files and multiple outputs" );
				std::stringstream depfilebuf;
				addOutputList( depfilebuf, bi, false );
				std::map<std::string, std::string> outV;
				outV["out"] = depfilebuf.str();
				std::string dfn = dFile;
				String::substituteVariables( dfn, false, outV );
				depFiles.push_back( dfn );
			}

			addOutputList( os, bi, false );
			os << ": override out := ";
			addOutputList( os, bi, false );
			os << "\n";

			std::vector< std::shared_ptr<BuildItem> > deps =
				bi->extractDependencies( DependencyType::EXPLICIT );
			addOutputList( os, bi, false );
			os << ": override in := ";
			bool notFirst = false;

			if ( bi->useName() )
			{
				os << escape_path( bi->getDir()->makefilename( bi->getName() ) );
				for ( auto &d: deps )
					addOutputList( os, d, true );
				notFirst = true;
			}
			else
			{
				for ( auto &d: deps )
				{
					addOutputList( os, d, notFirst );
					notFirst = true;
				}
			}
			os << "\n";

			addOutputList( os, bi, false );
			os << ": ";
			notFirst = false;
			if ( bi->useName() )
			{
				os << escape_path( bi->getDir()->makefilename( bi->getName() ) );
				for ( auto &d: deps )
					addOutputList( os, d, true );
				notFirst = true;
			}
			else
			{
				for ( auto &d: deps )
				{
					addOutputList( os, d, notFirst );
					notFirst = true;
				}
			}
			
			deps = bi->extractDependencies( DependencyType::IMPLICIT );
			if ( ! deps.empty() )
			{
				for ( auto &d: deps )
				{
					addOutputList( os, d, notFirst );
					notFirst = true;
				}
			}

			os << " |";
			addOutputDirMake( os, bi );

			deps = bi->extractDependencies( DependencyType::ORDER );
			if ( ! deps.empty() )
			{
				for ( auto &d: deps )
					addOutputList( os, d, true );
			}

			os << "\n\t@echo \"" << r.getDescription() << "\"";
			os << "\n\t@" << r.getCommand() << "\n";

			if ( bi->isTopLevelItem() )
			{
				PRECONDITION( bi->getOutputs().size() == 1,
							  "Expecting top level item to only have 1 output" );
				os << ".PHONY: " << bi->getName() << " clean-" << bi->getName() << "\n";
				os << bi->getName() << ": " << outd->makefilename( bi->getOutputs()[0] ) << '\n';
				defTargs.push_back( bi->getName() );

				os << "clean-" << bi->getName() << ":\n";
			}
		}
	}
}

static void
emitScope( std::ostream &os,
		   std::vector<std::string> &defTargs,
		   const Directory &outD,
		   const TransformSet &x,
		   int &scopeCount )
{
	for ( const std::shared_ptr<TransformSet> &i: x.getSubScopes() )
	{
		std::stringstream subscopebuf;
		subscopebuf << "sub_scope_" << (++scopeCount);
		std::string subscope = subscopebuf.str();
		std::stringstream subscopefn;
		subscopefn << subscope << ".build";
		std::string sfn = subscopefn.str();

		os << "\n" << subscope << ": " << sfn
		   << "\t@$(MAKE) -f " << subscopefn << "\n";

		std::ofstream ssf( outD.makefilename( sfn ) );
		ssf <<
			".PHONY: default all install clean\n"
			".SUFFIXES:\n"
			".ONESHELL:\n"
			".DEFAULT: default\n"
			"\n\n"
			"default: all\n";
		emitScope( ssf, defTargs, outD, *i, scopeCount );
	}

	std::vector<std::string> depFiles;
	emitTargets( os, defTargs, depFiles, x );
	os << '\n';
	for ( const std::string &d: depFiles )
		os << "-include " << d << '\n';
	if ( ! depFiles.empty() )
		os << '\n';
}

}


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
	Directory curD;
	{
		std::ofstream f( d->makefilename( "Makefile" ) );

		f <<
			".PHONY: all\n"
			".ONESHELL:\n"
			".SUFFIXES:\n"
			".DEFAULT: all\n"
			".NOTPARALLEL:\n"
			"\n"
			"TARGETS:=$(filter-out all,$(MAKECMDGOALS))\n"
			"MAKECMDGOALS:=\n"
			"MAKEFLAGS:=--no-print-directory\n"
			"all: Makefile.build\n\n"
			"\t@$(MAKE) -f Makefile.build $(TARGETS)\n\n"
			"Makefile.build: ";
		for ( const std::string &x: Lua::visitedFiles() )
			f << ' ' << x;
		f << "\n\t@echo \"Regenerating build files...\"\n";
		f << "\t@cd " << curD.fullpath() << " &&";
		for ( int a = 0; a < argc; ++a )
			f << ' ' << argv[a];
	}

	TransformSet xform( d );
	Scope::root().transform( xform, conf );

	std::ofstream rf( d->makefilename( "Makefile.build" ) );
	rf <<
		".PHONY: default all install clean\n"
		".SUFFIXES:\n"
		".ONESHELL:\n"
		".DEFAULT: all\n"
		"\n\ndefault: all\n";

	std::vector<std::string> defTargs;
	int scopeCount = 0;
	emitScope( rf, defTargs, *d, xform, scopeCount );

	rf << "all:";
	for ( const std::string &a: defTargs )
		rf << ' ' << a;

	rf << "\n\ninstall:\n\t@echo \"Installing...\"\n";

	rf << "\n\nclean:";
	for ( const std::string &a: defTargs )
		rf << " clean-" << a;
	rf << "\n\n";
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



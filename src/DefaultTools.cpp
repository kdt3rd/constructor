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

#include "DefaultTools.h"
#include "Scope.h"
#include "FileUtil.h"
#include "Tool.h"
#include "Debug.h"
#include <stdexcept>


////////////////////////////////////////


void
DefaultTools::checkAndAddCFamilies( Scope &s )
{
#ifdef WIN32
	throw std::runtime_error( "Not yet implemented" );
#endif

	std::map<std::string, std::string> exes = File::findExecutables(
		{
			"clang",
			"clang++",
			"gcc",
			"g++",
			"ar",
		} );
#ifdef WIN32
	bool makeDef = checkAndAddCl( s, exes, true );
#else
	bool makeDef = false;
#endif
	makeDef = checkAndAddClang( s, exes, ! makeDef );
	makeDef = checkAndAddGCC( s, exes, ! makeDef );
	checkAndAddArchiver( s, exes );
}


////////////////////////////////////////


#ifdef WIN32
bool
DefaultTools::checkAndAddCl( Scope &s, const std::map<std::string, std::string> &exelist, bool regAsDefault )
{
	return false;
}
#endif


////////////////////////////////////////


bool
DefaultTools::checkAndAddClang( Scope &s, const std::map<std::string, std::string> &exelist, bool regAsDefault )
{
	std::vector<std::string> cTools;
	for ( const auto &e: exelist )
	{
		const std::string &name = e.first;
		const std::string &exe = e.second;
		std::shared_ptr<Tool> t;
		if ( name == "clang" )
		{
			t = std::make_shared<Tool>( "cc", name );
			cTools.push_back( name );
			t->myExtensions = { ".c" };
			t->myOutputs = { ".o" };
			t->myExeName = exe;
			t->myOptions = 
				{
					{ "warnings", { { "none", { "-w" } },
									{ "default", {} },
									{ "some", { "-Wall" } },
									{ "strict", { "-Weverything", } },
									{ "most", { "-Weverything", } },
									{ "error", { "-Wall", "-Werror"  } }, } },
					{ "optimization", { { "debug", { "-g" } },
										{ "opt", { "-O3" } },
										{ "size", { "-Os" } },
										{ "optdebug", { "-g", "-O3" } }, } },
					{ "language", { { "C", {} },
									{ "C99", { "-std=c99" } },
									{ "C11", { "-std=c11" } } } },
					{ "threads", { { "on", { "-pthread" } },
								   { "off", {} }, } },
					{ "vectorize", { { "none", {} },
									 { "SSE", { "-msse" } },
									 { "SSE2", { "-msse2" } },
									 { "SSE3", { "-msse3", "-mtune=core2" } },
									 { "SSE4", { "-msse4", "-mtune=nehalem" } },
									 { "AVX", { "-mavx", "-mtune=intel" } },
									 { "AVX2", { "-mavx2", "-mtune=intel" } },
									 { "AVX512", { "-mavx512", "-mtune=intel" } },
									 { "native", { "-mtune=native", "-march=native" } },
						} },
				};
			t->myOptionDefaults = {
				{ "optimization", "opt" },
				{ "warnings", "default" },
				{ "language", "C" },
				{ "threads", "off" },
				{ "vectorize", "none" }
			};
			t->myImplDepName = "$out.d";
			t->myImplDepStyle = "gcc";
			t->myImplDepCmd = { "-MMD", "-MF", "$out.d" };
			t->myFlagPrefixes = { { "includes", "-I" } };
			t->myDescription = " CC $out";
			t->myCommand = { "$exe", "$threads", "$language", "$optimization", "$warnings", "$cflags", "$includes", "-c", "-o", "$out", "$in" };

			s.addTool( t );

			t = std::make_shared<Tool>( "ld", "clang_linker" );
			t->myExeName = exe;
			cTools.push_back( "clang_linker" );
			t->myInputTools = { "cc", "static_lib", "dynamic_lib" };
			t->myOptions = 
				{
					{ "optimization", { { "debug", { "-g" } },
										{ "opt", { "-O3" } },
										{ "size", { "-Os" } },
										{ "optdebug", { "-g", "-O3" } }, } },
					{ "language", { { "C", {} },
									  { "C99", { "-std=c99" } },
									  { "C11", { "-std=c11" } } } },
					{ "threads", { { "on", { "-pthread" } },
								   { "off", {} }, } },
					{ "vectorize", { { "none", {} },
									 { "SSE", { "-msse" } },
									 { "SSE2", { "-msse2" } },
									 { "SSE3", { "-msse3", "-mtune=core2" } },
									 { "SSE4", { "-msse4", "-mtune=nehalem" } },
									 { "AVX", { "-mavx", "-mtune=intel" } },
									 { "AVX2", { "-mavx2", "-mtune=intel" } },
									 { "AVX512", { "-mavx512", "-mtune=intel" } },
									 { "native", { "-mtune=native", "-march=native" } },
						} },
				};
			t->myOptionDefaults = {
				{ "optimization", "opt" },
				{ "language", "C" },
				{ "threads", "off" },
				{ "vectorize", "none" }
			};
			t->myFlagPrefixes = { { "libs", "-l" }, { "libdirs", "-L" } };
			t->myDescription = " LD $out";
			t->myCommand = { "$exe", "$threads", "$language", "$optimization", "$cflags", "-o", "$out", "$in", "$ldflags", "$libdirs", "$libs" };
			s.addTool( t );
		}
		else if ( name == "clang++" )
		{
			t = std::make_shared<Tool>( "cxx", name );
			cTools.push_back( name );
			t->myExtensions = { ".cpp", ".cc" };
			t->myAltExtensions = { ".c", ".C" };
			t->myOutputs = { ".o" };
			t->myExeName = exe;
			t->myOptions = 
				{
					{ "warnings", { { "none", { "-w" } },
									{ "default", {} },
									{ "some", { "-Wall" } },
									{ "strict", { "-Weverything" } },
									{ "most", { "-Weverything", "-Wno-padded", "-Wno-exit-time-destructors", "-Wno-global-constructors" } },
									{ "error", { "-Wall", "-Werror"  } }, } },
					{ "optimization", { { "debug", { "-g" } },
										{ "opt", { "-O3" } },
										{ "size", { "-Os" } },
										{ "optdebug", { "-g", "-O3" } }, } },
					{ "language", { { "c++", { "-x", "c++" } },
									{ "c++11", { "-x", "c++", "-std=c++11", "-Wno-c++98-compat" } },
									{ "c++14", { "-x", "c++", "-std=c++14", "-Wno-c++98-compat" } } } },
					{ "threads", { { "on", { "-pthread" } },
								   { "off", {} } } },
					{ "vectorize", { { "none", {} },
									 { "SSE", { "-msse" } },
									 { "SSE2", { "-msse2" } },
									 { "SSE3", { "-msse3", "-mtune=core2" } },
									 { "SSE4", { "-msse4", "-mtune=nehalem" } },
									 { "AVX", { "-mavx", "-mtune=intel" } },
									 { "AVX2", { "-mavx2", "-mtune=intel" } },
									 { "AVX512", { "-mavx512", "-mtune=intel" } },
									 { "native", { "-mtune=native", "-march=native" } },
						} },
				};
			t->myOptionDefaults = {
				{ "optimization", "opt" },
				{ "warnings", "default" },
				{ "language", "C" },
				{ "threads", "off" },
				{ "vectorize", "none" }
			};
			t->myImplDepName = "$out.d";
			t->myImplDepStyle = "gcc";
			t->myImplDepCmd = { "-MMD", "-MF", "$out.d" };
			t->myDescription = "CXX $out";
			t->myFlagPrefixes = { { "includes", "-I" } };
			t->myCommand = { "$exe", "$threads", "$language", "$optimization", "$warnings", "$cflags", "$includes", "-c", "-o", "$out", "$in" };

			s.addTool( t );

			t = std::make_shared<Tool>( "ld_cxx", "clang++_linker" );
			t->myExeName = exe;
			cTools.push_back( "clang++_linker" );
			t->myInputTools = { "cc", "cxx", "static_lib", "static_lib_cxx", "dynamic_lib", "dynamic_lib_cxx" };
			t->myOptions = 
				{
					{ "optimization", { { "debug", { "-g" } },
										{ "opt", { "-O3" } },
										{ "size", { "-Os" } },
										{ "optdebug", { "-g", "-O3" } }, } },
					{ "language", { { "c++", {} },
									{ "c++11", { "-std=c++11" } },
									{ "c++14", { "-std=c++14" } } } },
					{ "threads", { { "on", { "-pthread" } },
								   { "off", {} } } },
					{ "vectorize", { { "none", {} },
									 { "SSE", { "-msse" } },
									 { "SSE2", { "-msse2" } },
									 { "SSE3", { "-msse3", "-mtune=core2" } },
									 { "SSE4", { "-msse4", "-mtune=nehalem" } },
									 { "AVX", { "-mavx", "-mtune=intel" } },
									 { "AVX2", { "-mavx2", "-mtune=intel" } },
									 { "AVX512", { "-mavx512", "-mtune=intel" } },
									 { "native", { "-mtune=native", "-march=native" } },
						} },
				};
			t->myOptionDefaults = {
				{ "optimization", "opt" },
				{ "language", "C++" },
				{ "threads", "off" },
				{ "vectorize", "none" }
			};
			t->myFlagPrefixes = { { "libs", "-l" }, { "libdirs", "-L" } };
			t->myDescription = " LD $out";
			t->myCommand = { "$exe", "$threads", "$language", "$optimization", "$cflags", "-o", "$out", "$in", "$ldflags", "$libdirs", "$libs" };
			s.addTool( t );
		}
	}

	bool regToolset = ! cTools.empty();
	if ( regToolset )
	{
		s.addToolSet( "clang", cTools );
		if ( regAsDefault )
			s.useToolSet( "clang" );
	}
	return regToolset;
}


////////////////////////////////////////


bool
DefaultTools::checkAndAddGCC( Scope &s, const std::map<std::string, std::string> &exelist, bool regAsDefault )
{
	std::vector<std::string> cTools;
	for ( const auto &e: exelist )
	{
		const std::string &name = e.first;
		const std::string &exe = e.second;
		std::shared_ptr<Tool> t;
		if ( name == "gcc" )
		{
			std::cout << "TODO: Add version check for gcc to optionally enable features" << std::endl;

			t = std::make_shared<Tool>( "cc", name );
			cTools.push_back( name );
			t->myExtensions = { ".c" };
			t->myOutputs = { ".o" };
			t->myExeName = exe;
			t->myOptions = 
				{
					{ "warnings", { { "none", { "-w" } },
									{ "default", {} },
									{ "some", { "-Wall" } },
									{ "strict", { "-Wall", "-Wextra" } },
									{ "most", { "-Wall", "-Wextra", "-Wno-unused-parameter", "-Winit-self", "-Wcomment", "-Wcast-align", "-Wswitch", "-Wformat", "-Wmultichar", "-Wmissing-braces", "-Wparentheses", "-Wpointer-arith", "-Wsign-compare", "-Wreturn-type", "-Wwrite-strings", "-Wcast-align", "-Wno-unused" } },
									{ "error", { "-Wall", "-Werror"  } }, } },
					{ "optimization", { { "debug", { "-g" } },
										{ "opt", { "-O3" } },
										{ "size", { "-Os" } },
										{ "optdebug", { "-g", "-O3" } }, } },
					{ "language", { { "C", {} },
									{ "C99", { "-std=c99" } },
									{ "C11", { "-std=c11" } } } },
					{ "threads", { { "on", { "-pthread" } },
								   { "off", {} }, } },
					{ "vectorize", { { "none", {} },
									 { "SSE", { "-msse" } },
									 { "SSE2", { "-msse2" } },
									 { "SSE3", { "-msse3", "-mtune=core2" } },
									 { "SSE4", { "-msse4", "-mtune=nehalem" } },
									 { "AVX", { "-mavx", "-mtune=intel" } },
									 { "AVX2", { "-mavx2", "-mtune=intel" } },
									 { "AVX512", { "-mavx512", "-mtune=intel" } },
									 { "native", { "-mtune=native", "-march=native" } },
						} },
				};
			t->myOptionDefaults = {
				{ "optimization", "opt" },
				{ "warnings", "default" },
				{ "language", "C" },
				{ "threads", "off" },
				{ "vectorize", "none" }
			};
			t->myImplDepName = "$out.d";
			t->myImplDepStyle = "gcc";
			t->myImplDepCmd = { "-MMD", "-MF", "$out.d" };
			t->myFlagPrefixes = { { "includes", "-I" } };
			t->myDescription = " CC $out";
			t->myCommand = { "$exe", "$threads", "$language", "$optimization", "$warnings", "$cflags", "$includes", "-c", "-o", "$out", "$in" };

			s.addTool( t );

			t = std::make_shared<Tool>( "ld", "gcc_linker" );
			t->myExeName = exe;
			cTools.push_back( "gcc_linker" );
			t->myInputTools = { "cc", "static_lib", "dynamic_lib" };
			t->myOptions = 
				{
					{ "optimization", { { "debug", { "-g" } },
										{ "opt", { "-O3" } },
										{ "size", { "-Os" } },
										{ "optdebug", { "-g", "-O3" } }, } },
					{ "language", { { "C", {} },
									  { "C99", { "-std=c99" } },
									  { "C11", { "-std=c11" } } } },
					{ "threads", { { "on", { "-pthread" } },
								   { "off", {} }, } },
					{ "vectorize", { { "none", {} },
									 { "SSE", { "-msse" } },
									 { "SSE2", { "-msse2" } },
									 { "SSE3", { "-msse3", "-mtune=core2" } },
									 { "SSE4", { "-msse4", "-mtune=nehalem" } },
									 { "AVX", { "-mavx", "-mtune=intel" } },
									 { "AVX2", { "-mavx2", "-mtune=intel" } },
									 { "AVX512", { "-mavx512", "-mtune=intel" } },
									 { "native", { "-mtune=native", "-march=native" } },
						} },
				};
			t->myOptionDefaults = {
				{ "optimization", "opt" },
				{ "language", "C" },
				{ "threads", "off" },
				{ "vectorize", "none" }
			};
			t->myFlagPrefixes = { { "libs", "-l" }, { "libdirs", "-L" } };
			t->myDescription = " LD $out";
			t->myCommand = { "$exe", "$threads", "$language", "$optimization", "$cflags", "-o", "$out", "$in", "$ldflags", "$libdirs", "$libs" };
			s.addTool( t );
		}
		else if ( name == "g++" )
		{
			t = std::make_shared<Tool>( "cxx", name );
			cTools.push_back( name );
			t->myExtensions = { ".cpp", ".cc" };
			t->myAltExtensions = { ".c", ".C" };
			t->myOutputs = { ".o" };
			t->myExeName = exe;
			t->myOptions = 
				{
					{ "warnings", { { "none", { "-w" } },
									{ "default", {} },
									{ "some", { "-Wall" } },
									{ "strict", { "-Wall", "-Wextra" } },
									{ "most", { "-Wall", "-Wextra", "-Wno-unused-parameter", "-Winit-self", "-Wcomment", "-Wcast-align", "-Wswitch", "-Wformat", "-Wmultichar", "-Wmissing-braces", "-Wparentheses", "-Wpointer-arith", "-Wsign-compare", "-Wreturn-type", "-Wwrite-strings", "-Wcast-align", "-Wunused", "-Woverloaded-virtual", "-Wno-ctor-dtor-privacy", "-Wnon-virtual-dtor", "-Wpmf-conversions", "-Wsign-promo", "-Wmissing-field-initializers" } },
									{ "error", { "-Wall", "-Werror"  } }, } },
					{ "optimization", { { "debug", { "-g" } },
										{ "opt", { "-O3" } },
										{ "size", { "-Os" } },
										{ "optdebug", { "-g", "-O3" } }, } },
					{ "language", { { "c++", { "-x", "c++" } },
									{ "c++11", { "-x", "c++", "-std=c++11", "-Wno-c++98-compat" } },
									{ "c++14", { "-x", "c++", "-std=c++14", "-Wno-c++98-compat" } } } },
					{ "threads", { { "on", { "-pthread" } },
								   { "off", {} } } },
					{ "vectorize", { { "none", {} },
									 { "SSE", { "-msse" } },
									 { "SSE2", { "-msse2" } },
									 { "SSE3", { "-msse3", "-mtune=core2" } },
									 { "SSE4", { "-msse4", "-mtune=nehalem" } },
									 { "AVX", { "-mavx", "-mtune=intel" } },
									 { "AVX2", { "-mavx2", "-mtune=intel" } },
									 { "AVX512", { "-mavx512", "-mtune=intel" } },
									 { "native", { "-mtune=native", "-march=native" } },
						} },
				};
			t->myOptionDefaults = {
				{ "optimization", "opt" },
				{ "warnings", "default" },
				{ "language", "C" },
				{ "threads", "off" },
				{ "vectorize", "none" }
			};
			t->myImplDepName = "$out.d";
			t->myImplDepStyle = "gcc";
			t->myImplDepCmd = { "-MMD", "-MF", "$out.d" };
			t->myDescription = "CXX $out";
			t->myFlagPrefixes = { { "includes", "-I" } };
			t->myCommand = { "$exe", "$threads", "$language", "$optimization", "$warnings", "$cflags", "$includes", "-c", "-o", "$out", "$in" };

			s.addTool( t );

			t = std::make_shared<Tool>( "ld_cxx", "g++_linker" );
			t->myExeName = exe;
			cTools.push_back( "g++_linker" );
			t->myInputTools = { "cc", "cxx", "static_lib", "static_lib_cxx", "dynamic_lib", "dynamic_lib_cxx" };
			t->myOptions = 
				{
					{ "optimization", { { "debug", { "-g" } },
										{ "opt", { "-O3" } },
										{ "size", { "-Os" } },
										{ "optdebug", { "-g", "-O3" } }, } },
					{ "language", { { "c++", {} },
									{ "c++11", { "-std=c++11" } },
									{ "c++14", { "-std=c++14" } } } },
					{ "threads", { { "on", { "-pthread" } },
								   { "off", {} } } },
					{ "vectorize", { { "none", {} },
									 { "SSE", { "-msse" } },
									 { "SSE2", { "-msse2" } },
									 { "SSE3", { "-msse3", "-mtune=core2" } },
									 { "SSE4", { "-msse4", "-mtune=nehalem" } },
									 { "AVX", { "-mavx", "-mtune=intel" } },
									 { "AVX2", { "-mavx2", "-mtune=intel" } },
									 { "AVX512", { "-mavx512", "-mtune=intel" } },
									 { "native", { "-mtune=native", "-march=native" } },
						} },
				};
			t->myOptionDefaults = {
				{ "optimization", "opt" },
				{ "language", "C++" },
				{ "threads", "off" },
				{ "vectorize", "none" }
			};
			t->myFlagPrefixes = { { "libs", "-l" }, { "libdirs", "-L" } };
			t->myDescription = " LD $out";
			t->myCommand = { "$exe", "$threads", "$language", "$optimization", "$cflags", "-o", "$out", "$in", "$ldflags", "$libdirs", "$libs" };

			s.addTool( t );
		}
	}

	bool regToolset = ! cTools.empty();
	if ( regToolset )
	{
		s.addToolSet( "gcc", cTools );
		if ( regAsDefault )
			s.useToolSet( "gcc" );
	}
	return regToolset;
}


////////////////////////////////////////


void
DefaultTools::checkAndAddArchiver( Scope &s, const std::map<std::string, std::string> &exelist )
{
	for ( const auto &e: exelist )
	{
		const std::string &name = e.first;
		const std::string &exe = e.second;
		if ( name == "ar" )
		{
			std::string rmTool;
			bool haveRM = File::findExecutable( rmTool, "rm" );

			std::shared_ptr<Tool> t = std::make_shared<Tool>( "static_lib", name );
			t->myExtensions = { ".c", ".cpp" };
			t->myExeName = exe;
			t->myInputTools = { "cc" };
			t->myOutputPrefix = { "lib" };
			t->myOutputs = { ".a" };
			if ( haveRM )
				t->myCommand = { rmTool, "-f", "$out", "&&", "$exe", "rcs", "$out", "$in"};
			else
				t->myCommand = { "$exe", "rcs", "$out", "$in"};

			t->myDescription = " AR $out";

			s.addTool( t );

			t = std::make_shared<Tool>( "static_lib_cxx", name );
			t->myExeName = exe;
			t->myInputTools = { "cc", "cxx" };
			t->myOutputPrefix = { "lib" };
			t->myOutputs = { ".a" };
			if ( haveRM )
				t->myCommand = { rmTool, "-f", "$out", "&&", "$exe", "rcs", "$out", "$in"};
			else
				t->myCommand = { "$exe", "rcs", "$out", "$in"};
			t->myDescription = " AR $out";
			s.addTool( t );
		}
	}
}


////////////////////////////////////////





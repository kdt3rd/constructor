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


bool
DefaultTools::checkAndAddCl( Scope &s, bool regAsDefault )
{
#ifdef WIN32
	throw std::runtime_error( "Not yet implemented" );
#endif
	return false;
}


////////////////////////////////////////


bool
DefaultTools::checkAndAddClang( Scope &s, bool regAsDefault )
{
	std::map<std::string, std::string> exes = File::findExecutables(
		{
			"clang",
			"clang++",
			"ar",
		} );

	std::vector<std::string> cTools;
	for ( const auto &e: exes )
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
										{ "heavy", { "-O3", "-mtune=native", "-march=native" } },
										{ "optdebug", { "-g", "-O3" } }, } },
					{ "language", { { "C", {} },
									{ "C99", { "-std=c99" } },
									{ "C11", { "-std=c11" } } } },
					{ "threads", { { "on", { "-pthread" } },
								   { "off", {} }, } },
				};
			t->myOptionDefaults = {
				{ "optimization", "opt" },
				{ "warnings", "default" },
				{ "language", "C" },
				{ "threads", "off" }
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
										{ "heavy", { "-O3", "-mtune=native", "-march=native" } },
										{ "optdebug", { "-g", "-O3" } }, } },
					{ "language", { { "C", {} },
									  { "C99", { "-std=c99" } },
									  { "C11", { "-std=c11" } } } },
					{ "threads", { { "on", { "-pthread" } },
								   { "off", {} }, } },
				};
			t->myOptionDefaults = {
				{ "optimization", "opt" },
				{ "language", "C" },
				{ "threads", "off" }
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
										{ "heavy", { "-O3", "-mtune=native", "-march=native" } },
										{ "optdebug", { "-g", "-O3" } }, } },
					{ "language", { { "c++", { "-x", "c++" } },
									{ "c++11", { "-x", "c++", "-std=c++11", "-Wno-c++98-compat" } },
									{ "c++14", { "-x", "c++", "-std=c++14", "-Wno-c++98-compat" } } } },
					{ "threads", { { "on", { "-pthread" } },
								   { "off", {} } } },
				};
			t->myOptionDefaults = {
				{ "optimization", "opt" },
				{ "warnings", "default" },
				{ "language", "C" },
				{ "threads", "off" }
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
										{ "heavy", { "-O3", "-mtune=native", "-march=native" } },
										{ "optdebug", { "-g", "-O3" } }, } },
					{ "language", { { "c++", {} },
									{ "c++11", { "-std=c++11" } },
									{ "c++14", { "-std=c++14" } } } },
					{ "threads", { { "on", { "-pthread" } },
								   { "off", {} } } },
				};
			t->myOptionDefaults = {
				{ "optimization", "opt" },
				{ "language", "C++" },
				{ "threads", "off" }
			};
			t->myFlagPrefixes = { { "libs", "-l" }, { "libdirs", "-L" } };
			t->myDescription = " LD $out";
			t->myCommand = { "$exe", "$threads", "$language", "$optimization", "$cflags", "-o", "$out", "$in", "$ldflags", "$libdirs", "$libs" };
			s.addTool( t );
		}
		else if ( name == "ar" )
		{
			t = std::make_shared<Tool>( "static_lib", name );
			t->myExtensions = { ".c", ".cpp" };
			t->myExeName = exe;
			t->myInputTools = { "cc" };
			t->myOutputPrefix = { "lib" };
			t->myOutputs = { ".a" };
			t->myCommand = { "rm", "-f", "$out", "&&", "$exe", "rcs", "$out", "$in"};
			t->myDescription = " AR $out";

			s.addTool( t );
			t = std::make_shared<Tool>( "static_lib_cxx", name );
			t->myExeName = exe;
			t->myInputTools = { "cc", "cxx" };
			t->myOutputPrefix = { "lib" };
			t->myOutputs = { ".a" };
			t->myCommand = { "rm", "-f", "$out", "&&", "$exe", "rcs", "$out", "$in"};
			t->myDescription = " AR $out";
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
DefaultTools::checkAndAddGCC( Scope &s, bool regAsDefault )
{
	return false;
}



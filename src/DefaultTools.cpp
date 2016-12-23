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
#include "Toolset.h"
#include "Debug.h"
#include <stdexcept>


////////////////////////////////////////


namespace
{
#ifdef WIN32
#else
std::vector<std::string> theCLinkInputTools{ "cc", "static_lib", "dynamic_lib" };
std::vector<std::string> theCPPLinkInputTools{ "cc", "cxx", "static_lib", "static_lib_cxx", "dynamic_lib", "dynamic_lib_cxx" };
Tool::OptionGroup theCommonOptions
{
	{ "optimization", {
			{ "debug", { "-O0", "-g" } },
			{ "heavy", { "-O3", "-flto" } },
			{ "opt", { "-O3" } },
			{ "size", { "-Os" } },
			{ "optdebug", { "-g", "-O3" } }, } },
	{ "threads", {
			{ "on", { "-pthread" } },
			{ "off", {} }, } },
	{ "vectorize", {
			{ "none", {} },
			{ "SSE", { "-msse" } },
			{ "SSE2", { "-msse2" } },
			{ "SSE3", { "-msse3", "-mtune=core2" } },
			{ "SSE4", { "-msse4", "-mtune=nehalem" } },
			{ "AVX", { "-mavx", "-mtune=intel" } },
			{ "AVX2", { "-mavx2", "-mtune=intel" } },
			{ "AVX512", { "-mavx512", "-mtune=intel" } },
			{ "native", { "-mtune=native", "-march=native" } },
				} }
};
Tool::OptionSet theCLanguages{
	{ "C", {} },
	{ "C99", { "-std=c99" } },
	{ "C11", { "-std=c11" } } };
Tool::OptionSet theCPPLanguages{
	{ "c++", { "-x", "c++" } },
	{ "c++11", { "-x", "c++", "-std=c++11", "-Wc++11-compat" } },
	{ "c++14", { "-x", "c++", "-std=c++14", "-Wc++11-compat", "-Wc++14-compat" } },
	{ "c++17", { "-x", "c++", "-std=c++17", "-Wc++11-compat", "-Wc++14-compat" } }
};
Tool::OptionSet theCPPLinkLanguages{
	{ "c++", {} },
	{ "c++11", { "-std=c++11" } },
	{ "c++14", { "-std=c++14" } },
	{ "c++17", { "-std=c++17" } }
};
Tool::OptionDefaultSet theCDefaults{
	{ "optimization", "opt" },
	{ "warnings", "default" },
	{ "language", "C" },
	{ "threads", "off" },
	{ "vectorize", "none" }
};
Tool::OptionDefaultSet theCPPDefaults{
	{ "optimization", "opt" },
	{ "warnings", "default" },
	{ "language", "c++" },
	{ "threads", "off" },
	{ "vectorize", "none" }
};

std::vector<std::string> theCompileCmd{ "$exe", "$threads", "$language", "$optimization", "$warnings", "$vectorize", "$cflags", "$defines", "$includes", "-pipe", "-c", "-o", "$out", "$in" };
std::vector<std::string> theLinkCmd{ "$exe", "$threads", "$language", "$optimization", "$vectorize", "$cflags", "-pipe", "-o", "$out", "$in", "$ldflags", "$libdirs", "$libs" };

Tool::OptionDefaultSet theVarPrefixes{
	{ "includes", "-I" },
	{ "defines", "-D" },
	{ "libdirs", "-L" },
	{ "libs", "-l" }
};
#endif
} // empty namespace


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

	auto gccts = checkAndAddGCC( s, exes );
	auto clts = checkAndAddClang( s, exes );
#ifdef WIN32
	auto ts = checkAndAddCl( s, exes );
	if ( ts )
		s.useToolSet( ts->getname() );
	else
	{
#endif
		// on os/x, clang is much newer and preferred
		// but on linux, newer gcc seems to be ahead
		// currently
#ifdef __APPLE__
		if ( clts )
			s.useToolSet( clts->getName() );
		else if ( gccts )
			s.useToolSet( gccts->getName() );
#else
		if ( gccts )
			s.useToolSet( gccts->getName() );
		else if ( clts )
			s.useToolSet( clts->getName() );
#endif
#ifdef WIN32
	}
#endif

	auto ats = checkAndAddArchiver( s, exes );
	if ( ats )
		s.useToolSet( ats->getName() );
	addSelfGenerator( s );
}


////////////////////////////////////////


const std::vector<std::string> &
DefaultTools::getOptions( void )
{
	static std::vector<std::string> theOpts
	{
		"warnings",
		"optimization",
		"language",
		"threads",
		"vectorize"
	};
	return theOpts;
}


////////////////////////////////////////


#ifdef WIN32
std::shared_ptr<Toolset>
DefaultTools::checkAndAddCl( Scope &s, const std::map<std::string, std::string> &exelist, bool regAsDefault )
{
	return std::shared_ptr<Toolset>();
}
#endif


////////////////////////////////////////


std::shared_ptr<Toolset>
DefaultTools::checkAndAddClang( Scope &s, const std::map<std::string, std::string> &exelist )
{
	std::shared_ptr<Toolset> cTools = std::make_shared<Toolset>( "clang" );
	cTools->setTag( "compile" );

	static Tool::OptionSet commonWarnings{
		{ "none", { "-w" } },
		{ "default", {} },
		{ "some", { "-Wall" } },
		{ "strict", { "-Weverything", } },
		{ "most", { "-Weverything", } },
		{ "error", { "-Wall", "-Werror"  } } };

	for ( const auto &e: exelist )
	{
		const std::string &name = e.first;
		const std::string &exe = e.second;
		std::shared_ptr<Tool> t;
		if ( name == "clang" )
		{
			t = std::make_shared<Tool>( "cc", name );
			t->myExtensions = { ".c" };
			t->myOutputs = { ".o" };
			t->myExeName = exe;
			t->myOptions = theCommonOptions;
			t->myOptions["warnings"] = commonWarnings;
			t->myOptions["language"] = theCLanguages;
			t->myOptionDefaults = theCDefaults;
			t->myImplDepName = "$out.d";
			t->myImplDepStyle = "gcc";
			t->myImplDepCmd = { "-MMD", "-MF", "$out.d" };
			t->myFlagPrefixes = theVarPrefixes;
			t->myDescription = " CC $out_short";
			t->myCommand = theCompileCmd;

			s.addTool( t );
			cTools->addTool( t );

			t = std::make_shared<Tool>( "ld", "clang_linker" );
			t->myExeName = exe;
			t->myInputTools = theCLinkInputTools;
			t->myOptions = theCommonOptions;
			t->myOptions["language"] = theCLanguages;
			t->myOptionDefaults = theCDefaults;
			t->myFlagPrefixes = theVarPrefixes;
			t->myDescription = " LD $out_short";
			t->myCommand = theLinkCmd;
			s.addTool( t );
			cTools->addTool( t );
		}
		else if ( name == "clang++" )
		{
			t = std::make_shared<Tool>( "cxx", name );
			t->myExtensions = { ".cpp", ".cc" };
			t->myAltExtensions = { ".c", ".C" };
			t->myOutputs = { ".o" };
			t->myExeName = exe;
			t->myOptions = theCommonOptions;
			t->myOptions["warnings"] = commonWarnings;
			t->myOptions["warnings"]["most"] = { "-Weverything", "-Wno-padded", "-Wno-global-constructors", "-Wno-documentation-unknown-command", "-Wno-mismatched-tags", "-Wno-exit-time-destructors" };
			t->myOptions["language"] = theCPPLanguages;
			t->myOptions["language"]["c++11"] = { "-x", "c++", "-std=c++11", "-Wno-c++98-compat", "-Wno-c++98-compat-pedantic" };
			t->myOptions["language"]["c++14"] = { "-x", "c++", "-std=c++14", "-Wno-c++98-compat", "-Wno-c++98-compat-pedantic" };
			t->myOptions["language"]["c++17"] = { "-x", "c++", "-std=c++17", "-Wno-c++98-compat", "-Wno-c++98-compat-pedantic" };
			t->myOptionDefaults = theCPPDefaults;
			t->myImplDepName = "$out.d";
			t->myImplDepStyle = "gcc";
			t->myImplDepCmd = { "-MMD", "-MF", "$out.d" };
			t->myDescription = "CXX $out_short";
			t->myFlagPrefixes = theVarPrefixes;
			t->myCommand = theCompileCmd;

			s.addTool( t );
			cTools->addTool( t );

			t = std::make_shared<Tool>( "objcxx", name );
			t->myExtensions = { ".mm" };
			t->myAltExtensions = { ".MM" };
			t->myOutputs = { ".o" };
			t->myExeName = exe;
			t->myOptions = theCommonOptions;
			t->myOptions["warnings"] = commonWarnings;
			t->myOptions["warnings"]["most"] = { "-Weverything", "-Wno-padded", "-Wno-global-constructors", "-Wno-documentation-unknown-command", "-Wno-mismatched-tags", "-Wno-exit-time-destructors" };
			t->myOptions["language"]["c++"] = { "-ObjC++" };
			t->myOptions["language"]["c++11"] = { "-ObjC++", "-std=c++11", "-Wno-c++98-compat", "-Wno-c++98-compat-pedantic" };
			t->myOptions["language"]["c++14"] = { "-ObjC++", "-std=c++14", "-Wno-c++98-compat", "-Wno-c++98-compat-pedantic" };
			t->myOptions["language"]["c++17"] = { "-ObjC++", "-std=c++17", "-Wno-c++98-compat", "-Wno-c++98-compat-pedantic" };
			t->myOptionDefaults = theCPPDefaults;
			t->myImplDepName = "$out.d";
			t->myImplDepStyle = "gcc";
			t->myImplDepCmd = { "-MMD", "-MF", "$out.d" };
			t->myDescription = "OBJCXX $out_short";
			t->myFlagPrefixes = theVarPrefixes;
			t->myCommand = theCompileCmd;

			s.addTool( t );
			cTools->addTool( t );

			t = std::make_shared<Tool>( "ld_cxx", "clang++_linker" );
			t->myExeName = exe;
			t->myInputTools = theCPPLinkInputTools;
			t->myInputTools.push_back( "objcxx" );
			t->myOptions = theCommonOptions;
			t->myOptionDefaults = theCPPDefaults;
			t->myFlagPrefixes = theVarPrefixes;
			t->myDescription = " LD $out_short";
			t->myCommand = theLinkCmd;
			s.addTool( t );
			cTools->addTool( t );
		}
	}

	if ( cTools->empty() )
		cTools.reset();
	else
		s.addToolSet( cTools );

	return cTools;
}


////////////////////////////////////////


std::shared_ptr<Toolset>
DefaultTools::checkAndAddGCC( Scope &s, const std::map<std::string, std::string> &exelist )
{
	std::shared_ptr<Toolset> cTools = std::make_shared<Toolset>( "gcc" );
	cTools->setTag( "compile" );

	static Tool::OptionSet commonWarnings{
		{ "none", { "-w" } },
		{ "default", {} },
		{ "some", { "-Wall" } },
		{ "most", { "-Wall", "-Wextra" } },
		{ "strict", { "-Wall", "-Wextra" } },
		{ "error", { "-Wall", "-Werror"  } } };

	for ( const auto &e: exelist )
	{
		const std::string &name = e.first;
		const std::string &exe = e.second;
		std::shared_ptr<Tool> t;
		if ( name == "gcc" )
		{
			// TODO: Add version check for gcc to optionally enable features
			t = std::make_shared<Tool>( "cc", name );
			t->myExtensions = { ".c" };
			t->myOutputs = { ".o" };
			t->myExeName = exe;
			t->myOptions = theCommonOptions;
			t->myOptions["optimization"]["optdebug"] = { "-g", "-Og" };
			t->myOptions["warnings"] = commonWarnings;
			t->myOptions["warnings"]["most"] =
				{ "-Wall", "-Wextra", "-Wno-unused-parameter", "-Winit-self", "-Wcomment", "-Wcast-align", "-Wswitch", "-Wformat", "-Wmultichar", "-Wmissing-braces", "-Wparentheses", "-Wpointer-arith", "-Wsign-compare", "-Wreturn-type", "-Wwrite-strings", "-Wcast-align", "-Wno-unused" };
			t->myOptions["language"] = theCLanguages;
			t->myOptionDefaults = theCDefaults;
			t->myImplDepName = "$out.d";
			t->myImplDepStyle = "gcc";
			t->myImplDepCmd = { "-MMD", "-MF", "$out.d" };
			t->myFlagPrefixes = theVarPrefixes;
			t->myDescription = " CC $out_short";
			t->myCommand = theCompileCmd;

			s.addTool( t );
			cTools->addTool( t );

			t = std::make_shared<Tool>( "ld", "gcc_linker" );
			t->myExeName = exe;
			t->myInputTools = theCLinkInputTools;
			t->myOptions = theCommonOptions;
			t->myOptions["optimization"]["optdebug"] = { "-g", "-Og" };
			t->myOptions["language"] = theCLanguages;
			t->myOptionDefaults = theCDefaults;
			t->myFlagPrefixes = theVarPrefixes;
			t->myDescription = " LD $out_short";
			t->myCommand = theLinkCmd;
			s.addTool( t );
			cTools->addTool( t );
		}
		else if ( name == "g++" )
		{
			t = std::make_shared<Tool>( "cxx", name );
			t->myExtensions = { ".cpp", ".cc" };
			t->myAltExtensions = { ".c", ".C" };
			t->myOutputs = { ".o" };
			t->myExeName = exe;
			t->myOptions = theCommonOptions;
			t->myOptions["optimization"]["optdebug"] = { "-g", "-Og" };
			t->myOptions["warnings"] = commonWarnings;
			t->myOptions["warnings"]["most"] = { "-Wall", "-Wextra", "-Wno-unused-parameter", "-Winit-self", "-Wcomment", "-Wcast-align", "-Wswitch", "-Wformat", "-Wmultichar", "-Wmissing-braces", "-Wparentheses", "-Wpointer-arith", "-Wsign-compare", "-Wreturn-type", "-Wwrite-strings", "-Wcast-align", "-Wunused", "-Woverloaded-virtual", "-Wno-ctor-dtor-privacy", "-Wnon-virtual-dtor", "-Wpmf-conversions", "-Wsign-promo", "-Wmissing-field-initializers" };
			t->myOptions["language"] = theCPPLanguages;
			t->myOptionDefaults = theCPPDefaults;
			t->myImplDepName = "$out.d";
			t->myImplDepStyle = "gcc";
			t->myImplDepCmd = { "-MMD", "-MF", "$out.d" };
			t->myDescription = "CXX $out_short";
			t->myFlagPrefixes = theVarPrefixes;
			t->myCommand = theCompileCmd;
			s.addTool( t );
			cTools->addTool( t );

			t = std::make_shared<Tool>( "ld_cxx", "g++_linker" );
			t->myExeName = exe;
			t->myInputTools = theCPPLinkInputTools;
			t->myOptions = theCommonOptions;
			t->myOptions["optimization"]["optdebug"] = { "-g", "-Og" };
			t->myOptions["language"] = theCPPLinkLanguages;
			t->myOptionDefaults = theCPPDefaults;
			t->myFlagPrefixes = theVarPrefixes;
			t->myDescription = " LD $out_short";
			t->myCommand = theLinkCmd;

			s.addTool( t );
			cTools->addTool( t );
		}
	}

	if ( cTools->empty() )
		cTools.reset();
	else
		s.addToolSet( cTools );
	return cTools;
}


////////////////////////////////////////


std::shared_ptr<Toolset>
DefaultTools::checkAndAddArchiver( Scope &s, const std::map<std::string, std::string> &exelist )
{
	std::shared_ptr<Toolset> cTools = std::make_shared<Toolset>( "system_ar" );
	cTools->setTag( "archive" );

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
			t->myOutputPrefix = "lib";
			t->myOutputs = { ".a" };
			t->myFlagPrefixes = theVarPrefixes;
			if ( haveRM )
				t->myCommand = { rmTool, "-f", "$out", "&&", "$exe", "rcs", "$out", "$in"};
			else
				t->myCommand = { "$exe", "rcs", "$out", "$in"};

			t->myDescription = " AR $out_short";

			s.addTool( t );
			cTools->addTool( t );

			t = std::make_shared<Tool>( "static_lib_cxx", name );
			t->myExeName = exe;
			t->myInputTools = { "cc", "cxx", "objcxx" };
			t->myOutputPrefix = "lib";
			t->myOutputs = { ".a" };
			t->myFlagPrefixes = theVarPrefixes;
			if ( haveRM )
				t->myCommand = { rmTool, "-f", "$out", "&&", "$exe", "rcs", "$out", "$in"};
			else
				t->myCommand = { "$exe", "rcs", "$out", "$in"};
			t->myDescription = " AR $out_short";
			s.addTool( t );
			cTools->addTool( t );
		}
	}
	if ( cTools->empty() )
		cTools.reset();
	else
		s.addToolSet( cTools );

	return cTools;
}


////////////////////////////////////////


void
DefaultTools::addSelfGenerator( Scope &s )
{
	std::string selfTool;
	bool haveSelf = File::findExecutable( selfTool, File::getArgv0() );
	if ( ! haveSelf )
		selfTool = File::getArgv0();

	std::shared_ptr<Tool> t = std::make_shared<Tool>( "codegen_binary_cstring", "codegen_binary_cstring" );
	t->myExeName = selfTool;
	t->myCommand = { selfTool, "-embed_binary_cstring", "$out", "$codegen_info", "$in"};
	t->myDescription = "BLOB $out";
	s.addTool( t );
}


////////////////////////////////////////





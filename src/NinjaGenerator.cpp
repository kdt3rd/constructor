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
#include "LuaExtensions.h"
#include "BuildItem.h"
#include "TransformSet.h"
#include "Scope.h"
#include "Debug.h"
#include "StrUtil.h"
#include <fstream>
#include <iostream>
#include <iomanip>
#include <set>


////////////////////////////////////////


namespace
{

static std::string
escape( const std::string &s )
{
	std::string ret = s;
	if ( ret.find_first_of( '\n' ) != std::string::npos )
		throw std::runtime_error( "ninja does not allow newlines in names or values" );

	std::string::size_type vPos = ret.find_first_of( '$' );
	while ( vPos != std::string::npos )
	{
		ret.replace( vPos, 1, 2, '$' );
		vPos = ret.find_first_of( '$', vPos + 2 );
	}
	return ret;
}

static std::string
escape_path( const std::string &s )
{
	std::string ret = escape( s );

	std::string::size_type vPos = ret.find_first_of( ' ' );
	while ( vPos != std::string::npos )
	{
		ret.replace( vPos, 1, "$ ", 2 );
		vPos = ret.find_first_of( ' ', vPos + 2 );
	}

#ifdef WIN32
	std::string::size_type vPos = ret.find_first_of( ':' );
	while ( vPos != std::string::npos )
	{
		ret.replace( vPos, 1, "$:", 2 );
		vPos = ret.find_first_of( ':', vPos + 2 );
	}
#endif
	return ret;
}

static void
emitRules( std::ostream &os, const TransformSet &x )
{
	std::set< std::shared_ptr<Tool> > toolsInPlay;
	for ( const std::shared_ptr<BuildItem> &bi: x.getBuildItems() )
		toolsInPlay.insert( bi->getTool() );

	// @TODO: Need to add variable lookup to substitute variables
	//   so optimization flags, etc. can be swapped in
	VERBOSE( "Need to add variable lookup for rule definition" );
#ifdef WIN32
	std::cerr << "Need to add a check for max command length and use a response file instead" << std::endl;
#endif
	for ( const std::shared_ptr<Tool> &t: toolsInPlay )
	{
		if ( t )
		{
			const Rule &r = t->createRule( x );
			os << '\n';
			for ( auto &v: r.getVariables() )
				os << v.first << '=' << v.second << '\n';
			os << "\nrule " << r.getName() << '\n';
			os << " command = " << r.getCommand() << '\n';
			os << " description = " << r.getDescription() << '\n';
			const std::string &dFile = r.getDependencyFile();
			if ( ! dFile.empty() )
			{
				os << " depfile = " << dFile << '\n';
				const std::string &dStyle = r.getDependencyStyle();
				if ( ! dStyle.empty() )
					os << " deps = " << dStyle << '\n';
			}
			if ( r.isOutputRestat() )
				os << " restat = 1\n";
			const std::string &jPool = r.getJobPool();
			if ( ! jPool.empty() )
					os << " pool = " << jPool << '\n';
		}
	}
}

static void
emitVariables( std::ostream &os, const TransformSet &x )
{
	const VariableSet &vars = x.getVars();
	for ( auto i: vars )
	{
		if ( i.second.useToolFlagTransform() )
		{
			auto t = x.getTool( i.second.getToolTag() );
			if ( ! t )
				throw std::runtime_error( "Variable set to use tool flag transform, but no tool with tag '" + i.second.getToolTag() + "' found" );

			os << '\n' << i.first << "=" << i.second.prepended_value( t->getCommandPrefix( i.first ) );
		}
		else
			os << '\n' << i.first << "=" << i.second.value();
	}
	if ( ! vars.empty() )
		os << '\n';
}

static void
addOutputList( std::ostream &os, const std::shared_ptr<BuildItem> &bi )
{
	auto outd = bi->getOutDir();
	if ( outd )
	{
		for ( const std::string &bo: bi->getOutputs() )
			os << ' ' << escape_path( outd->makefilename( bo ) );
	}
	else
	{
		for ( const std::string &bo: bi->getOutputs() )
			os << ' ' << escape_path( bo );
	}

	if ( ! bi->getTool() )
	{
		std::vector< std::shared_ptr<BuildItem> > deps =
			bi->extractDependencies( DependencyType::EXPLICIT );
		for ( auto &d: deps )
			addOutputList( os, d );
	}
}

static void
emitTargets( std::ostream &os, const TransformSet &x )
{
	for ( const std::shared_ptr<BuildItem> &bi: x.getBuildItems() )
	{
		auto t = bi->getTool();
		if ( t )
		{
			auto outd = bi->getOutDir();
			os << "\nbuild";
			addOutputList( os, bi );
			os << ": " << t->getTag();
			if ( bi->useName() )
				os << ' ' << escape_path( bi->getDir()->makefilename( bi->getName() ) );

			std::vector< std::shared_ptr<BuildItem> > deps =
				bi->extractDependencies( DependencyType::EXPLICIT );
			for ( auto &d: deps )
				addOutputList( os, d );

			deps = bi->extractDependencies( DependencyType::IMPLICIT );
			if ( ! deps.empty() )
			{
				os << " |";
				for ( auto &d: deps )
					addOutputList( os, d );
			}
			deps = bi->extractDependencies( DependencyType::ORDER );
			if ( ! deps.empty() )
			{
				os << " ||";
				for ( auto &d: deps )
					addOutputList( os, d );
			}

			auto &bivars = bi->getVariables();
			for ( auto &bv: bivars )
			{
				std::string outv = bv.second.prepended_value( t->getCommandPrefix( bv.first ) );
				os << "\n  " << bv.first << "=" << outv;
			}

			if ( bi->isTopLevelItem() )
			{
				PRECONDITION( bi->getOutputs().size() == 1,
							  "Expecting top level item to only have 1 output" );
				os << "\nbuild " << escape( bi->getName() ) << ": phony "
				   << outd->makefilename( bi->getOutputs()[0] )
				   << '\n';
			}
			// don't need this since ninja implicitly builds
			// these anyway
//			if ( bi->isRoot() )
//			{
//				
//			}
		}
	}
}

static void
emitScope( std::ostream &os,
		   const Directory &outD,
		   const TransformSet &x,
		   int &scopeCount )
{
	for ( const std::shared_ptr<TransformSet> &i: x.getSubScopes() )
	{
		std::stringstream subscopefn;
		subscopefn << "sub_scope_" << (++scopeCount) << ".ninja";
		std::string sfn = subscopefn.str();
		std::ofstream ssf( outD.makefilename( sfn ) );
		emitScope( ssf, outD, *i, scopeCount );
		os << "\nsubninja " << sfn << '\n';
	}

	emitVariables( os, x );
	emitRules( os, x );
	emitTargets( os, x );
}

}


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
NinjaGenerator::emit( const std::shared_ptr<Directory> &d,
					  const Configuration &conf,
					  int argc, const char *argv[] )
{
	std::ofstream f( d->makefilename( "build.ninja" ) );
	f << "ninja_required_version = 1.3\n";
	f << "builddir = " << d->fullpath() << '\n';

	TransformSet xform( d );
	Scope::root().transform( xform, conf );

	int scopeCount = 0;
	emitScope( f, *d, xform, scopeCount );

	Directory curD;
	f <<
		"\nrule regen_constructor\n"
		"  command = cd $in" << " &&";
	for ( int a = 0; a < argc; ++a )
		f << ' ' << argv[a];
	f <<
		"\n  description = Regenerating build files..."
		"\n  generator = 1"
		"\n\n";

	f << "build build.ninja: regen_constructor " << curD.fullpath() << " |";
	for ( const std::string &x: Lua::visitedFiles() )
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



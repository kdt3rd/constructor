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

#include "Executable.h"

#include "Debug.h"
#include "PackageConfig.h"
#include "Library.h"
#include <set>


////////////////////////////////////////


Executable::Executable( std::string name )
		: CompileSet( std::move( name ) )
{
	setAsTopLevel( true );
	setUseNameAsInput( false );
	setDefaultTarget( true );
}


////////////////////////////////////////


Executable::~Executable( void )
{
}


////////////////////////////////////////


std::shared_ptr<BuildItem>
Executable::transform( TransformSet &xform ) const
{
	std::shared_ptr<BuildItem> ret = xform.getTransform( this );
	if ( ret )
		return ret;

	DEBUG( "transform Executable " << getName() );
	ret = std::make_shared<BuildItem>( getName(), getDir() );
	ret->setUseName( false );
	ret->setOutputDir( getOutputDir( xform ) );
	ret->setTopLevel( isTopLevel(), getPseudoTarget() );
	ret->setDefaultTarget( isDefaultTarget() );

	std::set<std::string> tags;
	std::queue< std::shared_ptr<BuildItem> > chainsToCheck;
	Variable outflags( "cflags" );
	for ( const ItemPtr &i: myItems )
	{
		std::shared_ptr<BuildItem> xi = i->transform( xform );
		xi->markAsDependent();

		const Library *libDep = dynamic_cast<const Library *>( i.get() );
		const PackageConfig *pkg = dynamic_cast<const PackageConfig *>( i.get() );
		if ( libDep || pkg )
		{
			VERBOSE( "Executable '" << getName() << "' depends on " << i->getName() );
			ret->addDependency( DependencyType::IMPLICIT, xi );
			outflags.addIfMissing( xi->getVariable( "cflags" ).values() );
			ret->addToVariable( "ldflags", xi->getVariable( "ldflags" ) );
			if ( libDep )
			{
				ret->addToVariable( "libs", libDep->getName() );
				ret->addToVariable( "libdirs", xi->getOutDir()->fullpath() );
			}

			ret->addToVariable( "libs", xi->getVariable( "libs" ) );
			ret->addToVariable( "libdirs", xi->getVariable( "libdirs" ) );
		}
		else if ( dynamic_cast<const Executable *>( i.get() ) )
		{
			// executables can't depend on other exes, make this
			// an order only dependency
			VERBOSE( "Executable '" << i->getName() << "' will be built before '" << getName() << "' because of declared dependency" );
			ret->addDependency( DependencyType::ORDER, xi );
		}
		else
			chainsToCheck.push( xi );
	}

	followChains( chainsToCheck, tags, ret, xform );

	if ( ! outflags.empty() )
	{
		std::vector< std::shared_ptr<BuildItem> > expDeps = ret->extractDependencies( DependencyType::EXPLICIT );
		for ( const auto &compItem: expDeps )
			compItem->addToVariable( "cflags", outflags );
	}

	if ( tags.empty() )
		throw std::runtime_error( "No tags available to determine linker for exe " + getName() );

	std::shared_ptr<Tool> t = xform.findToolForSet( "ld", tags );
	if ( ! t )
	{
		std::stringstream msgbuf;
		msgbuf << "Unable to find linker to handle the following tools: ";
		bool doC = false;
		for ( auto &lt: tags )
		{
			if ( doC )
				msgbuf << ", ";
			msgbuf << lt;
			doC = true;
		}
		throw std::runtime_error( msgbuf.str() );
	}
	ret->setTool( t );

	xform.recordTransform( this, ret );
	return ret;
}


////////////////////////////////////////


std::shared_ptr<Directory>
Executable::getOutputDir( TransformSet &xform ) const
{
	const Variable &o = getVariable( "exe_dir" );
	if ( o.empty() )
		return xform.getBinDir();

	auto ret = std::make_shared<Directory>( *(xform.getOutDir()) );
	for ( auto &d: o.values() )
		ret->cd( d );

	return ret;
}


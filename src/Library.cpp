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

#include "Library.h"

#include "Debug.h"
#include "PackageConfig.h"
#include "Executable.h"
#include <set>


////////////////////////////////////////


Library::Library( std::string name )
		: CompileSet( std::move( name ) )
{
	setAsTopLevel( true );
	setUseNameAsInput( false );
	setDefaultTarget( true );
}


////////////////////////////////////////


Library::~Library( void )
{
}


////////////////////////////////////////


std::shared_ptr<BuildItem>
Library::transform( TransformSet &xform ) const
{
	std::shared_ptr<BuildItem> ret = xform.getTransform( this );
	if ( ret )
		return ret;

	ret = std::make_shared<BuildItem>( getName(), getDir() );
	ret->setUseName( false );
	ret->setOutputDir( xform.getLibDir() );
	ret->setTopLevel( true );
	ret->setDefaultTarget( true );

	Variable outlibs( "libs" ), outlibdirs( "libdirs" );
	Variable outflags( "cflags" ), outldflags( "ldflags" );
	std::set<std::string> tags;
	std::queue< std::shared_ptr<BuildItem> > chainsToCheck;
	for ( const ItemPtr &i: myItems )
	{
		std::shared_ptr<BuildItem> xi = i->transform( xform );
		xi->markAsDependent();

		const Library *libDep = dynamic_cast<const Library *>( i.get() );
		const PackageConfig *pkg = dynamic_cast<const PackageConfig *>( i.get() );

		if ( libDep || pkg )
		{
			ret->addDependency( DependencyType::IMPLICIT, xi );
			outflags.addIfMissing( xi->getVariable( "cflags" ).values() );
			outldflags.add( xi->getVariable( "ldflags" ).values() );
			if ( libDep )
				outlibs.addIfMissing( i->getName() );
			outlibs.add( xi->getVariable( "libs" ).values() );
			outlibdirs.addIfMissing( xi->getVariable( "libdirs" ).values() );
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

	std::string libType;
	findVariableValueRecursive( libType, "library_type" );
	if ( libType.empty() )
		libType = xform.getVarValue( "default_library_type" );
	if ( libType.empty() )
	{
		libType = "static";
		VERBOSE( "No library type declared for '" << getName() << "', defaulting to static" );
	}

	std::shared_ptr<Tool> t = xform.findToolForSet( libType, tags );
	if ( ! t )
	{
		std::stringstream msgbuf;
		msgbuf << "Unable to find library tool to handle a library type '" << libType << "' with objects of the following tools: ";
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

	if ( libType == "static" )
	{
		if ( ! outlibs.empty() )
		{
			outlibs.removeDuplicatesKeepLast();
			ret->addToVariable( "libs", outlibs );
		}
		if ( ! outlibdirs.empty() )
			ret->addToVariable( "libdirs", outlibdirs );
		if ( ! outldflags.empty() )
			ret->addToVariable( "ldflags", outldflags );
	}

	xform.recordTransform( this, ret );
	return ret;
}

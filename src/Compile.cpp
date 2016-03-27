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

#include "Compile.h"

#include "Debug.h"
#include "FileUtil.h"
#include "PackageConfig.h"
#include "Library.h"
#include "Executable.h"
#include "ExternLibrary.h"
#include <queue>


////////////////////////////////////////


CompileSet::CompileSet( void )
		: Item( "__compile__" )
{
}


////////////////////////////////////////


CompileSet::CompileSet( std::string nm )
		: Item( std::move( nm ) )
{
}


////////////////////////////////////////


CompileSet::~CompileSet( void )
{
}


////////////////////////////////////////


void
CompileSet::addItem( const ItemPtr &i )
{
	i->setParent( shared_from_this() );
	myItems.push_back( i );
}


////////////////////////////////////////


void
CompileSet::addItem( std::string name )
{
	if ( ! dir().exists( name ) )
		throw std::runtime_error( "File '" + name + "' does not exist in directory '" + dir().fullpath() + "'" );

	VERBOSE( "CompileSet addItem( '" << name << "' )" );
	ItemPtr i = std::make_shared<Item>( std::move( name ) );
	i->setParent( shared_from_this() );
	myItems.push_back( i );
}


////////////////////////////////////////


std::shared_ptr<BuildItem>
CompileSet::transform( TransformSet &xform ) const
{
	std::shared_ptr<BuildItem> ret = xform.getTransform( this );
	if ( ret )
		return ret;

	DEBUG( "transform CompileSet " << getName() );
	ret = std::make_shared<BuildItem>( getName(), getDir() );
	ret->setUseName( false );
	ret->setOutputDir( xform.getOutDir() );

	std::set<std::string> tags;
	fillBuildItem( ret, xform, tags, false );

	xform.recordTransform( this, ret );
	return ret;
}


////////////////////////////////////////


void
CompileSet::followChains( std::queue<std::shared_ptr<BuildItem>> &chainsToCheck,
						  std::set<std::string> &tags,
						  const std::shared_ptr<BuildItem> &ret,
						  TransformSet &xform ) const
{
	while ( ! chainsToCheck.empty() )
	{
		// now we need to chain build until we can no more such
		// that if something like yacc produces a .c and a .h,
		// the .c is transformed to a .o
		std::shared_ptr<BuildItem> i = chainsToCheck.front();
		chainsToCheck.pop();

		const auto &outs = i->getOutputs();
		if ( outs.empty() )
		{
			// how to handle a .o that wouldn't have a tool
			// but would only be an implicit dependency...
			// library or executable would need to promote that
			// to an explicit dependency somehow
			i->extractTags( tags );
			if ( ! ret->flatten( i ) )
				ret->addDependency( DependencyType::EXPLICIT, i );
		}
		else
		{
			bool addedChain = false;
			for ( const std::string &bo: i->getOutputs() )
			{
				std::shared_ptr<BuildItem> si = chainTransform(
					bo, i->getOutDir(), xform );
				if ( si )
				{
					chainsToCheck.push( si );
					si->addDependency( DependencyType::CHAIN, i );
					addedChain = true;
				}
			}

			if ( ! addedChain )
			{
				i->extractTags( tags );
				if ( ! ret->flatten( i ) )
					ret->addDependency( DependencyType::EXPLICIT, i );
			}
		}
	}
}


////////////////////////////////////////


std::shared_ptr<BuildItem>
CompileSet::chainTransform( const std::string &name,
							const std::shared_ptr<Directory> &srcdir,
							TransformSet &xform ) const
{
	std::string ext = File::extension( name );
	std::shared_ptr<Tool> t = getTool( xform, ext );

	std::shared_ptr<BuildItem> ret;
	if ( t )
	{
		VERBOSE( name << ": chaining tool for '" << ext << "'" );
		ret = std::make_shared<BuildItem>( name, srcdir );

		VariableSet buildvars;
		extractVariables( buildvars );
		ret->setVariables( std::move( buildvars ) );

		ret->setTool( t );
		ret->setOutputDir( srcdir );
		xform.add( ret );
	}
	else
	{
		DEBUG( name << ": no tool found for extension '" << ext << "'" );
	}

	return ret;
}


////////////////////////////////////////


void
CompileSet::fillBuildItem( const std::shared_ptr<BuildItem> &bi, TransformSet &xform, std::set<std::string> &tags, bool propagateLibs, const std::vector<ItemPtr> &extraItems ) const
{
	Variable outdefs( "defines" );
	outdefs.setToolTag( "cc" );
	Variable outflags( "cflags" );
	Variable outinc( "includes" );
	outinc.setToolTag( "cc" );
	Variable outlibs( "libs" );
	Variable outlibdirs( "libdirs" );
	outlibdirs.setToolTag( "ld" );
	Variable outldflags( "ldflags" );

	std::queue< std::shared_ptr<BuildItem> > chainsToCheck;

	for ( const ItemPtr &i: myItems )
	{
		std::shared_ptr<BuildItem> ci = i->transform( xform );
		PRECONDITION( ci, "Transform failed on item " << i->getName() );

		const Library *libDep = dynamic_cast<const Library *>( i.get() );
		const PackageConfig *pkg = dynamic_cast<const PackageConfig *>( i.get() );
		const ExternLibrarySet *eLib = dynamic_cast<const ExternLibrarySet *>( i.get() );

		if ( libDep || pkg || eLib )
		{
			bi->addDependency( DependencyType::IMPLICIT, ci );
			outflags.addIfMissing( ci->getVariable( "cflags" ).values() );
			outinc.addIfMissing( ci->getVariable( "includes" ).values() );

			outdefs.addIfMissing( ci->getVariable( "defines" ).values() );

			if ( propagateLibs )
			{
				outldflags.add( ci->getVariable( "ldflags" ).values() );
				if ( libDep )
				{
					outlibs.addIfMissing( libDep->getName() );
					outlibdirs.addIfMissing( ci->getOutDir()->fullpath() );
				}

				outlibs.add( ci->getVariable( "libs" ).values() );
				outlibdirs.addIfMissing( ci->getVariable( "libdirs" ).values() );
			}
			if ( eLib )
				chainsToCheck.push( ci );
		}
		else if ( dynamic_cast<const Executable *>( i.get() ) )
		{
			// executables can't depend on other exes, make this
			// an order only dependency
			VERBOSE( "Executable '" << i->getName() << "' will be built before '" << getName() << "' because of declared dependency" );
			bi->addDependency( DependencyType::ORDER, ci );
		}
		else
		{
			// make it so items in this compile group compile even
			// though there is no parent
			if ( ( isTopLevel() || (!getParent()) ) &&
				 ! ci->getOutputs().empty() )
			{
				ci->setTopLevel( true, ci->getOutputs()[0] );
				ci->setDefaultTarget( true );
			}

			chainsToCheck.push( ci );
		}
	}

	for ( const ItemPtr &i: extraItems )
	{
		std::shared_ptr<BuildItem> ci = i->transform( xform );
		PRECONDITION( ci, "Transform failed on item " << i->getName() );

		const Library *libDep = dynamic_cast<const Library *>( i.get() );
		const PackageConfig *pkg = dynamic_cast<const PackageConfig *>( i.get() );
		const ExternLibrarySet *eLib = dynamic_cast<const ExternLibrarySet *>( i.get() );

		if ( libDep || pkg )
		{
			bi->addDependency( DependencyType::IMPLICIT, ci );
			outflags.addIfMissing( ci->getVariable( "cflags" ).values() );

			outdefs.addIfMissing( ci->getVariable( "defines" ).values() );

			if ( propagateLibs )
			{
				outldflags.add( ci->getVariable( "ldflags" ).values() );
				if ( libDep )
				{
					outlibs.addIfMissing( libDep->getName() );
					outlibdirs.addIfMissing( ci->getOutDir()->fullpath() );
				}

				outlibs.add( ci->getVariable( "libs" ).values() );
				outlibdirs.addIfMissing( ci->getVariable( "libdirs" ).values() );
			}
			if ( eLib )
				chainsToCheck.push( ci );
		}
		else if ( dynamic_cast<const Executable *>( i.get() ) )
		{
			// executables can't depend on other exes, make this
			// an order only dependency
			VERBOSE( "Executable '" << i->getName() << "' will be built before '" << getName() << "' because of declared dependency" );
			bi->addDependency( DependencyType::ORDER, ci );
		}
		else
		{
			// make it so items in this compile group compile even
			// though there is no parent
			if ( isTopLevel() || (!getParent()) )
			{
				ci->setTopLevel( true, ci->getOutputs()[0] );
				ci->setDefaultTarget( true );
			}

			chainsToCheck.push( ci );
		}
	}
	
	followChains( chainsToCheck, tags, bi, xform );

	std::vector< std::shared_ptr<BuildItem> > expDeps = bi->extractDependencies( DependencyType::EXPLICIT );
	for ( const auto &compItem: expDeps )
	{
		if ( !outflags.empty() )
			compItem->addToVariable( "cflags", outflags );
		if ( !outinc.empty() )
			compItem->addToVariable( "includes", outinc );
		if ( !outdefs.empty() )
			compItem->addToVariable( "defines", outdefs );
	}

	if ( ! outflags.empty() )
		bi->addToVariable( "cflags", outflags );
	if ( ! outdefs.empty() )
		bi->addToVariable( "defines", outdefs );
	if ( ! outinc.empty() )
		bi->addToVariable( "includes", outinc );

	if ( propagateLibs )
	{
		if ( ! outlibs.empty() )
		{
			outlibs.removeDuplicatesKeepLast();
			bi->addToVariable( "libs", outlibs );
		}
		if ( ! outlibdirs.empty() )
			bi->addToVariable( "libdirs", outlibdirs );
		if ( ! outldflags.empty() )
			bi->addToVariable( "ldflags", outldflags );
	}
}


////////////////////////////////////////





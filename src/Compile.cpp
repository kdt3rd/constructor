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

	ret = std::make_shared<BuildItem>( getName(), getDir() );
	ret->setUseName( false );
	ret->setOutputDir( xform.getOutDir() );

	std::queue< std::shared_ptr<BuildItem> > chainsToCheck;
	for ( const ItemPtr &i: myItems )
	{
		std::shared_ptr<BuildItem> ci = i->transform( xform );
		PRECONDITION( ci, "Transform failed on item " << i->getName() );
		chainsToCheck.push( ci );
	}

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
			if ( i->getTool() )
				ret->addDependency( DependencyType::EXPLICIT, i );
			else
				ret->addDependency( DependencyType::ORDER, i );
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
					si->addDependency( DependencyType::EXPLICIT, i );
					addedChain = true;
				}
			}

			if ( ! addedChain )
				ret->addDependency( DependencyType::EXPLICIT, i );
		}
	}

	xform.recordTransform( this, ret );
	return ret;
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
		ret = std::make_shared<BuildItem>( name, srcdir );
		ret->setTool( t );
		ret->setOutputDir( xform.getOutDir() );
		xform.add( ret );
	}

	return ret;
}


////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////


Executable::Executable( std::string name )
		: CompileSet( std::move( name ) )
{
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

	ret = std::make_shared<BuildItem>( getName(), getDir() );
	ret->setUseName( false );
	ret->setOutputDir( xform.getBinDir() );
	ret->setTopLevel( true );

	std::set<std::string> tags;
	for ( const ItemPtr &i: myItems )
	{
		std::shared_ptr<BuildItem> xi = i->transform( xform );
		xi->markAsDependent();

		const Library *libDep = dynamic_cast<const Library *>( i.get() );
		const PackageConfig *pkg = dynamic_cast<const PackageConfig *>( i.get() );
		if ( libDep || pkg )
		{
			ret->addDependency( DependencyType::IMPLICIT, xi );
			ret->addToVariable( "cflags", xi->getVariable( "cflags" ) );
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
		{
			xi->extractTags( tags );
			ret->addDependency( DependencyType::EXPLICIT, xi );
		}
	}

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
		}
		throw std::runtime_error( msgbuf.str() );
	}
	ret->setTool( t );

	xform.recordTransform( this, ret );
	return ret;
}


////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////


Library::Library( std::string name )
		: CompileSet( std::move( name ) )
{
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

	Variable outlibs( "libs" ), outlibdirs( "libdirs" );
	std::set<std::string> tags;
	for ( const ItemPtr &i: myItems )
	{
		std::shared_ptr<BuildItem> xi = i->transform( xform );
		xi->markAsDependent();

		const Library *libDep = dynamic_cast<const Library *>( i.get() );
		const PackageConfig *pkg = dynamic_cast<const PackageConfig *>( i.get() );

		if ( libDep || pkg )
		{
			ret->addDependency( DependencyType::IMPLICIT, xi );
			ret->addToVariable( "cflags", xi->getVariable( "cflags" ) );
			ret->addToVariable( "ldflags", xi->getVariable( "ldflags" ) );
			outlibs.moveToEnd( xi->getVariable( "libs" ).values() );
			outlibdirs.moveToEnd( xi->getVariable( "libdirs" ).values() );
		}
		else if ( dynamic_cast<const Executable *>( i.get() ) )
		{
			// executables can't depend on other exes, make this
			// an order only dependency
			VERBOSE( "Executable '" << i->getName() << "' will be built before '" << getName() << "' because of declared dependency" );
			ret->addDependency( DependencyType::ORDER, xi );
		}
		else
		{
			xi->extractTags( tags );
			ret->addDependency( DependencyType::EXPLICIT, xi );
		}

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
		}
		throw std::runtime_error( msgbuf.str() );
	}
	ret->setTool( t );

	if ( libType == "static" )
	{
		if ( ! outlibs.empty() )
			ret->addToVariable( "libs", outlibs );
		if ( ! outlibdirs.empty() )
			ret->addToVariable( "libdirs", outlibdirs );
	}

	xform.recordTransform( this, ret );
	return ret;
}


////////////////////////////////////////





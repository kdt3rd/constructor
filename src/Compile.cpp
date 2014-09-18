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
#include "LuaEngine.h"
#include "TransformSet.h"
#include "BuildItem.h"
#include "FileUtil.h"
#include "PackageConfig.h"
#include <iostream>


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


void
CompileSet::transform( std::vector< std::shared_ptr<BuildItem> > &b,
					   const TransformSet &xform ) const
{
	std::vector< std::shared_ptr<BuildItem> > subItems;
	std::string forceTool;
	for ( const ItemPtr &i: myItems )
	{
		subItems.clear();
		forceTool.clear();
		i->findVariableValueRecursive( forceTool, "tool" );
		applyTransform( i->name(), i->directory(), subItems, xform, forceTool );
		b.insert( b.end(), subItems.begin(), subItems.end() );
	}
}


////////////////////////////////////////


void
CompileSet::applyTransform( const std::string &name,
							const std::shared_ptr<Directory> &srcdir,
							std::vector< std::shared_ptr<BuildItem> > &b,
							const TransformSet &xform,
							const std::string &forceTool ) const
{
	std::string ext = File::extension( name );
	std::shared_ptr<Tool> t;
	if ( ! forceTool.empty() )
	{
		t = xform.findToolByTag( forceTool );
		if ( ! t )
			throw std::runtime_error( "Tool '" + forceTool + "' requested for '" + name + "', but tool not found" );
	}
	else
		t = xform.findTool( ext );

	std::shared_ptr<BuildItem> ni = std::make_shared<BuildItem>( name, srcdir );
	if ( t )
	{
		std::vector< std::shared_ptr<BuildItem> > subItems;
		ni->setTool( t );
		ni->setOutputDir( xform.output_dir() );
		for ( const std::string &bo: ni->outputs() )
		{
			// apply chain rules (i.e. foo.y produces foo.c produces foo.o)
			subItems.clear();
			applyTransform( bo, ni->out_dir(), subItems, xform );
			b.insert( b.end(), subItems.begin(), subItems.end() );
			for ( std::shared_ptr<BuildItem> &si: subItems )
				si->addDependency( DependencyType::EXPLICIT, ni );
		}
	}
	else
		b.push_back( ni );
}


////////////////////////////////////////


Executable::Executable( std::string name )
		: CompileSet( std::move( name ) )
{
}


////////////////////////////////////////


Executable::~Executable( void )
{
}


////////////////////////////////////////


void
Executable::transform( std::vector< std::shared_ptr<BuildItem> > &b,
					   const TransformSet &xform ) const
{
	std::shared_ptr<BuildItem> ni = std::make_shared<BuildItem>(
		name(), directory() );
	ni->setOutputDir( xform.output_dir() );

	std::vector< std::shared_ptr<BuildItem> > subItems;
	for ( const ItemPtr &i: myItems )
	{
		const Library *l = dynamic_cast<const Library *>( i.get() );
		const PackageConfig *p = dynamic_cast<const PackageConfig *>( i.get() );
		const Executable *e = dynamic_cast<const Executable *>( i.get() );

		subItems.clear();
		if ( l )
		{
			std::cout << "Need to handle transformation for exe '" << name() << "' depend on lib: " << i->name() << std::endl;
		}
		else if ( p )
		{
			std::cout << "Need to handle transformation for exe '" << name() << "' depend on external lib: " << i->name() << std::endl;
		}
		else if ( e )
		{
			throw std::runtime_error( "Executables can't depend on other executables" );
		}
		else
		{
			applyTransform( i->name(), i->directory(), subItems, xform );
			for ( std::shared_ptr<BuildItem> &si: subItems )
				ni->addDependency( DependencyType::EXPLICIT, si );
		}
	}

	b.push_back( ni );
}


////////////////////////////////////////


Library::Library( std::string name )
		: CompileSet( std::move( name ) )
{
}


////////////////////////////////////////


Library::~Library( void )
{
}


////////////////////////////////////////


void
Library::transform( std::vector< std::shared_ptr<BuildItem> > &b,
					const TransformSet &xform ) const
{
	for ( const ItemPtr &i: myItems )
	{
		std::string ext = File::extension( i->name() );
	}
}


////////////////////////////////////////





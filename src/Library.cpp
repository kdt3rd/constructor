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
	std::shared_ptr<BuildItem> ret = xform.getTransform( getID() );
	if ( ret )
		return ret;

	DEBUG( "transform Library " << getName() );
	ret = std::make_shared<BuildItem>( getName(), getDir() );
	ret->setUseName( false );
	ret->setOutputDir( xform.getLibDir() );
	ret->setTopLevel( true );
	ret->setDefaultTarget( true );

	std::string libType = myKind;
	if ( libType.empty() )
		libType = xform.getOptionValue( "default_library_kind" );
	if ( libType.empty() )
	{
		libType = "static";
		VERBOSE( "No library type declared for '" << getName() << "', defaulting to static" );
	}

	std::set<std::string> tags;
	fillBuildItem( ret, xform, tags, libType == "static" );

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

	xform.recordTransform( getID(), ret );
	return ret;
}

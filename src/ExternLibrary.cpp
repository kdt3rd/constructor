//
// Copyright (c) 2016 Kimball Thurston
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

#include "ExternLibrary.h"
#include "PackageSet.h"
#include "Debug.h"
#include <stdexcept>


////////////////////////////////////////


ExternLibrarySet::ExternLibrarySet( void )
		: OptionalSource( "__extern_lib__" )
{
}


////////////////////////////////////////


ExternLibrarySet::~ExternLibrarySet( void )
{
}


////////////////////////////////////////


void
ExternLibrarySet::addExternRef( std::string l, std::string v )
{
	myExternLibs.emplace_back( std::make_pair( std::move( l ), std::move( v ) ) );
}


////////////////////////////////////////


std::shared_ptr<BuildItem>
ExternLibrarySet::transform( TransformSet &xform ) const
{
	std::shared_ptr<BuildItem> ret = xform.getTransform( this );
	if ( ret )
		return ret;

	ret = std::make_shared<BuildItem>( getName(), getDir() );
	ret->setUseName( false );
	ret->setOutputDir( xform.getOutDir() );

	if ( matches( xform ) )
	{
		DEBUG( "transform ENABLED ExternLibrary " << getName() );
		std::set<std::string> tags;
		bool ok = true;
		std::vector<ItemPtr> extras;
		PackageSet &ps = PackageSet::get( xform.getSystem() );
		for ( auto &l: myExternLibs )
		{
			auto elib = ps.find( l.first, l.second, xform.getLibSearchPath(), xform.getPkgSearchPath() );
			if ( ! elib )
			{
				WARNING( "Unable to find external library '" << l.first << "' (version: " << (l.second.empty()?std::string("<any>"):l.second) << ") for system " << xform.getSystem() );
				ok = false;
			}
			else
				extras.push_back( elib );
		}

		if ( ok )
		{
			if ( ! myDefinitions.empty() )
				ret->setVariable( "defines", myDefinitions );

			fillBuildItem( ret, xform, tags, true, extras );
		}
		else if ( isRequired() )
			throw std::runtime_error( "Unable to resolve external libraries for required libraries" );
	}

	xform.recordTransform( this, ret );
	return ret;
}


////////////////////////////////////////






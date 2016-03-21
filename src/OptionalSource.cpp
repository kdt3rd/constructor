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

#include "OptionalSource.h"
#include "Debug.h"
#include <stdexcept>
#include "TransformSet.h"
#include "Library.h"
#include "PackageConfig.h"
#include "Executable.h"


////////////////////////////////////////


OptionalSource::OptionalSource( void )
		: CompileSet( "__optional_compile__" )
{
}


////////////////////////////////////////


OptionalSource::OptionalSource( std::string nm )
		: CompileSet( std::move( nm ) )
{
}


////////////////////////////////////////


OptionalSource::~OptionalSource( void )
{
}


////////////////////////////////////////


void
OptionalSource::addCondition( std::string tag, std::string val )
{
	if ( tag != "system" )
		throw std::logic_error( "NYI: optional source condition: " + tag );

	auto i = myConditions.find( tag );
	if ( i == myConditions.end() )
	{
		myConditions.emplace( std::make_pair( std::move( tag ), std::move( val ) ) );
	}
	else
	{
		i->second = std::move( val );
	}
}


////////////////////////////////////////


std::shared_ptr<BuildItem>
OptionalSource::transform( TransformSet &xform ) const
{
	std::shared_ptr<BuildItem> ret = xform.getTransform( this );
	if ( ret )
		return ret;

	ret = std::make_shared<BuildItem>( getName(), getDir() );
	ret->setUseName( false );
	ret->setOutputDir( xform.getOutDir() );

	if ( matches( xform ) )
	{
		DEBUG( "transform ENABLED OptionalSource " << getName() );
		std::set<std::string> tags;
		fillBuildItem( ret, xform, tags, false );
	}

	xform.recordTransform( this, ret );
	return ret;
}


////////////////////////////////////////


bool
OptionalSource::matches( TransformSet &xform ) const
{
	for ( auto i: myConditions )
	{
		if ( i.first == "system" )
		{
			if ( xform.getSystem() != i.second )
				return false;
		}
	}

	return true;
}


////////////////////////////////////////


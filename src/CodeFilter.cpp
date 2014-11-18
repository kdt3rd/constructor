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

#include "CodeFilter.h"

#include "Debug.h"


////////////////////////////////////////


CodeFilter::CodeFilter( std::string name )
		: CompileSet( std::move( name ) )
{
}


////////////////////////////////////////


CodeFilter::~CodeFilter( void )
{
}


////////////////////////////////////////


void
CodeFilter::setTool( const std::shared_ptr<Tool> &t )
{
	myTool = t;
}


////////////////////////////////////////


void
CodeFilter::setOutputs( std::vector<std::string> o )
{
	myOutputs = std::move( o );
}


////////////////////////////////////////


std::shared_ptr<BuildItem>
CodeFilter::transform( TransformSet &xform ) const
{
	std::shared_ptr<BuildItem> ret = xform.getTransform( this );
	if ( ret )
		return ret;

	ret = std::make_shared<BuildItem>( getName(), getDir() );
	ret->setUseName( false );

	VariableSet buildvars;
	extractVariables( buildvars );
	ret->setVariables( std::move( buildvars ) );

	if ( myTool )
	{
		ItemPtr genExe = myTool->getGeneratedExecutable();
		if ( genExe )
		{
			std::shared_ptr<BuildItem> exeDep = genExe->transform( xform );
			ret->addDependency( DependencyType::IMPLICIT, exeDep );
		}

		for ( const ItemPtr &i: myItems )
		{
			// huh, we can't call transform on these because then
			// cpp files we're going to filter will be transformed to
			// .o files.
			auto inp = std::make_shared<BuildItem>( i->getName(), i->getDir() );
			inp->setUseName( false );
			inp->setOutputDir( getDir() );
			inp->setOutputs( { i->getName() } );
			ret->addDependency( DependencyType::EXPLICIT, inp );
		}
		
		ret->setTool( myTool );
		auto outd = getDir()->reroot( xform.getArtifactDir() );
		ret->setOutputDir( outd );
		ret->setVariable( "current_output_dir", outd->fullpath() );
		ret->setOutputs( myOutputs );
	}

	xform.recordTransform( this, ret );

	return ret;
}


////////////////////////////////////////



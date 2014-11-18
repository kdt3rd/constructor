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

#include "CreateFile.h"

#include "Debug.h"
#include "TransformSet.h"
#include "FileUtil.h"


////////////////////////////////////////


CreateFile::CreateFile( const std::string &name )
		: Item( name )
{
}


////////////////////////////////////////


CreateFile::~CreateFile( void )
{
}


////////////////////////////////////////


void
CreateFile::setLines( const std::vector<std::string> &l )
{
	myLines = l;
}


////////////////////////////////////////


std::shared_ptr<BuildItem>
CreateFile::transform( TransformSet &xform ) const
{
	std::shared_ptr<BuildItem> ret = xform.getTransform( this );
	if ( ret )
		return ret;

	ret = std::make_shared<BuildItem>( getName(), getDir() );

	auto outd = getDir()->reroot( xform.getArtifactDir() );
	outd->updateIfDifferent( getName(), myLines );

	ret->setOutputDir( outd );
	ret->setOutputs( { getName() } );

	xform.recordTransform( this, ret );

	return ret;
}


////////////////////////////////////////



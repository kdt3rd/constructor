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

#include "BuildItem.h"
#include <stdexcept>
#include "FileUtil.h"
#include <iostream>


////////////////////////////////////////


BuildItem::BuildItem( const std::string &name,
					  const std::shared_ptr<Directory> &srcdir )
		: myName( name ), myDirectory( srcdir )
{
}


////////////////////////////////////////


BuildItem::BuildItem( std::string &&name,
					  const std::shared_ptr<Directory> &srcdir )
		: myName( std::move( name ) ), myDirectory( srcdir )
{
}


////////////////////////////////////////


BuildItem::~BuildItem( void )
{
}


////////////////////////////////////////


const std::string &
BuildItem::name( void ) const
{
	return myName;
}


////////////////////////////////////////


void
BuildItem::setOutputDir( const std::shared_ptr<Directory> &d )
{
	myOutDirectory = d;
}


////////////////////////////////////////


void
BuildItem::setTool( const std::shared_ptr<Tool> &t )
{
	if ( myTool )
		throw std::runtime_error( "Tool already specified for build item " + name() );

	myTool = t;
	if ( ! myTool )
		throw std::runtime_error( "Invalid tool specified for build item " + name() );

	for ( const std::string &o: myTool->outputs() )
		myOutputs.emplace_back( std::move( File::replaceExtension( name(), o ) ) );
}


////////////////////////////////////////





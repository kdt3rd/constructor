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

#include "TransformSet.h"

#include "Debug.h"
#include "StrUtil.h"


////////////////////////////////////////


TransformSet::TransformSet( const std::shared_ptr<Directory> &d )
		: myDirectory( d )
{
	PRECONDITION( myDirectory, "Invalid output directory specified" );

	myDirectory->promoteFull();
	myBinDirectory = std::make_shared<Directory>( *d );
	myBinDirectory->cd( "bin" );
	myBinDirectory->promoteFull();
	myLibDirectory = std::make_shared<Directory>( *d );
	myLibDirectory->cd( "lib" );
	myLibDirectory->promoteFull();
	myArtifactDirectory = std::make_shared<Directory>( *d );
	myArtifactDirectory->cd( "artifacts" );
	myArtifactDirectory->promoteFull();
}


////////////////////////////////////////


TransformSet::~TransformSet( void )
{
}


////////////////////////////////////////


void
TransformSet::addChildScope( const std::shared_ptr<TransformSet> &cs )
{
	myChildScopes.push_back( cs );
}


////////////////////////////////////////


void
TransformSet::addTool( const std::shared_ptr<Tool> &t )
{
	myTools.push_back( t );
}


////////////////////////////////////////


std::shared_ptr<Tool>
TransformSet::getTool( const std::string &tag ) const
{
	for ( auto &t: myTools )
		if ( t->getTag() == tag )
			return t;
	return std::shared_ptr<Tool>();
}


////////////////////////////////////////


void
TransformSet::mergeVariables( const VariableSet &vs )
{
	if ( myVars.empty() )
		myVars = vs;
	else
	{
		for ( auto i: vs )
			myVars.emplace( std::make_pair( i.first, i.second ) );
	}
}


////////////////////////////////////////


void
TransformSet::mergeOptions( const VariableSet &vs )
{
	if ( myOptions.empty() )
		myOptions = vs;
	else
	{
		for ( auto i: vs )
			myOptions.emplace( std::make_pair( i.first, i.second ) );
	}
}


////////////////////////////////////////


std::shared_ptr<Tool>
TransformSet::findTool( const std::string &ext ) const
{
	for ( auto &t: myTools )
		if ( t->handlesExtension( ext ) )
			return t;
	return std::shared_ptr<Tool>();
}


////////////////////////////////////////


std::shared_ptr<Tool>
TransformSet::findToolByTag( const std::string &tag,
							 const std::string &ext ) const
{
	for ( auto &t: myTools )
		if ( t->getTag() == tag && t->handlesExtension( ext ) )
			return t;

	DEBUG( "Tool Tag '" + tag + "' not found that handles extension '" + ext + "', falling back to normal tool search" );

	return findTool( ext );
}


////////////////////////////////////////


std::shared_ptr<Tool>
TransformSet::findToolForSet( const std::string &tag_prefix,
							  const std::set<std::string> &s ) const
{
	for ( auto &t: myTools )
		if ( t->handlesTools( s ) &&
			 String::startsWith( t->getTag(), tag_prefix ) )
			return t;

	return std::shared_ptr<Tool>();
}


////////////////////////////////////////


const std::string &
TransformSet::getVarValue( const std::string &v ) const
{
	auto i = myVars.find( v );
	if ( i != myVars.end() )
		return i->second.value();

	return String::empty();
}


////////////////////////////////////////


const std::string &
TransformSet::getOptionValue( const std::string &v ) const
{
	auto i = myOptions.find( v );
	if ( i != myOptions.end() )
		return i->second.value();

	return String::empty();
}


////////////////////////////////////////


bool
TransformSet::isTransformed( const Item *i ) const
{
	auto bi = myTransformMap.find( i );
	return bi != myTransformMap.end();
}


////////////////////////////////////////


std::shared_ptr<BuildItem>
TransformSet::getTransform( const Item *i ) const
{
	auto bi = myTransformMap.find( i );
	if ( bi != myTransformMap.end() )
		return bi->second;
	return std::shared_ptr<BuildItem>();
}


////////////////////////////////////////


void
TransformSet::recordTransform( const Item *i,
							   const std::shared_ptr<BuildItem> &bi )
{
	add( bi );
	myTransformMap[i] = bi;
}


////////////////////////////////////////


void
TransformSet::add( const std::shared_ptr<BuildItem> &bi )
{
	myBuildItems.push_back( bi );
}


////////////////////////////////////////


void
TransformSet::add( const std::vector< std::shared_ptr<BuildItem> > &items )
{
	myBuildItems.insert( myBuildItems.end(), items.begin(), items.end() );
}


////////////////////////////////////////







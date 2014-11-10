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
#include "StrUtil.h"
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


void
BuildItem::setName( const std::string &n )
{
	myName = n;
}


////////////////////////////////////////


const std::string &
BuildItem::getName( void ) const
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
BuildItem::addExternalOutput( const std::string &fn )
{
	myOutputs.push_back( fn );
}


////////////////////////////////////////


void
BuildItem::setTool( const std::shared_ptr<Tool> &t )
{
	if ( myTool )
		throw std::runtime_error( "Tool already specified for build item " + getName() );

	myTool = t;
	if ( ! myTool )
		throw std::runtime_error( "Invalid tool specified for build item " + getName() );

	for ( const std::string &o: myTool->getOutputs() )
		myOutputs.emplace_back( myTool->getOutputPrefix() + std::move( File::replaceExtension( getName(), o ) ) );

	if ( myOutputs.empty() )
		myOutputs.push_back( getName() );
}


////////////////////////////////////////


void
BuildItem::extractTags( std::set<std::string> &tags ) const
{
	if ( myTool )
		tags.insert( myTool->getTag() );
	else
	{
		std::vector<ItemPtr> deps = extractDependencies( DependencyType::EXPLICIT );
		for ( ItemPtr &i: deps )
			i->extractTags( tags );
	}
}


////////////////////////////////////////


const std::string &
BuildItem::getTag( void ) const
{
	if ( myTool )
		return myTool->getTag();

	return String::empty();
}


////////////////////////////////////////


void
BuildItem::setFlag( const std::string &n, const std::string &v )
{
	myFlags[n] = v;
}


////////////////////////////////////////


void
BuildItem::setVariables( VariableSet v )
{
	myVariables = std::move( v );
}


////////////////////////////////////////


void
BuildItem::setVariable( const std::string &name, const std::string &val )
{
	auto i = myVariables.find( name );
	if ( i != myVariables.end() )
		i->second.reset( val );
	else
		myVariables.emplace( std::make_pair( name, Variable( name, val ) ) );
}


////////////////////////////////////////


void
BuildItem::addToVariable( const std::string &name, const std::string &val )
{
	if ( val.empty() )
		return;

	auto i = myVariables.find( name );
	if ( i != myVariables.end() )
		i->second.moveToEnd( val );
	else
		myVariables.emplace( std::make_pair( name, Variable( name, val ) ) );
}


////////////////////////////////////////


void
BuildItem::addToVariable( const std::string &name, const Variable &val )
{
	auto i = myVariables.find( name );
	if ( i != myVariables.end() )
		i->second.moveToEnd( val.values() );
	else
		myVariables.emplace( std::make_pair( name, Variable( val ) ) );
}


////////////////////////////////////////


const Variable &
BuildItem::getVariable( const std::string &name ) const
{
	auto i = myVariables.find( name );
	if ( i != myVariables.end() )
		return i->second;
	return Variable::nil();
}


////////////////////////////////////////


const std::string &
BuildItem::getFlag( const std::string &n ) const
{
	auto i = myFlags.find( n );
	if ( i != myFlags.end() )
		return i->second;
	return String::empty();
}


////////////////////////////////////////





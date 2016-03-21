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
#include "Debug.h"
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
		myOutputs.emplace_back( myTool->getOutputPrefix() + File::replaceExtension( getName(), o ) );

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
		if ( deps.empty() )
			VERBOSE( getName() << " has no explicit dependencies" );
		for ( const ItemPtr &i: deps )
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
BuildItem::setOutputs( const std::vector<std::string> &outList )
{
	myOutputs = outList;
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
BuildItem::setVariable( const std::string &name, const std::vector<std::string> &val )
{
	auto i = myVariables.find( name );
	if ( i != myVariables.end() )
		i->second.reset( val );
	else
		myVariables.emplace( std::make_pair( name, Variable( name ) ) ).first->second.reset( val );
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
	{
		auto ni = myVariables.emplace( std::make_pair( name, Variable( val ) ) );
		if ( val.useToolFlagTransform() )
			ni.first->second.setToolTag( val.getToolTag() );
	}
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


bool
BuildItem::flatten( const std::shared_ptr<BuildItem> &i )
{
	if ( !(i->useName()) && i->getOutputs().empty() && ! i->getTool() )
	{
		for ( auto &d: i->extractDependencies( DependencyType::EXPLICIT ) )
			addDependency( DependencyType::EXPLICIT, d );
		for ( auto &d: i->extractDependencies( DependencyType::CHAIN ) )
			addDependency( DependencyType::CHAIN, d );
		for ( auto &d: i->extractDependencies( DependencyType::IMPLICIT ) )
			addDependency( DependencyType::IMPLICIT, d );
		for ( auto &d: i->extractDependencies( DependencyType::ORDER ) )
			addDependency( DependencyType::ORDER, d );

		for ( const auto &v: i->getVariables() )
		{
			auto mv = myVariables.find( v.first );
			if ( mv != myVariables.end() )
				mv->second.merge( v.second );
			else
				myVariables.emplace( std::make_pair( v.first, v.second ) );
		}
		return true;
	}
	return false;
}


////////////////////////////////////////



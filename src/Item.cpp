// Item.cpp -*- C++ -*-

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

#include "Item.h"
#include "StrUtil.h"
#include "FileUtil.h"
#include "Debug.h"
#include "TransformSet.h"
#include <iostream>


////////////////////////////////////////


namespace {

static Item::ID theLastID = 1;

} // empty namespace


////////////////////////////////////////


Item::Item( const std::string &name )
		: myID( theLastID++ ), myName( name ),
		  myDirectory( Directory::current() )
{
}


////////////////////////////////////////


Item::Item( std::string &&name )
		: myID( theLastID++ ), myName( std::move( name ) ),
		  myDirectory( Directory::current() )
{
}


////////////////////////////////////////


Item::~Item( void )
{
}


////////////////////////////////////////


const std::string &
Item::getName( void ) const 
{
	return myName;
}


////////////////////////////////////////


std::shared_ptr<BuildItem>
Item::transform( TransformSet &xform ) const
{
	std::shared_ptr<BuildItem> ret = xform.getTransform( this );
	if ( ret )
		return ret;

	ret = std::make_shared<BuildItem>( getName(), getDir() );

	VariableSet buildvars;
	extractVariables( buildvars );
	ret->setVariables( std::move( buildvars ) );

	std::shared_ptr<Tool> t = getTool( xform );

	if ( t )
	{
		DEBUG( getName() << " transformed by tool '" << t->getTag() << "' (" << t->getName() << ")" );
		ret->setTool( t );
		ret->setOutputDir( getDir()->reroot( xform.getArtifactDir() ) );
		std::string overOpt;
		for ( auto &i: t->allOptions() )
		{
			if ( hasToolOverride( i.first, overOpt ) )
				ret->setVariable( t->getOptionVariable( i.first ),
								  t->getOptionValue( i.first, overOpt ) );
		}
	}

	xform.recordTransform( this, ret );
	return ret;
}


////////////////////////////////////////


void
Item::copyDependenciesToBuild( TransformSet &xform ) const
{
	std::shared_ptr<BuildItem> ret = xform.getTransform( this );
	if ( ! ret )
		return;

	for ( auto dep: myDependencies )
	{
		if ( dep.first )
		{
			std::shared_ptr<BuildItem> d = xform.getTransform( dep.first.get() );
			if ( d )
				ret->addDependency( dep.second, d );
		}
	}
}


////////////////////////////////////////


void
Item::forceTool( const std::string &ext, const std::string &t )
{
	myForceTool[ext] = t;
}


////////////////////////////////////////


void
Item::overrideToolSetting( const std::string &s, const std::string &n )
{
	myOverrideToolOptions[s] = n;
}


////////////////////////////////////////


Variable &
Item::getVariable( const std::string &nm )
{
	auto v = myVariables.emplace( std::make_pair( nm, Variable( nm ) ) );
	return v.first->second;
}


////////////////////////////////////////


const Variable &
Item::getVariable( const std::string &nm ) const
{
	auto i = myVariables.find( nm );
	if ( i == myVariables.end() )
	{
		static const Variable nilVar = Variable( std::string() );
		return nilVar;
	}
	return i->second;
}


////////////////////////////////////////


void
Item::setVariable( const std::string &nm, const std::string &value,
				   bool doSplit )
{
	if ( doSplit )
		getVariable( nm ).reset( String::split( value, ' ' ) );
	else
		getVariable( nm ).reset( value );
}


////////////////////////////////////////


bool
Item::findVariableValueRecursive( std::string &val, const std::string &nm ) const
{
	auto x = myVariables.find( nm );
	if ( x == myVariables.end() )
	{
		ItemPtr i = getParent();
		if ( i )
			return i->findVariableValueRecursive( val, nm );

		val.clear();
		return false;
	}

	val = x->second.value();
	return true;
}


////////////////////////////////////////


void
Item::extractVariables( VariableSet &vs ) const
{
	ItemPtr i = getParent();
	if ( i )
		i->extractVariables( vs );
	for ( auto x = myVariables.begin(); x != myVariables.end(); ++x )
		vs.emplace( std::make_pair( x->first, x->second ) );
}


////////////////////////////////////////


void
Item::extractVariablesExcept( VariableSet &vs, const std::string &v ) const
{
	ItemPtr i = getParent();
	if ( i )
		i->extractVariablesExcept( vs, v );
	for ( auto x = myVariables.begin(); x != myVariables.end(); ++x )
	{
		if ( x->first != v )
			vs.emplace( std::make_pair( x->first, x->second ) );
	}
}


////////////////////////////////////////


void
Item::extractVariablesExcept( VariableSet &vs, const std::set<std::string> &vl ) const
{
	ItemPtr i = getParent();
	if ( i )
		i->extractVariablesExcept( vs, vl );
	for ( auto x = myVariables.begin(); x != myVariables.end(); ++x )
	{
		if ( vl.find( x->first ) == vl.end() )
			vs.emplace( std::make_pair( x->first, x->second ) );
	}
}


////////////////////////////////////////


std::shared_ptr<Tool>
Item::getTool( TransformSet &xform ) const
{
	std::string ext = File::extension( getName() );
	return getTool( xform, ext );
}
	

////////////////////////////////////////


std::shared_ptr<Tool>
Item::getTool( TransformSet &xform, const std::string &ext ) const
{
	auto x = myForceTool.find( ext );
	if ( x != myForceTool.end() )
	{
		DEBUG( "Overriding tool for extension '" << ext << "' to '" << x->second << "'" );
		return xform.findToolByTag( x->second, ext );
	}

	ItemPtr i = getParent();
	if ( i )
		return i->getTool( xform, ext );

	return xform.findTool( ext );
}


////////////////////////////////////////


bool
Item::hasToolOverride( const std::string &opt, std::string &val ) const
{
	auto x = myOverrideToolOptions.find( opt );
	if ( x != myOverrideToolOptions.end() )
	{
		val = x->second;
		return true;
	}

	ItemPtr i = getParent();
	if ( i )
		return i->hasToolOverride( opt, val );

	val.clear();
	return false;
}


////////////////////////////////////////



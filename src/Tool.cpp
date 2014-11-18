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

#include "Tool.h"
#include "LuaEngine.h"
#include "Scope.h"
#include "Debug.h"
#include "StrUtil.h"
#include "TransformSet.h"
#include <iostream>
#include <stdexcept>


////////////////////////////////////////


Tool::Tool( std::string t, std::string n )
		: myTag( std::move( t ) ), myName( std::move( n ) )
{
}


////////////////////////////////////////


Tool::~Tool( void )
{
}


////////////////////////////////////////


void
Tool::enableLanguage( const std::string &nm )
{
	auto lang = myOptions.find( "language" );
	if ( lang != myOptions.end() )
	{
		auto carelang = lang->second.find( nm );
		if ( carelang != lang->second.end() )
		{
			if ( ! myLanguage.empty() )
				throw std::runtime_error( "Only 1 language per tool is currently implemented" );

			myLanguage = nm;
		}
	}
}


////////////////////////////////////////


const std::string &
Tool::getLanguage( void ) const
{
	if ( myLanguage.empty() )
	{
		auto x = myOptionDefaults.find( "language" );
		if ( x != myOptionDefaults.end() )
			return x->second;

		auto y = myOptions.find( "language" );
		if ( y != myOptions.end() )
		{
			if ( ! y->second.empty() )
			{
				auto z = y->second.begin();
				return z->first;
			}
		}

		return String::empty();
	}
	return myLanguage;
}


////////////////////////////////////////


const std::string &
Tool::getCommandPrefix( const std::string &varname ) const
{
	auto o = myFlagPrefixes.find( varname );
	if ( o != myFlagPrefixes.end() )
		return o->second;
	return String::empty();
}


////////////////////////////////////////


Tool::OptionSet &
Tool::getOption( const std::string &name )
{
	return myOptions[name];
}


////////////////////////////////////////


const Tool::OptionSet &
Tool::getOption( const std::string &nm ) const
{
	auto o = myOptions.find( nm );
	if ( o == myOptions.end() )
		throw std::runtime_error( "Option '" + nm + "' does not exist in tool '" + getName() + "'" );

	return o->second;
}


////////////////////////////////////////


bool
Tool::hasOption( const std::string &name ) const
{
	return myOptions.find( name ) != myOptions.end();
}


////////////////////////////////////////


std::string
Tool::getDefaultOption( const std::string &opt ) const
{
	std::string ret;
	auto x = myOptionDefaults.find( opt );
	if ( x != myOptionDefaults.end() )
		ret = x->second;

	return std::move( ret );
}


////////////////////////////////////////


std::string
Tool::getOptionValue( const std::string &opt, const std::string &choice ) const
{
	auto i = myOptions.find( opt );
	if ( i == myOptions.end() )
		return std::string();

	auto x = i->second.find( choice );
	if ( x != i->second.end() )
	{
		std::stringstream rval;
		bool notfirst = false;
		for ( const std::string &oss: x->second )
		{
			if ( notfirst )
				rval << ' ';
			rval << oss;
			notfirst = true;
		}
		return rval.str();
	}
	return std::string();
}


////////////////////////////////////////


std::string
Tool::getOptionVariable( const std::string &opt ) const
{
	std::string varname = getTag();
	varname.push_back( '_' );
	varname.append( opt );
	String::sanitize( varname );
	return std::move( varname );
}


////////////////////////////////////////


void
Tool::addOption( const std::string &opt,
				 const std::string &nm,
				 const std::vector<std::string> &cmd )
{
	auto o = myOptions.find( opt );
	if ( o == myOptions.end() )
		throw std::runtime_error( "Option '" + opt + "' does not exist in tool '" + getName() + "'" );

	o->second[nm] = cmd;
}


////////////////////////////////////////


const std::string &
Tool::getExecutable( void ) const
{
	return myExeName;
}


////////////////////////////////////////


ItemPtr
Tool::getGeneratedExecutable( void ) const
{
	return myExePointer;
}


////////////////////////////////////////


bool
Tool::handlesExtension( const std::string &e ) const
{
	if ( e.empty() )
		return myExtensions.empty() && myAltExtensions.empty();

	for ( auto &me: myExtensions )
		if ( me == e )
			return true;

	for ( auto &ma: myAltExtensions )
		if ( ma == e )
			return true;

	return false;
}


////////////////////////////////////////


bool
Tool::handlesTools( const std::set<std::string> &s ) const
{
	if ( s.empty() )
		return false;

	for ( auto &i: s )
	{
		bool found = false;
		for ( auto &t: myInputTools )
		{
			if ( t == i )
			{
				found = true;
				break;
			}
		}

		if ( ! found )
			return false;
	}

	return true;
}


////////////////////////////////////////


namespace
{

struct ToolSubst
{
	const Tool &myTool;
	const TransformSet &myXformSet;
	std::string myExecutable;
	std::string myLastVar;
	bool myUseBraces;

	ToolSubst( const Tool &t, const TransformSet &x, bool useBraces )
			: myTool( t ), myXformSet( x ), myUseBraces( useBraces )
	{
		ItemPtr ei = myTool.getGeneratedExecutable();
		if ( ei )
		{
			std::shared_ptr<BuildItem> bi = myXformSet.getTransform( ei.get() );
			if ( ! bi )
				throw std::runtime_error( "Unable to find transformed build tool" );

			PRECONDITION( bi->getOutputs().size() == 1,
						  "Expecting executable build item to only have 1 output" );
			myExecutable = bi->getOutDir()->makefilename( bi->getOutputs()[0] );
		}
		else
			myExecutable = myTool.getExecutable();
	}

	const std::string &operator()( const std::string &v )
	{
		if ( v == "exe" )
			return myExecutable;

		myLastVar.clear();
		myLastVar.push_back( '$' );
		if ( myUseBraces )
			myLastVar.push_back( '{' );
		if ( myTool.hasOption( v ) )
			myLastVar.append( myTool.getOptionVariable( v ) );
		else
			myLastVar.append( v );
		if ( myUseBraces )
			myLastVar.push_back( '}' );

		return myLastVar;
	}
};

}


////////////////////////////////////////


Rule
Tool::createRule( const TransformSet &xset, bool useBraces ) const
{
	ToolSubst x( *this, xset, useBraces );

	std::string desc = myDescription;
	String::substituteVariables( desc, false, x );
	
	Rule ret( getTag(), desc );

	std::vector<std::string> cmd;
	for ( std::string ci: myCommand )
	{
		String::substituteVariables( ci, false, x );
		cmd.emplace_back( std::move( ci ) );
	}
	for ( std::string ci: myImplDepCmd )
	{
		String::substituteVariables( ci, false, x );
		cmd.emplace_back( std::move( ci ) );
	}
	ret.setCommand( std::move( cmd ) );

	for ( auto &i: myOptions )
	{
		std::string opt;
		if ( i.first == "language" )
			opt = getLanguage();
		else
		{
			opt = xset.getOptionValue( i.first );
			if ( opt.empty() )
				opt = getDefaultOption( i.first );
		}

		auto io = i.second.find( opt );
		if ( io != i.second.end() )
		{
			std::stringstream rval;
			bool notfirst = false;
			for ( const std::string &oss: io->second )
			{
				if ( notfirst )
					rval << ' ';
				rval << oss;
				notfirst = true;
			}
			ret.setVariable( getOptionVariable( i.first ), rval.str() );
		}
	}

	ret.setDependencyFile( myImplDepName );
	ret.setDependencyStyle( myImplDepStyle );
	VERBOSE( "Need to handle job pool and output restat in tool definition" );
//	ret.setJobPool();
//	ret.setOutputRestat();

	return std::move( ret );
}


////////////////////////////////////////


std::shared_ptr<Tool>
Tool::parse( const Lua::Value &v )
{
	const Lua::Table &t = v.asTable();

	std::string tag, name, desc;
	std::vector<std::string> inpExt;
	std::vector<std::string> altExt;
	std::vector<std::string> inpTools;
	std::vector<std::string> outpExt;
	ItemPtr exePtr;
	std::string exe, outputPrefix;
	OptionGroup opts;
	OptionDefaultSet optDefaults;
	OptionDefaultSet flagPrefixes;
	std::string impFile, impStyle;
	std::vector<std::string> impCmd;
	std::vector<std::string> cmd;
	for ( auto &i: t )
	{
		if ( i.first.type == Lua::KeyType::INDEX )
			continue;

		const std::string &k = i.first.tag;
		if ( k == "tag" )
			tag = i.second.asString();
		else if ( k == "name" )
			name = i.second.asString();
		else if ( k == "description" )
			desc = i.second.asString();
		else if ( k == "exe" )
		{
			if ( i.second.type() == LUA_TUSERDATA )
			{
				void *ud = i.second.asPointer();
				if ( ud )
					exePtr = *(reinterpret_cast<ItemPtr *>( ud ) );
				else
					throw std::runtime_error( "User data item is not a valid item" );
			}
			else if ( i.second.type() == LUA_TSTRING )
				exe = i.second.asString();
			else
				throw std::runtime_error( "Unknown type provided for executable" );
		}
		else if ( k == "input_extensions" )
		{
			inpExt = i.second.toStringList();
		}
		else if ( k == "alt_extensions" )
		{
			altExt = i.second.toStringList();
		}
		else if ( k == "output_extensions" )
		{
			outpExt = i.second.toStringList();
		}
		else if ( k == "output_prefix" )
		{
			outputPrefix = i.second.asString();
		}
		else if ( k == "input_tools" )
		{
			inpTools = i.second.toStringList();
		}
		else if ( k == "options" )
		{
			for ( auto &o: i.second.asTable() )
			{
				if ( o.first.type == Lua::KeyType::INDEX )
					throw std::runtime_error( "Expecting hash map of option name to option sets" );

				OptionSet &os = opts[o.first.tag];
				for ( auto &s: o.second.asTable() )
				{
					if ( s.first.type == Lua::KeyType::INDEX )
						throw std::runtime_error( "Expecting hash map of option commands to option names" );
					os[s.first.tag] = s.second.toStringList();
				}
			}
		}
		else if ( k == "option_defaults" )
		{
			for ( auto &od: i.second.asTable() )
			{
				if ( od.first.type == Lua::KeyType::INDEX )
					throw std::runtime_error( "Expecting hash map of option name to default value" );

				optDefaults[od.first.tag] = od.second.asString();
			}
		}
		else if ( k == "flag_prefixes" )
		{
			for ( auto &od: i.second.asTable() )
			{
				if ( od.first.type == Lua::KeyType::INDEX )
					throw std::runtime_error( "Expecting hash map of variable name to command line flag prefix" );

				flagPrefixes[od.first.tag] = od.second.asString();
			}
		}
		else if ( k == "implicit_dependencies" )
		{
			auto &id = i.second.asTable();
			auto idfn = id.find( "file" );
			auto idcmd = id.find( "cmd" );
			auto idst = id.find( "style" );

			if ( idfn == id.end() )
				throw std::runtime_error( "Expecting a file name definition for implicit_dependencies" );

			impFile = idfn->second.asString();
			if ( idcmd != id.end() )
				impCmd = idcmd->second.toStringList();
			if ( idst != id.end() )
				impStyle = idst->second.asString();
		}
		else if ( k == "cmd" )
		{
			cmd = i.second.toStringList();
		}
	}

	std::shared_ptr<Tool> ret = std::make_shared<Tool>( tag, name );
	ret->myDescription = desc;
	ret->myExePointer = exePtr;
	ret->myExeName = exe;
	ret->myExtensions = inpExt;
	ret->myAltExtensions = altExt;
	ret->myOutputs = outpExt;
	ret->myOutputPrefix = outputPrefix;
	ret->myCommand = cmd;
	ret->myInputTools = inpTools;
	ret->myFlagPrefixes = flagPrefixes;
	ret->myOptions = opts;
	ret->myOptionDefaults = optDefaults;
	ret->myImplDepName = impFile;
	ret->myImplDepStyle = impStyle;
	ret->myImplDepCmd = impCmd;

	return ret;
}


////////////////////////////////////////


std::shared_ptr<Tool>
Tool::createInternalTool( const std::string &tag,
						  const std::string &name,
						  const std::string &desc,
						  const std::string &exeName,
						  const ItemPtr &exe,
						  const std::vector<std::string> &command )
{
	std::shared_ptr<Tool> ret = std::make_shared<Tool>( tag, name );
	ret->myDescription = desc;
	ret->myExePointer = exe;
	if ( ! exe )
		ret->myExeName = exeName;
	ret->myCommand = command;
	return ret;
}


////////////////////////////////////////





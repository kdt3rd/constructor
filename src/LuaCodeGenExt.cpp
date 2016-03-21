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

#include "LuaCodeGenExt.h"

#include "Debug.h"

#include "LuaEngine.h"
#include "LuaValue.h"

#include "Scope.h"
#include "Compile.h"
#include "Item.h"
#include "CreateFile.h"
#include "CodeFilter.h"
#include "InternalExecutable.h"
#include "CodeGenerator.h"
#include "LuaItemExt.h"


////////////////////////////////////////


namespace
{


////////////////////////////////////////


int
luaCodeFilter( lua_State *L )
{
	if ( lua_gettop( L ) != 1 || ! lua_istable( L, 1 ) )
		throw std::runtime_error( "Expecting a single (table) argument to CodeFilter - use CodeFilter{ args... } as a quick way to create a table" );

	Lua::Value v;
	v.load( L, 1 );
	const Lua::Table &t = v.asTable();

	std::string tag, name, desc;
	std::vector<std::string> inputFiles;
	std::vector<std::string> outputFiles;
	std::vector<std::string> cmd;
	std::string exeName;
	ItemPtr exePtr;

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
				exePtr = Lua::extractItem( i.second );
			else
				exeName = i.second.asString();
		}
		else if ( k == "exe_source" || k == "sources" || k == "outputs" ||
				  k == "variables" )
		{
			// we're going to finish loading everything else
			// then loop over again so we have the name
			continue;
		}
		else if ( k == "cmd" )
			cmd = i.second.toStringList();
		else
			throw std::runtime_error( "Unhandled tag '" + k + "' in CodeFilter" );
	}

	if ( name.empty() )
		throw std::runtime_error( "Generator definition requires a name" );

	DEBUG( "luaCodeFilter " << name );
	std::shared_ptr<CodeFilter> ret = std::make_shared<CodeFilter>( name );

	for ( auto &i: t )
	{
		if ( i.first.type == Lua::KeyType::INDEX )
			continue;

		const std::string &k = i.first.tag;
		if ( k == "exe_source" )
		{
			// we're going to finish loading everything else
			// then loop over again so we have the name
			if ( exePtr )
				throw std::runtime_error( "Multiple executable sources specified for code generator" );
			auto e = std::make_shared<InternalExecutable>( exeName.empty() ? name : exeName );
			Lua::recurseAndAdd( e, i.second );
			exePtr = e;
		}
		else if ( k == "sources" )
			Lua::recurseAndAdd( ret, i.second );
		else if ( k == "outputs" )
			ret->setOutputs( i.second.toStringList() );
		else if ( k == "variables" )
		{
			const Lua::Table &vars = i.second.asTable();
			for ( auto &x: vars )
			{
				if ( x.first.type == Lua::KeyType::INDEX )
					continue;
				const std::string &vk = x.first.tag;
				ret->getVariable( vk ).reset( x.second.toStringList() );
			}
		}
	}

	if ( exePtr )
		ret->addDependency( DependencyType::ORDER, exePtr );

	std::shared_ptr<Tool> gt = Tool::createInternalTool( tag, name, desc,
														 exeName, exePtr,
														 cmd );
	ret->setTool( gt );
	Scope::current().addTool( gt );

	Scope::current().addItem( ret );
	Lua::pushItem( L, ret );
	return 1;
}

static int
luaGenerateSource( lua_State *L )
{
	if ( lua_gettop( L ) != 1 || ! lua_istable( L, 1 ) )
		throw std::runtime_error( "Expecting a single (table) argument to code.generate - use code.generate{ args... } as a quick way to create a table" );

	Lua::Value v;
	v.load( L, 1 );
	const Lua::Table &t = v.asTable();

	std::string name;
	std::vector<std::string> inputItems;
	std::vector<std::string> itemPrefix, filePrefix;
	std::vector<std::string> itemSuffix, fileSuffix;
	std::string itemIndent;
	std::string function;
	bool doCommas = false;

	for ( auto &i: t )
	{
		if ( i.first.type == Lua::KeyType::INDEX )
			continue;

		const std::string &k = i.first.tag;
		if ( k == "output" )
			name = i.second.asString();
		else if ( k == "input_items" )
			inputItems = i.second.toStringList();
		else if ( k == "file_prefix" )
			filePrefix = i.second.toStringList();
		else if ( k == "file_suffix" )
			fileSuffix = i.second.toStringList();
		else if ( k == "item_prefix" )
			itemPrefix = i.second.toStringList();
		else if ( k == "item_suffix" )
			itemSuffix = i.second.toStringList();
		else if ( k == "item_indent" )
			itemIndent = i.second.asString();
		else if ( k == "item_transform_func" )
		{
			// eventually, make this a lua function
			function = i.second.asString();
		}
		else if ( k == "comma_separate" )
			doCommas = i.second.asBool();
		else
			throw std::runtime_error( "Unhandled tag '" + k + "' in code.generate" );
	}

	if ( name.empty() )
		throw std::runtime_error( "code.generate definition requires an output" );
	DEBUG( "luaGenerateSource " << name );

	if ( function.empty() )
		throw std::runtime_error( "code.generate requires a transform function spec" );

	if ( function != "binary_cstring" )
		throw std::runtime_error( "code.generate unsupported function '" + function + "'" );

	if ( inputItems.empty() )
		throw std::runtime_error( "code.generate definition requires a list of input items" );

	std::shared_ptr<CodeGenerator> ret = std::make_shared<CodeGenerator>( name );

	for ( const std::string &i: inputItems )
		ret->addItem( i );

	ret->setItemInfo( itemPrefix, itemSuffix, itemIndent, doCommas );
	ret->setFileInfo( filePrefix, fileSuffix );

	Scope::current().addItem( ret );
	Lua::pushItem( L, ret );
	return 1;
}



////////////////////////////////////////


static int
luaCreateFile( lua_State *L )
{
	int N = lua_gettop( L );
	if ( N != 2 || ! lua_isstring( L, 1 ) || ! lua_istable( L, 2 ) )
		throw std::runtime_error( "CreateFile expects 2 arguments: file name, table of strings - one for each line" );

	std::string name = Lua::Parm<std::string>::get( L, N, 1 );
	DEBUG( "luaCreateFile " << name );
	Lua::Value v;
	v.load( L, 2 );
	std::shared_ptr<CreateFile> ret = std::make_shared<CreateFile>( name );
	ret->setLines( v.toStringList() );

	Scope::current().addItem( ret );
	Lua::pushItem( L, ret );
	return 1;
}

}


////////////////////////////////////////


namespace Lua
{

void
registerCodeGenExt( void )
{
	Engine &eng = Engine::singleton();

	eng.pushLibrary( "code" );
	ON_EXIT{ eng.popLibrary(); };

	eng.registerFunction( "filter", &luaCodeFilter );
	eng.registerFunction( "generate", &luaGenerateSource );
	eng.registerFunction( "create", &luaCreateFile );
}


////////////////////////////////////////


} // Lua




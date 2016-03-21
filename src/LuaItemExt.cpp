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

#include "LuaItemExt.h"
#include "LuaEngine.h"

#include "Debug.h"
#include "Item.h"
#include "Scope.h"
#include <stdexcept>


////////////////////////////////////////


namespace
{

static const char *kConsItemLuaMeta = "item";

static int
itemGetName( lua_State *L )
{
	if ( lua_gettop( L ) != 1 )
		throw std::runtime_error( "Item:name() expects only one argument - self" );
	ItemPtr p = Lua::extractItem( L, 1 );
	const std::string &n = p->getName();
	DEBUG( "item:GetName -> " << n );
	lua_pushlstring( L, n.c_str(), n.size() );
	return 1;
}

static int
itemAddDependency( lua_State *L )
{
	if ( lua_gettop( L ) != 3 )
		throw std::runtime_error( "Item:addDependency() expects 3 arguments - self, dependencyType, dependency" );
	ItemPtr p = Lua::extractItem( L, 1 );
	DEBUG( "item:addDependency " << p->getName() );
	size_t len = 0;
	const char *dtp = lua_tolstring( L, 2, &len );
	if ( dtp )
	{
		std::string dt( dtp, len );
		ItemPtr d = Lua::extractItem( L, 3 );
		if ( dt == "explicit" )
			p->addDependency( DependencyType::EXPLICIT, d );
		else if ( dt == "implicit" )
			p->addDependency( DependencyType::IMPLICIT, d );
		else if ( dt == "order" )
			p->addDependency( DependencyType::ORDER, d );
		else if ( dt == "chain" )
			p->addDependency( DependencyType::CHAIN, d );
		else
			throw std::runtime_error( "Invalid dependency type: expect explicit, implicit, order, or chain" );
	}
	else
		throw std::runtime_error( "Unable to extract string in addDependency" );
	return 0;
}

static int
itemHasDependency( lua_State *L )
{
	if ( lua_gettop( L ) != 2 )
		throw std::runtime_error( "Item:addDependency() expects 2 arguments - self and dependency check" );
	ItemPtr p = Lua::extractItem( L, 1 );
	DEBUG( "item:hasDependency " << p->getName() );
	ItemPtr d = Lua::extractItem( L, 2 );
	bool hd = p->hasDependency( d );
	lua_pushboolean( L, hd ? 1 : 0 );
	return 1;
}

static int
itemVariables( lua_State *L )
{
	int N = lua_gettop( L );
	if ( N != 2 )
		throw std::runtime_error( "Item:variables() expects 2 arguments - self, system" );
	ItemPtr p = Lua::extractItem( L, 1 );
	std::string s = Lua::Parm<std::string>::get( L, N, 2 );
	DEBUG( "item:variables " << p->getName() << " for system " << s );
	Lua::Value ret;
	Lua::Table &t = ret.initTable();
	for ( auto &v: p->getVariables() )
		t[v.first].initString( std::move( v.second.value( s ) ) );

	ret.push( L );
	return 1;
}

static int
itemClearVariable( lua_State *L )
{
	if ( lua_gettop( L ) != 2 )
		throw std::runtime_error( "Item:clearVariable() expects 2 arguments - self, variable name" );
	ItemPtr p = Lua::extractItem( L, 1 );
	std::string nm = Lua::Parm<std::string>::get( L, 2, 2 );
	DEBUG( "item:clearVariable " << p->getName() );

	VariableSet &vars = p->getVariables();
	auto x = vars.find( nm );
	if ( x != vars.end() )
		vars.erase( x );
	return 0;
}

static int
itemSetVariable( lua_State *L )
{
	int N = lua_gettop( L );
	if ( N < 3 )
		throw std::runtime_error( "Item:setVariable() expects 3 or 4 arguments - self, variable name, variable value, bool env check (nil)" );
	ItemPtr p = Lua::extractItem( L, 1 );
	std::string nm = Lua::Parm<std::string>::get( L, N, 2 );
	DEBUG( "item:setVariable " << p->getName() << " " << nm );
	Lua::Value v;
	v.load( L, 3 );
	bool envCheck = false;
	if ( N == 4 )
		envCheck = Lua::Parm<bool>::get( L, N, 4 );

	VariableSet &vars = p->getVariables();
	auto x = vars.find( nm );
	if ( v.type() == LUA_TNIL )
	{
		if ( x != vars.end() )
			vars.erase( x );
		return 0;
	}

	if ( x == vars.end() )
		x = vars.emplace( std::make_pair( nm, Variable( nm, envCheck ) ) ).first;

	if ( v.type() == LUA_TTABLE )
		x->second.reset( v.toStringList() );
	else if ( v.type() == LUA_TSTRING )
		x->second.reset( v.asString() );
	else
		throw std::runtime_error( "Item:setVariable() - unhandled variable value type, expect nil, table or string" );

	return 0;
}

static int
itemAddToVariable( lua_State *L )
{
	if ( lua_gettop( L ) != 3 )
		throw std::runtime_error( "Item:addToVariable() expects 3 arguments - self, variable name, variable value to add" );
	ItemPtr p = Lua::extractItem( L, 1 );
	std::string nm = Lua::Parm<std::string>::get( L, 3, 2 );
	DEBUG( "item:addToVariable " << p->getName() << " " << nm );
	Lua::Value v;
	v.load( L, 3 );
	if ( v.type() == LUA_TNIL )
		return 0;

	VariableSet &vars = p->getVariables();
	auto x = vars.find( nm );
	// should this be an error?
	if ( x == vars.end() )
		x = vars.emplace( std::make_pair( nm, Variable( nm ) ) ).first;

	if ( v.type() == LUA_TTABLE )
		x->second.add( v.toStringList() );
	else if ( v.type() == LUA_TSTRING )
		x->second.add( v.asString() );
	else
		throw std::runtime_error( "Item:addToVariable() - unhandled variable value type, expect nil, table or string" );
	return 0;
}

static int
itemInheritVariable( lua_State *L )
{
	if ( lua_gettop( L ) != 3 )
		throw std::runtime_error( "Item:inheritVariable() expects 3 arguments - self, variable name, boolean" );
	ItemPtr p = Lua::extractItem( L, 1 );
	std::string nm = Lua::Parm<std::string>::get( L, 3, 2 );
	DEBUG( "item:inheritVariable " << p->getName() << " " << nm );
	bool v = Lua::Parm<bool>::get( L, 3, 3 );
	VariableSet &vars = p->getVariables();
	auto x = vars.find( nm );
	// should this be an error?
	if ( x == vars.end() )
		x = vars.emplace( std::make_pair( nm, Variable( nm ) ) ).first;

	x->second.inherit( v );
	return 0;
}

static int
itemForceTool( lua_State *L )
{
	int N = lua_gettop( L );
	if ( N == 2 || N == 3 )
	{
		ItemPtr p = Lua::extractItem( L, 1 );
		DEBUG( "item:forceTool " << p->getName() );
		if ( N == 3 )
		{
			p->forceTool( Lua::Parm<std::string>::get( L, 3, 2 ),
						  Lua::Parm<std::string>::get( L, 3, 3 ) );
		}
		else
			p->forceTool( Lua::Parm<std::string>::get( L, 2, 2 ) );
	}
	else
		throw std::runtime_error( "Item:forceTool() expects 2 or 3 arguments - self, [extension,] tool name" );

	return 0;
}

static int
itemOverrideToolSetting( lua_State *L )
{
	if ( lua_gettop( L ) != 3 )
		throw std::runtime_error( "Item:overrideToolSetting() expects 3 arguments - self, tool setting name, setting value" );
	ItemPtr p = Lua::extractItem( L, 1 );
	DEBUG( "item:overrideToolSetting " << p->getName() );
	p->overrideToolSetting( Lua::Parm<std::string>::get( L, 3, 2 ),
							Lua::Parm<std::string>::get( L, 3, 3 ) );

	return 0;
}

static int
itemAddIncludes( lua_State *L )
{
	int N = lua_gettop( L );
	if ( N < 2 )
		throw std::runtime_error( "Item:addIncludes() expects at least 2 arguments - self then string values" );
	ItemPtr p = Lua::extractItem( L, 1 );

	DEBUG( "item:addIncludes " << p->getName() );
	auto &vars = p->getVariables();
	auto incPath = vars.find( "includes" );
	if ( incPath == vars.end() )
		incPath = vars.emplace( std::make_pair( "includes", Variable( "includes" ) ) ).first;

	Variable &v = incPath->second;
	v.inherit( true );
	v.setToolTag( "cc" );

	for ( int i = 2; i <= N; ++i )
	{
		std::string iname = Lua::Parm<std::string>::get( L, N, i );

		if ( File::isAbsolute( iname.c_str() ) )
			v.add( std::move( iname ) );
		else
			v.add( Directory::current()->makefilename( iname ) );
	}

	return 0;
}

static int
itemAddDefines( lua_State *L )
{
	int N = lua_gettop( L );
	if ( N < 2 )
		throw std::runtime_error( "Item:addDefines() expects at least 2 arguments - self then string values" );
	ItemPtr p = Lua::extractItem( L, 1 );

	DEBUG( "item:addDefines " << p->getName() );
	auto &vars = p->getVariables();
	auto defs = vars.find( "defines" );
	if ( defs == vars.end() )
		defs = vars.emplace( std::make_pair( "defines", Variable( "defines" ) ) ).first;

	Variable &v = defs->second;
	v.inherit( true );
	v.setToolTag( "cc" );

	for ( int i = 2; i <= N; ++i )
		v.add( Lua::Parm<std::string>::get( L, N, i ) );

	return 0;
}

static int
itemAddSystemDefines( lua_State *L )
{
	int N = lua_gettop( L );
	if ( N < 3 )
		throw std::runtime_error( "Item:addSystemDefines() expects at least 3 arguments - self then system, then string values" );
	ItemPtr p = Lua::extractItem( L, 1 );

	DEBUG( "item:addSystemDefines " << p->getName() );
	auto &vars = p->getVariables();
	auto defs = vars.find( "defines" );
	if ( defs == vars.end() )
		defs = vars.emplace( std::make_pair( "defines", Variable( "defines" ) ) ).first;

	Variable &v = defs->second;
	v.setToolTag( "cc" );
	v.inherit( true );

	std::vector<std::string> vals = Lua::Parm< std::vector<std::string> >::recursive_get( L, 2, N );
	if ( vals.size() < 2 )
	{
		std::string errmsg = "system_defines expects a string value for the system name, and then defines or sets of defines to add";
		luaL_error( L, errmsg.c_str() );
		return 1;
	}
	std::string name = std::move( vals.front() );
	vals.erase( vals.begin() );

	v.addPerSystem( name, vals );

	return 0;
}

static int
itemAddArtifactInclude( lua_State *L )
{
	int N = lua_gettop( L );
	if ( N < 1 )
		throw std::runtime_error( "Item:includeArtifactDir() expects at least 1 argument - self" );
	ItemPtr p = Lua::extractItem( L, 1 );

	DEBUG( "item:addArtifactInclude " << p->getName() );
	auto &vars = p->getVariables();
	auto incPath = vars.find( "includes" );
	if ( incPath == vars.end() )
		incPath = vars.emplace( std::make_pair( "includes", Variable( "includes" ) ) ).first;

	Variable &v = incPath->second;
	v.inherit( true );
	v.setToolTag( "cc" );
	std::string path = "$builddir";
	path.push_back( File::pathSeparator() );
	path.append( "artifacts" );
	path.push_back( File::pathSeparator() );
	Directory tmp( *(Directory::current()) );
	tmp.cdUp();
	std::string parpath = path;
	parpath.append( tmp.relpath() );
	v.add( parpath );
	path.append( Directory::current()->relpath() );
	v.add( path );

	return 0;
}

static int
itemSetTopLevel( lua_State *L )
{
	int N = lua_gettop( L );
	if ( N != 2 )
		throw std::runtime_error( "Item:setTopLevel() expects 2 arguments - self, boolean" );

	ItemPtr p = Lua::extractItem( L, 1 );
	DEBUG( "item:setTopLevel " << p->getName() );
	bool tl = Lua::Parm<bool>::get( L, N, 2 );
	p->setAsTopLevel( tl );
	if ( tl )
		Scope::current().addItem( p );
	else
		Scope::current().removeItem( p );

	return 0;
}

static int
itemSetDefaultTarget( lua_State *L )
{
	int N = lua_gettop( L );
	if ( N != 2 )
		throw std::runtime_error( "Item:setDefaultTarget() expects 2 arguments - self, boolean" );

	ItemPtr p = Lua::extractItem( L, 1 );
	DEBUG( "item:setDefaultTarget " << p->getName() );
	p->setDefaultTarget( Lua::Parm<bool>::get( L, N, 2 ) );

	return 0;
}

static int
itemSetPseudoTarget( lua_State *L )
{
	int N = lua_gettop( L );
	if ( N < 1 || N > 2 )
		throw std::runtime_error( "Item:setPseudoTarget() expects 1 or 2 arguments - self, [name]" );

	ItemPtr p = Lua::extractItem( L, 1 );
	// only makes sense if this is also a top level item
	DEBUG( "item:setPseudoTarget " << p->getName() );
	p->setAsTopLevel( true );
	Scope::current().addItem( p );
	if ( N == 2 )
		p->setPseudoTarget( Lua::Parm<std::string>::get( L, N, 2 ) );

	return 0;
}

static int
itemSetUseName( lua_State *L )
{
	int N = lua_gettop( L );
	if ( N != 2 )
		throw std::runtime_error( "Item:setUseName() expects 2 arguments - self, bool" );

	ItemPtr p = Lua::extractItem( L, 1 );
	DEBUG( "item:setUseNameAsInput " << p->getName() );
	p->setUseNameAsInput( Lua::Parm<bool>::get( L, N, 2 ) );
	Scope::current().addItem( p );

	return 0;
}


////////////////////////////////////////


static int
itemCreate( lua_State *L )
{
	if ( lua_gettop( L ) != 1 || ! lua_isstring( L, 1 ) )
		throw std::runtime_error( "Item.new expects a single argument - a name" );

	size_t len = 0;
	const char *p = lua_tolstring( L, 1, &len );
	if ( p )
	{
		DEBUG( "item:createItem " << std::string( p, len ) );
		ItemPtr np = std::make_shared<Item>( std::string( p, len ) );
		Lua::pushItem( L, np );
	}
	else
		throw std::runtime_error( "Unable to extract item name" );

	return 1;
}

static const struct luaL_Reg item_m[] =
{
	{ "__tostring", itemGetName },
	{ "name", itemGetName },
	{ "add_dependency", itemAddDependency },
	{ "depends", itemHasDependency },
	{ "variables", itemVariables },
	{ "clear_variable", itemClearVariable },
	{ "set_variable", itemSetVariable },
	{ "add_to_variable", itemAddToVariable },
	{ "inherit_variable", itemInheritVariable },
	{ "force_tool", itemForceTool },
	{ "override_option", itemOverrideToolSetting },
	{ "includes", itemAddIncludes },
	{ "defines", itemAddDefines },
	{ "system_defines", itemAddSystemDefines },
	{ "include_artifact_dir", itemAddArtifactInclude },
	{ "set_top_level", itemSetTopLevel },
	{ "set_default_target", itemSetDefaultTarget },
	{ "set_pseudo_target", itemSetPseudoTarget },
	{ "set_use_name_for_input", itemSetUseName },
	{ nullptr, nullptr }
};

static const struct luaL_Reg item_class_f[] =
{
	{ "new", itemCreate },
	{ nullptr, nullptr }
};

}


////////////////////////////////////////


namespace Lua
{


////////////////////////////////////////


ItemPtr
extractItem( lua_State *L, int i )
{
	void *ud = luaL_checkudata( L, i, kConsItemLuaMeta );
	if ( ! ud )
		throw std::runtime_error( "User data is not a constructor item" );
	return *( reinterpret_cast<ItemPtr *>( ud ) );
}


////////////////////////////////////////


ItemPtr
extractItem( const Lua::Value &v )
{
	if ( v.type() == LUA_TUSERDATA )
	{
		void *ud = v.asPointer();
		if ( ud )
			return *(reinterpret_cast<ItemPtr *>( ud ) );

		throw std::runtime_error( "User data is not a constructor item" );
	}

	throw std::runtime_error( "Argument is not a user data item" );
}


////////////////////////////////////////


void
pushItem( lua_State *L, ItemPtr i )
{
	if ( ! i )
	{
		lua_pushnil( L );
		return;
	}

	void *sptr = lua_newuserdata( L, sizeof(ItemPtr) );
	if ( ! sptr )
		throw std::runtime_error( "Unable to create item userdata" );

	luaL_setmetatable( L, kConsItemLuaMeta );
	new (sptr) ItemPtr( std::move( i ) );
}


////////////////////////////////////////


void
registerItemExt( void )
{
	Engine &eng = Engine::singleton();

	eng.registerClass( kConsItemLuaMeta, item_class_f, item_m );
}


////////////////////////////////////////


} // Lua




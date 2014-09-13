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

#include "Scope.h"
#include <stack>
#include <stdexcept>
#include "LuaEngine.h"
#include "Tool.h"
#include "Debug.h"
#include "ScopeGuard.h"


////////////////////////////////////////


namespace
{
static std::shared_ptr<Scope> theRootScope = std::make_shared<Scope>( std::shared_ptr<Scope>() );
static std::stack< std::shared_ptr<Scope> > theScopes;


////////////////////////////////////////


void
luaAddTool( const Lua::Value &v )
{
	Scope::current().addTool( Tool::parse( v ) );
}

void
luaAddToolSet( const Lua::Value &v )
{
	std::string name;
	std::vector<std::string> tools;
	const Lua::Table &t = v.asTable();
	for ( auto &i: t )
	{
		if ( i.first.type == Lua::KeyType::INDEX )
			continue;

		const std::string &k = i.first.tag;
		if ( k == "name" )
			name = i.second.asString();
		else if ( k == "tools" )
			tools = i.second.toStringList();
		else
			throw std::runtime_error( "Unknown tag '" + k + "' in AddToolSet" );
	}
	if ( name.empty() )
		throw std::runtime_error( "Missing name in AddToolSet" );
	if ( tools.empty() )
		throw std::runtime_error( "Need list of tools in AddToolSet" );

	Scope::current().addToolSet( name, tools );
}

void
luaAddToolOption( const std::string &t, const std::string &g,
				  const std::string &name,
				  const std::vector<std::string> &cmd )
{
	bool found = false;
	for ( auto &tool: Scope::current().tools() )
	{
		if ( tool->name() == t )
		{
			found = true;
			tool->addOption( g, name, cmd );
		}
	}

	if ( ! found )
		throw std::runtime_error( "Unable to find tool '" + t + "' in current scope" );
}

int
luaEnableLangs( lua_State *L )
{
	int N = lua_gettop( L );
	for ( int i = 1; i <= N; ++i )
	{
		if ( lua_isstring( L, i ) )
		{
			size_t len = 0;
			const char *s = lua_tolstring( L, i, &len );
			std::string curLang( s, len );
			for ( auto &t: Scope::current().tools() )
				t->enableLanguage( curLang );
		}
		else
		{
			std::cout << "WARNING: ignoring non-string argument to EnableLanguages" << std::endl;
		}
	}
	return 0;
}

//int
//luaSubProj( lua_State *L )
//{
//	Scope::pushScope( Scope::current().newSubScope( false ) );
//	ON_EXIT{ Scope::popScope(); };
//	return SubDir( L );
//}

} // empty namespace


////////////////////////////////////////


Scope::Scope( std::shared_ptr<Scope> parent )
		: myParent( parent )
{
}


////////////////////////////////////////


Scope::~Scope( void )
{
}


////////////////////////////////////////


std::shared_ptr<Scope>
Scope::newSubScope( bool inherits )
{
	mySubScopes.emplace_back( std::make_shared<Scope>( shared_from_this() ) );
	if ( inherits )
	{
		
	}
	return mySubScopes.back();
}


////////////////////////////////////////


void
Scope::addTool( const std::shared_ptr<Tool> &t )
{
	myTools.push_back( t );
}


////////////////////////////////////////


std::shared_ptr<Tool>
Scope::findTool( const std::string &extension ) const
{
	for ( auto &t: myTools )
		if ( t->handlesExtension( extension ) )
			return t;

	if ( myInheritParentScope )
		return parent()->findTool( extension );
	return std::shared_ptr<Tool>();
}


////////////////////////////////////////


void
Scope::addToolSet( const std::string &name,
				   const std::vector<std::string> &tools )
{
	if ( myToolSets.find( name ) != myToolSets.end() )
		throw std::runtime_error( "ToolSet '" + name + "' already defined" );

	myToolSets[name] = tools;
}


////////////////////////////////////////


void
Scope::useToolSet( const std::string &tset )
{
	myEnabledToolsets.push_back( tset );
}


////////////////////////////////////////


void
Scope::addItem( const ItemPtr &i )
{
	myItems.push_back( i );
}


////////////////////////////////////////


Scope &
Scope::root( void )
{
	return *(theRootScope);
}


////////////////////////////////////////


Scope &
Scope::current( void )
{
	if ( theScopes.empty() )
		return *(theRootScope);
	return *(theScopes.top());
}


////////////////////////////////////////


void
Scope::pushScope( const std::shared_ptr<Scope> &scope )
{
	theScopes.push( scope );
}


////////////////////////////////////////


void
Scope::popScope( void )
{
	if ( theScopes.empty() )
		throw std::runtime_error( "unbalanced Scope management -- too many pops for pushes" );
	theScopes.pop();
}


////////////////////////////////////////


void
Scope::registerFunctions( void )
{
	Lua::Engine &eng = Lua::Engine::singleton();
	eng.registerFunction( "AddTool", &luaAddTool );
	eng.registerFunction( "AddToolOption", &luaAddToolOption );
	eng.registerFunction( "AddToolSet", &luaAddToolSet );
	eng.registerFunction( "EnableLanguages", &luaEnableLangs );
//	eng.registerFunction( "SubProject", &luaSubProj );
}


////////////////////////////////////////




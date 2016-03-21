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

#include "LuaSysExt.h"
#include "LuaEngine.h"
#include "OSUtil.h"
#include "PackageSet.h"
#include "LuaItemExt.h"


////////////////////////////////////////


namespace
{

static bool
luaExternalLibraryExists( const std::string &name, const std::string &reqVersion )
{
	DEBUG( "luaExternalLibraryExists " << name );
	if ( PackageSet::get( OS::system() ).find( name, reqVersion ) )
		return true;
	return false;
}

static int
luaExternalLibrary( lua_State *L )
{
	int N = lua_gettop( L );
	if ( N < 1 || N > 2 )
	{
		throw std::runtime_error( "Unexpected arguments to sys.library, expect name and optional version check" );
	}

	std::string name = Lua::Parm<std::string>::get( L, N, 1 );
	std::string ver = Lua::Parm<std::string>::get( L, N, 2 );

	DEBUG( "luaExternalLibrary " << name );
	Lua::pushItem( L, PackageSet::get( OS::system() ).find( name, ver ) );
	return 1;
}

static int
luaRequiredExternalLibrary( lua_State *L )
{
	int N = lua_gettop( L );
	if ( N < 1 || N > 2 )
	{
		throw std::runtime_error( "Unexpected arguments to sys.required_library, expect name and optional version check" );
	}

	std::string name = Lua::Parm<std::string>::get( L, N, 1 );
	std::string ver = Lua::Parm<std::string>::get( L, N, 2 );

	DEBUG( "luaRequiredExternalLibrary " << name );
	auto p = PackageSet::get( OS::system() ).find( name, ver );
	if ( ! p )
	{
		if ( ! ver.empty() )
			throw std::runtime_error( "ERROR: Package '" + name + "' and required version " + ver + " not found" );
		else
			throw std::runtime_error( "ERROR: Package '" + name + "' not found" );
	}

	Lua::pushItem( L, p );
	return 1;
}

} // empty namespace


////////////////////////////////////////


namespace Lua
{


////////////////////////////////////////


void
registerSysExt( void )
{
	Engine &eng = Engine::singleton();

	eng.pushLibrary( "sys" );
	ON_EXIT{ eng.popLibrary(); };

	eng.registerFunction( "is64bit", &OS::is64bit );
	eng.registerFunction( "machine", &OS::machine );
	eng.registerFunction( "release", &OS::release );
	eng.registerFunction( "version", &OS::version );
	eng.registerFunction( "system", &OS::system );
	eng.registerFunction( "node", &OS::node );

	eng.registerFunction( "library_exists", &luaExternalLibraryExists );
	eng.registerFunction( "library", &luaExternalLibrary );
	eng.registerFunction( "required_library", &luaRequiredExternalLibrary );
}


////////////////////////////////////////


} // src




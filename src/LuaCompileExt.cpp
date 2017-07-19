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

#include "LuaCompileExt.h"

#include "Debug.h"

#include "LuaEngine.h"
#include "LuaValue.h"
#include "ScopeGuard.h"
#include "LuaItemExt.h"
#include "Scope.h"
#include "Library.h"
#include "Executable.h"
#include "Configuration.h"
#include "OptionalSource.h"
#include "ExternLibrary.h"
#include "Util.h"


////////////////////////////////////////


namespace
{

static std::shared_ptr<Library> theCurLib;
static std::map<std::string, ItemPtr> theDefinedLibs;
static std::shared_ptr<Executable> theCurExe;
static std::map<std::string, ItemPtr> theDefinedExes;


////////////////////////////////////////


static int
luaCompile( lua_State *L )
{
	Configuration::checkDefault();

	int N = lua_gettop( L );
	if ( N == 0 )
	{
		lua_pushnil( L );
		return 1;
	}

	DEBUG( "luaCompile" );
	std::shared_ptr<CompileSet> ret = std::make_shared<CompileSet>();
	for ( int i = 1; i <= N; ++i )
		Lua::recurseAndAdd( ret, L, i );

	if ( theCurExe )
		theCurExe->addItem( ret );
	else if ( theCurLib )
		theCurLib->addItem( ret );
	else
		Scope::current().addItem( ret );

	Lua::pushItem( L, ret );
	return 1;
}

static std::vector<std::shared_ptr<OptionalSource>>
extractOptionalCompile( const Lua::Value &v )
{
	std::vector<std::shared_ptr<OptionalSource>> opts;

	const Lua::Value::Table &t = v.asTable();
	if ( ! t.empty() )
	{
		auto i = t.begin();
		if ( i->first.type == Lua::KeyType::STRING )
		{
			auto src = t.find( "source" );
			if ( src == t.end() )
				throw std::runtime_error( "missing source argument to optional_source{ source={...} [, system=\"Foo\", defines={...}, has_lib=\"Bar\", libs={...} ] }" );
			auto sys = t.find( "system" );
			auto libs = t.find( "libs" );
			if ( sys == t.end() && libs == t.end() )
				throw std::runtime_error( "missing conditional argument(s) to optional_source{ source={...} [, system=\"Foo\", libs={...} ] [, defines={...} ] }" );

			auto s = std::make_shared<OptionalSource>();

			const Lua::Value::Table &srcT = src->second.asTable();
			for ( auto cSrc: srcT )
			{
				if ( cSrc.first.type != Lua::KeyType::INDEX )
					throw std::runtime_error( "Bad source table to optional_source, expect array of source" );
				if ( cSrc.second.type() == LUA_TSTRING )
				{
					s->addItem( cSrc.second.asString() );
				}
				else if ( cSrc.second.type() == LUA_TUSERDATA )
				{
					s->addItem( Lua::extractItem( cSrc.second ) );
				}
				else
					throw std::runtime_error( "Bad item in source table passed to optional_source, expect array of strings or items" );
			}

			if ( sys != t.end() )
				s->addCondition( "system", sys->second.asString() );
			if ( libs != t.end() )
			{
				if ( libs->second.type() != LUA_TTABLE )
					throw std::runtime_error( "libs argument to optional_source should be a table" );

				const Lua::Value::Table &elibs = libs->second.asTable();
				for ( auto clib: elibs )
				{
					if ( clib.first.type != Lua::KeyType::INDEX )
						throw std::runtime_error( "Bad libs table to optional_source, expect array of libs" );
					if ( clib.second.type() == LUA_TSTRING )
					{
						s->addExternRef( clib.second.asString(), std::string() );
					}
					else if ( clib.second.type() == LUA_TTABLE )
					{
						const Lua::Value::Table &st = clib.second.asTable();
						auto lname = st.find( lua_Integer(1) );
						auto vertag = st.find( lua_Integer(2) );
						if ( lname == st.end() || vertag == st.end() )
							throw std::runtime_error( "Invalid array of {name,version} to system_libs" );

						s->addExternRef( lname->second.asString(),
										 vertag->second.asString() );
					}
					else
						throw std::runtime_error( "Bad item in libs table passed optional_source, expect array of strings or {name,version} table pairs" );
				}
			}
			
			auto defs = t.find( "defines" );
			if ( defs != t.end() )
			{
				auto dlist = defs->second.toStringList();
				for ( auto &d: dlist )
					s->addDefine( std::move( d ) );
			}

			opts.emplace_back( std::move( s ) );
		}
		else
		{
			for ( ; i != t.end(); ++i )
			{
				if ( i->second.type() == LUA_TTABLE )
				{
					util::append( opts, extractOptionalCompile( i->second ) );
				}
				else
					throw std::runtime_error( "Bad item in array passed to optional_source, expect array of optional_source tables {system=, source={}}" );
			}
		}
	}

	return opts;
}

static int
luaOptCompile( lua_State *L )
{
	Configuration::checkDefault();

	int N = lua_gettop( L );
	if ( N == 0 )
	{
		lua_pushnil( L );
		return 1;
	}

	DEBUG( "luaOptCompile" );
	std::vector<std::shared_ptr<OptionalSource>> opts;
	if ( lua_istable( L, 1 ) && N == 1 )
	{
		Lua::Value tmp( L, 1 );
		opts = extractOptionalCompile( tmp );
	}
	else if ( lua_isstring( L, 1 ) && N >= 2 )
	{
		auto s = std::make_shared<OptionalSource>();
		s->addCondition( "system", Lua::Parm<std::string>::get( L, N, 1 ) );

		for ( int i = 2; i <= N; ++i )
		{
			if ( lua_isstring( L, i ) )
			{
				s->addItem( Lua::Parm<std::string>::get( L, N, i ) );
			}
			else if ( lua_isuserdata( L, i ) )
			{
				s->addItem( Lua::extractItem( L, i ) );
			}
			else
			{
				throw std::runtime_error( "optional_source a string or item as rest of parameter list when passed a string first" );
			}
		}
		opts.emplace_back( std::move( s ) );
	}
	else
	{
		throw std::runtime_error( "optional_source supports two call syntax optional_source{ system=\"Foo\", source={} } (or a table of these tables), and optional_source( \"Foo\", \"bar\", ... )" );
	}

	if ( theCurExe )
	{
		for ( auto &i: opts )
			theCurExe->addItem( i );
	}
	else if ( theCurLib )
	{
		for ( auto &i: opts )
			theCurLib->addItem( i );
	}

	if ( ! opts.empty() )
	{
		Scope::current().addItem( opts.front() );
		Lua::pushItem( L, opts.front() );
	}
	else
		lua_pushnil( L );

	return 1;
}


////////////////////////////////////////


static int
luaExecutable( lua_State *L )
{
	Configuration::checkDefault();
	Lua::clearCompileContext();

	int N = lua_gettop( L );
	if ( N < 1 )
		throw std::runtime_error( "Command 'executable' expects a name as the first argument with optional link objects" );
	std::string ename = Lua::Parm<std::string>::get( L, N, 1 );

	DEBUG( "luaExecutable " << ename );
	theCurExe = std::make_shared<Executable>( std::move( ename ) );
	for ( int i = 2; i <= N; ++i )
		Lua::recurseAndAdd( theCurExe, L, i );

	// keep separate lists from the exe - should
	// be able to have a binary and a library of
	// the same name
	if ( theDefinedExes.find( theCurExe->getName() ) != theDefinedExes.end() )
		throw std::logic_error( "Multiple executables by the name '" + ename + "' defined" );
	theDefinedExes[theCurExe->getName()] = theCurExe;

	Scope::current().addItem( theCurExe );
	Lua::pushItem( L, theCurExe );
	return 1;
}


static int
luaLibrary( lua_State *L )
{
	Configuration::checkDefault();
	Lua::clearCompileContext();

	int N = lua_gettop( L );
	if ( N < 1 )
		throw std::runtime_error( "Command 'library' expects a name as the first argument with optional link objects" );
	std::string ename = Lua::Parm<std::string>::get( L, N, 1 );

	DEBUG( "luaLibrary " << ename );
	theCurLib = std::make_shared<Library>( std::move( ename ) );
	for ( int i = 2; i <= N; ++i )
		Lua::recurseAndAdd( theCurLib, L, i );

	// keep separate lists from the exe - should
	// be able to have a binary and a library of
	// the same name
	if ( theDefinedLibs.find( theCurLib->getName() ) != theDefinedLibs.end() )
		throw std::logic_error( "Multiple libraries by the name '" + ename + "' defined" );

	theDefinedLibs[theCurLib->getName()] = theCurLib;
	Scope::current().addItem( theCurLib );
	Lua::pushItem( L, theCurLib );

	return 1;
}

static int
luaSetKind( lua_State *L )
{
	int N = lua_gettop( L );
	DEBUG( "luaSetKind" );
	if ( N != 1 )
		throw std::logic_error( "Expect only 1 argument to kind - the appropriate kind for the object being built" );

	std::string k = Lua::Parm<std::string>::get( L, N, 1 );
	if ( theCurExe )
		theCurExe->setKind( k );
	else if ( theCurLib )
		theCurLib->setKind( k );
	else
		throw std::runtime_error( "No current library or executable for setting build kind" );

	return 0;
}

template <typename Set>
inline void
recurseAndAddLibs( const std::shared_ptr<Set> &item,
				   lua_State *L, int i )
{
	if ( lua_isnil( L, i ) )
		return;
	if ( lua_isstring( L, i ) )
	{
		size_t len = 0;
		const char *n = lua_tolstring( L, i, &len );
		std::string lname( n, len );
		auto l = theDefinedLibs.find( lname );
		if ( l == theDefinedLibs.end() )
			throw std::runtime_error( "Unable to find library by name '" + lname + "', make sure it is declared first" );
		item->addItem( l->second );
	}
	else if ( lua_isuserdata( L, i ) )
	{
		ItemPtr x = Lua::extractItem( L, i );
		bool found = false;
		for ( auto &z: theDefinedLibs )
		{
			if ( z.second == x )
			{
				found = true;
				item->addItem( x );
				break;
			}
		}
		if ( ! found )
		{
			WARNING( "Item '" << x->getName() << "' not a defined library" );
			item->addItem( x );
		}
	}
	else if ( lua_istable( L, i ) )
	{
		lua_pushnil( L );
		while ( lua_next( L, i ) )
		{
			recurseAndAddLibs( item, L, lua_gettop( L ) );
			lua_pop( L, 1 );
		}
	}
	else
		throw std::runtime_error( "Unhandled argument type to libs" );
}

static int
luaUseLibraries( lua_State *L )
{
	int N = lua_gettop( L );

	DEBUG( "luaUseLibraries" );
	if ( theCurExe )
	{
		for ( int i = 1; i <= N; ++i )
			recurseAndAddLibs( theCurExe, L, i );
	}
	else if ( theCurLib )
	{
		for ( int i = 1; i <= N; ++i )
			recurseAndAddLibs( theCurLib, L, i );
	}
	else
		throw std::runtime_error( "No current library or executable for libs request" );

	return 0;
}

static std::vector<std::shared_ptr<ExternLibrarySet>>
extractSysLibs( const Lua::Value &v )
{
	std::vector<std::shared_ptr<ExternLibrarySet>> sLibs;

	const Lua::Value::Table &t = v.asTable();
	if ( ! t.empty() )
	{
		auto i = t.begin();
		if ( i->first.type == Lua::KeyType::STRING )
		{
			auto sys = t.find( "system" );
			auto libs = t.find( "libs" );
			auto defs = t.find( "defines" );
			if ( sys == t.end() )
				throw std::runtime_error( "missing system argument to system_libs{ system=\"Foo\", libs={...} [, defines={...}] }" );
			if ( libs == t.end() )
				throw std::runtime_error( "missing libs argument to system_libs{ system=\"Foo\", libs={...} }" );

			auto sLib = std::make_shared<ExternLibrarySet>();
			sLib->addCondition( "system", sys->second.asString() );
			sLib->setRequired( true );
			if ( libs->second.type() != LUA_TTABLE )
				throw std::runtime_error( "libs argument to system_libs should be a table" );

			if ( defs != t.end() )
			{
				auto dlist = defs->second.toStringList();
				for ( auto &d: dlist )
					sLib->addDefine( std::move( d ) );
			}

			const Lua::Value::Table &elibs = libs->second.asTable();
			for ( auto clib: elibs )
			{
				if ( clib.first.type != Lua::KeyType::INDEX )
					throw std::runtime_error( "Bad libs table to system_libs, expect array of libs" );
				if ( clib.second.type() == LUA_TSTRING )
				{
					sLib->addExternRef( clib.second.asString(), std::string() );
				}
				else if ( clib.second.type() == LUA_TTABLE )
				{
					const Lua::Value::Table &st = clib.second.asTable();
					auto lname = st.find( lua_Integer(1) );
					auto vertag = st.find( lua_Integer(2) );
					if ( lname == st.end() || vertag == st.end() )
						throw std::runtime_error( "Invalid array of {name,version} to system_libs" );

					sLib->addExternRef( lname->second.asString(),
										vertag->second.asString() );
				}
				else
					throw std::runtime_error( "Bad item in libs table passed system_libs, expect array of strings or {name,version} table pairs" );
			}
			sLibs.emplace_back( std::move( sLib ) );
		}
		else
		{
			for ( ; i != t.end(); ++i )
			{
				if ( i->second.type() == LUA_TTABLE )
				{
					util::append( sLibs, extractSysLibs( i->second ) );
				}
				else
					throw std::runtime_error( "Bad item in array passed to system_libs, expect array of system_libs tables {system=, libs={}}" );
			}
		}
	}

	return sLibs;
}

static int
luaUseSystemLibs( lua_State *L )
{
	int N = lua_gettop( L );
	if ( N < 1 )
		throw std::runtime_error( "Missing argument to system_libs" );

	DEBUG( "luaUseSystemLibs" );

	std::vector<std::shared_ptr<ExternLibrarySet>> sLibs;

	if ( lua_istable( L, 1 ) && N == 1 )
	{
		Lua::Value tmp( L, 1 );
		sLibs = extractSysLibs( tmp );
	}
	else if ( lua_isstring( L, 1 ) && N >= 2 )
	{
		auto sLib = std::make_shared<ExternLibrarySet>();
		sLib->addCondition( "system", Lua::Parm<std::string>::get( L, N, 1 ) );
		sLib->setRequired( true );

		for ( int i = 2; i <= N; ++i )
		{
			if ( lua_istable( L, i ) )
			{
				Lua::Value tmp( L, i );
				const Lua::Value::Table &st = tmp.asTable();
				auto lname = st.find( lua_Integer(1) );
				auto vertag = st.find( lua_Integer(2) );
				if ( lname == st.end() || vertag == st.end() )
					throw std::runtime_error( "Invalid array of {name,version} to system_libs" );

				sLib->addExternRef( lname->second.asString(),
									vertag->second.asString() );
			}
			else if ( lua_isstring( L, i ) )
			{
				sLib->addExternRef( Lua::Parm<std::string>::get( L, N, i ), std::string() );
			}
			else
			{
				throw std::runtime_error( "system_libs expects either a string, or a table of 2 strings for library reference" );
			}
		}
		sLibs.emplace_back( std::move( sLib ) );
	}
	else
	{
		throw std::runtime_error( "system_libs supports two call syntax system_libs{ system=\"Foo\", libs={} } (or a table of tables), and system_libs( \"Foo\", \"bar\", ... {\"baz\", \">3.2\"}, ... )" );
	}

	if ( theCurExe )
	{
		for ( auto &i: sLibs )
			theCurExe->addItem( i );
	}
	else if ( theCurLib )
	{
		for ( auto &i: sLibs )
			theCurLib->addItem( i );
	}
	else
	{
		
	}
	if ( ! sLibs.empty() )
	{
		Scope::current().addItem( sLibs.front() );
		Lua::pushItem( L, sLibs.front() );
	}
	else
		lua_pushnil( L );
	return 1;
}

static int
luaAddExternalLib( lua_State *L )
{
	DEBUG( "luaAddExternalLib" );

	int N = lua_gettop( L );
	if ( N < 1 )
		throw std::runtime_error( "Missing argument to external_lib" );
	if ( ! lua_istable( L, 1 ) )
		throw std::runtime_error( "Expecting table argument to external_lib" );

	std::shared_ptr<ExternLibrarySet> ret = std::make_shared<ExternLibrarySet>();

	Lua::Value e( L, 1 );
	const Lua::Table &t = e.asTable();
	for ( auto &i: t )
	{
		if ( i.first.type == Lua::KeyType::INDEX )
			continue;

		const std::string &k = i.first.tag;
		if ( k == "lib" || k == "extra_libs" )
		{
			if ( i.second.type() == LUA_TTABLE )
			{
				const Lua::Table &libT = i.second.asTable();
				for ( auto &l: libT )
				{
					if ( l.second.type() == LUA_TSTRING )
						ret->addExternRef( l.second.asString(), std::string() );
					else if ( l.second.type() == LUA_TTABLE )
					{
						const Lua::Table &defT = l.second.asTable();
						auto lname = defT.find( lua_Integer(1) );
						auto vertag = defT.find( lua_Integer(2) );
						if ( lname == defT.end() || vertag == defT.end() )
							throw std::runtime_error( "Invalid array of {name,version} to external_lib" );
						ret->addExternRef( lname->second.asString(),
										   vertag->second.asString() );
					}
					else
						throw std::runtime_error( "Invalid argument to external_lib " + k + " field" );
				}
			}
			else if ( i.second.type() == LUA_TSTRING )
			{
				ret->addExternRef( i.second.asString(), std::string() );
			}
			else
				throw std::runtime_error( "Invalid type passed to lib argument for external_lib" );
		}
		else if ( k == "required" )
		{
			ret->setRequired( i.second.asBool() );
		}
		else if ( k == "defines" )
		{
			auto dlist = i.second.toStringList();
			for ( auto &d: dlist )
				ret->addDefine( std::move( d ) );
		}
		else if ( k == "source" )
		{
			Lua::recurseAndAdd( ret, i.second );
		}
		else
			throw std::runtime_error( "Unknown field " + k + " in external_lib" );
	}

	if ( theCurExe )
		theCurExe->addItem( ret );
	else if ( theCurLib )
		theCurLib->addItem( ret );
	else
		Scope::current().addItem( ret );

	Lua::pushItem( L, ret );
	return 1;
}


////////////////////////////////////////


static int
luaDefaultLibraryKind( lua_State *L )
{
	static const char *errMsg = "default_library_kind expects a tag as the first argument (\"static\", \"shared\", \"both\")";
	int N = lua_gettop( L );
	if ( N != 1 )
		throw std::runtime_error( errMsg );
	std::string kname = Lua::Parm<std::string>::get( L, N, 1 );
	// todo: add framework here???
	if ( kname == "static" || kname == "shared" || kname == "both" )
	{
		auto &opts = Scope::current().getOptions();
		auto ne = opts.emplace( std::make_pair( "default_library_kind", Variable( "default_library_kind" ) ) );
		ne.first->second.reset( std::move( kname ) );
	}
	else
		throw std::runtime_error( errMsg );
	return 0;
}

static int
luaDefaultExeKind( lua_State *L )
{
	static const char *errMsg = "default_executable_kind expects a tag as the first argument (\"cmd\", \"app\")";
	int N = lua_gettop( L );
	if ( N != 1 )
		throw std::runtime_error( errMsg );
	std::string kname = Lua::Parm<std::string>::get( L, N, 1 );

	if ( kname == "cmd" || kname == "app" )
	{
		auto &opts = Scope::current().getOptions();
		auto ne = opts.emplace( std::make_pair( "default_executable_kind", Variable( "default_executable_kind" ) ) );
		ne.first->second.reset( std::move( kname ) );
	}
	else
		throw std::runtime_error( errMsg );
	return 0;
}

}


////////////////////////////////////////


namespace Lua
{


////////////////////////////////////////


void clearCompileContext( void )
{
	if ( theCurLib )
	{
		if ( theCurLib->empty() )
		{
			WARNING( "Removing empty (no source added) library target " << theCurLib->getName() );
			Scope::current().removeItem( theCurLib );
		}
		theCurLib.reset();
	}
	if ( theCurExe )
	{
		if ( theCurExe->empty() )
		{
			WARNING( "Removing empty (no source added) executable target " << theCurExe->getName() );
			Scope::current().removeItem( theCurExe );
		}
		theCurExe.reset();
	}
}


////////////////////////////////////////


void registerCompileExt( void )
{
	Engine &eng = Engine::singleton();

	eng.registerFunction( "source", &luaCompile );
	eng.registerFunction( "optional_source", &luaOptCompile );
	eng.registerFunction( "executable", &luaExecutable );
	eng.registerFunction( "library", &luaLibrary );
	eng.registerFunction( "kind", &luaSetKind );
	eng.registerFunction( "libs", &luaUseLibraries );
	eng.registerFunction( "system_libs", &luaUseSystemLibs );
	eng.registerFunction( "external_lib", &luaAddExternalLib );

	eng.registerFunction( "default_library_kind", &luaDefaultLibraryKind );
	eng.registerFunction( "default_executable_kind", &luaDefaultExeKind );
}


////////////////////////////////////////


} // src




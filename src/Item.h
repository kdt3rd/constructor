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

#pragma once

#include <cstdint>
#include <vector>
#include <map>
#include <string>
#include <memory>
#include "LuaEngine.h"
#include "Variable.h"
#include "Dependency.h"

class Item;

/// @brief base class for everything else in the system
///
/// In a build system, there are either targets, sources,
/// or transformers. All of these things will be items
/// such that we can track dependencies.
class Item : public Dependency<Item>
{
public:
	typedef uint64_t ID;

	Item( const std::string &name );
	Item( std::string &&name );
	virtual ~Item( void );

	inline ID id( void ) const;
	virtual const std::string &name( void ) const;

//	virtual void transform( std::vector<BuildItem> &b,
//	const VariableSet &config );


	inline VariableSet &variables( void );
	inline const VariableSet &variables( void ) const;
	Variable &variable( const std::string &nm );
	void setVariable( const std::string &nm, const std::string &value,
					  bool doSplit = false );

	static ItemPtr extract( lua_State *l, int i );
	static void push( lua_State *l, ItemPtr i );
	static void registerFunctions( void );

protected:
	VariableSet myVariables;

private:
	ID myID;
	std::string myName;
};

using ItemPtr = Item::ItemPtr;


////////////////////////////////////////


inline Item::ID
Item::id( void ) const
{
	return myID;
}


////////////////////////////////////////


inline VariableSet &
Item::variables( void )
{
	return myVariables;
}
inline const VariableSet &
Item::variables( void ) const
{
	return myVariables;
}


////////////////////////////////////////



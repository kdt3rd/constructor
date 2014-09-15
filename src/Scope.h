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

#include <memory>
#include <string>
#include <vector>

#include "Variable.h"
#include "Item.h"
#include "Tool.h"
#include "BuildItem.h"


////////////////////////////////////////


///
/// @brief Class Scope provides an abstraction around a collection of items.
///
/// This is probably most commonly at least a directory (and
/// sub-directories). It can have it's own unique config / toolsets /
/// whatever as well as "global" variables.  Variables can be
/// inherited from a parent scope or not.
///
class Scope : public std::enable_shared_from_this<Scope>
{
public:
	Scope( std::shared_ptr<Scope> parent );
	~Scope( void );

	inline std::shared_ptr<Scope> parent( void ) const;
	std::shared_ptr<Scope> newSubScope( bool inherits );
	inline const std::vector< std::shared_ptr<Scope> > &subScopes( void ) const;

	inline VariableSet &vars( void );
	inline const VariableSet &vars( void ) const;

	inline std::vector< std::shared_ptr<Tool> > &tools( void );
	inline const std::vector< std::shared_ptr<Tool> > &tools( void ) const;

	void addTool( const std::shared_ptr<Tool> &t );
	std::shared_ptr<Tool> findTool( const std::string &extension ) const;

	void addToolSet( const std::string &name,
					 const std::vector<std::string> &tools );
	void useToolSet( const std::string &tset );

	void addItem( const ItemPtr &i );

	void transform( std::vector< std::shared_ptr<BuildItem> > &items,
					const std::shared_ptr<Directory> &outdir,
					const Configuration &conf ) const;

	static Scope &root( void );
	static Scope &current( void );
	static void pushScope( const std::shared_ptr<Scope> &scope );
	static void popScope( void );

private:
	void grabTools( Scope &o );

	std::weak_ptr<Scope> myParent;
	VariableSet myVariables;
	bool myInheritParentScope = false;
	std::vector< std::shared_ptr<Scope> > mySubScopes;

	std::vector<ItemPtr> myItems;

	std::map<std::string, std::vector<std::string> > myToolSets;

	std::map<std::string, std::vector< std::shared_ptr<Tool> > > myTagMap;
	std::vector< std::shared_ptr<Tool> > myTools;
	std::vector<std::string> myEnabledToolsets;
	std::map<std::string, std::vector<std::shared_ptr<Tool> > > myExtensionMap;
};


////////////////////////////////////////


inline const std::vector< std::shared_ptr<Scope> > &Scope::subScopes( void ) const
{
	return mySubScopes;
}


////////////////////////////////////////


//inline void Scope::inherit( bool yesno ) { myInheritParentScope = yesno; }
//inline bool Scope::inherit( void ) const { return myInheritParentScope; }


////////////////////////////////////////


inline std::shared_ptr<Scope> Scope::parent( void ) const { return myParent.lock(); }


////////////////////////////////////////


inline VariableSet &Scope::vars( void ) { return myVariables; }
inline const VariableSet &Scope::vars( void ) const { return myVariables; }


////////////////////////////////////////


inline std::vector< std::shared_ptr<Tool> > &
Scope::tools( void )
{
	return myTools;
}
inline const std::vector< std::shared_ptr<Tool> > &
Scope::tools( void ) const
{
	return myTools;
}



////////////////////////////////////////



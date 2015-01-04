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
#include <set>
#include "Variable.h"
#include "Dependency.h"
#include "Directory.h"

class Item;
class TransformSet;
class BuildItem;
class Tool;

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

	inline ID getID( void ) const;
	virtual const std::string &getName( void ) const;

	inline const Directory &dir( void ) const;
	inline const std::shared_ptr<Directory> &getDir( void ) const;

	virtual std::shared_ptr<BuildItem> transform( TransformSet &xform ) const;
	virtual void copyDependenciesToBuild( TransformSet &xform ) const;

	virtual void forceTool( const std::string &t );
	virtual void forceTool( const std::string &ext, const std::string &t );

	virtual void overrideToolSetting( const std::string &s, const std::string &n );

	inline VariableSet &getVariables( void );
	inline const VariableSet &getVariables( void ) const;
	Variable &getVariable( const std::string &nm );
	const Variable &getVariable( const std::string &nm ) const;
	void setVariable( const std::string &nm, const std::string &value,
					  bool doSplit = false );
	bool findVariableValueRecursive( std::string &val, const std::string &nm ) const;
	void extractVariables( VariableSet &vs ) const;
	void extractVariablesExcept( VariableSet &vs, const std::string &v ) const;
	void extractVariablesExcept( VariableSet &vs, const std::set<std::string> &vl ) const;

	void setPseudoTarget( const std::string &nm );

	inline void setAsTopLevel( bool b );
	inline bool isTopLevel( void ) const;
	inline void setUseNameAsInput( bool b );
	inline bool isUseNameAsInput( void ) const;

	// usually only matters if this is a top-level target, but
	// defines whether it is built by default or has to be specified
	inline void setDefaultTarget( bool b );
	inline bool isDefaultTarget( void ) const;

protected:
	virtual std::shared_ptr<Tool> getTool( TransformSet &xform ) const;
	virtual std::shared_ptr<Tool> getTool( TransformSet &xform, const std::string &ext ) const;
	bool hasToolOverride( const std::string &opt, std::string &val ) const;

	VariableSet myVariables;

private:
	ID myID;
	std::string myName;
	std::string myPseudoName;
	std::shared_ptr<Directory> myDirectory;

	std::string myForceToolAll;
	std::map<std::string, std::string> myForceToolExt;
	std::map<std::string, std::string> myOverrideToolOptions;
	bool myIsTopLevel = false;
	bool myUseName = true;
	bool myDefaultTarget = true;
};

using ItemPtr = Item::ItemPtr;


////////////////////////////////////////


inline Item::ID
Item::getID( void ) const
{
	return myID;
}


////////////////////////////////////////


inline VariableSet &
Item::getVariables( void )
{
	return myVariables;
}
inline const VariableSet &
Item::getVariables( void ) const
{
	return myVariables;
}


////////////////////////////////////////


inline const Directory &
Item::dir( void ) const
{
	return *myDirectory;
}
inline
const std::shared_ptr<Directory> &
Item::getDir( void ) const
{
	return myDirectory;
}


////////////////////////////////////////


inline void Item::setAsTopLevel( bool b ) { myIsTopLevel = b; }
inline bool Item::isTopLevel( void ) const { return myIsTopLevel; }


////////////////////////////////////////


inline void Item::setUseNameAsInput( bool b ) { myUseName = b; }
inline bool Item::isUseNameAsInput( void ) const { return myUseName; }


////////////////////////////////////////


inline void Item::setDefaultTarget( bool b ) { myDefaultTarget = b; }
inline bool Item::isDefaultTarget( void ) const { return myDefaultTarget; }


////////////////////////////////////////



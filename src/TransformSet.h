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
#include <map>
#include <set>
#include <vector>

#include "Pool.h"
#include "Tool.h"
#include "Variable.h"
#include "Directory.h"
#include "BuildItem.h"


////////////////////////////////////////

class Item;

class TransformSet
{
public:
	typedef std::vector< std::shared_ptr<BuildItem> > BuildItemList;

	TransformSet( const std::shared_ptr<Directory> &dir, std::string activeSystem );
	~TransformSet( void );

	inline const std::string &getSystem( void ) const;

	inline const std::shared_ptr<Directory> &getOutDir( void ) const;
	inline const std::shared_ptr<Directory> &getBinDir( void ) const;
	inline const std::shared_ptr<Directory> &getLibDir( void ) const;
	inline const std::shared_ptr<Directory> &getArtifactDir( void ) const;

	void addChildScope( const std::shared_ptr<TransformSet> &cs );
	inline const std::vector< std::shared_ptr<TransformSet> > &getSubScopes( void ) const;

	void addPool( const std::shared_ptr<Pool> &p );
	bool hasPool( const std::string &name ) const;
	inline const std::vector< std::shared_ptr<Pool> > &getPools( void ) const;

	void addTool( const std::shared_ptr<Tool> &t );
	std::shared_ptr<Tool> getTool( const std::string &tag ) const;
	void mergeVariables( const VariableSet &vs );
	void mergeOptions( const VariableSet &vs );

	inline void setLibSearchPath( const std::vector<std::string> &lsearch ) { myLibPath = lsearch; }
	const std::vector<std::string> &getLibSearchPath( void ) const { return myLibPath; }
	void setPkgSearchPath( const std::vector<std::string> &psearch ) { myPkgPath = psearch; }
	const std::vector<std::string> &getPkgSearchPath( void ) const { return myPkgPath; }

	std::shared_ptr<Tool> findTool( const std::string &ext ) const;
	std::shared_ptr<Tool> findToolByTag( const std::string &tag,
										 const std::string &ext = std::string() ) const;
	std::shared_ptr<Tool> findToolForSet( const std::string &tag_prefix,
										  const std::set<std::string> &s ) const;

	inline const VariableSet &getVars( void ) const;
	const std::string &getVarValue( const std::string &v ) const;

	inline const VariableSet &getOptions( void ) const;
	const std::string &getOptionValue( const std::string &v ) const;

	bool isTransformed( const Item *i ) const;
	std::shared_ptr<BuildItem> getTransform( const Item *i ) const;
	void recordTransform( const Item *i,
						  const std::shared_ptr<BuildItem> &bi );
	void add( const std::shared_ptr<BuildItem> &bi );
	void add( const BuildItemList &items );

	inline const BuildItemList &getBuildItems( void ) const;

private:
	std::string myCurrentSystem;

	std::shared_ptr<Directory> myDirectory;
	std::shared_ptr<Directory> myBinDirectory;
	std::shared_ptr<Directory> myLibDirectory;
	std::shared_ptr<Directory> myArtifactDirectory;
	std::vector< std::shared_ptr<Tool> > myTools;
	std::vector< std::shared_ptr<Pool> > myPools;

	std::vector<std::string> myLibPath;
	std::vector<std::string> myPkgPath;

	VariableSet myVars;
	VariableSet myOptions;

	std::vector< std::shared_ptr<BuildItem> > myBuildItems;
	std::map< const Item *, std::shared_ptr<BuildItem> > myTransformMap;

	std::vector< std::shared_ptr<TransformSet> > myChildScopes;
};


////////////////////////////////////////


inline const std::string &
TransformSet::getSystem( void ) const
{
	return myCurrentSystem;
}

inline const std::shared_ptr<Directory> &
TransformSet::getOutDir( void ) const
{
	return myDirectory;
}

inline const std::shared_ptr<Directory> &
TransformSet::getBinDir( void ) const
{
	return myBinDirectory;
}

inline const std::shared_ptr<Directory> &
TransformSet::getLibDir( void ) const
{
	return myLibDirectory;
}

inline const std::shared_ptr<Directory> &
TransformSet::getArtifactDir( void ) const
{
	return myArtifactDirectory;
}


////////////////////////////////////////


inline const VariableSet &
TransformSet::getVars( void ) const
{
	return myVars;
}
inline const VariableSet &
TransformSet::getOptions( void ) const
{
	return myOptions;
}


////////////////////////////////////////


inline const TransformSet::BuildItemList &
TransformSet::getBuildItems( void ) const
{
	return myBuildItems;
}


////////////////////////////////////////


inline const std::vector< std::shared_ptr<TransformSet> > &
TransformSet::getSubScopes( void ) const
{
	return myChildScopes;
}


////////////////////////////////////////


inline const std::vector< std::shared_ptr<Pool> > &
TransformSet::getPools( void ) const
{
	return myPools;
}


////////////////////////////////////////








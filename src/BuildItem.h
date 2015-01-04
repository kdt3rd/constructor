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

#include <set>
#include <map>
#include "Dependency.h"
#include "Tool.h"

class TransformSet;


////////////////////////////////////////


class BuildItem : public Dependency<BuildItem>
{
public:
	BuildItem( const std::string &name,
			   const std::shared_ptr<Directory> &srcdir );
	BuildItem( std::string &&name,
			   const std::shared_ptr<Directory> &srcdir );
	virtual ~BuildItem( void );

	void setName( const std::string &n );
	virtual const std::string &getName( void ) const;
	inline void setUseName( bool v );
	inline bool useName( void ) const;

	inline const std::shared_ptr<Directory> &getDir( void ) const;
	inline const std::shared_ptr<Directory> &getOutDir( void ) const;

	void setOutputDir( const std::shared_ptr<Directory> &outdir );
	void addExternalOutput( const std::string &fn );

	void setTool( const std::shared_ptr<Tool> &t );
	inline const std::shared_ptr<Tool> &getTool( void ) const;
	void extractTags( std::set<std::string> &tags ) const;
	const std::string &getTag( void ) const;

	// the tool will normally auto-create this, but for
	// some code generation / filtering operations, this is useful
	void setOutputs( const std::vector<std::string> &outList );

	inline const std::vector<std::string> &getOutputs( void ) const;

	void setFlag( const std::string &n, const std::string &v );
	const std::string &getFlag( const std::string &n ) const;

	void setVariables( VariableSet v );
	void setVariable( const std::string &name, const std::string &val );
	void setVariable( const std::string &name, const std::vector<std::string> &val );
	void addToVariable( const std::string &name, const std::string &val );
	void addToVariableAtEnd( const std::string &name, const std::string &val );
	void addToVariable( const std::string &name, const Variable &val );
	void addToVariableAtEnd( const std::string &name, const Variable &val );

	const Variable &getVariable( const std::string &name ) const;
	inline const VariableSet &getVariables( void ) const;

	// "top level" indicates that it should appear
	// in the list of pseudo targets with a short name for
	// easy build targeting (i.e. "make foo")
	inline void setTopLevel( bool tl, const std::string &nm = std::string() );
	inline bool isTopLevelItem( void ) const;
	inline const std::string &getTopLevelName( void ) const;

	inline void setDefaultTarget( bool d );
	inline bool isDefaultTarget( void ) const;

	inline void markAsDependent( void );
	// basically, this should be true if no one depends on me, to be
	// used to find non "top level" targets that won't be built
	// otherwise
	inline bool isRoot( void ) const;

private:
	std::string myName;
	std::string myPseudoName;

	std::shared_ptr<Tool> myTool;
	std::vector<std::string> myOutputs;
	std::shared_ptr<Directory> myDirectory;
	std::shared_ptr<Directory> myOutDirectory;

	std::map<std::string, std::string> myFlags;
	VariableSet myVariables;

	bool myIsTopLevel = false;
	bool myIsDependent = false;
	bool myUseName = true;
	bool myDefaultTarget = true;
};


////////////////////////////////////////


inline void BuildItem::setUseName( bool v ) { myUseName = v; }
inline bool BuildItem::useName( void ) const { return myUseName; }


////////////////////////////////////////


inline
const std::shared_ptr<Directory> &
BuildItem::getDir( void ) const
{
	return myDirectory;
}


////////////////////////////////////////


inline
const std::shared_ptr<Directory> &
BuildItem::getOutDir( void ) const
{
	return myOutDirectory;
}


////////////////////////////////////////


inline const std::shared_ptr<Tool> &
BuildItem::getTool( void ) const
{
	return myTool;
}


////////////////////////////////////////


inline
const std::vector<std::string> &
BuildItem::getOutputs( void ) const
{
	return myOutputs;
}


////////////////////////////////////////


inline const VariableSet &
BuildItem::getVariables( void ) const
{
	return myVariables;
}


////////////////////////////////////////


inline void
BuildItem::setTopLevel( bool tl, const std::string &nm ) 
{
	myIsTopLevel = tl;
	myPseudoName = nm;
}
inline bool
BuildItem::isTopLevelItem( void ) const
{
	return myIsTopLevel;
}
inline const std::string &
BuildItem::getTopLevelName( void ) const
{
	if ( myPseudoName.empty() )
		return getName();
	return myPseudoName;
}


////////////////////////////////////////


inline void BuildItem::setDefaultTarget( bool d ) { myDefaultTarget = d; }
inline bool BuildItem::isDefaultTarget( void ) const { return myDefaultTarget; }


////////////////////////////////////////


inline void
BuildItem::markAsDependent( void )
{
	myIsDependent = true;
}
inline bool
BuildItem::isRoot( void ) const
{
	return myIsDependent;
}


////////////////////////////////////////





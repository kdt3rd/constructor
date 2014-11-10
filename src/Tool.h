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

#include <string>
#include <vector>
#include <map>
#include <set>

#include "Rule.h"
#include "Item.h"
#include "LuaValue.h"

class TransformSet;


////////////////////////////////////////


class Tool
{
public:
	typedef std::map<std::string, std::vector<std::string> > OptionSet;
	typedef std::map<std::string, OptionSet> OptionGroup;
	typedef std::map<std::string, std::string> OptionDefaultSet;

	Tool( std::string t, std::string n );
	virtual ~Tool( void );

	inline const std::string &getTag( void ) const;
	inline const std::string &getName( void ) const;

	void enableLanguage( const std::string &name );
	const std::string &getLanguage( void ) const;

	const std::string &getCommandPrefix( const std::string &varname ) const;

	OptionSet &getOption( const std::string &name );
	const OptionSet &getOption( const std::string &name ) const;
	bool hasOption( const std::string &name ) const;
	std::string getDefaultOption( const std::string &opt ) const;
	std::string getOptionValue( const std::string &opt, const std::string &choice ) const;
	std::string getOptionVariable( const std::string &opt ) const;
	void addOption( const std::string &opt,
					const std::string &name,
					const std::vector<std::string> &cmd );
	inline const OptionGroup &allOptions( void ) const;

	inline const std::string &getOutputPrefix( void ) const;
	inline const std::vector<std::string> &getOutputs( void ) const;

	const std::string &getExecutable( void ) const;
	ItemPtr getGeneratedExecutable( void ) const;

	inline bool hasImplicitDependencies( void ) const;
	inline const std::string &getImplicitDependencyFilename( void ) const;
	inline const std::vector<std::string> &getImplicitDependencyOptions( void ) const;

	inline const std::vector<std::string> &getExtensions( void ) const;
	bool handlesExtension( const std::string &e ) const;

	bool handlesTools( const std::set<std::string> &s ) const;

	Rule createRule( const TransformSet &x, bool useBraces = false ) const;

	static std::shared_ptr<Tool> parse( const Lua::Value &v );

private:
	friend class Scope;

	std::string myTag;
	std::string myName;
	std::string myDescription;

	ItemPtr myExePointer;
	std::string myExeName;

	std::vector<std::string> myExtensions;
	std::vector<std::string> myAltExtensions;
	std::string myOutputPrefix;
	std::vector<std::string> myOutputs;
	std::vector<std::string> myCommand;
	std::vector<std::string> myInputTools;

	OptionDefaultSet myFlagPrefixes;

	OptionGroup myOptions;
	OptionDefaultSet myOptionDefaults;
	std::string myLanguage;

	std::string myImplDepName;
	std::string myImplDepStyle;
	std::vector<std::string> myImplDepCmd;
};


////////////////////////////////////////


inline const std::string &
Tool::getTag( void ) const
{ return myTag; }
inline const std::string &
Tool::getName( void ) const
{ return myName; }


////////////////////////////////////////


inline bool
Tool::hasImplicitDependencies( void ) const
{ return ! myImplDepName.empty(); }

inline const std::string &
Tool::getImplicitDependencyFilename( void ) const
{ return myImplDepName; }

inline const std::vector<std::string> &
Tool::getImplicitDependencyOptions( void ) const
{ return myImplDepCmd; }


////////////////////////////////////////


inline const Tool::OptionGroup &
Tool::allOptions( void ) const
{ return myOptions; }



////////////////////////////////////////



inline const std::string &Tool::getOutputPrefix( void ) const
{ return myOutputPrefix; }

inline const std::vector<std::string> &
Tool::getOutputs( void ) const
{ return myOutputs; }


////////////////////////////////////////


inline const std::vector<std::string> &
Tool::getExtensions( void ) const
{ return myExtensions; }

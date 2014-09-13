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

#include "Rule.h"
#include "Item.h"
#include "LuaValue.h"

class Configuration;


////////////////////////////////////////


class Tool
{
public:
	typedef std::map<std::string, std::vector<std::string> > OptionSet;
	typedef std::map<std::string, OptionSet> OptionGroup;
	typedef std::map<std::string, std::string> OptionDefaultSet;

	Tool( std::string t, std::string n );
	virtual ~Tool( void );

	inline const std::string &tag( void ) const;
	inline const std::string &name( void ) const;

	void enableLanguage( const std::string &name );
	const std::string &language( void ) const;

	OptionSet &option( const std::string &name );
	const OptionSet &option( const std::string &name ) const;
	std::string defaultOption( const std::string &opt ) const;
	void addOption( const std::string &opt,
					const std::string &name,
					const std::vector<std::string> &cmd );

	const std::string &executable( void ) const;
	ItemPtr generatedExecutable( void ) const;

	inline bool hasImplicitDependencies( void ) const;
	inline const std::string &implicitDependencyFilename( void ) const;
	inline const std::vector<std::string> &implicitDependencyOptions( void ) const;

	inline const std::vector<std::string> &extensions( void ) const;
	bool handlesExtension( const std::string &e ) const;

	Rule rule( const OptionDefaultSet &scopeOptions, const Configuration &conf ) const;

	static std::shared_ptr<Tool> parse( const Lua::Value &v );

private:
	std::string myTag;
	std::string myName;
	std::string myDescription;

	ItemPtr myExePointer;
	std::string myExeName;

	std::vector<std::string> myExtensions;
	std::vector<std::string> myAltExtensions;
	std::vector<std::string> myOutputs;
	std::vector<std::string> myCommand;
	std::vector<std::string> myInputTools;

	OptionGroup myOptions;
	OptionDefaultSet myOptionDefaults;
	std::string myLanguage;

	std::string myImplDepName;
	std::vector<std::string> myImplDepCmd;
};


////////////////////////////////////////


inline const std::string &
Tool::tag( void ) const
{ return myTag; }
inline const std::string &
Tool::name( void ) const
{ return myName; }


////////////////////////////////////////


inline bool
Tool::hasImplicitDependencies( void ) const
{ return ! myImplDepName.empty(); }

inline const std::string &
Tool::implicitDependencyFilename( void ) const
{ return myImplDepName; }

inline const std::vector<std::string> &
Tool::implicitDependencyOptions( void ) const
{ return myImplDepCmd; }


////////////////////////////////////////


inline const std::vector<std::string> &
Tool::extensions( void ) const
{ return myExtensions; }

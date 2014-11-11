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


////////////////////////////////////////


///
/// @brief Class Variable provides an abstraction around storing a name
///        with associated values
///
class Variable
{
public:
	Variable( std::string n, bool checkEnv = false );
	Variable( const std::string &n, const std::string &val );

	inline const std::string &name( void ) const;

	inline void inherit( bool yesno );
	inline bool inherit( void ) const;

	inline bool useToolFlagTransform( void ) const;
	inline const std::string &getToolTag( void ) const;
	void setToolTag( std::string tag );

	inline bool empty( void ) const;
	void clear( void );
	void add( std::string v );
	void add( std::vector<std::string> v );
	void addIfMissing( const std::string &v );
	void addIfMissing( const std::vector<std::string> &v );
	void moveToEnd( const std::string &v );
	void moveToEnd( const std::vector<std::string> &v );
	// removes duplicates, keeping last entry
	// does NOT change relative ordering
	void removeDuplicatesKeepLast( void );

	void reset( std::string v );
	void reset( std::vector<std::string> v );

	const std::string &value( void ) const;

	// if any of the values in the Variable don't begin
	// with the provided prefix, it is preprended to that
	// value.
	std::string prepended_value( const std::string &prefix ) const;

	inline const std::vector<std::string> &values( void ) const;

	static const Variable &nil( void );
private:
	std::string replace_vars( const std::string &v );

	std::string myName;
	std::vector<std::string> myValues;
	mutable std::string myCachedValue;
	std::string myToolTag;
	bool myInherit = false;
};


typedef std::map<std::string, Variable> VariableSet;


////////////////////////////////////////////////////////////////////////////////


inline const std::string &
Variable::name( void ) const
{
	return myName;
}


////////////////////////////////////////


inline void Variable::inherit( bool yesno ) { myInherit = yesno; }
inline bool Variable::inherit( void ) const { return myInherit; }


////////////////////////////////////////


inline const std::string &
Variable::getToolTag( void ) const
{
	return myToolTag;
}
inline bool
Variable::useToolFlagTransform( void ) const
{
	return ! myToolTag.empty();
}


////////////////////////////////////////


inline bool
Variable::empty( void ) const
{
	return myValues.empty();
}


////////////////////////////////////////


inline const std::vector<std::string> &
Variable::values( void ) const
{
	return myValues;
}


////////////////////////////////////////



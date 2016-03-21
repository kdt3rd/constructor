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
	~Variable( void );
	Variable( const Variable & ) = default;
	Variable( Variable && ) = default;
	Variable &operator=( const Variable & ) = default;
	Variable &operator=( Variable && ) = default;

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
	void addPerSystem( const std::string &s, std::string v );
	void addPerSystem( const std::string &s, std::vector<std::string> v );
	void addIfMissing( const std::string &v );
	void addIfMissing( const std::vector<std::string> &v );
	void addIfMissingSystem( const std::string &s, const std::string &v );
	void addIfMissingSystem( const std::string &s, const std::vector<std::string> &v );
	void moveToEnd( const std::string &v );
	void moveToEnd( const std::vector<std::string> &v );
	// removes duplicates, keeping last entry
	// does NOT change relative ordering
	void removeDuplicatesKeepLast( void );

	// based on the prefix in the flag and the
	// disposition for that prefix, either keep
	// the first duplicate or the last duplicate
	// (false means last, true means first)
	// so to clean up common cflags + ldflags:
	// { { "-W", true }, {"-D", true}, {"-I", true},
	//   { "-L", true }, {"-l", false} }
	void removeDuplicates( const std::map<std::string, bool> &prefixDisposition );

	void reset( std::string v );
	void reset( std::vector<std::string> v );

	// adds any values in other not in current
	void merge( const Variable &other );

	const std::string &value( const std::string &sys ) const;

	// if any of the values in the Variable don't begin
	// with the provided prefix, it is preprended to that
	// value.
	std::string prepended_value( const std::string &prefix, const std::string &sys ) const;

	inline const std::vector<std::string> &values( void ) const;
	inline const std::map<std::string, std::vector<std::string>> &system_values( void ) const;

	static const Variable &nil( void );
private:
	std::string replace_vars( const std::string &v );

	std::string myName;
	std::vector<std::string> myValues;
	std::map<std::string, std::vector<std::string>> mySystemValues;
	mutable std::string myCachedValue;
	mutable std::string myCachedSystem;
	std::string myToolTag;
	bool myInherit = false;
};

inline bool operator<( const Variable &a, const Variable &b )
{
	return a.name() < b.name();
}

inline bool operator==( const Variable &a, const Variable &b )
{
	return ( a.name() == b.name() &&
			 a.inherit() == b.inherit() &&
			 a.getToolTag() == b.getToolTag() &&
			 a.values() == b.values() &&
			 a.system_values() == b.system_values() );
}
inline bool operator!=( const Variable &a, const Variable &b )
{
	return !( a == b );
}

typedef std::map<std::string, Variable> VariableSet;

void merge( VariableSet &vs, const VariableSet &other );


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


inline const std::map<std::string, std::vector<std::string>> &
Variable::system_values( void ) const
{
	return mySystemValues;
}


////////////////////////////////////////



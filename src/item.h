// item.h -*- C++ -*-

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

enum class DependencyType : uint8_t
{
	CHAIN = 0,
	EXPLICIT,
	IMPLICIT,
	ORDER
};
	
		
/// @brief base class for everything else in the system
///
/// In a build system, there are either targets
class item
{
public:
	typedef uint64_t ID;
	static const ID UNKNOWN = static_cast<uint64_t>(-1);

	item( const std::string &name );
	item( std::string &&name );
	virtual ~item( void );

	inline ID id( void ) const;
	inline const std::string &name( void ) const;

	void add_dependency( DependencyType dt, ID otherObj );
	void add_dependency( DependencyType dt, const std::string &otherObj );
	inline void add_dependency( DependencyType dt, const item &otherObj );

	inline bool has_unresolved_dependencies( void ) const;
	void update_dependency( const std::string &name, ID id );

	/// Will throw if unable to resolve all dependencies from all live items -
	/// should only be called once prior to generating the build files
	static void check_dependencies( void );
private:
	ID myID;
	std::string myName;

	std::vector<std::pair<DependencyType, std::string>> myUnresolvedDependencies;
	std::map<ID, DependencyType> myDependencies;
};


////////////////////////////////////////


inline item::ID
item::id( void ) const
{
	return myID;
}


////////////////////////////////////////


inline const std::string &
item::name( void ) const 
{
	return myName;
}


////////////////////////////////////////


inline void
item::add_dependency( DependencyType dt, const item &otherObj )
{
	add_dependency( dt, otherObj.id() );
}


////////////////////////////////////////


inline bool
item::has_unresolved_dependencies( void ) const
{
	return ! myUnresolvedDependencies.empty();
}


////////////////////////////////////////



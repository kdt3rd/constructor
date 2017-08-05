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
#include <vector>
#include <stdexcept>
#include <algorithm>


////////////////////////////////////////


enum class DependencyType : uint8_t
{
	CHAIN = 0,
	EXPLICIT,
	IMPLICIT,
	ORDER
};

template <typename Item>
class Dependency : public std::enable_shared_from_this<Item>
{
public:
	typedef std::shared_ptr<Item> ItemPtr;
	typedef std::weak_ptr<Item> WeakItemPtr;

	Dependency( void )
	{}
	virtual ~Dependency( void )
	{}

	virtual const std::string &getName( void ) const = 0;

	inline void setParent( ItemPtr p );
	inline ItemPtr getParent( void ) const;

	inline void addDependency( DependencyType dt, ItemPtr o );

	/// (recursively) check if this item has a dependency on
	/// the other passed in
	inline bool hasDependency( const ItemPtr &other ) const;

	/// for a chain dependency,
	/// Recursively traverses the chain dependencies and
	/// constructs a list of them. if an item has 2 chain dependencies
	/// and both depend on a 3rd item, that 3rd item will appear
	/// later than any of the items that depend on it
	///
	/// for all other dependency types,
	/// returns the list of items that this item has that dependency type on
	inline std::vector<ItemPtr> extractDependencies( DependencyType dt ) const;
	
protected:
	inline void recurseChain( std::vector<ItemPtr> &chain ) const
	{
		for ( auto &dep: myDependencies )
		{
			if ( dep.second != DependencyType::CHAIN )
				continue;

			chain.push_back( dep.first );
			dep.first->recurseChain( chain );
		}
	}

	std::map<ItemPtr, DependencyType> myDependencies;
	WeakItemPtr myParent;
};


////////////////////////////////////////


template <typename Item>
inline void
Dependency<Item>::setParent( ItemPtr p )
{
	myParent = p;
}
template <typename Item>
inline typename Dependency<Item>::ItemPtr
Dependency<Item>::getParent( void ) const
{
	return myParent.lock();
}



////////////////////////////////////////


template <typename Item>
inline void
Dependency<Item>::addDependency( DependencyType dt, ItemPtr otherObj )
{
	if ( ! otherObj )
		return;

	if ( otherObj->hasDependency( this->shared_from_this() ) )
		throw std::runtime_error( "Attempt to create a circular dependency between '" + getName() + "' and '" + otherObj->getName() + "'" );

	auto cur = myDependencies.find( otherObj );
	if ( cur == myDependencies.end() )
		myDependencies[otherObj] = dt;
	else if ( cur->second > dt )
		cur->second = dt;
}


////////////////////////////////////////


template <typename Item>
bool
Dependency<Item>::hasDependency( const ItemPtr &other ) const
{
	if ( myDependencies.find( other ) != myDependencies.end() )
		return true;

	for ( auto &dep: myDependencies )
	{
		if ( dep.first->hasDependency( other ) )
			return true;
	}

	return false;
}


////////////////////////////////////////


template <typename Item>
std::vector<typename Dependency<Item>::ItemPtr>
Dependency<Item>::extractDependencies( DependencyType dt ) const
{
	std::vector<ItemPtr> retval;
	typedef typename std::vector<ItemPtr>::iterator::difference_type difference_type;

	if ( dt == DependencyType::CHAIN )
	{
		recurseChain( retval );

		if ( ! retval.empty() )
		{
			// go through in reverse order and remove any duplicates
			// that follow
			std::reverse( std::begin( retval ), std::end( retval ) );

			size_t i = 0;
			while ( i != retval.size() )
			{
				for ( size_t j = i + 1; j < retval.size(); ++j )
				{
					if ( retval[i] == retval[j] )
					{
						retval.erase( retval.begin() + static_cast<difference_type>( j ) );
						j = i;
					}
				}
				++i;
			}
			std::reverse( std::begin( retval ), std::end( retval ) );
		}
	}
	else
	{
		for ( auto &dep: myDependencies )
		{
			if ( dep.second != dt )
				continue;

			retval.push_back( dep.first );
		}
	}

	std::sort( retval.begin(), retval.end(),
			   []( const ItemPtr &a, const ItemPtr &b )
			   {
				   return (a->getName() < b->getName() ||
						   (a->getName() == b->getName() &&
							a->getDir()->fullpath() < b->getDir()->fullpath()));
			   }
			   );

	return std::move( retval );
}

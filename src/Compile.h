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

#include "Item.h"
#include "TransformSet.h"
#include <queue>
#include <set>


////////////////////////////////////////


class CompileSet : public Item
{
public:
	CompileSet( void );
	virtual ~CompileSet( void );

	void addItem( const ItemPtr &i );
	void addItem( std::string name );
	inline bool empty( void ) const { return myItems.empty(); }
	inline size_t size( void ) const { return myItems.size(); }

	virtual std::shared_ptr<BuildItem> transform( TransformSet &xform ) const;

protected:
	CompileSet( std::string name );

	void
	followChains( std::queue<std::shared_ptr<BuildItem>> &chainsToCheck,
				  std::set<std::string> &tags,
				  const std::shared_ptr<BuildItem> &ret,
				  TransformSet &xform ) const;

	std::shared_ptr<BuildItem>
	chainTransform( const std::string &name,
					const std::shared_ptr<Directory> &srcdir,
					TransformSet &xform ) const;

	void fillBuildItem( const std::shared_ptr<BuildItem> &bi, TransformSet &xform, std::set<std::string> &tags, bool propagateLibs, const std::vector<ItemPtr> &extraItems = std::vector<ItemPtr>() ) const;

	std::vector<ItemPtr> myItems;
};




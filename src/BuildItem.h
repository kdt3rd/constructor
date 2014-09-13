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

#include "Dependency.h"
#include "Tool.h"


////////////////////////////////////////


class BuildItem : public Dependency<BuildItem>
{
public:
	BuildItem( const std::string &name );
	BuildItem( std::string &&name );
	virtual ~BuildItem( void );

	virtual const std::string &name( void ) const;

	void setTool( const std::shared_ptr<Tool> &t );
	inline const std::shared_ptr<Tool> &tool( void ) const;

	void addOutput( const std::string &o );
	void addOutput( std::string &&o );
	inline const std::vector<std::string> &outputs( void ) const;

private:
	std::string myName;

	std::shared_ptr<Tool> myTool;
	std::vector<std::string> myOutputs;
};


////////////////////////////////////////


inline const std::shared_ptr<Tool> &
BuildItem::tool( void ) const
{
	return myTool;
}



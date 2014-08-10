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
#include <string>

#include "variable_set.h"


////////////////////////////////////////


///
/// @brief Class scope provides an abstraction around a collection of items.
///
/// This is probably most commonly a directory (or sub-directory). It
/// can have it's own unique config / toolsets / whatever as well as
/// "global" variables.  Variables can be inherited from a parent
/// scope or not.
///
class scope 
{
public:
	scope( std::shared_ptr<scope> parent );
	~scope( void );

	inline std::shared_ptr<scope> parent_scope( void ) const;
	std::shared_ptr<scope> new_sub_scope( void );

	inline void inherit( bool yesno );
	inline bool inherit( void ) const;

	variable_set &vars( void );
	const variable_set &vars( void ) const;

private:
	std::weak_ptr<scope> myParent;
	variable_set myVariables;
	bool myInheritParentScope = false;
};


////////////////////////////////////////



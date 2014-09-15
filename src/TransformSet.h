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

#include "Tool.h"
#include "Variable.h"
#include "Directory.h"


////////////////////////////////////////


class TransformSet
{
public:
	TransformSet( const std::shared_ptr<Directory> &dir );
	~TransformSet( void );

	inline const std::shared_ptr<Directory> &output_dir( void ) const;

	void addTool( const std::shared_ptr<Tool> &t );
	void mergeVariables( const VariableSet &vs );

	std::shared_ptr<Tool> findTool( const std::string &ext ) const;
	inline const VariableSet &vars( void ) const;

private:
	std::shared_ptr<Directory> myDirectory;
	std::vector< std::shared_ptr<Tool> > myTools;
	VariableSet myVars;
};


////////////////////////////////////////


inline const std::shared_ptr<Directory> &
TransformSet::output_dir( void ) const
{
	return myDirectory;
}


////////////////////////////////////////


inline const VariableSet &
TransformSet::vars( void ) const
{
	return myVars;
}






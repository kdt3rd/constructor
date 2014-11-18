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

#include "Compile.h"
#include "Directory.h"


////////////////////////////////////////


class CodeGenerator : public CompileSet
{
public:
	CodeGenerator( std::string name );
	virtual ~CodeGenerator( void );

	void setItemInfo( const std::vector<std::string> &itemPrefix,
					  const std::vector<std::string> &itemSuffix,
					  const std::string &itemIndent,
					  bool doCommas );
	void setFileInfo( const std::vector<std::string> &filePrefix,
					  const std::vector<std::string> &fileSuffix );
					  
	virtual std::shared_ptr<BuildItem> transform( TransformSet &xform ) const;

	static void emitCode( const std::string &outfn,
						  const std::vector<std::string> &inputs,
						  const std::string &filePrefix,
						  const std::string &fileSuffix,
						  const std::string &itemPrefix,
						  const std::string &itemSuffix,
						  const std::string &itemIndent,
						  bool doCommas );

private:
	void processEntry( const std::string &tag,
					   const std::shared_ptr<Directory> &tmpd,
					   const std::vector<std::string> &list,
					   const std::shared_ptr<BuildItem> &ret,
					   std::vector<std::string> &varlist ) const;

	std::vector<std::string> myItemPrefix, myItemSuffix;
	std::vector<std::string> myFilePrefix, myFileSuffix;
	std::string myItemIndent;
	bool myDoCommas = false;
};


////////////////////////////////////////



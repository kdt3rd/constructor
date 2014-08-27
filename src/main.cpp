// main.cpp -*- C++ -*-

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

#include "item.h"
#include "LuaEngine.h"
#include <stdexcept>
#include <iostream>
#include "OSUtil.h"
#include "FileUtil.h"
#include "PackageConfig.h"


////////////////////////////////////////


int
main( int argc, char *argv[] )
{
	try
	{
		OS::registerFunctions();
		File::registerFunctions();
		PackageConfig::registerFunctions();

		std::string subdir;
		if ( argc > 1 )
			subdir = argv[1];

		File::startParsing( subdir );

//		item::check_dependencies();
		
	}
	catch ( std::exception &e )
	{
		std::cerr << "ERROR: " << e.what() << std::endl;
		return -1;
	}
	catch ( ... )
	{
		std::cerr << "Unhandled exception" << std::endl;
		return -1;
	}

	return 0;
}


////////////////////////////////////////






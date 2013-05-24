#
# Copyright (c) 2013 Kimball Thurston
#
# Permission is hereby granted, free of charge, to any person obtaining
# a copy of this software and associated documentation files (the "Software"),
# to deal in the Software without restriction, including without limitation
# the rights to use, copy, modify, merge, publish, distribute, sublicense,
# and/or sell copies of the Software, and to permit persons to whom the
# Software is furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included
# in all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
# EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
# MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
# IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
# CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
# TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE
# OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
#

import os
import sys
import subprocess
from constructor.utility import FindOptionalExecutable, CheckEnvironOverride
from constructor.output import Info, Debug, Error, Warn
from constructor.dependency import Dependency
from constructor.driver import CurDir
from constructor.module import ProcessFiles

variables = {}

if sys.platform.startswith( "linux" ):
    variables["CC"] = CheckEnvironOverride( "CC", FindOptionalExecutable( "gcc" ) )
    variables["CXX"] = CheckEnvironOverride( "CXX", FindOptionalExecutable( "g++" ) )
elif sys.platform.startswith( "darwin" ):
    variables["CC"] = CheckEnvironOverride( "CC", FindOptionalExecutable( "clang" ) )
    variables["CXX"] = CheckEnvironOverride( "CXX", FindOptionalExecutable( "clang++" ) )
elif sys.platform.startswith( "win" ):
    variables["CC"] = CheckEnvironOverride( "CC", FindOptionalExecutable( "cl" ) )
    variables["CXX"] = CheckEnvironOverride( "CXX", FindOptionalExecutable( "cl" ) )
else:
    Warn( "Un-handled platform '%s', defaulting compiler to gcc" % sys.platform )
    variables["CC"] = CheckEnvironOverride( "CC", FindOptionalExecutable( "gcc" ) )
    variables["CXX"] = CheckEnvironOverride( "CXX", FindOptionalExecutable( "g++" ) )

variables["CFLAGS"] = CheckEnvironOverride( "CFLAGS", [] )
variables["CXXFLAGS"] = CheckEnvironOverride( "CXXFLAGS", [] )
variables["WARNINGS"] = CheckEnvironOverride( "WARNINGS", [] )

def _SetCompiler( cc=None, cxx=None ):
    if cc is not None:
        if os.environ.get( "CC" ):
            Info( "Compiler '%s' overridden with environment flag to '%s'" % (cc, os.environ.get( "CC" )) )
        else:
            CurDir().set_variable( "CC", FindExecutable( cc ) )
    if cxx is not None:
        if os.environ.get( "CC" ):
            Info( "Compiler '%s' overridden with environment flag to '%s'" % (cc, os.environ.get( "CC" )) )
        else:
            CurDir().set_variable( "CXX", FindExecutable( cxx ) )

_defBuildShared = True
_defBuildStatic = True
def _SetDefaultLibraryStyle( style ):
    global _defBuildShared
    global _defBuildStatic
    if style == "both":
        _defBuildShared = True
        _defBuildStatic = True
    elif style == "shared":
        _defBuildShared = True
        _defBuildStatic = False
    elif style == "static":
        _defBuildShared = False
        _defBuildStatic = True
    else:
        Error( "Unknown default library build style '%s'" % style)

def _CFlags( *f ):
    CurDir().add_to_variable( "CFLAGS", f )
    pass

def _CXXFlags( *f ):
    CurDir().add_to_variable( "CXXFLAGS", f )
    pass

def _Warnings( *f ):
    CurDir().add_to_variable( "WARNINGS", f )
    pass

def _Library( *f ):
    pass

def _Executable( *f ):
    pass

def _OptExecutable( *f ):
    pass

def _Compile( *f ):
    ret = ProcessFiles( f )
    CurDir().add_targets( ret )
    return ret

def _CPPTarget( f ):
    Debug( "Processing C++ target '%s'" % f )

def _CTarget( f ):
    Debug( "Processing C target '%s'" % f )

def _NilTarget( f ):
    Debug( "Processing nil target '%s'" % f )

modules = [ "external_pkg" ]
rules = []
features = [ { "name": "shared", "type": "boolean", "help": "Build shared libraries", "default": True },
             { "name": "static", "type": "boolean", "help": "Build static libraries", "default": True } ]

extension_handlers = {
    ".cpp": _CPPTarget,
    ".c": _CTarget,
    ".o": _NilTarget,
}

functions_by_phase = {
    "config":
    {
        "SetCompiler": _SetCompiler,
        "SetDefaultLibraryStyle": _SetDefaultLibraryStyle,
        "CFlags": _CFlags,
        "CXXFlags": _CXXFlags,
        "Warnings": _Warnings,
    },
    "build":
    {
        "Library": _Library,
        "Executable": _Executable,
        "OptExecutable": _OptExecutable,
        "Compile": _Compile,
        "CFlags": _CFlags,
        "CXXFlags": _CXXFlags,
        "Warnings": _Warnings,
    }
}

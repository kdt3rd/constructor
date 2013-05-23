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
from constructor.utility import FindOptionalExecutable
from constructor.output import Info, Debug, Error
from constructor.dependency import Dependency
from constructor.driver import CurDir

if sys.platform.startswith( "linux" ):
    _def_cc = FindOptionalExecutable( "gcc" )
    _def_cxx = FindOptionalExecutable( "g++" )
elif sys.platform.startswith( "darwin" ):
    _def_cc = FindOptionalExecutable( "clang" )
    _def_cxx = FindOptionalExecutable( "clang++" )
elif sys.platform.startswith( "win" ):
    _def_cc = FindOptionalExecutable( "cl" )
    _def_cxx = FindOptionalExecutable( "cl" )

def _SetCompiler( cc=None, cxx=None ):
    if cc is not None:
        _def_cc = FindExecutable( cc )
    if cxx is not None:
        _def_cxx = FindExecutable( cxx )

_defBuildShared = True
_defBuildStatic = True
def _SetDefaultLibraryStyle( style ):
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
    pass

def _CXXFlags( *f ):
    pass

def _Warnings( *f ):
    pass

def _Library( *f ):
    pass

def _Executable( *f ):
    pass

modules = [ "external_pkg" ]
rules = []
features = [ { "name": "shared", "type": "boolean", "help": "Build shared libraries", "default": True },
             { "name": "static", "type": "boolean", "help": "Build static libraries", "default": True } ]

variables = {
    "CC": _def_cc,
    "CXX": _def_cxx,
    "CFLAGS": "",
    "CXXFLAGS": "",
    "WARNINGS": ""
    }

extension_handlers = None
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
        "CFlags": _CFlags,
        "CXXFlags": _CXXFlags,
        "Warnings": _Warnings,
    }
 }

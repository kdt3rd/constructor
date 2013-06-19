#
# Copyright (c) 2012 Kimball Thurston
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

from constructor.utility import FindOptionalExecutable, CheckEnvironOverride
from constructor.output import Info, Debug, Error, Warn
from constructor.dependency import Dependency, FileDependency
from constructor.driver import CurDir, Feature
from constructor.module import ProcessFiles
from constructor.rule import Rule
from constructor.target import Target, GetTarget, AddTarget
from constructor.version import Version
from constructor.cobject import ExtractObjects

_bison = FindOptionalExecutable( "bison" )
_byacc = FindOptionalExecutable( "byacc" )

if _bison:
    _yaccCmd = [ _bison, "-y", "-d", "--file-prefix", "$file_prefix", "--name-prefix", "$sym_prefix", "$in" ]
elif _byacc:
    # bison generates re-entrant code, ask byacc to as well
    _yaccCmd = [ _byacc, "-P", "-b", "$file_prefix", "-p", "$sym_prefix", "-d", "$in" ]
else:
    Error( "No suitable (modern) bison or byacc could be located" )

rules = {
    "yacc": Rule( tag="yacc", cmd=_yaccCmd, desc = "YACC ($in)" ),
    }

extension_handlers = {
    ".y": _YACCTarget,
}

def _YACC( name, filename ):
    retval = []
    if os.path.isabs( filename ):
        Error( "Unable to handle absolute paths '%s' in YACC" % filename )
    curd = CurDir()
    (base, tail) = os.path.split( filename )
    out_dir = os.path.join( curd.bin_path, base )
    out_c = os.path.join( out_dir, name + ".tab.c" )
    out_h = os.path.join( out_dir, name + ".tab.h" )
    return retval

functions_by_phase = {
    "build":
    {
        "YACC": _YACC
        "YACCXX": _YACCXX
    }
}

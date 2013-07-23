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

from constructor.utility import FindOptionalExecutable, CheckEnvironOverride
from constructor.output import Info, Debug, Error, Warn
from constructor.dependency import Dependency, FileDependency
from constructor.driver import CurDir, Feature, Driver, RootTarget
from constructor.module import ProcessFiles
from constructor.rule import Rule
from constructor.target import Target, GetTarget, AddTarget
from constructor.version import Version
from constructor.cobject import ExtractObjects

modules = [ "cc" ]

if sys.platform.startswith( "win" ):
    Error( "Unit test execution not finished for windows" )

rules = {
    "run_rv0_test": Rule( tag="run_rv0_test", 
                          cmd=["$in", "|", "tee", "$out"],
                          desc="TEST ($in)" )
}

def _UnitTest( name, sources, condition ):
    curd = CurDir()
    test_log = os.path.join( curd.bin_path, "test_" + name + ".log" )
    if condition == "return_0":
        shorte = OptExecutable( "test_" + name, sources )
        t = AddTarget( curd, "test", test_log, outputs=test_log, rule=rules["run_rv0_test"] )
        t.add_dependency( shorte.dependencies[0] )
        RootTarget( "test", "test" ).add_dependency( t )
    pass

functions_by_phase = {
    "build":
    {
        "UnitTest": _UnitTest
    }
}

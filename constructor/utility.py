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

import sys
import os
import subprocess

# Python 2.xx and 3 are incompatible in regards to iterating over things
# let's define our own iterate function that does the Right Thing (tm)
iterate = lambda x: x.iteritems()
if sys.version_info[0] == 3:
    iterate = lambda x:x.items()


def FindOptionalExecutable( exe ):
    if exe is None:
        return None

    if sys.platform == "win32" or sys.platform == "win64":
        if exe[:-4] != ".exe":
            exe = exe + ".exe"
        pass

    is_exe = lambda fpath: os.path.isfile( fpath ) and os.access( fpath, os.X_OK )

    fpath, fname = os.path.split( exe )
    if fpath:
        if is_exe( exe ):
            return exe
    else:
        for path in os.environ["PATH"].split( os.pathsep ):
            path = path.strip( '"' )
            exe_file = os.path.join( path, exe )
            if is_exe( exe_file ):
                return exe_file
    return None

def FindExecutable( exe ):
    exe_file = FindOptionalExecutable( exe )
    if exe_file is None:
        raise OSError( errno.ENOENT, "No such file or directory trying to find executable", exe )
    return exe_file

def Is64bit():
    if sys.platform == "win64":
        return True
    elif sys.platform == "win32":
        return False
    (syst, node, rel, ver, machine) = os.uname()
    if machine == "x86_64":
        return True

    return False

_dist = None
_rel = None
def Distribution():
    global _dist
    global _rel
    if _dist is None:
        (syst, node, rel, ver, machine) = os.uname()
        if syst == "Linux":
            _dist = subprocess.check_output( ['lsb_release', '-s', '-i' ], universal_newlines=True )
            _rel = subprocess.check_output( ['lsb_release', '-s', '-r' ], universal_newlines=True )

        Error( "Unimplemented function" )
    return _dist

def Release():
    global _rel
    if _rel is None:
        # use the above distribution function to populate both
        d = Distribution()

    return _rel


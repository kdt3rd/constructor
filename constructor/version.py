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
#

import subprocess
import re
from .output import FatalException, Debug, Info, Error

class Version(object):
    def __init__( self, binary=None, verarg='--version', regex=None, separator='.', version=None ):
        if version:
            self.version = version
        else:
            proc = subprocess.Popen( [binary, verarg], stdout=subprocess.PIPE, universal_newlines=True )
            ver = proc.stdout.read().strip()
            if regex:
                m = re.search( regex, ver )
                if not m:
                    Error( "Version output did not match requested regex '%s':\n%s" % ( regex, ver ) )
                ver = m.group( 1 )
            self.version = ver
        self.split_ver = self.version.split( separator )
        self.separator = separator

    def __repr__( self ):
        return "Version( version='%s', separator='%s' )" % (self.version, self.separator)
    def __str__( self ):
        return self.version

    def __lt__( self, other ):
        if isinstance( other, Version ):
            if self.separator < other.separator:
                return True
            elif self.separator > other.separator:
                return False
            ver = other.split_ver
        elif isinstance( other, list ):
            ver = other
        else:
            xxx = str(other)
            ver = xxx.split( self.separator )
        ml = len(self.split_ver)
        for x in range( 0, len(ver) ):
            if ml <= x:
                return True
            if self.split_ver[x] < ver[x]:
                return True
            elif self.split_ver[x] > ver[x]:
                return False
        return False
    def __eq__(self, other):
        if not isinstance( other, Version ):
            other = Version( version=other, separator=self.separator )
        return not self<other and not other<self
    def __ne__(self, other):
        if not isinstance( other, Version ):
            other = Version( version=other, separator=self.separator )
        return self<other or other<self
    def __gt__(self, other):
        if not isinstance( other, Version ):
            other = Version( version=other, separator=self.separator )
        return other<self
    def __ge__(self, other):
        if not isinstance( other, Version ):
            other = Version( version=other, separator=self.separator )
        return not self<other
    def __le__(self, other):
        if not isinstance( other, Version ):
            other = Version( version=other, separator=self.separator )
        return not other<self

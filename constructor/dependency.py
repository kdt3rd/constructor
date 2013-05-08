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

from .output import Debug, Error

class Dependency(object):
    """Base class for all things that represent a dependency tree"""

    def __init__( self, orderonly ):
        self._dependencies = {}
        self.orderonly = orderonly

    def add_dependency( self, group, dep ):
        if not isinstance( dep, Dependency ):
            Error( "Attempt to add dependency that is not a subclass of Dependency" )

        try:
            deps = self._dependencies[group]
        except KeyError:
            deps = []
            self._dependencies[group] = deps

        found = False
        for d in deps:
            if d is dep:
                found = True
                break
        if not found:
            deps.append( dep )

    def dependencies( self, group ):
        if group in self._dependencies:
            return self._dependencies[group]
        return []
            
class FileDependency(Dependency):
    """Simple sub-class to represent a direct file in a dependency tree"""
    def __init__( self, infile, orderonly ):
        super(FileDependency, self).__init__( orderonly )
        self.filename = infile


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

class CObject(object):
    """ Base class for things passed through the various functions.
    This is mostly just a typing thing to be able to extract items and
    organize them by type, but also provides a central class to add
    and pass usage metadata along with."""

    def __init__( self, objtype ):
        self.type = objtype
        self.chained_usage = None

    def add_chained_usage( self, x ):
        if self.chained_usage is None:
            self.chained_usage = []
        if isinstance( x, (list, tuple) ):
            for i in x:
                self.add_chained_usage( i )
        else:
            if not isinstance( x, CObject ):
                Error( "Attempt to chain usage of a non-CObject item" )
            self.chained_usage.append( x )

    def extract_chained_usage( self, retval ):
        if self.chained_usage:
            for i in self.chained_usage:
                xxx = retval.get( i.type )
                if xxx is None:
                    retval[i.type] = [ i ]
                else:
                    for a in range( 0, len(xxx) ):
                        if xxx[a] is i:
                            del xxx[a]
                    xxx.append( i )
                i.extract_chained_usage( retval )

def ExtractObjects( *arbitrary_list ):
    """ Returns a dictionary of lists"""
    retval = {}
    def _get_or_add_key( a, name ):
        if name is None:
            _get_or_add_key( a, "unknown" )
            return

        xxx = retval.get( name )
        if xxx is None:
            retval[name] = [ a ]
        else:
            xxx.append( a )

    def _recursor( *f ):
        for a in f:
            if isinstance( a, str ):
                _get_or_add_key( a, "str" )
            elif isinstance( a, CObject ):
                Debug( "Adding object of type '%s' to extraction" % a.type )
                _get_or_add_key( a, a.type )
            elif isinstance( a, (list,tuple) ):
                for x in a:
                    _recursor( x )

    _recursor( arbitrary_list )
    return retval

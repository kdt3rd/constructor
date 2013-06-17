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
    def __init__( self, objtype ):
        self.type = objtype

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

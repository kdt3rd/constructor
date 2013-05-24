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

import sys
import traceback

_is_debug = False
_is_verbose = False
_current_context = []

def SetDebug( flag ):
    global _is_debug
    _is_debug = flag

def SetVerbose( flag ):
    global _is_verbose
    _is_verbose = flag

def IsVerbose():
    global _is_verbose
    return _is_verbose

def PushDebugContext( ctxt ):
    global _current_context
    _current_context.append( ctxt )

def PopDebugContext():
    global _current_context
    if len(_current_context) > 0:
        _current_context.pop()

def AddDebugContext( output ):
    global _current_context
    if len(_current_context) > 0:
        output.write( _current_context[-1] )
        output.write( ":\n   " )

def Debug( msg ):
    global _is_debug
    if _is_debug:
        AddDebugContext( sys.stdout )
        sys.stdout.write( msg )
        sys.stdout.write( '\n' )
        sys.stdout.flush()

def Info( msg ):
    AddDebugContext( sys.stdout )
    sys.stdout.write( msg )
    sys.stdout.write( '\n' )
    sys.stdout.flush()

def Warn( msg ):
    AddDebugContext( sys.stderr )
    sys.stderr.write( "WARNING: " )
    sys.stderr.write( msg )
    sys.stderr.write( '\n' )
    sys.stderr.flush()

def Error( msg ):
    AddDebugContext( sys.stderr )
    sys.stderr.write( "ERROR: " )
    sys.stderr.write( msg )
    sys.stderr.write( '\n\n' )
    sys.stderr.flush()
    sys.exit( 1 )

def BadException( msg ):
    AddDebugContext( sys.stderr )
    sys.stderr.write( msg )
    sys.stderr.write( '\n\n' )
    einfo = traceback.format_exc()
    sys.stderr.write( einfo )
    sys.stderr.flush()
    sys.exit( 1 )

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

from .dependency import Dependency
from .target import Target
from .output import Debug, Error
from .utility import iterate

class Module(object):
    """Class encapsulating a module, which defines how certain types of code are compiled, or how other things are processed"""
    _extension_handlers = {}

    def _handleFile( f, operation ):
        (base, ext) = os.path.splitext( f )
        handler = _extension_handlers.get( ext )
        if handler is None:
            for e, h in iterate( _extension_handlers ):
                if f.endswith( e ):
                    handler = h
                    break

        if handler is not None:
            res = getattr( handler, operation )( f )
            if isinstance( res, list ) or instance( res, Dependency ):
                return res
            else:
                Error( "Handler '%s' did not return a valid dependency object or list of dependencies handling file '%s'" % ( handler.get_name(), f ) )
        else:
            Error( "No handler defined for extension '%s' (file '%s')" % (ext,f) )

    def _handleTarget( t, operation ):
        fn = t.get_output_file()
        res = _handleFile( fn, operation )
        if isinstance( res, list ):
            for r in res:
                res.add_dependency( "build", t )
        elif isinstance( res, Dependency ):
            res.add_dependency( "build", t )
        else:
            Error( "Handler '%s' did not return a valid dependency object or list of dependencies handling target '%s'" % ( handler.get_name(), t.get_output_file() ) )
        return res

    def __init__( self, name, rules = None, features = None, variables = None, extensions = None, funcs = None ):
        self.name = name
        self.rules = rules
        self.features = features
        self.variables = variables
        self.extensions = extensions
        self.provided_functions = funcs
        if extensions is not None:
            for k, v in iterate( extensions ):
                curh = _extension_handlers.get( k )
                if curh is None:
                    _extension_handlers[k] = self
                else:
                    Error( "Modules '%s' and '%s' have conflicting/ambiguous handlers for extension '%s'" % ( name, curh.name, k ) )

_modules = {}
_loading_modules = []

def ProcessFiles( operation, *args ):
    ret = []
    for f in args:
        if isinstance( f, Target ):
            ret.append( Module._handleTarget( f, operation ) )
        if isinstance( f, (str, basestring) ):
            ret.append( Module._handleFile( f, operation ) )
        elif isinstance( f, list ):
            ret.append( ProcessFiles( *f ) )
        elif isinstance( f, tuple ):
            for x in f:
                ret.append( ProcessFiles( x ) )
    return ret

def EnableModule( name, packageprefix=None ):
    global _modules
    global _loading_modules
    if _modules.get( name ) is not None:
        return

    if name in _loading_modules:
        Error( "Dependency cycle in modules found, please re-factor modules. module chain: %s" % _loading_modules )

    _loading_modules.append( name )
    try:
        mname = name
        if packageprefix is not None:
            mname = packageprefix + '.' + name
        else:
            mname = 'constructor.modules.' + name

        try:
            Debug( "Attempting to import module '%s' using module package path" % name )
            gmod = importlib.import_module( mname )
        except ImportError as e:
            Error( "Unable to locate module '%s' for module '%s'" % (modname,name) )

        mods = getattr( gmod, "modules" )
        if mods is not None:
            for m in mods:
                if isinstance( m, (str, basestring) ):
                    EnableModule( m )
                elif isinstance( m, list ):
                    EnableModule( m[0], m[1] )
                else:
                    Error( "Invalid dependent module specification: %s, need string or 2-element array" % m )

        m = Module( name = name, rules = getattr( gmod, "rules" ),
                    features = getattr( gmod, "features" ),
                    variables = getattr( gmod, "variables" ),
                    extensions = getattr( gmod, "extension_handlers" ),
                    funcs = getattr( gmod, "functions" ) )
        _modules[name] = m
    finally:
        _loading_modules.pop()

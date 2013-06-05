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
from .dependency import Dependency, FileDependency
from .target import Target
from .output import Debug, Error, FatalException, Info
from .utility import iterate
from .directory import CurDir

_extension_handlers = {}

class Module(object):
    """Class encapsulating a module, which defines how certain types of code are compiled, or how other things are processed"""
    def __init__( self, name, rules = None, features = None, variables = None, extensions = None, funcs = None ):
        global _extension_handlers
        self.name = name
        self.rules = rules
        self.features = features
        self.variables = variables
        self.extensions = extensions
        self.provided_functions = funcs
        if extensions is not None:
            for k, v in iterate( extensions ):
                k = k.lower()
                curh = _extension_handlers.get( k )
                if curh is None:
                    _extension_handlers[k] = self
                else:
                    Error( "Modules '%s' and '%s' have conflicting/ambiguous handlers for extension '%s'" % ( name, curh.name, k ) )

    def add_globals( self, globs, phase ):
        if self.provided_functions is not None:
            pfuncs = self.provided_functions.get( phase )
            if pfuncs is not None:
                globs.update( pfuncs )

    def dispatch_handler( self, f, base, ext ):
        return self.extensions[ext]( f, base, ext )

    def set_variables( self, curd ):
        if self.variables:
            for k, v in iterate( self.variables ):
                curd.set_variable( k, v )

def _handleFile( f ):
    global _extension_handlers
    (base, ext) = os.path.splitext( f )
    ext = ext.lower()
    handler = _extension_handlers.get( ext )
    if handler is None:
        tmp = f.lower()
        for e, h in iterate( _extension_handlers ):
            if tmp.endswith( e ):
                ext = e
                base = f[:-len(e)]
                handler = h
                break

    if handler is not None:
        res = handler.dispatch_handler( f, base, ext )
        if isinstance( res, list ) or isinstance( res, Dependency ):
            return res
        else:
            Error( "Module '%s', handling file '%s', extension handler for extension '%s' did not return a valid dependency object or list" % ( handler.name, f, ext ) )
    else:
        Error( "No handler defined for extension '%s' (file '%s')" % (ext,f) )

def ProcessFiles( *args ):
    ret = []
    for f in args:
        if isinstance( f, Target ):
            res = _handleFile( f.output_file )
            if isinstance( res, list ):
                for r in res:
                    r.add_dependency( 'build', f )
                ret.extend( res )
            else:
                res.add_dependency( 'build', f )
                ret.append( res )
        elif isinstance( f, str ):
            res = _handleFile( f )
            curd = CurDir()
            d = FileDependency( infile=os.path.join( curd.src_dir, f ), orderonly=False )
            if isinstance( res, list ):
                for r in res:
                    r.add_dependency( 'build', d )
                ret.extend( res )
            else:
                res.add_dependency( 'build', d )
                ret.append( res )
        elif isinstance( f, list ):
            ret.extend( ProcessFiles( *f ) )
        elif isinstance( f, tuple ):
            for x in f:
                ret.extend( ProcessFiles( x ) )
    return ret

_modules = {}
_loading_modules = []

def EnableModule( name, packageprefix=None ):
    global _modules
    global _loading_modules
    newmod = _modules.get( name )
    if newmod is None:
        if name in _loading_modules:
            Error( "Dependency cycle in modules found, please re-factor modules. module chain: %s" % _loading_modules )

        _loading_modules.append( name )
        try:
            if packageprefix is None:
                packageprefix = 'constructor.modules'

            try:
                Debug( "Attempting to import module '%s' using module package path" % name )
                gmod = __import__( "%s.%s" % ( packageprefix, name ), fromlist=[packageprefix] )
            except:
                FatalException( "Unable to load module '%s'" % name )

            mods = getattr( gmod, "modules" )
            if mods is not None:
                for m in mods:
                    if isinstance( m, str ):
                        EnableModule( m )
                    elif isinstance( m, list ):
                        EnableModule( m[0], m[1] )
                    else:
                        Error( "Invalid dependent module specification: %s, need string or 2-element array" % m )

            newmod = Module( name = name, rules = getattr( gmod, "rules" ),
                             features = getattr( gmod, "features" ),
                             variables = getattr( gmod, "variables" ),
                             extensions = getattr( gmod, "extension_handlers" ),
                             funcs = getattr( gmod, "functions_by_phase" ) )
            _modules[name] = newmod
        finally:
            _loading_modules.pop()
    curdir = CurDir()
    curdir.enable_module( newmod )



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
import errno
from .dependency import Dependency
from .output import Info, Debug, Error
from .utility import iterate

_curdir = None
def CurDir():
    global _curdir
    return _curdir

def SetCurDir( d ):
    global _curdir
    _curdir = d

def CurBinaryPath():
    global _curdir
    if _curdir:
        return _curdir.bin_path
    Error( "Binary Path not yet configured" )

def CurFullSourcePath():
    return CurDir().src_path

def CurRelativeSourcePath():
    return CurDir().rel_src_path

class Directory(Dependency):
    def __init__( self, path, relpath, binpath, pardir = None, globs = None ):
        super(Directory, self).__init__()
        self.pardir = pardir
        self.globs = globs
        self._cur_namespace = None
        self.src_path = path
        self.rel_src_path = relpath
        self.bin_path = binpath
        # synonym function to handle plurality
        self.add_target = self.add_targets
        # Stuff that the user really turns on / triggers via directives
        self.subdirs = {}
        self.targets = None
        self.modules = None
        self.rules = None
        self.variables = None

    def enable_module( self, mod ):
        if self.modules is None:
            self.modules = {}
        self.modules[mod.name] = mod
        if self._cur_namespace is not None:
            mod.add_globals( self._cur_namespace, "config" )
        mod.set_variables( self )

    def add_module_features( self, driver ):
        if self.modules is not None:
            for n, m in iterate( self.modules ):
                if m.features is not None:
                    for f in m.features:
                        driver.add_feature( f["name"], f["type"], f["help"], f["default"] )
        for sn, sd in iterate( self.subdirs ):
            sd.add_module_features( driver )

    def get_used_module_rules( self, recurse ):
        rl = []
        if self.modules is not None:
            for n, m in iterate( self.modules ):
                if m.rules is not None:
                    for tag, rule in iterate( m.rules ):
                        if rule.is_used():
                            rl.append( rule )
        if recurse:
            for sn, sd in iterate( self.subdirs ):
                subrl = sd.get_used_module_rules( True )
                rl.extend( subrl )
        return rl

    def get_variable( self, varname, expand=True ):
        if self.variables:
            lu = self.variables.get( varname )
            if expand:
                recurval = '$' + varname
                if lu and self.pardir and recurval in lu:
                    parlu = self.pardir.get_variable( varname, expand )
                    if parlu:
                        for x in range( 0, len(lu) ):
                            if lu[x] == recurval:
                                lu[x:x+1] = parlu
        if lu:
            return lu
        if self.pardir:
            return self.pardir.get_variable( varname, expand )
        return None

    def set_variable( self, varname, f ):
        if self.variables is None:
            self.variables = { varname: f }
        else:
            self.variables[varname] = f

    def add_to_variable( self, varname, f ):
        if not isinstance( f, list ):
            if isinstance( f, tuple ):
                tl = []
                for x in f:
                    tl.append( x )
                f = tl
            else:
                f = [ f ]

        if self.variables is None:
            self.variables = { varname: f }
        else:
            v = self.variables.get( varname )
            if v is None:
                self.variables[varname] = f
            else:
                for x in f:
                    v.append( x )

    def transform_variable( self, name, val ):
        if self.modules is not None:
            for n, m in iterate( self.modules ):
                val = m.transform_variable( name, val )
        if self.pardir is not None:
            val = self.pardir.transform_variable( name, val )
        return val

    def set_globals( self, globs ):
        self.globs = globs

    def get_globals( self, phase=None ):
        retval = self.globs
        if retval is None:
            retval = self.pardir.get_globals( phase )

        if phase is None:
            return retval

        retval = retval.copy()
        if self.modules is not None:
            for k, v in iterate( self.modules ):
                v.add_globals( retval, phase )
        self._cur_namespace = retval
        return retval

    def set_bin_dir( self, path ):
        self.bin_path = path
        if len(self.rel_src_path) > 0:
            self.bin_path = os.path.join( path, self.rel_src_path )
        for sn, sd in iterate( self.subdirs ):
            sd.set_bin_dir( path )

    def get_root_bin_dir( self ):
        if self.pardir is None:
            return self.bin_path
        return self.pardir.get_root_bin_dir()

    def add_targets( self, *targs ):
        if self.targets is None:
            self.targets = []

        for t in targs:
            if isinstance( t, list ):
                for x in t:
                    self.targets[:] = [y for y in self.targets if y is not x]
                    self.targets.append( x )
            else:
                self.targets[:] = [x for x in self.targets if x is not t]
                self.targets.append( t )

    def add_sub_dir( self, name ):
        try:
            dobj = self.subdirs[name]
        except KeyError as e:
            newd = self.rel_src_path
            if len(newd) > 0:
                newd = os.path.join( newd, name )
            else:
                newd = name

            dobj = Directory( os.path.join( self.src_path, name ),
                              newd,
                              os.path.join( self.bin_path, name ),
                              self )

            if self.bin_path:
                dobj.set_bin_dir( self.get_root_bin_dir() )

            self.add_dependency( dobj )
            self.subdirs[name] = dobj

        return dobj

    def add_to_globals( self, phase, globnames, locnames ):
        if self.globs is None:
            self.globs = self.get_globals().copy()

        phaseglobs = self.get_globals( phase )
        new_globs = {}
        def _search_globs( ns ):
            all_names = ns.get( "__all__" )
            if all_names is None:
                for k, v in iterate( ns ):
                    if k[0] != '_' and k not in phaseglobs:
                        Debug( "Adding '%s' to globals" % k )
                        new_globs[k] = v
            else:
                for k in all_names:
                    if k not in phaseglobs:
                        Debug( "Adding '%s' to globals" % k )
                        new_globs[k] = ns[k]
        _search_globs( globnames )
        _search_globs( locnames )
        self.globs.update( new_globs )


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
from .dependency import Dependency
from .output import Info
from .utility import iterate

_curdir = None
def CurDir():
    global _curdir
    return _curdir

def SetCurDir( d ):
    global _curdir
    _curdir = d

class Directory(Dependency):
    def __init__( self, path, relpath, binpath, pardir = None, globs = None ):
        super(Directory, self).__init__( False )
        self.pardir = pardir
        self.globs = globs
        self._cur_namespace = None
        self.src_dir = path
        self.rel_src_dir = relpath
        self.bin_path = binpath
        # Stuff that the user really turns on / triggers via directives
        self.subdirs = {}
        self.targets = None
        self.modules = None
        self.rules = None
        self.variables = None

    def enableModule( self, mod ):
        if self.modules is None:
            self.modules = {}
        self.modules[mod.name] = mod
        if self._cur_namespace is not None:
            mod.addGlobals( self._cur_namespace, "config" )

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
                v.addGlobals( retval, phase )
        self._cur_namespace = retval
        return retval

    def set_bin_dir( self, path ):
        self.bin_path = path
        if len(self.rel_src_dir) > 0:
            self.bin_path = os.path.join( path, self.rel_src_dir )
        for sd in self.subdirs:
            sd.set_bin_dir( path )

    def make_bin_tree( self):
        Info( "make bin tree: %s" % self.bin_path )

    def add_sub_dir( self, name ):
        try:
            dobj = self.subdirs[name]
        except KeyError as e:
            newd = self.rel_src_dir
            if len(newd) > 0:
                newd = os.path.join( newd, name )
            else:
                newd = name

            dobj = Directory( os.path.join( self.src_dir, name ),
                              newd,
                              os.path.join( self.bin_path, name ),
                              self )
            self.add_dependency( "config", dobj )
            self.subdirs[name] = dobj

        return dobj

    def addToGlobals( self, phase, namespace ):
        if self.globs is None:
            self.globs = self.get_globals().copy()

        phaseglobs = self.get_globals( phase )
        new_globs = {}
        all_names = namespace.get( "__all__" )
        if all_names is None:
            for k, v in iterate( namespace ):
                if k[0] != '_' and k not in phaseglobs:
                    new_globs[k] = v
        else:
            for k in all_names:
                if k not in phaseglobs:
                    new_globs[k] = namespace[k]
        self.globs.update( new_globs )

    def _configPassPostProc( phase, curdir, namespace ):
        curdir.addToGlobals( phase, namespace )


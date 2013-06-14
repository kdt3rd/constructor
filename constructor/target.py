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
from .output import Debug, Error


class Target(Dependency):
    def __init__( self, targtype, shortname, outfile = None, rule = None, srcdir = None ):
        super(Target, self).__init__()
        self.type = targtype
        self.name = shortname
        self.variables = None
        self.rule = rule
        self.src_dir = srcdir
        if rule:
            rule.add_use()
        self.filename = outfile

    def set_variable( self, name, val ):
        if self.variables is None:
            self.variables = {}
        self.variables[name] = []
        self.add_to_variable( name, val )

    def add_to_variable( self, name, val ):
        if self.variables is None:
            self.variables = {}
        if isinstance( val, tuple ):
            for x in val:
                self.add_to_variable( name, x )
        elif isinstance( val, list ):
            v = self.variables.get( name )
            if not v:
                v = [ '$' + name ]
                self.variables[name] = v
            v.extend( val )
        else:
            v = self.variables.get( name )
            if not v:
                v = [ '$' + name ]
                self.variables[name] = v
            v.append( val )

###
### Have a global symbol table of targets so we can
### enable short names if the user wants to find targets
### by name, potentially out of order of creation
###

_symbol_table = {}

def GetTarget( targtype, name, curdir = None ):
    global _symbol_table
    symName = targtype + ":" + name
    pt = _symbol_table.get( symName )
    if pt is None:
        Debug( "Creating pseudo target %s" % symName )
        pt = Target( targtype=targtype, shortname=name, srcdir=curdir )
        if curdir:
            curdir.add_target( pt )
        _symbol_table[symName] = pt
    else:
        Debug( "Retrieved (pseudo?) target %s" % symName )
    if curdir and not pt.src_dir:
        pt.src_dir = curdir
        curdir.add_target( pt )
    return pt

def AddTarget( curdir, targtype, name, target = None, outpath = None, rule = None ):
    if target is None:
        target = GetTarget( targtype, name, curdir )

    target.filename = outpath
    target.rule = rule
    if rule:
        rule.add_use()

    return target

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


from .output import Error, Debug
from .target import Target

class PseudoTarget(Target):
    def __init__( self, targtype=None, name=None, target=None ):
        super(PseudoTarget, self).__init__( outfile=None, rule=None, orderonly=False )
        self.target = target
        self.target_type = targtype
        self.name = name

    def set_target( self, targ ):
        if self.target is not None:
            Error( "Attempt to store duplicate target %s : %s" % (self.target_type, self.name) )
        self.target = targ
        if len(targ._dependencies) == 0:
            self.target._dependencies = self._dependencies
            self._dependencies = {}
        elif len(self._dependencies) > 0:
            Error( "Need to handle joining dependencies together" )

    def add_dependency( self, group, dep ):
        if self.target:
            self.target.add_dependency( group, dep )
        else:
            super(PseudoTarget, self).add_dependency( group, dep )

    def dependencies( self, group ):
        if self.target:
            return self.target.dependencies( group )
        return super(PseudoTarget, self).dependencies( group )

_symbol_table = {}

def GetTarget( targtype, name ):
    global _symbol_table
    symName = targtype + ":" + name
    pt = _symbol_table.get( symName )
    if pt is None:
        Debug( "Creating pseudo target %s" % symName )
        pt = PseudoTarget( targtype=targtype, name=name )
        _symbol_table[symName] = pt
    return pt

def AddTarget( targtype, name, target = None, outpath = None, rule = None ):
    if target is None:
        if outpath and rule:
            target = Target( outfile=outpath, rule=rule )
        else:
            Error( "No target or output / rule combination provided" )
    if target is None:
        Error( "Invalid target provided" )
    pt = GetTarget( targtype, name )
    pt.set_target( target )
    return pt

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

from .phase import Phase
from .output import Debug, Error
from .utility import iterate

import constructor.generators

class Generator(Phase):
    _generators = {}
    def __init__( self, name, flatten ):
        super( Generator, self ).__init__( name )
        self.flatten = flatten

def AddGeneratorClass( name, classRef ):
    g = Generator._generators.get( name )
    if g is not None:
        if g is not classRef:
            Debug( "Overriding previous generator class '%s' with new class" % name )
    Generator._generators[name] = classRef
    
def LoadGenerator( classname, modname=None, package=None ):
    if package is None:
        package = 'constructor.generators'
    if modname is None:
        modname = classname.lower()

    try:
        gmod = __import__( "%s.%s" % ( package, modname ), fromlist=[package] )
    except ImportError as e:
        Error( "Unable to locate module '%s' for generator class '%s'" % (modname,classname) )

    found = False
    for k in dir( gmod ):
        if k == classname:
            v = getattr( gmod, k )
            if issubclass( v, Generator ):
                if modname is None:
                    AddGeneratorClass( classname.lower(), v )
                else:
                    AddGeneratorClass( modname.split( '.' )[-1], v )
                found = True
                break
    if not found:
        Error( "Loaded module, but unable to find generator class '%s'" % classname )

def GetGeneratorClass( name ):
    return Generator._generators.get( name )

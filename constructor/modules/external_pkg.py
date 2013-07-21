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
import sys
import subprocess
from constructor.utility import FindOptionalExecutable
from constructor.output import Info, Debug, Error
from constructor.target import Target
from constructor.driver import CurDir
from constructor.cobject import CObject

class _ExternalPackage(CObject):
    def __init__( self, name, cflags = None, iflags = None, lflags = None, ver = None ):
        super(_ExternalPackage, self).__init__( 'syslib' )
        self.name = name
        self.cflags = cflags
        self.iflags = iflags
        self.lflags = lflags
        self.version = ver

# pkg-config may exist even under windows
_pkgconfig = FindOptionalExecutable( "pkg-config" )
_externResolvers = []
_externPackages = {}

def _AddExternalResolver( f ):
    global _externResolvers
    _externResolvers.append( f )

def _FindExternalLibrary( name, version=None ):
    global _pkgconfig
    global _externPackages
    global _externResolvers
    if sys.platform.startswith( 'darwin' ):
        possible = [ os.path.join( os.path.sep, 'System', 'Library', 'Frameworks', name + '.framework' ),
                     os.path.join( os.path.sep, 'Library', 'Frameworks', name + '.framework' )
                   ]
        for frm in possible:
            if os.path.exists( frm ):
                Debug( "Found Apple Framework '%s' in %s" % ( name, frm ) )
                e = _ExternalPackage( name=name,
                                      iflags=[ '-I', os.path.join( frm, 'Headers' ) ],
                                      lflags=[ '-framework', name ] )
                _externPackages[name] = e
                return e

    if _pkgconfig is not None:
        try:
            lib = name
            if version is not None:
                lib += " "
                lib += version
            Debug( "Running pkg-config with library name/version '%s'" % lib )
            # universal_newlines opens it as a text stream instead of (encoded)
            # byte stream...
            if sys.platform == "win32" or sys.platform == "win64":
                raise NotImplementedError
            devnull = open('/dev/null', 'w')
            def runPkgConfig( outflag ):
                args = [_pkgconfig, outflag, lib]
                proc = subprocess.Popen( args, stdout=subprocess.PIPE, stderr=devnull, universal_newlines=True )
                return proc.stdout.read()
            ver = runPkgConfig( '--modversion' )
            cflags = runPkgConfig( '--cflags-only-other' )
            iflags = runPkgConfig( '--cflags-only-I' )
            lflags = runPkgConfig( '--libs' )
            devnull.close()
            e = _ExternalPackage( name=name, cflags=cflags.strip().split(), iflags=iflags.strip().split(), lflags=lflags.strip().split(), ver=ver.strip() )
            _externPackages[name] = e
            return e
        except Exception as e:
            Info( "pkg-config returned an error: %s" % str(e) )
            pass

    # still here, pkg-config failed
    # try the user provided external resolver if specified
    for e in _externResolvers:
        try:
            retval = e( name, version )
            _externPackages[name] = retval
            if retval is not None:
                return retval
        except:
            pass

    # Nothing found so far, check system path
    if sys.platform == "win32" or sys.platform == "win64":
        raise NotImplementedError

    libso = os.path.join( os.path.sep, 'usr', 'local', 'lib', 'lib' + name + '.so' )
    libstat = os.path.join( os.path.sep, 'usr', 'local', 'lib', 'lib' + name + '.a' )
    if os.path.exists( libso ) or os.path.exists( libstat ):
        e = _ExternalPackage( name=name, iflags=[ '-I', '/usr/local/include'], lflags=['-L', '/usr/local/lib', '-l'+name ] )
        _externPackages[name] = e
        return e

    libso = os.path.join( os.path.sep, 'usr', 'lib', 'lib' + name + '.so' )
    libstat = os.path.join( os.path.sep, 'usr', 'lib', 'lib' + name + '.a' )
    if os.path.exists( libso ) or os.path.exists( libstat ):
        e = _ExternalPackage( name=name, lflags=[ '-l' + name ] )
        _externPackages[name] = e
        return e

    # Nothing found
    return None

def _HaveExternal( name ):
    global _externPackages
    if name in _externPackages:
        return True
    return False

functions_by_phase = { "config":
                       {
                           "AddExternalResolver": _AddExternalResolver,
                           "FindExternalLibrary": _FindExternalLibrary,
                           "HaveExternal": _HaveExternal,
                       },
                       "build":
                       {
                           "HaveExternal": _HaveExternal,
                       }
                     }

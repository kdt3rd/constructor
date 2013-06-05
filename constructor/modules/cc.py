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
from constructor.utility import FindOptionalExecutable, CheckEnvironOverride
from constructor.output import Info, Debug, Error, Warn
from constructor.dependency import Dependency, FileDependency
from constructor.driver import CurDir
from constructor.module import ProcessFiles
from constructor.rule import Rule
from constructor.pseudotarget import PseudoTarget, GetTarget, AddTarget

variables = {}

_objExt = '.o'
_exeExt = ''

if sys.platform.startswith( "linux" ):
    variables["AR"] = CheckEnvironOverride( "AR", FindOptionalExecutable( "ar" ) )
    variables["LD"] = CheckEnvironOverride( "LD", FindOptionalExecutable( "ld" ) )
    variables["CC"] = CheckEnvironOverride( "CC", FindOptionalExecutable( "gcc" ) )
    variables["CXX"] = CheckEnvironOverride( "CXX", FindOptionalExecutable( "g++" ) )
    _depFlags = [ '-MMD', '-MF', '$out.d' ]
elif sys.platform.startswith( "darwin" ):
    variables["AR"] = CheckEnvironOverride( "AR", FindOptionalExecutable( "ar" ) )
    variables["LD"] = CheckEnvironOverride( "LD", FindOptionalExecutable( "ld" ) )
    variables["CC"] = CheckEnvironOverride( "CC", FindOptionalExecutable( "clang" ) )
    variables["CXX"] = CheckEnvironOverride( "CXX", FindOptionalExecutable( "clang++" ) )
    _depFlags = [ '-MMD', '-MF', '$out.d' ]
elif sys.platform.startswith( "win" ):
    variables["CC"] = CheckEnvironOverride( "CC", FindOptionalExecutable( "cl" ) )
    variables["CXX"] = CheckEnvironOverride( "CXX", FindOptionalExecutable( "cl" ) )
    _objExt = '.obj'
    _exeExt = '.exe'
    _depFlags = []
else:
    Warn( "Un-handled platform '%s', defaulting compiler to gcc" % sys.platform )
    variables["AR"] = CheckEnvironOverride( "AR", FindOptionalExecutable( "ar" ) )
    variables["LD"] = CheckEnvironOverride( "LD", FindOptionalExecutable( "ld" ) )
    variables["CC"] = CheckEnvironOverride( "CC", FindOptionalExecutable( "gcc" ) )
    variables["CXX"] = CheckEnvironOverride( "CXX", FindOptionalExecutable( "g++" ) )
    _depFlags = [ '-MMD', '-MF', '$out.d' ]

variables["CFLAGS"] = CheckEnvironOverride( "CFLAGS", [] )
variables["CXXFLAGS"] = CheckEnvironOverride( "CXXFLAGS", [] )
variables["WARNINGS"] = CheckEnvironOverride( "WARNINGS", [] )
variables["CWARNINGS"] = CheckEnvironOverride( "CWARNINGS", [] )
variables["CXXWARNINGS"] = CheckEnvironOverride( "CXXWARNINGS", [] )
variables["INCLUDE"] = CheckEnvironOverride( "INCLUDE", [] )
variables["ARFLAGS"] = CheckEnvironOverride( "ARFLAGS", [] )
variables["LDFLAGS"] = CheckEnvironOverride( "LDFLAGS", [] )
variables["RPATH"] = CheckEnvironOverride( "RPATH", [] )

_cppCmd = ['$CXX', '$CXXFLAGS', '$WARNINGS', '$CXXWARNINGS', '$INCLUDE']
_ccCmd = ['$CCC', '$CFLAGS', '$WARNINGS', '$CWARNINGS', '$INCLUDE']
if len(_depFlags) > 0:
    _cppCmd.extend( _depFlags )
    _ccCmd.extend( _depFlags )
_cppCmd.extend( ['-c', '$in', '-o', '$out'] )
_ccCmd.extend( ['-c', '$in', '-o', '$out'] )

modules = [ "external_pkg" ]
rules = {
    'cpp': Rule( tag='cpp', cmd=_cppCmd, desc='C++ ($in)', depfile='$out.d' ),
    'cc': Rule( tag='cc', cmd=_ccCmd, desc='C ($in)', depfile='$out.d' ),
    'exe': Rule( tag='exe',
                 cmd=['$LD', '$RPATH', '$LDFLAGS', '$in', '$libs', '-o', '$out'],
                 desc='Linking ($out)' ),
    'lib_static': Rule( tag='lib_static',
                        cmd=['rm', '-f', '$out', ';', '$AR', '-c', '$in', '-o', '$out'],
                        desc='Static ($out)' ),
    'lib_shared': Rule( tag='lib_shared',
                        cmd=['$CC', '$CFLAGS', '$WARNINGS', '$INCLUDE', '-c', '$in', '-o', '$out'],
                        desc='Shared ($out)' )
    }

features = [ { "name": "shared", "type": "boolean", "help": "Build shared libraries", "default": True },
             { "name": "static", "type": "boolean", "help": "Build static libraries", "default": True } ]

def _SetCompiler( cc=None, cxx=None ):
    if cc is not None:
        if os.environ.get( "CC" ):
            Info( "Compiler '%s' overridden with environment flag to '%s'" % (cc, os.environ.get( "CC" )) )
        else:
            CurDir().set_variable( "CC", FindExecutable( cc ) )
    if cxx is not None:
        if os.environ.get( "CC" ):
            Info( "Compiler '%s' overridden with environment flag to '%s'" % (cc, os.environ.get( "CC" )) )
        else:
            CurDir().set_variable( "CXX", FindExecutable( cxx ) )

_defBuildShared = True
_defBuildStatic = True
def _SetDefaultLibraryStyle( style ):
    global _defBuildShared
    global _defBuildStatic
    if style == "both":
        _defBuildShared = True
        _defBuildStatic = True
    elif style == "shared":
        _defBuildShared = True
        _defBuildStatic = False
    elif style == "static":
        _defBuildShared = False
        _defBuildStatic = True
    else:
        Error( "Unknown default library build style '%s'" % style)

def _CFlags( *f ):
    CurDir().add_to_variable( "CFLAGS", f )
    pass

def _CXXFlags( *f ):
    CurDir().add_to_variable( "CXXFLAGS", f )
    pass

def _Warnings( *f ):
    CurDir().add_to_variable( "WARNINGS", f )
    pass

def _Include( *f ):
    CurDir().add_to_variable( "INCLUDE", f )
    pass

def _Library( *f ):
    name = None
    objs = []
    other = []
    for a in f:
        if isinstance( a, str ):
            if name:
                Error( "Multiple names specified creating library not allowed" )
            name = a
        elif isinstance( a, PseudoTarget ):
            if a.target_type == "object":
                objs.append( a )
            else:
                other.append( a )

    pass

class _ExeTargetInfo(object):
    def __init__( self ):
        self.name = None
        self.objs = []
        self.libs = []
        self.syslibs = []
        self.other = []

    def extract( self, *f ):
        for a in f:
            if isinstance( a, str ):
                if self.name:
                    Error( "Multiple names specified creating library not allowed" )
                self.name = a
            elif isinstance( a, PseudoTarget ):
                if a.target_type == "object":
                    self.objs.append( a )
                elif a.target_type == "lib":
                    self.libs.append( a )
                elif a.target_type == "syslib":
                    self.syslibs.append( a )
                else:
                    self.other.append( a )
            elif isinstance( a, list ):
                for x in a:
                    self.extract( x )
            elif isinstance( a, tuple ):
                for x in a:
                    self.extract( x )

def _Executable( *f ):
    einfo = _ExeTargetInfo();
    einfo.extract( f )

    curd = CurDir()
    out = os.path.join( curd.bin_path, einfo.name ) + _exeExt
    e = AddTarget( "exe", einfo.name, outpath=out, rule=rules["exe"] )
    for o in einfo.objs:
        e.add_dependency( "build", o )
    CurDir().add_targets( e )
    #for l in einfo.libs:
    pass

def _OptExecutable( *f ):
    pass

def _Compile( *f ):
    ret = ProcessFiles( f )
    CurDir().add_targets( ret )
    return ret

def _CPPTarget( f, base, ext ):
    Debug( "Processing C++ target '%s'" % f )
    curd = CurDir()
    out = os.path.join( curd.bin_path, base ) + _objExt
    return AddTarget( "object", out, outpath=out, rule=rules['cpp'] )

def _CTarget( f, base, ext ):
    Debug( "Processing C target '%s'" % f )
    curd = CurDir()
    out = os.path.join( curd.bin_path, base ) + _objExt
    return AddTarget( "object", out, outpath=out, rule=rules['cc'] )

def _NilTarget( f, base, ext ):
    Debug( "Processing nil target '%s'" % f )
    return FileDependency( infile=f, orderonly=False )

extension_handlers = {
    ".cpp": _CPPTarget,
    ".c": _CTarget,
    ".o": _NilTarget,
}

functions_by_phase = {
    "config":
    {
        "SetCompiler": _SetCompiler,
        "SetDefaultLibraryStyle": _SetDefaultLibraryStyle,
        "CFlags": _CFlags,
        "CXXFlags": _CXXFlags,
        "Warnings": _Warnings,
    },
    "build":
    {
        "Library": _Library,
        "Executable": _Executable,
        "OptExecutable": _OptExecutable,
        "Compile": _Compile,
        "CFlags": _CFlags,
        "CXXFlags": _CXXFlags,
        "Warnings": _Warnings,
    }
}

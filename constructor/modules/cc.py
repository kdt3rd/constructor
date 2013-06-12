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
from constructor.driver import CurDir, Feature
from constructor.module import ProcessFiles
from constructor.rule import Rule
from constructor.target import Target, GetTarget, AddTarget

variables = {}

_objExt = '.o'
_exeExt = ''
_libPrefix = 'lib'
_staticLibSuffix = '.a'
_sharedLibSuffix = '.so'

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
    _sharedLibSuffix = '.dylib'
elif sys.platform.startswith( "win" ):
    variables["CC"] = CheckEnvironOverride( "CC", FindOptionalExecutable( "cl" ) )
    variables["CXX"] = CheckEnvironOverride( "CXX", FindOptionalExecutable( "cl" ) )
    _objExt = '.obj'
    _exeExt = '.exe'
    _libPrefix = ''
    _staticLibSuffix = '.lib'
    _sharedLibSuffix = '.dll'
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

def _prependToListSkipVars( pre, out, val ):
    if isinstance( val, list ):
        for v in val:
            _prependToListSkipVars( pre, out, v )
    elif val.startswith( '$' ):
        out.append( val )
    else:
        out.append( pre + val )

def _TransformVariable( name, val ):
    if not isinstance( val, list ):
        return val
    if len(val) == 0:
        return val

    if name == "WARNINGS" or name == "CWARNINGS" or name == "CXXWARNINGS":
        nv = []
        _prependToListSkipVars( '-W', nv, val )
        Debug( "Tranform '%s': %s  -> %s" % (name, val, nv) )
        return nv
    return val

variable_transformer = _TransformVariable

_cppCmd = ['$CXX', '$CXXFLAGS', '$WARNINGS', '$CXXWARNINGS', '$INCLUDE']
_ccCmd = ['$CC', '$CFLAGS', '$WARNINGS', '$CWARNINGS', '$INCLUDE']
if len(_depFlags) > 0:
    _cppCmd.extend( _depFlags )
    _ccCmd.extend( _depFlags )
_cppCmd.extend( ['-c', '$in', '-o', '$out'] )
_ccCmd.extend( ['-c', '$in', '-o', '$out'] )

modules = [ "external_pkg" ]
rules = {
    'cpp': Rule( tag='cpp', cmd=_cppCmd, desc='C++ ($in)', depfile='$out.d' ),
    'cc': Rule( tag='cc', cmd=_ccCmd, desc='C ($in)', depfile='$out.d' ),
    'c_exe': Rule( tag='c_exe',
                 cmd=['$CC', '$RPATH', '$LDFLAGS', '-o', '$out', '$in', '$libs'],
                 desc='EXE ($out)' ),
    'cxx_exe': Rule( tag='cxx_exe',
                 cmd=['$CXX', '$RPATH', '$LDFLAGS', '-o', '$out', '$in', '$libs'],
                 desc='EXE C++ ($out)' ),
    'lib_static': Rule( tag='lib_static',
                        cmd=['rm', '-f', '$out', ';', '$AR', '-c', '$in', '-o', '$out'],
                        desc='Static ($out)' ),
    'lib_shared': Rule( tag='lib_shared',
                        cmd=['$CC', '$CFLAGS', '$WARNINGS', '$INCLUDE', '-c', '$in', '-o', '$out'],
                        desc='Shared ($out)' ),
    'cxx_lib_shared': Rule( tag='lib_shared',
                        cmd=['$CC', '$CFLAGS', '$WARNINGS', '$INCLUDE', '-c', '$in', '-o', '$out'],
                        desc='Shared ($out)' )
    }

_defBuildShared = True
_defBuildStatic = True

features = [ { "name": "shared", "type": "boolean", "help": "Build shared libraries", "default": _defBuildShared },
             { "name": "static", "type": "boolean", "help": "Build static libraries", "default": _defBuildStatic } ]

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

class _TargetInfo(object):
    def __init__( self ):
        self.name = None
        self.objs = []
        self.libs = []
        self.syslibs = []
        self.other = []
        self.uses_cxx = False

    def check_for_cxx( self, a ):
        if self.uses_cxx:
            return

        if isinstance( a, Target ):
            if a.rule is rules['cpp']:
                self.uses_cxx = True

        if a.dependencies:
            for d in a.dependencies:
                self.check_for_cxx( d )
                if self.uses_cxx:
                    break
        if a.implicit_dependencies:
            for d in a.implicit_dependencies:
                self.check_for_cxx( d )
                if self.uses_cxx:
                    break

    def extract( self, *f ):
        for a in f:
            if isinstance( a, str ):
                if self.name:
                    Error( "Multiple names specified creating library not allowed" )
                self.name = a
            elif isinstance( a, Target ):
                if a.type == "object":
                    self.objs.append( a )
                elif a.type == "lib":
                    self.libs.append( a )
                elif a.type == "syslib":
                    self.syslibs.append( a )
                else:
                    self.other.append( a )
                self.check_for_cxx( a )
            elif isinstance( a, list ):
                for x in a:
                    self.extract( x )
            elif isinstance( a, tuple ):
                for x in a:
                    self.extract( x )
            elif isinstance( a, Dependency ):
                self.check_for_cxx( a )

def _Library( *f ):
    linfo = _TargetInfo()
    linfo.extract( f )
    curd = CurDir()
    if Feature( "static" ):
        libname = _libPrefix + linfo.name + _staticLibSuffix
        out = os.path.join( curd.bin_path, libname )
        l = AddTarget( "lib", out, outpath=out, rule=rules["lib_static"]
        for o in linfo.objs:
            l.add_dependency( o )
        for dl in linfo.libs:
            l.add_implicit_dependency( dl )
        shortl = AddTarget( "lib", einfo.name )
        shortl.add_dependency( l )
        curd.add_targets( l, shortl )
        GetTarget( "all", "all" ).add_dependency( shortl )
    if Feature( "shared" ):
        libname = _libPrefix + linfo.name + _sharedLibSuffix
        Info( "Need to add .so versioning..." )
    pass

def _Executable( *f ):
    einfo = _TargetInfo()
    einfo.extract( f )

    curd = CurDir()
    out = os.path.join( curd.bin_path, einfo.name ) + _exeExt
    if einfo.uses_cxx:
        e = AddTarget( "exe", out, outpath=out, rule=rules["cxx_exe"] )
    else:
        e = AddTarget( "exe", out, outpath=out, rule=rules["c_exe"] )
    for o in einfo.objs:
        e.add_dependency( o )
    for l in einfo.libs:
        e.add_implicit_dependency( l )
    shorte = AddTarget( "exe", einfo.name )
    shorte.add_dependency( e )

    GetTarget( "all", "all" ).add_dependency( shorte )
    CurDir().add_targets( e, shorte )

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

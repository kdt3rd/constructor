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
from constructor.version import Version
from constructor.cobject import ExtractObjects

variables = {}

_objExt = ".o"
_exeExt = ""
_libPrefix = "lib"
_staticLibSuffix = ".a"
_sharedLibSuffix = ".so"
_sharedFlag = "-shared"
if sys.platform.startswith( "linux" ):
    variables["AR"] = CheckEnvironOverride( "AR", FindOptionalExecutable( "ar" ) )
    variables["LD"] = CheckEnvironOverride( "LD", FindOptionalExecutable( "ld" ) )
    variables["CC"] = CheckEnvironOverride( "CC", FindOptionalExecutable( "gcc" ) )
    variables["CXX"] = CheckEnvironOverride( "CXX", FindOptionalExecutable( "g++" ) )
    _depFlags = [ "-MMD", "-MF", "$out.d" ]
elif sys.platform.startswith( "darwin" ):
    variables["AR"] = CheckEnvironOverride( "AR", FindOptionalExecutable( "ar" ) )
    variables["LD"] = CheckEnvironOverride( "LD", FindOptionalExecutable( "ld" ) )
    variables["CC"] = CheckEnvironOverride( "CC", FindOptionalExecutable( "clang" ) )
    variables["CXX"] = CheckEnvironOverride( "CXX", FindOptionalExecutable( "clang++" ) )
    _depFlags = [ "-MMD", "-MF", "$out.d" ]
    _sharedLibSuffix = ".dylib"
    _sharedFlag = "-dynamiclib"
elif sys.platform.startswith( "win" ):
    variables["CC"] = CheckEnvironOverride( "CC", FindOptionalExecutable( "cl" ) )
    variables["CXX"] = CheckEnvironOverride( "CXX", FindOptionalExecutable( "cl" ) )
    _objExt = ".obj"
    _exeExt = ".exe"
    _libPrefix = ""
    _staticLibSuffix = ".lib"
    _sharedLibSuffix = ".dll"
    _depFlags = []
else:
    Warn( "Un-handled platform '%s', defaulting compiler to gcc" % sys.platform )
    variables["AR"] = CheckEnvironOverride( "AR", FindOptionalExecutable( "ar" ) )
    variables["LD"] = CheckEnvironOverride( "LD", FindOptionalExecutable( "ld" ) )
    variables["CC"] = CheckEnvironOverride( "CC", FindOptionalExecutable( "gcc" ) )
    variables["CXX"] = CheckEnvironOverride( "CXX", FindOptionalExecutable( "g++" ) )
    _depFlags = [ "-MMD", "-MF", "$out.d" ]

variables["CFLAGS"] = CheckEnvironOverride( "CFLAGS", [] )
variables["CXXFLAGS"] = CheckEnvironOverride( "CXXFLAGS", [] )
variables["WARNINGS"] = CheckEnvironOverride( "WARNINGS", [] )
variables["CWARNINGS"] = CheckEnvironOverride( "CWARNINGS", [] )
variables["CXXWARNINGS"] = CheckEnvironOverride( "CXXWARNINGS", [] )
variables["INCLUDE"] = CheckEnvironOverride( "INCLUDE", [] )
variables["ARFLAGS"] = CheckEnvironOverride( "ARFLAGS", ["cur"] )
variables["LDFLAGS"] = CheckEnvironOverride( "LDFLAGS", [] )
variables["RPATH"] = CheckEnvironOverride( "RPATH", [] )

def _prependToListSkipVars( pre, out, val ):
    if isinstance( val, list ):
        for v in val:
            _prependToListSkipVars( pre, out, v )
    elif val.startswith( "$" ):
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
        _prependToListSkipVars( "-W", nv, val )
        Debug( "Tranform '%s': %s  -> %s" % (name, val, nv) )
        return nv
    return val

variable_transformer = _TransformVariable

_cppCmd = ["$CXX", "$CXXFLAGS", "$WARNINGS", "$CXXWARNINGS", "$INCLUDE"]
_ccCmd = ["$CC", "$CFLAGS", "$WARNINGS", "$CWARNINGS", "$INCLUDE"]
if len(_depFlags) > 0:
    _cppCmd.extend( _depFlags )
    _ccCmd.extend( _depFlags )
_cppCmd.extend( ["-c", "$in", "-o", "$out"] )
_ccCmd.extend( ["-c", "$in", "-o", "$out"] )

modules = [ "external_pkg" ]
rules = {
    "cpp": Rule( tag="cpp", cmd=_cppCmd, desc="C++ ($in)", depfile="$out.d" ),
    "cc": Rule( tag="cc", cmd=_ccCmd, desc="C ($in)", depfile="$out.d" ),
    "c_exe": Rule( tag="c_exe",
                 cmd=["$CC", "$CFLAGS", "$RPATH", "$LDFLAGS", "-o", "$out", "$in", "$libs"],
                 desc="EXE ($out)" ),
    "cxx_exe": Rule( tag="cxx_exe",
                 cmd=["$CXX", "$CXXFLAGS", "$RPATH", "$LDFLAGS", "-o", "$out", "$in", "$libs"],
                 desc="EXE C++ ($out)" ),
    "lib_static": Rule( tag="lib_static",
                        cmd=["rm", "-f", "$out", ";", "$AR", "$ARFLAGS", "$out", "$in"],
                        desc="Static ($out)" ),
    "lib_shared": Rule( tag="lib_shared",
                        cmd=["$CC", "$CFLAGS", "$WARNINGS", "$INCLUDE", _sharedFlag, "-o", "$out", "$in", "$libs"],
                        desc="Shared ($out)" ),
    "cxx_lib_shared": Rule( tag="cxx_lib_shared",
                        cmd=["$CXX", "$CFLAGS", "$WARNINGS", "$INCLUDE", _sharedFlag, "-o", "$out", "$in", "$libs"],
                        desc="Shared ($out)" )
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

def _GetCompilerVersion():
    comp = CurDir().get_variable( "CC" )
    if comp and len(comp) > 0:
        if isinstance(comp, list):
            comp = comp[0]
        if comp.find( "gcc" ):
            regex="\(GCC\)\s+(\d+\.\d+\.\d+)"
        if comp.find( "clang" ):
            regex="version\s+(\d+\.\d+)\s+"
        return Version( binary=comp, regex=regex )
    else:
        Error( "Unable to find compiler" )

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

def _check_for_cxx( a ):
    if isinstance( a, Target ):
        if a.rule is rules["cpp"]:
            return True
        if a.dependencies:
            for d in a.dependencies:
                if _check_for_cxx( d ):
                    return True
        if a.implicit_dependencies:
            for d in a.implicit_dependencies:
                if _check_for_cxx( d ):
                    return True
    return False

def _extract_lib_name( f ):
    (head, tail) = os.path.split( f )
    if tail.startswith( _libPrefix ):
        tail = tail[len(_libPrefix):]
    if tail.endswith( _staticLibSuffix ):
        tail = tail[:-len(_staticLibSuffix)]
    if tail.endswith( _sharedLibSuffix ):
        tail = tail[:-len(_sharedLibSuffix)]
    return tail

def _Library( *f ):
    linfo = ExtractObjects( f )
    try:
        name = linfo["str"]
        if len(name) > 1:
            Error( "Only one name allowed for Library" )
        name = name[0]
    except KeyError:
        Error( "No name specified for Library" )

    objs = linfo.get( "object" )
    if objs is None:
        Error( "No object files / compiled source specified to create library" )

    for o in objs:
        o.extract_chained_usage( linfo )

    libs = linfo.get( "lib" )
    syslibs = linfo.get( "syslib" )
    uses_cxx = False
    for o in objs:
        if _check_for_cxx( o ):
            uses_cxx = True
            Info( "Library has C++ targets" )
            break

    curd = CurDir()

    if Feature( "static" ):
        libname = _libPrefix + name + _staticLibSuffix
        out = os.path.join( curd.bin_path, libname )
        l = AddTarget( curd, "lib", out, outputs=out, rule=rules["lib_static"] )
        if libs:
            for dl in libs:
                l.add_implicit_dependency( dl )
                l.add_chained_usage( dl )

        if syslibs:
            l.add_chained_usage( syslibs )

        for o in objs:
            l.add_dependency( o )

        shortl = AddTarget( curd, "lib", libname )
        shortl.add_dependency( l )
        GetTarget( "all", "all" ).add_dependency( shortl )
    if Feature( "shared" ):
        libname = _libPrefix + name + _sharedLibSuffix
        Info( "Need to add .so versioning ala -Wl,-soname=libfoo.so.1 ..." )
        out = os.path.join( curd.bin_path, libname )
        if uses_cxx:
            l = AddTarget( curd, "lib", out, outputs=out, rule=rules["cxx_lib_shared"] )
        else:
            l = AddTarget( curd, "lib", out, outputs=out, rule=rules["lib_shared"] )

        for o in objs:
            l.add_dependency( o )

        if libs:
            for dl in libs:
                l.add_implicit_dependency( dl )
                l.add_to_variable( "libs", ("-L", dl.src_dir.bin_path, "-l" + _extract_lib_name( dl.name ) ) )
                l.add_chained_usage( dl )

        if syslibs:
            for dl in syslibs:
                if dl.lflags:
                    l.add_to_variable( "libs", dl.lflags )
                l.add_chained_usage( dl )

        shortl = AddTarget( curd, "lib", libname )
        shortl.add_dependency( l )
        GetTarget( "all", "all" ).add_dependency( shortl )

    return l

def _Executable( *f ):
    einfo = ExtractObjects( f )
    try:
        name = einfo["str"]
        if len(name) > 1:
            Error( "Only one name allowed for Executable" )
        name = name[0]
    except KeyError:
        Error( "No name specified for Executable" )

    curd = CurDir()
    out = os.path.join( curd.bin_path, name ) + _exeExt
    objs = einfo.get( "object" )
    if objs is None:
        Error( "No object files / compiled source specified to create executable" )
    uses_cxx = False
    for o in objs:
        o.extract_chained_usage( einfo )
        if _check_for_cxx( o ):
            uses_cxx = True
            break

    if uses_cxx:
        e = AddTarget( curd, "exe", out, outputs=out, rule=rules["cxx_exe"] )
    else:
        e = AddTarget( curd, "exe", out, outputs=out, rule=rules["c_exe"] )

    libs = einfo.get( "lib" )
    if libs:
        for dl in libs:
            e.add_implicit_dependency( dl )
            e.add_to_variable( "libs", [ "-L", dl.src_dir.bin_path, "-l" + _extract_lib_name( dl.name ) ] )
    else:
        Debug( "Executable '%s' has no internal libs" % name )

    syslibs = einfo.get( "syslib" )
    if syslibs:
        for dl in syslibs:
            if dl.cflags:
                if uses_cxx:
                    e.add_to_variable( "CXXFLAGS", dl.cflags )
                else:
                    e.add_to_variable( "CFLAGS", dl.cflags )
            if dl.iflags:
                e.add_to_variable( "INCLUDE", dl.iflags )
            if dl.lflags:
                e.add_to_variable( "libs", dl.lflags )
    else:
        Debug( "Executable '%s' has no syslibs" % name )

    for o in objs:
        e.add_dependency( o )

    shorte = AddTarget( curd, "exe", name )
    shorte.add_dependency( e )

    GetTarget( "all", "all" ).add_dependency( shorte )
    pass

def _OptExecutable( *f ):
    pass

def _Compile( *f ):
    cinfo = ExtractObjects( f )
    plainsrcs = cinfo.get( "str" )
    if plainsrcs:
        objs = ProcessFiles( plainsrcs )
    else:
        objs = []

    gensrc = cinfo.get( "source" )
    if gensrc:
        objs.extend( ProcessFiles( gensrc ) )

    targcflags = []
    targiflags = []
    libs = cinfo.get( "lib" )
    if libs:
        for dl in libs:
            targiflags.append( "-I" )
            targiflags.append( dl.src_dir.src_path )
            targiflags.append( "-I" )
            targiflags.append( dl.src_dir.bin_path )

    syslibs = cinfo.get( "syslib" )
    if syslibs:
        for dl in syslibs:
            if dl.cflags:
                targcflags.extend( dl.cflags )
            if dl.iflags:
                targiflags.extend( dl.iflags )

    for o in objs:
        if len(targcflags) > 0:
            if _check_for_cxx( o ):
                o.add_to_variable( "CXXFLAGS", targcflags )
            else:
                o.add_to_variable( "CFLAGS", targcflags )
        if len(targiflags) > 0:
            o.add_to_variable( "INCLUDE", targiflags )
        if libs:
            o.add_chained_usage( libs )
        if syslibs:
            o.add_chained_usage( syslibs )

    fixedobjs = cinfo.get( "object" )
    if fixedobjs:
        objs.extend( fixedobjs )
    
    return objs

def _CPPTarget( f, base, ext ):
    Debug( "Processing C++ target '%s'" % f )
    curd = CurDir()
    if os.path.isabs( f ):
        if f.startswith( curd.bin_path ):
            out = base + _objExt
        else:
            Error( "Unable to handle absolute paths '%s' in CPP Target" % f )
    else:
        out = os.path.join( curd.bin_path, base ) + _objExt
    return AddTarget( curd, "object", out, outputs=out, rule=rules["cpp"] )

def _CTarget( f, base, ext ):
    Debug( "Processing C target '%s'" % f )
    curd = CurDir()
    if os.path.isabs( f ):
        if f.startswith( curd.bin_path ):
            out = base + _objExt
        else:
            Error( "Unable to handle absolute paths '%s' in C Target" % f )
    else:
        out = os.path.join( curd.bin_path, base ) + _objExt
    out = os.path.join( curd.bin_path, base ) + _objExt
    return AddTarget( curd, "object", out, outputs=out, rule=rules["cc"] )

def _ObjTarget( f, base, ext ):
    Debug( "Processing no-op pass through target '%s'" % f )
    return FileDependency( infile=f, objtype="object" )

def _HeaderTarget( f, base, ext ):
    Debug( "Processing no-op pass through target '%s'" % f )
    return FileDependency( infile=f, objtype="header" )

extension_handlers = {
    ".cpp": _CPPTarget,
    ".cc": _CPPTarget,
    ".C": _CPPTarget,
    ".c": _CTarget,
    _objExt: _ObjTarget,
    ".h": _HeaderTarget,
}

functions_by_phase = {
    "config":
    {
        "SetCompiler": _SetCompiler,
        "GetCompilerVersion": _GetCompilerVersion,
        "SetDefaultLibraryStyle": _SetDefaultLibraryStyle,
        "CFlags": _CFlags,
        "CXXFlags": _CXXFlags,
        "Warnings": _Warnings,
    },
    "build":
    {
        "GetCompilerVersion": _GetCompilerVersion,
        "Library": _Library,
        "Executable": _Executable,
        "OptExecutable": _OptExecutable,
        "Compile": _Compile,
        "CFlags": _CFlags,
        "CXXFlags": _CXXFlags,
        "Warnings": _Warnings,
    }
}

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
#

import os
import string
import errno
import sys

from ..output import Info, Debug, Error
from ..generator import Generator
from ..directory import Directory
from ..utility import iterate, GetEnvironOverrides, FileOutput, FindExecutable
from ..dependency import Dependency, FileDependency
from ..target import Target
from ..version import Version

###
### NB: We use the fileout class from utility to provide transparent
### encoding such that python 2.x and 3 work, but don't have to have
### encodes all over the place
###
def _escape( s ):
    assert '\n' not in s, 'ninja syntax does not allow newlines'
    return s.replace( '$', '$$' )

def _escape_path( f ):
    return f.replace( '$ ', '$$ ' ).replace( ' ', '$ ' ).replace( ':', '$:' )

class NinjaWriter(FileOutput):
    def __init__( self, fn ):
        super(NinjaWriter, self).__init__( fn )

    def emit_rules( self, rules, version=None ):
        if rules is None:
            return

        newdeps = False
        if version and version >= "1.3":
            if version != "git":
                newdeps = True
        for rule in rules:
            self.write( 'rule %s\n' % rule.name )
            self.write( '  command = %s\n' % rule.command )
            if rule.description:
                self.write( '  description = %s\n' % rule.description )
            if rule.dependency_file:
                self.write( '  depfile = %s\n' % rule.dependency_file )
                if newdeps:
                    self.write( '  deps = gcc\n' )
                    if sys.platform.startswith( "win" ):
                        Error( "Need to add appropriate check whether to emit deps = msvc instead" )
            if rule.check_if_changed:
                self.write( '  restat = 1\n' )
            self.write( '\n' )
        pass

    def emit_variables( self, curdir ):
        if curdir.variables is None:
            return
        for k, v in iterate( curdir.variables ):
            self.write( _escape( k ) )
            self.write( ' =' )
            xx = curdir.transform_variable( k, v )
            if isinstance( xx, list ):
                self.write( ' '.join( xx ) )
            else:
                self.write( ' ' )
                self.write( xx )
            self.write( '\n' )
        self.write( '\n' )

    def emit_deplist( self, prefix, deps ):
        if deps is None:
            return
        if len(deps) == 0:
            return
        self.write( prefix )
        for d in deps:
            dname = None
            if hasattr( d, 'filename' ):
                dname = d.filename
            if dname is None:
                dname = d.name
            self.write( '%s ' % dname )
        
    def emit_target( self, t, curdir ):
        outDeps = True

        if t.filename:
            if t.rule is None:
                Error( "Target '%s' (type '%s') specifies an output file, but has no rule" % (t.name, t.type) )

            self.write( 'build %s: %s' % ( t.filename, t.rule.name ) )
        elif t.dependencies and len(t.dependencies) > 0:
            self.write( 'build %s: phony ' % t.name )
        else:
            outDeps = False

        if outDeps:
            self.emit_deplist( ' ', t.dependencies )
            self.emit_deplist( ' | ', t.implicit_dependencies )
            self.emit_deplist( ' || ', t.order_only_dependencies )
            self.write( '\n' )
        if t.variables:
            for k, v in iterate( t.variables ):
                self.write( '  ' )
                self.write( _escape( k ) )
                self.write( ' =' )
                xx = curdir.transform_variable( k, v )
                if isinstance( xx, list ):
                    self.write( ' '.join( xx ) )
                else:
                    self.write( ' ' )
                    self.write( xx )
                self.write( '\n' )

    def emit_targets( self, curdir ):
        if curdir.targets:
            curdir.targets.sort()
            for t in curdir.targets:
                if isinstance( t, list ):
                    for x in t:
                        self.emit_target( x, curdir )
                else:
                    self.emit_target( t, curdir )

    def emit_unix_style_rebuild( self, curdir, fn, cf ):
        self.write( '\n\nrule regen_config\n' )
        self.write( '  command = cd %s && env ' % os.path.abspath( '.' ) )
        for g, v in iterate( GetEnvironOverrides() ):
            self.write( ' %s=%s' % (g, _escape(v)) )
        # be careful, python might swallow real arg0 if you run something
        # as python xxx.py, although being called as a script is ok
        for i in range(0,len(sys.argv)):
            self.write( ' ' )
            self.write( sys.argv[i] )
        self.write( '\n  description = Regenerating build\n  generator = 1\n' )
        self.write( '\nbuild build.ninja: regen_config |' )
        for x in cf:
            self.write( ' ' )
            self.write( _escape_path( x ) )
        self.write( '\n\n' )

def _create_file( curdir, flatten ):
    try:
        os.makedirs( curdir.bin_path )
    except OSError as e:
        if e.errno is not errno.EEXIST:
            raise
    fn = os.path.join( curdir.bin_path, "build.ninja" )
    out = NinjaWriter( fn )
    return out, fn

def _emit_dir( curdir, cf=None, flatten=False, version=None ):
    if flatten:
        Debug( "Generating a flattened build file" )
    else:
        Debug( "Generating a build tree" )
    if curdir.targets is None and len(curdir.subdirs) == 0:
        return None
    ( out, fn ) = _create_file( curdir, flatten )
    Debug( "Emitting rules for '%s'" % curdir.rel_src_path )
    out.emit_rules( curdir.get_used_module_rules( False ), version )
    Debug( "Emitting variables for '%s'" % curdir.rel_src_path )
    out.emit_variables( curdir )
    Debug( "Emitting subdirs for '%s'" % curdir.rel_src_path )
    for sn, sd in iterate( curdir.subdirs ):
        sfn = _emit_dir( sd, version=version )
        if sfn:
            out.write( 'subninja %s\n' % _escape_path( sfn ) )
    Debug( "Emitting targets for '%s'" % curdir.rel_src_path )
    out.emit_targets( curdir )
    if cf:
        # top level, add the rule to rebuild the ninja files
        if sys.platform == 'win32':
            raise NotImplementedError
        out.emit_unix_style_rebuild( curdir, fn, cf )
    return fn

class Ninja(Generator):
    def __init__( self, flatten ):
        super( Ninja, self ).__init__( "ninja", flatten )
        try:
            ninja = FindExecutable( "ninja" )
        except:
            FatalException( "ninja generator instantiated, but unable to find ninja executable" )
        self.version = Version( ninja )

    def process_dir( self, curdir ):
        config_file_list = []
        def _traverse_config_files( deps ):
            if deps is None:
                return
            for d in deps:
                if isinstance( d, Directory ):
                    _traverse_config_files( d.implicit_dependencies )
                else:
                    config_file_list.append( d.filename )
        _traverse_config_files( curdir.implicit_dependencies )
        for cf in config_file_list:
            Info( "Build depends on: %s" % cf )

        _emit_dir( curdir, config_file_list, self.flatten, self.version )


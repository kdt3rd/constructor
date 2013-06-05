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

from ..output import Info
from ..generator import Generator
from ..directory import Directory
from ..utility import iterate, GetEnvironOverrides, FileOutput
from ..dependency import Dependency, FileDependency
from ..pseudotarget import PseudoTarget

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

def _create_file( curdir ):
    try:
        os.makedirs( curdir.bin_path )
    except OSError as e:
        if e.errno is not errno.EEXIST:
            raise
    fn = os.path.join( curdir.bin_path, "build.ninja" )
    out = FileOutput( fn )
    return out, fn

def _emit_rules( out, curdir ):
    for n, m in iterate( curdir.modules ):
        if m.rules is not None:
            for tag, rule in iterate( m.rules ):
                if rule.is_used():
                    out.write( 'rule %s\n' % rule.name )
                    out.write( '  command = %s\n' % rule.command )
                    if rule.description:
                        out.write( '  description = %s\n' % rule.description )
                    if rule.dependency_file:
                        out.write( '  depfile = %s\n' % rule.dependency_file )
                    if rule.check_if_changed:
                        out.write( '  restat = 1\n' )
                    out.write( '\n' )
    pass

def _emit_variables( out, curdir ):
    if curdir.variables is None:
        return
    for k, v in iterate( curdir.variables ):
        out.write( _escape( k ) )
        out.write( ' =' )
        if isinstance( v, list ):
            out.write( ' '.join( v ) )
        else:
            out.write( ' ' )
            out.write( v )
        out.write( '\n' )
    out.write( '\n' )

def _emit_target( out, t ):
    if isinstance( t, PseudoTarget ):
        if t.target:
            _emit_target( out, t.target )
            return
        else:
            Error( "Attempt to emit un-resolved pseudo target '%s' (type: '%s')" % (t.name, t.target_type) )

    out.write( 'build %s: %s ' % ( t.output_file, t.rule.name ) )
    deps = t.dependencies( 'build' )
    oonly = ""
    for d in deps:
        if isinstance( d, FileDependency ):
            if d.orderonly:
                oonly = oonly + d.filename + ' '
            else:
                out.write( '%s ' % d.filename )
    if len(oonly) > 0:
        out.write( '| %s' % oonly )
    out.write( '\n' )

def _emit_targets( out, curdir ):
    if curdir.targets:
        for t in curdir.targets:
            if isinstance( t, list ):
                for x in t:
                    _emit_target( out, x )
            else:
                _emit_target( out, t )

def _emit_unix_style_rebuild( out, curdir, fn, cf ):
    out.write( '\n\nrule regen_config\n' )
    out.write( '  command = cd %s && env ' % os.path.abspath( '.' ) )
    for g, v in iterate( GetEnvironOverrides() ):
        out.write( ' %s=%s' % (g, _escape(v)) )
    # be careful, python might swallow real arg0 if you run something
    # as python xxx.py
    for i in range(0,len(sys.argv)):
        out.write( ' ' )
        out.write( sys.argv[i] )
    out.write( '\n  description = Regenerating build\n  generator = 1\n' )
    out.write( '\nbuild build.ninja: regen_config |' )
    for x in cf:
        out.write( ' ' )
        out.write( _escape_path( x ) )
    out.write( '\n\n' )

def _emit_dir( curdir, cf=None ):
    if curdir.targets is None and len(curdir.subdirs) == 0:
        return None
    ( out, fn ) = _create_file( curdir )
    _emit_rules( out, curdir )
    _emit_variables( out, curdir )
    for sn, sd in iterate( curdir.subdirs ):
        sfn = _emit_dir( sd )
        if sfn:
            out.write( 'subninja %s\n' % _escape_path( sfn ) )
    _emit_targets( out, curdir )
    if cf:
        # top level, add the rule to rebuild the ninja files
        if sys.platform == 'win32':
            raise NotImplementedError
        _emit_unix_style_rebuild( out, curdir, fn, cf )
    return fn

class Ninja(Generator):
    def __init__( self ):
        super( Ninja, self ).__init__( "ninja" )

    def process_dir( self, curdir ):
        config_file_list = []
        def _traverse_config_files( deps ):
            for d in deps:
                if isinstance( d, Directory ):
                    _traverse_config_files( d.dependencies( "config" ) )
                else:
                    config_file_list.append( d.filename )
        _traverse_config_files( curdir.dependencies( "config" ) )
        for cf in config_file_list:
            Info( "Build depends on: %s" % cf )
        _emit_dir( curdir, config_file_list )


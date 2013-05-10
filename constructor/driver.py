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
import optparse
from .output import *
from .utility import *
from .phase import Phase, DirParsePhase
from .directory import Directory
from .generator import Generator, GetGeneratorClass

class Driver(object):
    """The master class for constructor that stores the parsed contents for
       an entire build tree"""
    _singleton = None

    def __init__( self, defgenerator ):
        if Driver._singleton is not None:
            Error( "More than one Build Tree object instantiated" )
        Driver._singleton = self

        self.build = None
        self.build_dir = None
        self.build_set = {}

        self.generator = defgenerator

        self.phases = [ DirParsePhase( "config", "config", True, Directory._configPassPostProc ),
                        DirParsePhase( "build", "build", False ) ]

        self.user_features = None
        self.user_feature_defs = None

        self.parser = optparse.OptionParser( usage="usage: %prog [options]", version="%prog 1.0" )
        self.parser.add_option( '--build',
                                 help='Allows one to specify build type',
                                 type="string",
                                 default=None )
        self.parser.add_option( '--build-dir',
                                 type="string",
                                 help='Specifies output directory used for building (by default <current directory>/<build-type-outlocation>)' )
        self.parser.add_option( '--source-dir',
                                 type="string",
                                 help='Specifies the source root to find build files (by default <current directory>)' )
        self.parser.add_option( '--generator',
                                 type="string",
                                 help='Allows one to specify the generator type',
                                 default=self.generator )
        self.parser.add_option( '--verbose',
                                 help='Enables verbose output',
                                 action="store_true", dest="verbose" )
        self.parser.add_option( '--debug',
                                 help='Enables debug output',
                                 action="store_true", dest="verbose" )

    def process( self ):
        # Pre-seed the source root directory so we can load
        # the user's config file, which then allows us to build up
        # the actual argument parser
        src_root = os.getcwd()
        for i in range(0,len(sys.argv)):
            arg = sys.argv[i]
            if arg == "--source-dir":
                if (i + 1) < len(sys.argv):
                    src_root = os.path.abspath( sys.argv[i+1] )
            elif arg.startswith( "--source-dir=" ):
                src_root = os.path.abspath( arg.split('=')[1] )
            elif arg == "--debug":
                SetDebug( True )
            elif arg == "--verbose":
                SetVerbose( True )

        globs = {}
        for key, val in iterate( globals() ):
            if key[0] != '_' and ( key == "os" or key == "sys" or key.lower() != key ):
                globs[key] = val

        self.root_dir = Directory( src_root, "", None, None, globs )
        # Config pass is sort of special since user can potentially
        # register other phases, and we need to parse the options
        # after configuration
        self.cur_phase = self.phases[0]
        conf = self.phases[0].process( self.root_dir )
        self.parse_options()

        if self.build_dir is None:
            self.build_dir = src_root

        if not isinstance( self.phases[len(self.phases) - 1], Generator ):
            Debug( "Creating generator pass for generator '%s'" % self.generator )
            self.phases.append( GetGeneratorClass( self.generator )() )

        for p in self.phases[1:]:
            self.cur_phase = p
            p.process( self.root_dir )
        del(self.cur_phase)

    def add_phase( self, after, phase ):
        if not isinstance( phase, Phase ):
            Error( "Attempt to add processing phase, but need a subclass of Phase" )
        foundidx = -1
        for p in self.phases:
            if p.name == after:
                foundidx = idx
                break
        if foundidx < 0:
            Error( "Attempt to add processing phase after unknown phase '%s'" % after )
        self.phases.insert( foundidx + 1, phase )

    def add_feature( self, name, typename, help=None, default=None ):
        if self.user_features is not None:
            Error( "Invalid request to define feature '%s': All available features must be defined before testing to see if any are set" % name )

        if self.user_feature_defs is None:
            self.user_feature_defs = {}

        ## Hrm, set_defaults doesn't allow us to specify destinations
        ## by user argument string, so stash off for future use
        self.user_feature_defs[name] = [ typename, default ]

        if typename == "boolean":
            self.parser.add_option( '--enable-' + name,
                                     action="store_true",
                                     help="Enable " + help,
                                     dest=name )
            self.parser.add_option( '--disable-' + name,
                                     action="store_false",
                                     help="Disable " + help,
                                     dest=name )
        elif typename == "string":
            self.parser.add_option( '--' + name,
                                     type="string",
                                     help=help,
                                     dest=name,
                                     default=default )
        elif typename == "path":
            self.parser.add_option( '--' + name,
                                     type="string",
                                     help=help,
                                     metavar="PATH",
                                     dest=name,
                                     default=default )
        else:
            Error( "Unknown typename '%s' for user defined feature '%s'" % (typename, name) )

    def feature( self, name ):
        if self.user_features is None:
            self.parse_options()
        if name not in self.user_features:
            Error( "Unknown feature requested '%s', use DefineFeature first" % name )
        return self.user_features[name]

    def parse_options( self ):
        if self.user_features is not None:
            return
        (options, args) = self.parser.parse_args()

        if options.build is not None and len(options.build) > 0:
            self.build = options.build

        if options.build_dir is not None and len(options.build_dir) > 0:
            self.build_dir = options.build_dir
            
        if self.build_dir is None:
            self.build_dir = self.root_dir.src_dir

        self.generator = options.generator

        if len(self.build_set) == 0:
            Error( "Attempt to parse options triggered prior to any build configurations specified" )

        self.root_dir.set_bin_dir( os.path.abspath( os.path.join( self.build_dir, self.build_set[self.build] ) ) )

        self.user_features = {}
        if self.user_feature_defs is not None:
            for k, info in iterate( self.user_feature_defs ):
                oVal = getattr( options, k, None )
                t = info[0]
                default = info[1]
                if t == "boolean":
                    if oVal is not None:
                        self.user_features[k] = oVal
                    else:
                        self.user_features[k] = default
                elif t == "string":
                    if oVal is not None:
                        self.user_features[k] = oVal
                    else:
                        self.user_features[k] = default
                elif t == "path":
                    if oVal is not None and len(oVal) > 0:
                        self.user_features[k] = os.path.abspath( oVal )
                    else:
                        self.user_features[k] = default
                else:
                    Error( "Attempt to parse user option '%s' with unknown type '%s'\n" % (k, t) )

    def add_config( self, tag, outloc, default ):
        self.build_set[tag] = outloc
        if default and self.build is None:
            self.build = tag

def DefineFeature( name, typename, help=None, default=None ):
    Driver._singleton.add_feature( name, typename, help, default )

def Feature( name ):
    return Driver._singleton.feature( name )

def BuildConfig( tag, outloc, default = False ):
    Driver._singleton.add_config( tag, outloc, default )

def AddPhase( name, after, phase ):
    Driver._singleton.add_phase( name, after, phase )

def SubDir( name ):
    Driver._singleton.cur_phase.subdir( name )

def GetCurrentSourceDir():
    Driver._singleton.parse_options()
    return Driver._singleton.cur_phase.cur_dir.src_dir

def GetCurrentSourceRelDir():
    Driver._singleton.parse_options()
    return Driver._singleton.cur_phase.cur_dir.rel_src_dir

def GetBuildRootDir():
    Driver._singleton.parse_options()
    return Driver._singleton.root_dir.bin_path

def GetCurrentBinaryDir():
    Driver._singleton.parse_options()
    return Driver._singleton.cur_phase.cur_dir.bin_dir

def Building( b ):
    Driver._singleton.parse_options()
    return Driver._singleton.build == b

if __name__ == "__main__":
    Driver().process()

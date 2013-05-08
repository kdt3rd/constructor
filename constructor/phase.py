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

from .output import Debug, SetDebugContext
from .dependency import *

_constructor_extension = ".py"
def SetConstructorExtension( ext ):
    global _constructor_extension
    _constructor_extension = ext

class Phase(object):
    """Class to handle various phases of creating build files"""

    def __init__( self, name, fileroot, optional, postproc = None ):
        self.name = name
        self.file_root = fileroot
        self.optional = optional
        self.post_proc = postproc
        self.cur_dir = None

    def subdir( self, name ):
        if self.cur_dir is None:
            Error( "Attempt to process a sub directory with no current directory" )
        newd = self.cur_dir.add_sub_dir( name )
        self.process_dir( newd )

    def process_dir( self, curdir ):
        oldd = self.cur_dir
        try:
            self.cur_dir = curdir
            global _constructor_extension
            filename = self.file_root + _constructor_extension
            fn = os.path.join( curdir.src_dir, filename )
            namespace = curdir.get_globals().copy()
            Debug( "Parsing file '%s'" % fn )
            with open( fn, "r" ) as f:
                curdir.add_dependency( "config", FileDependency( fn, False ) )
                SetDebugContext( fn )
                exec( compile( f.read() + "\n", filename, 'exec' ), namespace, namespace )
            if self.post_proc:
                Debug( "Running post process after file '%s'" % fn )
                self.post_proc( curdir, namespace )
        except IOError as e:
            if e.errno == errno.ENOENT:
                if not self.optional:
                    Error( "Unable to read required file '%s'" % fn )
            else:
                Error( "Unable to read and process file '%s': %s" % (fn, e.strerror) )
        except Exception as e:
            Error( "Error processing file '%s': %s" % ( fn, str(e) ) )
        finally:
            self.cur_dir = oldd
            cur_file = None


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

from ..output import Info
from ..generator import Generator
from ..directory import Directory

class Ninja(Generator):
    def __init__( self ):
        super( Ninja, self ).__init__( "ninja" )

    def process_dir( self, curdir ):
        curdir.make_bin_tree()
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


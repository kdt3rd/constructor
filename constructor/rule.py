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

class Rule(object):
    def __init__( self, **rargs ):
        self.name = rargs['tag']
        cmd = rargs['cmd']
        if isinstance( cmd, list ):
            command = ""
            for c in cmd:
                command += ' ' + c
        else:
            command = cmd
        self.command = command
        self.description = rargs.get( 'desc' )
        self.use_input_in_desc = rargs.get( 'use_input_in_desc' )
        self.dependency_file = rargs.get( 'depfile' )
        self.check_if_changed = rargs.get( 'check_if_changed' )
        self._use_count = 0

    def add_use( self ):
        self._use_count = self._use_count + 1

    def is_used( self ):
        return self._use_count > 0



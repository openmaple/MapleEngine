#!/usr/bin/python
#
# Copyright (C) [2020] Futurewei Technologies, Inc. All rights reverved.
#
# Licensed under the Mulan Permissive Software License v2.
# You can use this software according to the terms and conditions of the MulanPSL - 2.0.
# You may obtain a copy of MulanPSL - 2.0 at:
#
#   https://opensource.org/licenses/MulanPSL-2.0
#
# THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER
# EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR
# FIT FOR A PARTICULAR PURPOSE.
# See the MulanPSL - 2.0 for more details.
#

# ---------------------------------------------------------------------
# Be sure to add the python path that points to the LLDB shared library.
#
# # To use this in the embedded python interpreter using "lldb" just
# import it with the full path using the "command script import"
# command
#   (lldb) command script import /path/to/cmdtemplate.py
# ---------------------------------------------------------------------

from __future__ import print_function

import os
import inspect
import lldb
import optparse
import shlex
import sys
from mlldb import m_config_lldb


class MDebuggerCommand:
    program = 'mdebugger'

    @classmethod
    def register_lldb_command(cls, debugger, module_name):

        parser = cls.create_options()
        cls.__doc__ = parser.format_help()
        # Add any commands contained in this module to LLDB
        command = 'command script add -c %s.%s %s' % (module_name,
                                                      cls.__name__,
                                                      cls.program)
        debugger.HandleCommand(command)
        if m_config_lldb.debug:
            print('The "{0}" command has been installed, type "help {0}" or "{0} '
                  '--help" for detailed help.'.format(cls.program))
        command = 'command alias mdb mdebugger'
        debugger.HandleCommand(command)
        for cmd in m_config_lldb.plugins:
            command = 'command script import ' + os.environ['MAPLE_DEBUGGER_LLDB_SRC'] + '/LLDB/mlldb/'+  cmd + '.py'
            #print(cmd)
            debugger.HandleCommand(command)

        # Create Settings
        command = 'settings set target.inline-breakpoint-strategy always'
        debugger.HandleCommand(command)
        command = 'settings set stop-line-count-before 0'
        debugger.HandleCommand(command)
        command = 'settings set stop-line-count-after 0'
        debugger.HandleCommand(command)
        command = 'platform shell echo Maple System (Version 0.5.0) for lldb Initialized'
        debugger.HandleCommand(command)

    @classmethod
    def create_options(cls):

        usage = "usage: %prog [options] arg"
        description = ('Basically:  mdebugger : Controls, debugs and tests the Maple Debugger')

        """Controls, debugs and tests the Maple Debugger environment
        mdb is an alias of mdebugger command
        mdebugger --debug [on|off]: Enables/disables debugging of the Maple Debugger environment
        mdebugger --test [maple-command]: Tests that a Maple command is working
        mdebugger --system : Performs a Maple Debugger health check
        """

        # Pass add_help_option = False, since this keeps the command in line
        #  with lldb commands, and we wire up "help command" to work by
        # providing the long & short help methods below.
        parser = optparse.OptionParser(
            description=description,
            prog=cls.program,
            usage=usage,
            add_help_option=False)

        parser.add_option(
            '-d',
            '--debug',
            action='store_true',
            dest='mdb_debug',
            help='[on|off]: Enables/disables debugging of the Maple Debugger environment',
            default=False)

        parser.add_option(
            '-t',
            '--test',
            action='store_true',
            dest='mdb_test',
            help='[maple-command]: Tests that a Maple command is working',
            default=False)

        parser.add_option(
            '-s',
            '--system',
            action='store_true',
            dest='mdb_system',
            help=': Performs a Maple Debugger health check',
            default=False)

        parser.add_option(
            '-h',
            '--help',
            action='store_true',
            dest='debugger_help',
            help=description,
            default=False)

        return parser

    def get_short_help(self):
        return "Wrapper for the lldb debugger "

    def get_long_help(self):
        return self.help_string

    def __init__(self, debugger, unused):
        self.parser = self.create_options()
        self.help_string = self.parser.format_help()

    def __call__(self, debugger, command, exe_ctx, result):
        # Use the Shell Lexer to properly parse up command options just like a
        # shell would
        command_args = shlex.split(command)

        try:
            (options, args) = self.parser.parse_args(command_args)
            print(options)
            print(args)
            if options.debugger_help:
                print("mdebugger : Controls, debugs and tests the Maple Debugger\n"
                    "    mdb is an alias of mdebugger command\n"
                    "    mdebugger --debug [on|off]: Enables/disables debugging of the Maple Debugger environment\n"
                    "    mdebugger --system : Performs a Maple Debugger health check\n"
                    "    mdebugger --test [maple-command]: Tests that a Maple command is working\n"
                    "    mdebugger --help: Displays this message")
            else:
                print("Executing command")

        except:
            # if you don't handle exceptions, passing an incorrect argument to
            # the OptionParser will cause LLDB to exit (courtesy of OptParse
            # dealing with argument errors by throwing SystemExit)
            result.SetError("option parsing failed")
            return
        # not returning anything is akin to returning success

#    def __del__(self):
#        system_debug.close()
#        system_debug.unlink()

def __lldb_init_module(debugger, dict):
    # Register all classes that have a register_lldb_command method
    for _name, cls in inspect.getmembers(sys.modules[__name__]):
        if inspect.isclass(cls) and callable(getattr(cls,
                                                     "register_lldb_command",
                                                     None)):
            cls.register_lldb_command(debugger, __name__)



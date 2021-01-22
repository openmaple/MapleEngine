#
# Copyright (C) [2020-2021] Futurewei Technologies, Inc. All rights reverved.
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

from __future__ import print_function

import inspect
import traceback
import lldb
import optparse
import shlex
import sys
import m_config_lldb


class MTypeCommand:
    program = 'mtype'

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

    @classmethod
    def create_options(cls):

        usage = "usage: %prog [options] arg"
        description = ('Basically:  mtype <regex>: print a matching class and its inheritance hierarchy by a given search expression')

        """print a matching class and its inheritance hierarchy by a given search expression
        mtype: given a regex, searches and prints all matching class names if multiple are found,
        or print detailed information of class inheritance hierarchy if a single match is found
        mtype <regular-express>: e.g. mtype _2Fjava
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
            '-h',
            '--help',
            action='store_true',
            dest='type_help',
            help=description,
            default=False)

        return parser

    def get_short_help(self):
        return "Prints a matching class and its inheritance hierarchy by a given search expression"

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
            if options.type_help:
                print("mtype <regex of a mangled Maple class name>\n"
                      "mtype: given a regex, searches and prints all matching class \n"
                      "       names if multiple are found, or print detailed information of \n"
                      "       class inheritance hierarchy if a single match is found\n"
                      "mtype <regular-express>: e.g. mtype _2Fjava")
            else:
                print("Executing command")

        except Exception:
            # if you don't handle exceptions, passing an incorrect argument to
            # the OptionParser will cause LLDB to exit (courtesy of OptParse
            # dealing with argument errors by throwing SystemExit)
            result.SetError("option parsing failed")
            print("-"*60)
            print("Exception caught in mtype code:")
            traceback.print_stack(file=sys.stdout)
            print("-"*60)
            traceback.print_exc(file=sys.stdout)
            print("-"*60)
            return
        # not returning anything is akin to returning success


class MSymbolCommand:
    program = 'msymbol'

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

    @classmethod
    def create_options(cls):

        usage = "usage: %prog [options] arg"
        description = ('Basically:  msymbol <regex>: prints a matching symbol list or it\'s detailed infomation')

        """prints a matching symbol list or it's detailed infomation
        msymbol: given a regex, searches and prints all matching symbol names if multiple are found,
        or print detailed information of the symbol if a single match is found
        msymbol <regular-express>: e.g. msymbol sun.*executor
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
            '-h',
            '--help',
            action='store_true',
            dest='symbol_help',
            help=description,
            default=False)

        return parser

    def get_short_help(self):
        return "Prints a matching symbol list or it's detailed infomation"

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
            if options.symbol_help:
                print("msymbol <regex of a symbol>\n"
                      "msymbol: Given a regex, searches and prints all matching symbol \n"
                      "         names if multiple are found, or print detailed information \n"
                      "         of the symbol if a single match is found\n"
                      "msymbol <regular-express>: e.g. msymbol sun.*executor")
            else:
                print("Executing command")

        except Exception:
            # if you don't handle exceptions, passing an incorrect argument to
            # the OptionParser will cause LLDB to exit (courtesy of OptParse
            # dealing with argument errors by throwing SystemExit)
            result.SetError("option parsing failed")
            print("-"*60)
            print("Exception caught in msymbol code:")
            traceback.print_stack(file=sys.stdout)
            print("-"*60)
            traceback.print_exc(file=sys.stdout)
            print("-"*60)
            return
        # not returning anything is akin to returning success


def __lldb_init_module(debugger, dict):
    # Register all classes that have a register_lldb_command method
    for _name, cls in inspect.getmembers(sys.modules[__name__]):
        if inspect.isclass(cls) and callable(getattr(cls,
                                                     "register_lldb_command",
                                                     None)):
            cls.register_lldb_command(debugger, __name__)



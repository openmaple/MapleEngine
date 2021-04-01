#
# Copyright (C) [2020-2021] Futurewei Technologies, Inc. All rights reserved.
#
# OpenArkCompiler is licensed underthe Mulan Permissive Software License v2.
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

import inspect
import lldb
import optparse
import shlex
import sys
import m_config_lldb

maple_commands_info = '\n'\
    'mbreak:     Sets and manages Maple breakpoints\n'\
    'mbacktrace: Displays Maple backtrace in multiple modes\n'\
    'mup:        Selects and prints the Maple stack frame that called this one\n'\
    'mdown:      Selects and prints the Maple stack frame called by this one\n'\
    'mlist:      Lists source code in multiple modes\n'\
    'msrcpath:   Adds and manages the Maple source code search paths\n'\
    'mlocal:     Displays selected Maple frame arguments, local variables and stack dynamic data\n'\
    'mprint:     Prints Maple runtime object data\n'\
    'mtype:      Prints a matching class and its inheritance hierarchy by a given search expression\n'\
    'msymbol:    Prints a matching symbol list or its detailed infomatioin\n'\
    'mstepi:     Steps a specified number of Maple instructions\n'\
    'mnexti:     Steps one Maple instruction, but proceeds through subroutine calls\n'\
    'mset:       Sets and displays Maple debugger settings\n'\
    'mfinish:    Execute until the selected Maple stack frame returns\n'\
    'mhelp:      Lists Maple help information\n'

maple_commands_detail_info = {
    'mbreak':   'mbreak <symbol>: Sets a new Maple breakpoint at symbol\n'\
                'mbreak --set <symbol>: Alias for "mbreak <symbol>"\n'\
                'mbreak --enable <symbol|index>: Enables an existing Maple breakpoint at symbol\n'\
                'mbreak --disable <symbol|index>: Disables an existing Maple breakpoint at symbol\n'\
                'mbreak --clear <symbol|index>: Deletes an existing Maple breakpoint at symbol\n'\
                'mbreak --clearall : Deletes all existing Maple breakpoints\n'\
                'mbreak --listall : Lists all existing Maple breakpoints\n'\
                'mbreak --ignore <symbol | index> <count>: Sets ignore count for specified Maple breakpoints\n',

    'mbacktrace':   'mbt: An alias for the "mbacktrace" command\n'\
                    'mbacktrace: Prints backtrace of Maple frames\n'\
                    'mbacktrace --asm: Prints the backtrace of Maple frames in assembly format\n'\
                    'mbacktrace --full: Prints the backtrace of mixed gdb native frames and Maple frames\n',

    'mup':      'mup: Moves up one Maple frame that called the selected Maple frame\n'\
                'mup --n: Moves up n Maple frames from currently selected Maple frame\n',

    'mdown':    'mdown: Moves down one Maple frame called by selected Maple frame\n'\
                'mdown --n: Moves down n Maple frames called from currently selected Maple frame\n',

    'mlist':    'mlist: Lists the source code associated with current Maple frame\n'\
                'mlist --asm: Lists the assembly instructions associated with current Maple frame\n',

    'msrcpath': 'msp: An alias of the "msrcpath" command\n'\
                'msrcpath: Displays all the search paths of Maple application and library source code\n'\
                'msrcpath --show: An Alias of the "msrcpath" command without arugments\n'\
                'msrcpath --add <path>: Adds one path to the top of the list\n'\
                'msrcpath --del <path>: Deletes specified path from the list\n',

    'mlocal':   'mlocal: Displays function local variables of the selected Maple frame\n'\
                'mlocal [-s|--stack]: Displays runtime operand stack changes of current selected Maple frame\n',

    'mprint':   'mprint: Displays Maple object data\n'\
                'mprint <addr-in-hex>: e.g. mprint 0x13085\n',

    'mtype' :   'mtype: Given a regex, searches and prints all matching class names if multiple are found, or\n'\
                'prints detailed information of class inheritance hierarchy if a single match is found\n'\
                'mtype <regular-express>: e.g. mtype _2Fjava \n',

    'msymbol' : 'msymbol: Given a regex, searches and prints all matching symbol names if multiple are found, or\n'\
                'prints detailed information of the symbol if a single match is found\n'\
                'msymbol <regular-express>: e.g. msymbol sun.*executor\n',

    'mstepi':   'msi: An alias of "mstepi" command\n'\
                'mstepi: Steps into the next Maple instruction\n'\
                'mstepi [n]: Steps into the next nth Maple instruction\n'\
                'mstepi --abs [n]: Steps into the specified index of Maple instruction\n',

    'mnexti':   'mnexti: Steps one Maple instruction, but proceeds through subroutine calls\n',

    'mfinish':  'mfinish: Executes until selected Maple stack frame returns\n',

    'mset':     'mset <name> <value>: Sets Maple debugger environment settings\n'\
                'mset verbose on|off: Enables/disables the Maple debugger verbose mode\n'\
                'mset --add  <key name of list> <list item value>: Adds a new key/value into a list\n'\
                '  example: mset --add maple_lib_asm_path ~/gitee/maple_engine/maple_build/out/x864\n'\
                'mset --del  <key name of list> <list item value>: Deletes one key/value from a list\n'\
                '  example: maple_lib_asm_path ~/gitee/maple_engine/maple_build/out/x864\n'\
                'mset --show: Displays current settings\n',

    'mhelp':    'mhelp: Lists all Maple commands and their summaries\n'\
                'mhelp <maple-command-full-name>: Details the usage of the specified Maple command\n',
}

class MHelpCommand:
    program = 'mhelp'

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
        description = ('Basically:  mhelp [-h] <maple-command> : Displays Maple commands and their summaries')

        # Pass add_help_option = False, since this keeps the command in line
        #  with lldb commands, and we wire up "help command" to work by
        # providing the long & short help methods below.
        parser = optparse.OptionParser(
            description=description,
            prog=cls.program,
            usage=usage,
            add_help_option=False)

        """list Maple help information
        mhelp: Displays all Maple commands and command summary
        mhelp <maple-command-full-name>: Detailed usage of specified Maple command
        """

        parser.add_option(
            '-h',
            '--help',
            action='store_true',
            dest='help_mhelp',
            help=description,
            default=False)

        return parser

    def get_short_help(self):
        return "Displays Maple commands and their summaries"

    def get_long_help(self):
        return self.help_string

    def mhelp_func(self, args):
        #s = args.split()
        if len(args) == 0:
            self.help_all()
            return

        if len(args) > 1:
            print(self.get_short_help())
            return

        if args[0] in maple_commands_detail_info:
            self.help_one_command(args[0])
            return
        elif args[0] == 'all':
            #print (maple_commands_detail_info)
            for x, y in maple_commands_detail_info.items():
                print("Maple Command:",x)
                for z in y.splitlines():
                    print("    ", z)
            return
        else:
            print(self.usage)
            return

    def help_all(self):
        #version = str(maple_debugger_version_major) + '.' + str(maple_debugger_version_minor)
        #print("Maple Version " + version)
        print(maple_commands_info)

    def help_one_command(self, cmd_name):
        #version = str(maple_debugger_version_major) + '.' + str(maple_debugger_version_minor)
        #print("Maple Version " + version)
        if not cmd_name in maple_commands_detail_info:
            return
        print (maple_commands_detail_info[cmd_name])

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
            if options.help_mhelp:
                self.help_all()
            else:
                self.mhelp_func(args)

        except Exception:
            # if you don't handle exceptions, passing an incorrect argument to
            # the OptionParser will cause LLDB to exit (courtesy of OptParse
            # dealing with argument errors by throwing SystemExit)
            result.SetError("option parsing failed")
            print("-"*60)
            print("Exception caught in mhelp code:")
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



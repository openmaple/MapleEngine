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

import gdb
from m_util import gdb_print

maple_debugger_version_major = 1
maple_debugger_version_minor = 2

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
    'mstep:      Steps program until it reaches a different source line of Maple Application\n'\
    'mnext:      Steps program, proceeding through subroutine calls of Maple application\n'\
    'mset:       Sets and displays Maple debugger settings\n'\
    'mfinish:    Execute until the selected Maple stack frame returns\n'\
    'mhelp:      Lists Maple help information\n'

maple_commands_detail_info = {
    'mbreak':   'mbreak <symbol>: Sets a new Maple breakpoint at symbol\n'\
                'mbreak -set <symbol>: Alias for "mbreak <symbol>"\n'\
                'mbreak -enable <symbol|index>: Enables an existing Maple breakpoint at symbol\n'\
                'mbreak -disable <symbol|index>: Disables an existing Maple breakpoint at symbol\n'\
                'mbreak -clear <symbol|index>: Deletes an existing Maple breakpoint at symbol\n'\
                'mbreak -clearall : Deletes all existing Maple breakpoints\n'\
                'mbreak -listall : Lists all existing Maple breakpoints\n'\
                'mbreak -ignore <symbol | index> <count>: Sets ignore count for specified Maple breakpoints\n',

    'mbacktrace':   'mbt: An alias for the "mbacktrace" command\n'\
                    'mbacktrace: Prints backtrace of Maple frames\n'\
                    'mbacktrace -asm: Prints the backtrace of Maple frames in assembly format\n'\
                    'mbacktrace -mir: prints backtrace of Maple frames in Maple IR format\n'\
                    'mbacktrace -full: Prints the backtrace of mixed gdb native frames and Maple frames\n',

    'mup':      'mup: Moves up one Maple frame that called the selected Maple frame\n'\
                'mup -n: Moves up n Maple frames from currently selected Maple frame\n',

    'mdown':    'mdown: Moves down one Maple frame called by selected Maple frame\n'\
                'mdown -n: Moves down n Maple frames called from currently selected Maple frame\n',

    'mlist':    'mlist: Lists the source code associated with current Maple frame\n'\
                'mlist -asm: Lists the assembly instructions associated with current Maple frame\n'\
                'mlist . | mlist -asm:. | mlist -mir:. : Lists code located by the filename and line number of current Maple frame\n'\
                'mlist line-num : Lists current source code file at line of [line-num]\n'\
                'mlist filename:line-num : Lists specified source code file at line of [line-num]\n'\
                'mlist +|-[num]: Lists current source code file offsetting from previous listed line, offset can be + or -\n'\
                'mlist -asm:+|-[num]: Lists current assembly instructions offsetting from previous listed line. offset can be + or -\n'\
                'mlist -mir : Lists Maple IR associated with current Maple frame\n'\
                'mlist -mir:+|-[num]: Lists current Maple IR offsetting from previous listed line. offset can be + or -\n',

    'msrcpath': 'msp: An alias of the "msrcpath" command\n'\
                'msrcpath: Displays all the search paths of Maple application and library source code\n'\
                'msrcpath -show: An Alias of the "msrcpath" command without arugments\n'\
                'msrcpath -add <path>: Adds one path to the top of the list\n'\
                'msrcpath -del <path>: Deletes specified path from the list\n',

    'mlocal':   'mlocal: Displays function local variables of the selected Maple frame\n'\
                'mlocal [-s|-stack]: Displays runtime operand stack changes of current selected Maple frame\n',

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
                'mstepi -c:  Displays the current opcode count\n'\
                'mstepi -abs [n]: Steps into the specified index of Maple instruction\n',

    'mnexti':   'mnexti: Steps one Maple instruction, but proceeds through subroutine calls\n',

    'mstep' :   'mstep: Steps program until it reaches a different source line of Maple Application\n',

    'mnext' :   'mnext: Steps program, proceeding through subroutine calls of Maple application\n',

    'mfinish':  'mfinish: Executes until selected Maple stack frame returns\n',

    'mset':     'mset <name> <value>: Sets Maple debugger environment settings\n'\
                'mset verbose on|off: Enables/disables the Maple debugger verbose mode\n'\
                'mset -add  <key name of list> <list item value>: Adds a new key/value into a list\n'\
                '  example: mset -add maple_lib_asm_path ~/gitee/maple_engine/maple_build/out/x864\n'\
                'mset -del  <key name of list> <list item value>: Deletes one key/value from a list\n'\
                '  example: maple_lib_asm_path ~/gitee/maple_engine/maple_build/out/x864\n'\
                'mset -show: Displays current settings\n',

    'mhelp':    'mhelp: Lists all Maple commands and their summaries\n'\
                'mhelp <maple-command-full-name>: Details the usage of the specified Maple command\n',
}

class MapleHelpCmd(gdb.Command):
    """list Maple help information
    mhelp: Displays all Maple commands and command summary
    mhelp <maple-command-full-name>: Detailed usage of specified Maple command
    """

    def __init__(self):
        gdb.Command.__init__ (self,
                              "mhelp",
                              gdb.COMMAND_USER,
                              gdb.COMPLETE_NONE)


    def invoke(self, args, from_tty):
        self.mhelp_func(args, from_tty)


    def usage(self):
        gdb_print("mhelp [command name]")


    def mhelp_func(self, args, from_tty):
        s = args.split()
        if len(s) == 0:
            self.help_all()
            return

        if len(s) > 1:
            self.usage()
            return

        if s[0] in maple_commands_detail_info:
            self.help_one_command(s[0])
            return
        else:
            self.usage()
            return

    def help_all(self):
        version = str(maple_debugger_version_major) + '.' + str(maple_debugger_version_minor)
        gdb_print("Maple Version " + version)
        gdb_print(maple_commands_info)

    def help_one_command(self, cmd_name):
        version = str(maple_debugger_version_major) + '.' + str(maple_debugger_version_minor)
        gdb_print("Maple Version " + version)
        if not cmd_name in maple_commands_detail_info:
            return
        gdb_print (maple_commands_detail_info[cmd_name])


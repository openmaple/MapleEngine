#!/usr/bin/env python3
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
import gdb

maple_debugger_version_major = 1
maple_debugger_version_minor = 0

maple_commands_info = '\n'\
    'mbreak:     Set and manage Maple breakpoints\n'\
    'mbacktrace: Display Maple backtrace in multiple modes\n'\
    'mup:        Select and print Maple stack frame that called this one\n'\
    'mdown:      Select and print Maple stack frame called by this one\n'\
    'mlist:      List source code in multiple modes\n'\
    'msrcpath:   Add and manage the Maple source code search paths\n'\
    'mlocal:     Display selected Maple frame arguments, local variables and stack dynamic data\n'\
    'mprint:     Print Maple runtime object data\n'\
    'mtype:      Print a matching class and its inheritance hierarchy by a given search expression\n'\
    'mstepi:     Step specified number of Maple instructions\n'\
    'mnexti:     Step one Maple instruction, but proceed through subroutine calls\n'\
    'mset:       Sets and displays Maple debugger settings\n'\
    'mhelp:      list Maple help information\n'

maple_commands_detail_info = {
    'mbreak':   'mbreak <symbol>: set a new Maple breakpoint at symbol\n'\
                'mbreak -set <symbol>: same to "mbreak <symbol>"\n'\
                'mbreak -enable <symbol|index>: enable an existing Maple breakpoint at symbol\n'\
                'mbreak -disable <symbol|index>: disable an existing Maple breakpoint at symbol\n'\
                'mbreak -clear <symbol|index>: delete an existing Maple breakpoint at symbol\n'\
                'mbreak -clearall : delete all existing Maple breakpoint\n'\
                'mbreak -listall : list all existing Maple breakpoints\n'\
                'mbreak -ignore <symbol | index> <count> : set ignore count for specified Maple breakpoints\n',

    'mbacktrace':   'mbt is the alias of mbacktrace command\n'\
                    'mbacktrace: print backtrace of Maple frames\n'\
                    'mbacktrace -asm: print backtrace of Maple frames in assembly format\n'\
                    'mbreaktrace -full: print backtrace of mixed gdb native frames and Maple frames\n',
    
    'mup':      'mup: move up one Maple frame that called selected Maple frame\n'\
                'mup -n: move up n Maple frames from currently selected Maple frame\n',

    'mdown':    'mdown: move down one Maple frame called by selected Maple frame\n'\
                'mdown -n: move down n Maple frames called from currently selected Maple frame\n',

    'mlist':    'mlist: list source code associated with current Maple frame\n'\
                'mlist -asm: list assemble instructions associated with current Maple frame\n',

    'msrcpath': 'msp is the alias of msrcpath command\n'\
                'msrcpath: display all the search paths of Maple application and library source code\n'\
                'msrcpath -show: same to msrcpath command without arugment\n'\
                'msrcpath -add <path>:  add one path to the top of the list\n'\
                'msrcpath -del <path>:  delete specified path from the list\n',

    'mlocal':   'mlocal: display function local variabls of currently selected Maple frame\n'\
                'mlocal [-s|-stack]: display runtime operand stack changes of current selected Maple frame\n',

    'mprint':   'print a Maple object data\n'\
                'mprint <addr-in-hex>: e.g mprint 0x13085\n',

    'mtype' :   'given a regex, search and print all matching class names if multiple are found, or print detail\n'\
                'information of class inheritance hierarchy if only one match is found\n'\
                'mtype <regular-express>: e.g mtype _2Fjava \n',

    'mstepi':   'msi is the alias of mstepi command\n'\
                'mstepi: step in next Maple instruction\n'\
                'mstepi [n]: step in to next nth Maple instruction\n'\
                'mstepi -abs [n]: step in specified index of Maple instruction\n',

    'mnexti':   'mnexti: Step one Maple instruction, but proceed through subroutine calls\n',

    'mset':     'mset <name> <value>: set Maple debugger environment settings\n'\
                'mset verbose on|off: turn on or off of Maple debugger verbose mode\n'\
                'mset -add  <key name of list> <list item value>: add new key/value into a list\n'\
                '  example: mset -add maple_lib_asm_path /home/carl/gitee/maple_engine/maple_build/out/x864\n'\
                'mset -del  <key name of list> <list item value>: delete one key/value from a list\n'\
                '  example: maple_lib_asm_path /home/carl/gitee/maple_engine/maple_build/out/x864\n'\
                'mset -show: view current settings\n',

    'mhelp':    'mhelp: to list all Maple commands and command summary\n'\
                'mhelp <maple-command-full-name>: detail usage of specified Maple command\n',
}

class MapleHelpCmd(gdb.Command):
    """
    show help information of all Maple commands
    """

    def __init__(self):
        gdb.Command.__init__ (self,
                              "mhelp",
                              gdb.COMMAND_USER,
                              gdb.COMPLETE_NONE)
        

    def invoke(self, args, from_tty):
        gdb.execute('set pagination off', to_string = True)
        self.mhelp_func(args, from_tty)
        gdb.execute('set pagination on', to_string = True)


    def usage(self):
        print("mhelp [command name]")
        

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
        print("Maple Version", version)
        print(maple_commands_info)

    def help_one_command(self, cmd_name):
        version = str(maple_debugger_version_major) + '.' + str(maple_debugger_version_minor)
        print("Maple Version", version)
        if not cmd_name in maple_commands_detail_info:
            return
        print (maple_commands_detail_info[cmd_name])
        

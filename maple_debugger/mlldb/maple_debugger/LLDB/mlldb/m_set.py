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
import m_config_lldb
import time


msettings = {}

#mexcluded = {"m_debug.py", "m_set.py", "m_gdb.py", "m_help.py"}
mexcluded = {"m_set.py", "m_lldb.py", "m_help.py"}
mtrclmts  = os.getenv('MAPLE_TRACE_LIMIT','')
mtrclimit = int(mtrclmts) if mtrclmts.isdigit() else 80

def trace_maple_debugger(frame, event, arg, mtrace_data = [0]):
    """implements the trace function of Maple debugger
    The trace output includes the elapsed time of each target function for performance profiling

    params:
       frame: the frame to be traced
       event: event which occurs, both "call" and "return" are needed
       arg:   Return value for "return" and None for "call"

    returns:
       trace_maple_debugger itself
    """
    if event == "call":
        co = frame.f_code
        filename = co.co_filename.split('/')[-1]
        if filename.startswith("m_") and filename not in mexcluded:
            funcname, lineno, caller = co.co_name, frame.f_lineno, frame.f_back
            callsite = caller.f_code.co_filename.split('/')[-1] + ':' + str(caller.f_lineno) if caller else "None"
            if co.co_argcount == 0:
                argstr = 'arg#0'
            else:
                argstr = 'arg#%d, %s' % (co.co_argcount,
                        ', '.join(str(frame.f_locals[v]) for v in co.co_varnames[:co.co_argcount]))
                argstr = (argstr[:mtrclimit] + '...' if len(argstr) > mtrclimit \
                        else argstr).replace('\n', '\\n').replace('\033', '\\033')
            mtrace_data[0] += 1
            print("+ " * mtrace_data[0] + "==> %s:%d, %s, @(%s), {%s}" % \
                    (filename, lineno, funcname, callsite, argstr))
            mtrace_data.append(time.time())
    elif event == "return" and mtrace_data[0] > 0:
        co = frame.f_code
        filename = co.co_filename.split('/')[-1]
        if filename.startswith("m_") and filename not in mexcluded:
            etime = int((time.time() - mtrace_data.pop()) * 1000000)
            funcname, lineno, retstr = co.co_name, frame.f_lineno, str(arg)
            retstr = (retstr[:mtrclimit] + '...' if len(retstr) > mtrclimit \
                    else retstr).replace('\n', '\\n').replace('\033', '\\033')
            print("- " * mtrace_data[0] + "<-- %s:%d, %s, %sus, ret: %s" % \
                    (filename, lineno, funcname, etime, retstr))
            mtrace_data[0] -= 1
    return trace_maple_debugger


class MSetCommand:
    program = 'mset'

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
        description = ('Basically:  mset <name> <value>: Sets Maple debugger environment settings')

        # Pass add_help_option = False, since this keeps the command in line
        #  with lldb commands, and we wire up "help command" to work by
        # providing the long & short help methods below.
        parser = optparse.OptionParser(
            description=description,
            prog=cls.program,
            usage=usage,
            add_help_option=False)



        """sets and displays Maple debugger settings
        mset <name> <value>: sets Maple debugger environment settings
        mset verbose on|off: enables/disables Maple debugger verbose mode
        mset -add  <key name of list> <list item value>: adds new key/value into a list
          example: mset -add maple_lib_asm_path ~/gitee/maple_engine/maple_build/out/x864
        mset -del  <key name of list> <list item value>: deletes one key/value from a list
          example: maple_lib_asm_path ~/gitee/maple_engine/maple_build/out/x864
        """

        parser.add_option(
            '-s',
            '--show',
            action='store_true',
            dest='mset_show',
            help=': Displays current settings',
            default=False)

        parser.add_option(
            '-a',
            '--add',
            action='store_true',
            dest='mset_add',
            help='<key name of list> <list item value>: adds new key/value into a list, example:                                               -add maple_lib_asm_path ~/gitee/maple_engine/maple_build/out/x864',
            default=False)

        parser.add_option(
            '-d',
            '--del',
            action='store_true',
            dest='mset_del',
            help='<key name of list> <list item value>: deletes one key/value from a list, example:                                            -del maple_lib_asm_path ~/gitee/maple_engine/maple_build/out/x864',
            default=False)

        parser.add_option(
            '-t',
            '--trace',
            action='store_true',
            dest='mset_trace',
            help='on|off: enables/disables Maple debugger trace',
            default=False)

        parser.add_option(
            '-v',
            '--verbose',
            action='store_true',
            dest='mset_verbose',
            help='on|off: enables/disables Maple debugger verbose mode',
            default=False)

        parser.add_option(
            '-h',
            '--help',
            action='store_true',
            dest='mset_help',
            help='Displays help for mset',
            default=False)

        return parser

    def get_short_help(self):
        return "Sets and displays Maple debugger settings"

    def get_long_help(self):
        return self.help_string

    def __init__(self, debugger, unused):
        self.parser = self.create_options()
        self.help_string = self.parser.format_help()

        global msettings
        msettings['verbose'] = 'off'
        msettings['trace'] = 'off'
        #msettings['event'] = 'disconnect'
        #msettings['opcode'] = 'off' # for msi, ms, mfinish, mni, mn command to show instruction info
        #msettings['stack'] = 'off' # for msi and mni to show evaluation stack info
        #m_debug.Debug = False
        msettings['maple_lib_asm_path'] = []



    def __call__(self, debugger, command, exe_ctx, result):
        # Use the Shell Lexer to properly parse up command options just like a
        # shell would
        command_args = shlex.split(command)

        try:
            (options, args) = self.parser.parse_args(command_args)
            print(options)
            print(args)
            if options.mset_help:
                print("mset:\n"
                     "  -To set Maple debugger environment:\n"
                     "    syntax: mset <name> <value>\n"
                     "    example: mset verbose on\n"
                     "  -To add/del items in list:\n"
                     "    syntax:  mset -add <key name of list> <list item value>\n"
                     "    example: mset -add maple_lib_asm_path ~/gitee/maple_engine/maple_build/out/x864\n"
                     "             mset -del maple_lib_asm_path ~/gitee/maple_engine/maple_build/out/x864\n"
                     "  -To display current settings:\n"
                     "    syntax:  mset -show\n"
                     "  -To load user defined configuration file in json format:\n"
                     "    syntax:  mset -lconfig <configuration json file>")

            elif options.mset_trace:
                onoff = args[0]
                buf = "mset --trace " + str(onoff)
                print(buf)
                self.set_trace(onoff)

            elif options.mset_verbose:
                onoff = args[0]
                buf = "mset --verbose " + str(onoff)
                print(buf)
                self.set_verbose(onoff)

            elif options.mset_show:
                print("msettings=", msettings)

            elif options.mset_add:
                key = args[0]
                value = args[1]
                buf = "mset --add " + str(key) + ' ' + str(value)
                print(buf)
                self.add_kv(key, value)

            elif options.mset_del:
                key = args[0]
                value = args[1]
                buf = "mset --del " + str(key) + ' ' + str(value)
                print(buf)
                self.del_kv(key, value)

        except Exception:
            # if you don't handle exceptions, passing an incorrect argument to
            # the OptionParser will cause LLDB to exit (courtesy of OptParse
            # dealing with argument errors by throwing SystemExit)
            result.SetError("option parsing failed")
            print("-"*60)
            print("Exception caught in mset code:")
            traceback.print_stack(file=sys.stdout)
            print("-"*60)
            traceback.print_exc(file=sys.stdout)
            print("-"*60)
            return
        # not returning anything is akin to returning success

    def set_trace(self, onoff):
        global msettings
        msettings['trace'] = 'on' if onoff == 'on' else 'off'
        sys.setprofile(trace_maple_debugger if onoff == 'on' else None)

    def set_verbose(self, onoff):
        global msettings
        msettings['verbose'] = 'on' if onoff == 'on' else 'off'

    def add_kv(self, key, value):
        global msettings
        if key == 'maple_lib_asm_path' and not value in msettings[key]:
            msettings[key].append(value)

    def del_kv(self, key, value):
        global msettings
        if key == 'maple_lib_asm_path' and value in msettings[key]:
            msettings[key].remove(value)

def __lldb_init_module(debugger, dict):
    # Register all classes that have a register_lldb_command method
    for _name, cls in inspect.getmembers(sys.modules[__name__]):
        if inspect.isclass(cls) and callable(getattr(cls,
                                                     "register_lldb_command",
                                                     None)):
            cls.register_lldb_command(debugger, __name__)



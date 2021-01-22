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
import traceback
import optparse
import shlex
import sys
import copy

#import m_block
import m_frame
import mdata.m_symbol
import mdata.m_datastore
from shared import m_util
from shared.m_util import MColors
from shared.m_util import mdb_print
import m_config_lldb as m_debug
from mlldb import m_breakpoint

class MBreakpointCommand:
    """Set and manage Maple breakpoints
    mbreak <symbol>: Set a new Maple breakpoint at symbol
    mbreak <symbol>: Set a new Maple breakpoint at symbol
    mbreak -set <symbol>: Same to 'mbreak <symbol>'
    mbreak -enable <symbol|index>: Enable an existing Maple breakpoint at symbol
    mbreak -disable <symbol|index>: Disable an existing Maple breakpoint at symbol
    mbreak -clear <symbol|index>: Delete an existing Maple breakpoint at symbol
    mbreak -clearall : Delete all existing Maple breakpoints
    mbreak -listall : List all existing Maple breakpoints
    mbreak -ignore <symbol | index> <count>: Set ignore count for specified Maple breakpoints
    """
    program = 'mbreak'

    def __init__(self, debugger, unused):
        """
        mbp_table:
        mbp_table is a runtime table mbreak command keeps.
        mbp_table is a dict, key of mbp_table item is the symbol.
        value of each item in mbp_table is also a dict.
        item dict defined as
        {
            'count'   : int, a count down number of a Maple symbol to be ignored
            'disabled': True|False,
            'object'  : Instance object of class MapleBreakpoint,
            'address' : Breakpoint reported by 'info b' command + 0x4
        }
        """
        self.parser = self.create_options()
        self.help_string = self.parser.format_help()

        self.mbp_object = None
        self.mbp_id = None
        self.symbol_index = 1
        self.initialized_mdb_bp = False

    def init_mdb_breakpoint(self):
        # create the native mdb Breakpoint object
        self.mbp_object = m_breakpoint.MapleBreakpoint()
        m_breakpoint.glo_self = self.mbp_object
        # mbp_id is the breakpoint object id, it is an object, not a number
        self.mbp_id = self.mbp_object.mbp_id
        # TODO: verify self.mbp_object = m_breakpoint.MapleBreakpoint()


    @classmethod
    def register_lldb_command(cls, debugger, module_name):
        parser = cls.create_options()
        cls.__doc__ = parser.format_help()
        # Add any commands contained in this module to LLDB
        command = 'command script add -c %s.%s %s' % (module_name,
                                                      cls.__name__,
                                                      cls.program)
        debugger.HandleCommand(command)

        if m_debug.debug:
            print('The "{0}" command has been installed, type "help {0}" or "{0} '
                  '--help" for detailed help.'.format(cls.program))

        #TODO: re-enable this instack trace
        #command = 'command alias in_call_stack breakpoint command add --python-function in_call_stack.in_call_stack -k name -v %1'
        #debugger.HandleCommand(command)
        command = 'command alias mb mbreak'
        debugger.HandleCommand(command)


    @classmethod
    def create_options(cls):

        usage = "usage: %prog [options] arg"
        description = ('Basically:  mbreak <symbol>: Sets a new Maple breakpoint at symbol')

        # Pass add_help_option = False, since this keeps the command in line
        #  with lldb commands, and we wire up "help command" to work by
        # providing the long & short help methods below.
        parser = optparse.OptionParser(
            description=description,
            prog=cls.program,
            usage=usage,
            add_help_option=False)

        parser.add_option(
            '-s',
            '--set',
            action='store_true',
            dest='mbreak_set',
            help='<symbol>:  Sets a breakpoint for <symbol>',
            default=False)

        parser.add_option(
            '-e',
            '--enable',
            action='store_true',
            dest='mbreak_enable',
            help='<symbol | index>: Enables an existing Maple breakpoint at symbol',
            default=False)

        parser.add_option(
            '-d',
            '--disable',
            action='store_true',
            dest='mbreak_disable',
            help='<symbol | index>: Disables an existing Maple breakpoint at symbol',
            default=False)

        parser.add_option(
            '-c',
            '--clear',
            action='store_true',
            dest='mbreak_clear',
            help='<symbol | index>: Deletes an existing Maple breakpoint at symbol',
            default=False)

        parser.add_option(
            '-C',
            '--clearall',
            action='store_true',
            dest='mbreak_clear_all',
            help=': Deletes all existing Maple breakpoints',
            default=False)

        parser.add_option(
            '-l',
            '--listall',
            action='store_true',
            dest='mbreak_list_all',
            help=': Lists all existing Maple breakpoints',
            default=False)

        parser.add_option(
            '-i',
            '--ignore',
            action='store_true',
            dest='mbreak_ignore',
            help='<symbol | index> <count> : Sets ignore count for specified Maple breakpoints',
            default=False)

        parser.add_option(
            '-r',
            '--regex',
            action='store_true',
            dest='mbreak_regex_set',
            help='<symbol>:  Sets a breakpoint for <regex>',
            default=False)

        parser.add_option(
            '-b',
            '--debug',
            action='store_true',
            dest='mbreak_debug',
            help='<symbol | index>:  Sets/Unsets a breakpoint for debug info',
            default=False)

        parser.add_option(
            '-h',
            '--help',
            action='store_true',
            dest='mbreak_help',
            help=description,
            default=False)

        return parser

    def get_short_help(self):
        return "Sets and manages Maple breakpoints"

    def get_long_help(self):
        return self.help_string

    def __call__(self, debugger, command, exe_ctx, result):
        # Use the Shell Lexer to properly parse up command options just like a
        # shell would
        command_args = shlex.split(command)

        try:
            (options, args) = self.parser.parse_args(command_args)
            if m_debug.debug:
                print(options)
                print(args)
            target = debugger.GetSelectedTarget()

            #if m_breakpoint.mbp_glo_table and self.mbp_object.mbp_table:
            #    self.mbp_object.mbp_table = copy.deepcopy(m_breakpoint.mbp_glo_table)
            #    print("Copying global to local")

            if options.mbreak_help:
                self.get_long_help()
                return

            elif options.mbreak_set:
                symbol = args[0]
                #m_breakpoint.m_bp_create(args[0], True)
                buf = "set breakpoint " + str(symbol)
                #print(buf)
                self.set_breakpoint(symbol, False)

            elif options.mbreak_enable:
                #print("enable: ", args[0])
                self.enable_breakpoint(args[0])

            elif options.mbreak_disable:
                #print("disable: ", args[0])
                self.disable_breakpoint(args[0])

            elif options.mbreak_clear:
                #print("clear/remove: ", args[0])
                self.clear_breakpoint(args[0])

            elif options.mbreak_clear_all:
                #print("clearing/removing all ")
                self.clearall_breakpoint()

            elif options.mbreak_list_all:
                self.listall_breakpoint()

            elif options.mbreak_ignore:
                #print("ignore: ", args[0], args[1])
                self.ignore_breakpoint(args[0], args[1])

            elif options.mbreak_debug:
                #print("debug: ", args[0], args[1])
                self.debug_breakpoint(args[0], args[1])

            elif options.mbreak_regex_set:
                symbol = args[0]
                buf = "set regex breakpoint " + str(symbol)
                #print(buf)
                self.set_breakpoint(symbol, True)

            else:
                print("mbreakpoint: ")
                #self.set_breakpoint(symbol, false)
                self.set_breakpoint("main", False)

            #TODO:  Globals stopped working, WHY?
            #m_breakpoint.mbp_glo_table = copy.deepcopy(self.mbp_object.mbp_table)
            #Save breakpoint dict to a  local file
            self.mbp_object.update_mbp()

        except Exception:
            # if you don't handle exceptions, passing an incorrect argument to
            # the OptionParser will cause LLDB to exit (courtesy of OptParse
            # dealing with argument errors by throwing SystemExit)
            result.SetError("option parsing failed")
            print("-"*60)
            print("Exception caught in mbreak code:")
            traceback.print_stack(file=sys.stdout)
            print("-"*60)
            traceback.print_exc(file=sys.stdout)
            print("-"*60)
            return
        # not returning anything is akin to returning success

    def set_breakpoint(self, symbol, is_regex):
        mbp_exist, mbp_id = m_breakpoint.does_mbp_exist()
        print("set_breakpoint: mbp_exist=", mbp_exist, "mbp_id=", mbp_id)
        if mbp_exist:
            if mbp_id != self.mbp_id:
                print("There are one or more breakpoints already created at maple::maple_invoke_method.")
                print("In order to use mbreakpoint command, please delete those breakpoints first")
                print("")
                return

        if self.initialized_mdb_bp == False:
            self.init_mdb_breakpoint()
            self.initialized_mdb_bp = True

        if symbol in self.mbp_object.mbp_table:
            self.mbp_object.mbp_table[symbol]['disabled'] = False
        else:
            self.mbp_object.mbp_table[symbol] = {}
            self.mbp_object.mbp_table[symbol]['disabled'] = False
            self.mbp_object.mbp_table[symbol]['debug'] = False
            self.mbp_object.mbp_table[symbol]['is_regex'] = is_regex
            self.mbp_object.mbp_table[symbol]['hit_count'] = 0
            self.mbp_object.mbp_table[symbol]['ignore_count'] = 0
            self.mbp_object.mbp_table[symbol]['index'] = self.symbol_index

            #TODO: Enter new breakpoint here
            #symbol =  ""
            #breakpoint.SetCondition('$rdi == '+)

            # NOTE!!! symbol we pass in here is NOT a mirbin_info symbol.
            #print("2.3")
            #addr = m_symbol.get_symbol_address(symbol)
            #print("2.4")
            #if not addr:
            #    self.mbp_object.mbp_table[symbol]['address'] = 0
            #    self.mbp_object.mbp_table[symbol]['hex_addr'] = None
            #else:
            #    self.mbp_object.mbp_table[symbol]['address'] = int(addr, 16)
            #    self.mbp_object.mbp_table[symbol]['hex_addr'] = addr
            #    self.mbp_object.add_known_addr_symbol_into_addr_sym_table(symbol)
            self.symbol_index += 1

        self.mbp_object.update_mbp()
        #print("Table:", self.mbp_object.mbp_table)

    def disable_breakpoint(self, s):
        if not self.mbp_object:
            return
        buf = "disable breakpoint " + str(s)
        print(buf)
        if s.isdigit():
            symbol = self.lookup_symbol_by_index(int(s))
        else:
            symbol = s

        if symbol is None:
            buf = "no symbol is found for index " + str(s)
            print(buf)
            return

        if symbol in self.mbp_object.mbp_table:
            self.mbp_object.mbp_table[symbol]['disabled'] = True
        else:
            buf = "disable symbol: " + str(symbol) + " not found"
            print(buf)


    def enable_breakpoint(self, s):
        if not self.mbp_object:
            return
        buf = "enable breakpoint " + str(s)
        print(buf)
        if s.isdigit():
            symbol = self.lookup_symbol_by_index(int(s))
        else:
            symbol = s

        if symbol is None:
            buf = "no symbol is found for index " + str(s)
            print(buf)
            return

        if symbol in self.mbp_object.mbp_table:
            self.mbp_object.mbp_table[symbol]['disabled'] = False
        else:
            buf = "enable symbol: " + str(symbol) + " not found"
            print(buf)


    def clear_breakpoint(self, s):
        if not self.mbp_object:
            return
        buf = "clear breakpoint " + str(s)
        print(buf)
        if s.isdigit():
            symbol = self.lookup_symbol_by_index(int(s))
        else:
            symbol = s

        if symbol is None:
            buf = "no symbol is found for index " + str(s)
            print(buf)
            return

        if symbol in self.mbp_object.mbp_table:
            self.mbp_object.clear_one_symbol(symbol)
        else:
            buf = "clear symbol: " + str(symbol) + " not found"
            print(buf)


    def ignore_breakpoint(self, s, c):
        if not self.mbp_object:
            return
        buf = "ignore breakpoint " + str(s) + ' ' + str(c)
        print(buf)
        if not c.isdigit():
            print ("ignore count must be a number")
            return

        if s.isdigit():
            symbol = self.lookup_symbol_by_index(int(s))
        else:
            symbol = s

        if symbol is None:
            buf = "no symbol is found for index " + str(s)
            print(buf)
            return

        count = int(c)
        if count < 0:
            count = 0
        self.mbp_object.mbp_table[symbol]['ignore_count'] = count

    def debug_breakpoint(self, s, t):
        if not self.mbp_object:
            return
        buf = "debug breakpoint " + str(s) + ' ' + str(t)
        print(buf)
        debug_setting = True

        if s.isdigit():
            symbol = self.lookup_symbol_by_index(int(s))
        else:
            symbol = s

        if symbol is None:
            buf = "no symbol is found for index " + str(s)
            print(buf)
            return

        if t.isdigit():
            if int(t) == 0:
                debug_setting = False
            elif int(t) == 1:
                debug_setting = True
            else:
                print ("debug settting must be True or False")
                return
        elif str(t) == "False":
            debug_setting = False
        elif str(t) == "True":
            debug_setting = True
        elif (type(t)==bool):
            debug_setting = t

        self.mbp_object.mbp_table[symbol]['debug'] = debug_setting

    def clearall_breakpoint(self):
        if not self.mbp_object:
            return
        print ("clear all breakpoint")
        self.mbp_object.clear_all_symbol()

    def listall_breakpoint(self):
        if not self.mbp_object:
            return

        #setup color constants
        MColors.init_maple_colors()
        print(MColors.yellow('\nCurrent Maple Breakpoints:'))

        # sort the dict with the index, so that we can display in index order
        blist = [{k:v} for k, v in sorted(self.mbp_object.mbp_table.items(), key=(lambda x:x[1]['index']))]
        #blist = [{k:v} for k, v in sorted(m_breakpoint.mbp_glo_table.items(), key=(lambda x:x[1]['index']))]
        #print(blist)
        buf = ""
        bp_addr = "0x7ffff5aeadea"
        bp_info = "in maple::maple_invoke_method(maple::method_header_t const*, maple::MFunction const*) at /vagrant/maple_engine/maple_engine/src/invoke_method.cpp:150"
        addr, bp_info = m_breakpoint.get_maple_invoke_bp_stop_addr(buf)
        bp_addr = hex(addr)
        if not bp_info:
            bp_info = "maple::maple_invoke_method()"
            bp_addr = "pending address"

        #m_util.enable_color_ouput(True)
        for v in blist:
            key = [*v][0]
            state = MColors.bp_state_rd('disabled') if v[key]['disabled'] else MColors.bp_state_gr('enabled')
            #state = state + MColors.ENDC
            #state = MColors.BP_STATE_RD + 'disabled' + MColors.ENDC if v[key]['disabled'] else MColors.BP_STATE_GR + 'enabled' + MColors.ENDC
            #print('  #%i %s, %s, %s: %s, %s: %s, %s: %s, %s: %s @ %s %s' % \
            #        (v[key]['index'], key, state, \
            #        'hit_count', MColors.bp_attr(v[key]['hit_count']),\
            #        'ignore_count',  MColors.bp_attr(v[key]['ignore_count']),\
            #        'is_regex',  MColors.bp_attr(v[key]['is_regex']),\
            #        'debug',  MColors.bp_attr(v[key]['debug']),\
            #         MColors.bp_addr(bp_addr),\
            #         MColors.sp_sname(bp_info)))

            #state = MColors.BP_STATE_RD + 'disabled' + MColors.ENDC if v[key]['disabled'] else MColors.BP_STATE_GR + 'enabled' + MColors.ENDC
            print('#%i %s %s %s %d %s %d at %s in %s' % \
                    (v[key]['index'], m_util.color_symbol(MColors.BP_SYMBOL, key), state, MColors.BP_ATTR + 'hit_count' + MColors.ENDC, v[key]['hit_count'],\
                     MColors.BP_ATTR + 'ignore_count' + MColors.ENDC,  v[key]['ignore_count'],\
                     MColors.BP_ADDR + bp_addr + MColors.ENDC, bp_info))

    def lookup_symbol_by_index(self,index):
        for k,v in self.mbp_object.mbp_table.items():
            if v['index'] == index:
                return k
        return None


# Class End

def __lldb_init_module(debugger, dict):
    # Register all classes that have a register_lldb_command method
    for _name, cls in inspect.getmembers(sys.modules[__name__]):
        if inspect.isclass(cls) and callable(getattr(cls,
                                                     "register_lldb_command",
                                                     None)):
            cls.register_lldb_command(debugger, __name__)



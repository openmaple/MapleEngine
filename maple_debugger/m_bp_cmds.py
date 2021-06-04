#
# Copyright (C) [2021] Futurewei Technologies, Inc. All rights reserved.
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

import gdb
import m_block
import m_frame
import m_breakpoint
import m_symbol
import m_util
from m_util import MColors
from m_util import gdb_print
import m_debug

class MapleBreakpointCmd(gdb.Command):
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

    def __init__(self):
        gdb.Command.__init__ (self,
                              "mbreak",
                              gdb.COMMAND_BREAKPOINTS,
                              gdb.COMPLETE_NONE)
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
        self.mbp_object = None
        self.mbp_id = None

        self.mbp_dync_object = None
        self.mbp_dync_id = None

        self.symbol_index = 1
        self.initialized_gdb_bp = False

        # create alias mb to mbreak
        m_util.gdb_exec('alias mb = mbreak')

    def init_gdb_breakpoint(self):
        # create a gdb.Breakpoint object
        self.mbp_object = m_breakpoint.MapleBreakpoint()
        self.mbp_id = m_breakpoint.get_mbp_id()

        self.mbp_dync_object = m_breakpoint.MapleBreakpointDync()
        self.mbp_dync_id = m_breakpoint.get_mbp_dync_id()


    def invoke(self, args, from_tty):
        self.mbp_func(args, from_tty)

    def usage(self):
        gdb_print("  mbreak <symbol>: Sets a new Maple breakpoint at symbol\n"
                  "  mbreak -set <symbol>: An alias for 'mbreak <symbol>'\n"
                  "  mbreak -enable <symbol|index>: Enables an existing Maple breakpoint at symbol\n"
                  "  mbreak -disable <symbol|index>: Disables an existing Maple breakpoint at symbol\n"
                  "  mbreak -clear <symbol|index>: Deletes an existing Maple breakpoint at symbol\n"
                  "  mbreak -clearall : Deletes all existing Maple breakpoints\n"
                  "  mbreak -listall : Lists all existing Maple breakpoints\n"
                  "  mbreak -ignore <symbol | index> <count> : Sets ignore count for specified Maple breakpoints\n"
                  "  mbreak : Displays usage and syntax")

    def mbp_func(self, args, from_tty):
        '''
        mbreak cmd syntax:
        # mbreak <symbol>
        # mbreak -set <symbol>
        # mbreak -disable <symbol | index>
        # mbreak -enable <symbol | index>
        # mbreak -clear <symbol | index>
        # mbreak -clearall
        # mbreak -listall
        # mbreak -ignore <symbol | index> <count>
        '''
        s = str(args)
        if len(s) is 0: #nothing specified
            self.usage()
            return
        x = s.replace('_mirbin_info','').replace('_mirbin_code', '').split()
        if len(x) == 1: #symbol or clearall or listall
            if x[0] == '-clearall':
                self.clearall_breakpoint()
            elif x[0] == '-listall' or x[0] == '-list' or x[0] == '-ls':
                self.listall_breakpoint()
            elif x[0] == '-debug' :
                self.debug()
            elif x[0] == '-restart' :
                self.clearup_bp_symbol_data()
            elif x[0][0] != '-' : # a symbol is passed in
                self.set_breakpoint(x[0])
            else:
                self.usage()
                return
        elif len(x) is 2: # disable/enable/clear/set
            if x[0] == '-disable':
                self.disable_breakpoint(x[1])
            elif x[0] == '-enable':
                self.enable_breakpoint(x[1])
            elif x[0] == '-clear':
                self.clear_breakpoint(x[1])
            elif x[0] == '-set':
                self.set_breakpoint(x[1])
            else:
                self.usage()
                return
        elif len(x) is 3: # ignore
            if x[0] == '-ignore':
                self.ignore_breakpoint(x[1],x[2])
            else:
                self.usage()
                return
        else:
            self.usage()
            return

    def set_breakpoint(self, symbol):
        if str(symbol) == "main":
            gdb_print("Setting Maple Breakpoint named 'main' is not allowed")
            gdb_print("")
            return

        buf = "set breakpoint " + str(symbol)
        gdb_print(buf)
        mbp_exist, mbp_id = m_breakpoint.is_mbp_existed()
        #print ("mbp_id = ", mbp_id, "self.mbp_id=", self.mbp_id)
        if mbp_exist:
            if mbp_id != self.mbp_id:
                gdb_print("There are one or more breakpints already created at maple::maple_invoke_method.")
                gdb_print("In order to use mbreakpoint command, please delete those breakpoints first")
                gdb_print("")
                return

        mbp_dync_exist, mbp_dync_id = m_breakpoint.is_mbp_dync_existed()
        #print ("mbp_dync_id = ", mbp_dync_id, "self.mbp_dync_id=", self.mbp_dync_id)
        if mbp_dync_exist:
            if mbp_dync_id != self.mbp_dync_id:
                gdb_print("There are one or more breakpints already created at maple::InvokeInterpretMethod.")
                gdb_print("In order to use mbreakpoint command, please delete those breakpoints first")
                gdb_print("")
                return

        if self.initialized_gdb_bp == False:
            self.init_gdb_breakpoint()
            self.initialized_gdb_bp = True

        if symbol in m_breakpoint.mbp_table:
            m_breakpoint.mbp_table[symbol]['disabled'] = False
        else:
            m_breakpoint.mbp_table[symbol] = {}
            m_breakpoint.mbp_table[symbol]['disabled'] = False
            m_breakpoint.mbp_table[symbol]['hit_count'] = 0
            m_breakpoint.mbp_table[symbol]['ignore_count'] = 0
            m_breakpoint.mbp_table[symbol]['index'] = self.symbol_index
            # NOTE!!! symbol we pass in here is NOT a mirbin_info symbol.
            # !!!Workaround for symbol is 'main'. This main could be a inferior's main
            # other than JS's main.
            addr = m_symbol.get_symbol_address(symbol)
            if not addr:
                m_breakpoint.mbp_table[symbol]['address'] = 0
                m_breakpoint.mbp_table[symbol]['hex_addr'] = None
            else:
                m_breakpoint.mbp_table[symbol]['address'] = int(addr, 16)
                m_breakpoint.mbp_table[symbol]['hex_addr'] = addr
                self.mbp_object.add_known_addr_symbol_into_addr_sym_table(symbol)
                self.mbp_dync_object.add_known_addr_symbol_into_addr_sym_table(symbol)
            self.symbol_index += 1

        self.mbp_object.update_mbp()
        self.mbp_dync_object.update_mbp_dync()

    def disable_breakpoint(self, s):
        if not self.mbp_object:
            return
        buf = "disable breakpoint " + str(s)
        gdb_print(buf)
        if s.isdigit():
            symbol = self.lookup_symbol_by_index(int(s))
        else:
            symbol = s

        if symbol is None:
            buf = "no symbol is found for index " + str(s)
            gdb_print(buf)
            return

        if symbol in m_breakpoint.mbp_table:
            m_breakpoint.mbp_table[symbol]['disabled'] = True
        else:
            buf = "disable symbol: " + str(symbol) + " not found"
            gdb_print(buf)

        self.mbp_object.update_mbp()
        self.mbp_dync_object.update_mbp_dync()

    def enable_breakpoint(self, s):
        if not self.mbp_object:
            return
        buf = "enable breakpoint " + str(s)
        gdb_print(buf)
        if s.isdigit():
            symbol = self.lookup_symbol_by_index(int(s))
        else:
            symbol = s

        if symbol is None:
            buf = "no symbol is found for index " + str(s)
            gdb_print(buf)
            return

        if symbol in m_breakpoint.mbp_table:
            m_breakpoint.mbp_table[symbol]['disabled'] = False
        else:
            buf = "enable symbol: " + str(symbol) + " not found"
            gdb_print(buf)

        self.mbp_object.update_mbp()
        self.mbp_dync_object.update_mbp_dync()

    def clear_breakpoint(self, s):
        if not self.mbp_object:
            return
        buf = "clear breakpoint " + str(s)
        gdb_print(buf)
        if s.isdigit():
            symbol = self.lookup_symbol_by_index(int(s))
        else:
            symbol = s

        if symbol is None:
            buf = "no symbol is found for index " + str(s)
            gdb_print(buf)
            return

        if symbol in m_breakpoint.mbp_table:
            self.mbp_object.clear_one_symbol(symbol)
        else:
            buf = "clear symbol: " + str(symbol) + " not found"
            gdb_print(buf)

        self.mbp_object.update_mbp()
        self.mbp_dync_object.update_mbp_dync()

    def ignore_breakpoint(self, s, c):
        if not self.mbp_object:
            return
        buf = "ignore breakpoint " + str(s) + ' ' + str(c)
        gdb_print(buf)
        if not c.isdigit():
            gdb_print ("ignore count must be a number")
            return

        if s.isdigit():
            symbol = self.lookup_symbol_by_index(int(s))
        else:
            symbol = s

        if symbol is None:
            buf = "no symbol is found for index " + str(s)
            gdb_print(buf)
            return

        count = int(c)
        if count < 0:
            count = 0
        m_breakpoint.mbp_table[symbol]['ignore_count'] = count

    def clearall_breakpoint(self):
        if not self.mbp_object:
            return
        gdb_print ("clear all breakpoint")
        self.mbp_object.clear_all_symbol()
        self.mbp_object.update_mbp()
        self.mbp_dync_object.update_mbp_dync()

    def listall_breakpoint(self):
        if not self.mbp_object:
            return

        gdb_print ("list all Maple breakpoints")
        # sort the dict with the index, so that we can display in index order
        blist = [{k:v} for k, v in sorted(m_breakpoint.mbp_table.items(), key=(lambda x:x[1]['index']))]
        bp_info = self.mbp_object.bp_info if self.mbp_object.bp_info else "in maple::maple_invoke_method()"
        bp_addr = hex(self.mbp_object.bp_addr) if self.mbp_object.bp_addr else "pending address"
        for v in blist:
            key = [*v][0]
            state = MColors.BP_STATE_RD + 'disabled' + MColors.ENDC if v[key]['disabled'] else MColors.BP_STATE_GR + 'enabled' + MColors.ENDC
            gdb_print('#%i %s %s %s %d %s %d at %s %s' % \
                    (v[key]['index'], m_util.color_symbol(MColors.BP_SYMBOL, key), state, MColors.BP_ATTR + 'hit_count' + MColors.ENDC, v[key]['hit_count'],\
                     MColors.BP_ATTR + 'ignore_count' + MColors.ENDC,  v[key]['ignore_count'],\
                     MColors.BP_ADDR + bp_addr + MColors.ENDC, bp_info))

    def debug(self):
        if not self.mbp_object:
            return
        gdb_print(" ======= Maple breakpoint mbp table: ===========")
        self.mbp_object.display_mbp_table()
        gdb_print(" ======= Maple breakpoint addr_sym_table: ===========")
        self.mbp_object.display_mbp_addr_sym_table()

    def lookup_symbol_by_index(self,index):
        for k,v in m_breakpoint.mbp_table.items():
            if v['index'] == index:
                return k
        return None

    def clearup_bp_symbol_data(self):
        if self.mbp_object:
            self.mbp_object.clearup_mbp_symbol_related_data()

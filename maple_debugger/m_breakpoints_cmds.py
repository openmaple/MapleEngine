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
import sys
import m_block
import m_frame
import m_breakpoint
import m_symbol
import time
from inspect import currentframe, getframeinfo
import m_util

class MapleBreakpointCmd(gdb.Command):
    """
    class MapleBreakpointCmd defines and Maple mreak command and
    the breakpoint management such as add, delete, clearall, ignore, list, and debug info
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
            'object'  : instance object of class MapleBreakpoint,
            'address' : breakpoint reported by 'info b' command + 0x4
        }
        """
        self.mbp_table = {}
        self.mbp_object = None
        self.mbp_id = None
        self.symbol_index = 1
        self.initialized_gdb_bp = False

        # create alias mb to mbreak
        gdb.execute('alias mb = mbreak')

    def init_gdb_breakpoint(self):
        # create a gdb.Breakpoint object
        self.mbp_object = m_breakpoint.MapleBreakpoint()
        self.mbp_id = m_breakpoint.get_mbp_id()

    def invoke(self, args, from_tty):
        gdb.execute('set pagination off', to_string = True)
        self.mbp_func(args, from_tty)
        gdb.execute('set pagination on', to_string = True)

    def usage(self):
        print ("  mbreak <symbol>: set a new Maple breakpoint at symbol")
        print ("  mbreak -set <symbol>: same to 'mbreak <symbol>")
        print ("  mbreak -enable <symbol|index>: enable an existing Maple breakpoint at symbol")
        print ("  mbreak -disable <symbol|index>: disable an existing Maple breakpoint at symbol")
        print ("  mbreak -clear <symbol|index>: delete an existing Maple breakpoint at symbol")
        print ("  mbreak -clearall : delete all existing Maple breakpoints")
        print ("  mbreak -listall : list all existing Maple breakpoints")
        print ("  mbreak -ignore <symbol | index> <count> : set ignore count for specified Maple breakpoints")
        print ("  mbreak : usage and syntax")

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
        x = s.split()
        if len(x) == 1: #symbol or clearall or listall
            if x[0] == '-clearall':
                self.clearall_breakpoint()
            elif x[0] == '-listall' or x[0] == '-list' or x[0] == '-ls':
                self.listall_breakpoint()
            elif x[0] == '-debug' :
                self.debug()
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
        print ("set breakpoint ", symbol)
        mbp_exist, mbp_id = m_breakpoint.is_mbp_existed()
        if mbp_exist:
            if mbp_id != self.mbp_id:
                print("There are one or more breakpints already created at maple::maple_invoke_method.")
                print("In order to use mbreakpoint command, please delete those breakpoints first")
                print("")
                return

        if self.initialized_gdb_bp == False:
            self.init_gdb_breakpoint()
            self.initialized_gdb_bp = True

        if symbol in self.mbp_table:
            self.mbp_table[symbol]['disabled'] = False
        else: 
            self.mbp_table[symbol] = {}
            self.mbp_table[symbol]['disabled'] = False
            self.mbp_table[symbol]['hit_count'] = 0
            self.mbp_table[symbol]['ignore_count'] = 0
            self.mbp_table[symbol]['index'] = self.symbol_index
            self.symbol_index += 1
        
        # make a copy of this new item into mbp_object's dict
        self.mbp_object.add_one_symbol(symbol, self.mbp_table[symbol])

    def disable_breakpoint(self, s):
        if not self.mbp_object:
            return
        print ("disable breakpoint ", s)
        if s.isnumeric():
            symbol = self.lookup_symbol_by_index(int(s))
        else:
            symbol = s

        if symbol is None:
            print ("no symbol is found for index ", s)
            return

        if symbol in self.mbp_table:
            self.mbp_table[symbol]['disabled'] = True
            self.mbp_object.set_bp_attr(symbol, 'disabled', True)
        else: 
            print ("disable symbol: ", symbol, " not found")

    def enable_breakpoint(self, s):
        if not self.mbp_object:
            return
        print ("enable breakpoint ", s)
        if s.isnumeric():
            symbol = self.lookup_symbol_by_index(int(s))
        else:
            symbol = s

        if symbol is None:
            print ("no symbol is found for index ", s)
            return

        if symbol in self.mbp_table:
            self.mbp_table[symbol]['disabled'] = False
            self.mbp_object.set_bp_attr(symbol, 'disabled', False)
        else: 
            print ("enable symbol: ", symbol, " not found")

    def clear_breakpoint(self, s):
        if not self.mbp_object:
            return
        print ("clear breakpoint ", s)
        if s.isnumeric():
            symbol = self.lookup_symbol_by_index(int(s))
        else:
            symbol = s
 
        if symbol is None:
            print ("no symbol is found for index ", s)
            return

        if symbol in self.mbp_table:
            self.mbp_object.clear_one_symbol(symbol)
            self.mbp_table = self.mbp_object.get_mbp_table()
        else: 
            print ("clear symbol: ", symbol, " not found")

    def ignore_breakpoint(self, s, c):
        if not self.mbp_object:
            return
        print ("ignore breakpoint ", s, c)
        if not c.isnumeric():
            print ("ignore count must be a number")
            return

        if s.isnumeric():
            symbol = self.lookup_symbol_by_index(int(s))
        else:
            symbol = s

        if symbol is None:
            print ("no symbol is found for index ", s)
            return
        
        count = int(c)
        if count < 0:
            count = 0
        self.mbp_table[symbol]['ignore_count'] = count
        self.mbp_object.set_bp_attr(symbol, 'ignore_count', count)


    def clearall_breakpoint(self):
        if not self.mbp_object:
            return
        print ("clear all breakpoint")
        self.mbp_object.clear_all_symbol()
        self.mbp_table.clear()    
        

    def listall_breakpoint(self):
        if not self.mbp_object:
            return

        print ("list all breakpoint")

        # fetch the main copy from mbp_object's table. Always update the table using the table in
        # mbreakpoint object as it is the main copy
        self.mbp_table = self.mbp_object.get_mbp_table()

        # sort the dict with the index, so that we can display in index order
        blist = [{k:v} for k, v in sorted(self.mbp_table.items(), key=(lambda x:x[1]['index']))]
        bp_info = self.mbp_object.bp_info if self.mbp_object.bp_info else "in maple::maple_invoke_method()"
        bp_addr = hex(self.mbp_object.bp_addr) if self.mbp_object.bp_addr else "pending address"
        for i in range(len(blist)):
            key = [*blist[i]][0]
            if blist[i][key]['disabled'] is True:
                info = '%s disabled hit_count %d ignore_count %d at %s %s' % (key, blist[i][key]['hit_count'], blist[i][key]['ignore_count'], bp_addr, bp_info)
            else:
                info = '%s enabled hit_count %d ignore_count %d at %s %s' % (key, blist[i][key]['hit_count'], blist[i][key]['ignore_count'], bp_addr, bp_info)
            sys.stdout.write('#%i %s\n' % (blist[i][key]['index'], info))

    def debug(self):
        if not self.mbp_object:
            return
        print(" ======= gdb.breakpoint class bp table: ===========")
        self.mbp_object.disaply_mbp_table()
        print(" ======= Maple break cmd bp table: ===========")
        if self.mbp_table:
            print(self.mbp_table)

    def lookup_symbol_by_index(self,index):
        for k,v in self.mbp_table.items():
            if v['index'] == index:
                return k
        return None

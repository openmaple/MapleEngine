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
import m_symbol
import m_frame
import m_datastore
from inspect import currentframe, getframeinfo
import m_util

def is_mbp_existed():
    """ determine where a Maple breakpoint exists """
    blist = gdb.breakpoints()
    for b in blist:
        m_util.debug_print(__file__, getframeinfo(currentframe()).lineno, is_mbp_existed,\
                        "b.type=", b.type, "b.location=", b.location, "b.thread=", b.thread, "b.enabled=", b.enabled,\
                        "b.is_valid()=", b.is_valid())
        if 'maple::maple_invoke_method' in b.location and b.is_valid():
            return True, b
    return False, None

def get_mbp_id():
    """ get a Maple breakpoint gdb.Breakpoint object id """
    blist = gdb.breakpoints()
    for b in blist:
        m_util.debug_print(__file__, getframeinfo(currentframe()).lineno, get_mbp_id,\
                        "b.type=", b.type, "b.location=", b.location, "b.thread=", b.thread, "b.enabled=", b.enabled,\
                        "b.is_valid()=", b.is_valid())
        if 'maple::maple_invoke_method' in b.location and b.is_valid():
            return b
    return None

def is_maple_invoke_bp_plt_disabled():
    """ determine where Maple breakpoint plt disable or not """
    buf = gdb.execute("info b", to_string=True)
    match_pattern = "<maple::maple_invoke_method(maple::method_header_t const*, maple::MFunction const*)@plt>"
    buf = buf.split('\n')
    for line in buf:
        if match_pattern in line:
            on_off = line.split()[1]
            if on_off is 'y': # it is enabled
                return False
            else:
                return True
    return True #plt does not exist, so, it is disabled

def disable_maple_invoke_bp_plt():
    """ disable a Maple breakpoint plt """
    buf = gdb.execute("info b", to_string=True)
    match_pattern = "<maple::maple_invoke_method(maple::method_header_t const*, maple::MFunction const*)@plt>"
    buf = buf.split('\n')
    for line in buf:
        if match_pattern in line:
            cmd = 'disable ' + line.split()[0]
            gdb.execute(cmd)
                
def get_maple_invoke_bp_stop_addr():
    """
    get Maple breakpoint address and its coresponding information

    params:
      None

    Return:
      None on address and None on breakpoint information if no information found, or
      1, addr: int. breakpoint stop address
      2, info: additional breakpoint information stating breakpoint source code and line

    Return data example:
      addr = 0x00007ffff5b5f061
      info = at /home/che/gitee/maple_engine/maple_engine/src/invoke_method.cpp:151
    """

    buf = gdb.execute("info b", to_string=True)
    match_pattern = "in maple::maple_invoke_method(maple::method_header_t const*, maple::MFunction const*)"
    buf = buf.split('\n')
    for line in buf:
        m_util.debug_print(__file__, getframeinfo(currentframe()).lineno, get_maple_invoke_bp_stop_addr, "line=", line)
        if match_pattern in line:
            x = line.split(match_pattern)
            addr = x[0].split()[-1]
            try:
                addr = int(addr, 16)
            except:
                return None, None

            info = match_pattern + " ".join(x[1:])
            m_util.debug_print(__file__, getframeinfo(currentframe()).lineno, get_maple_invoke_bp_stop_addr,\
                              "addr = ", addr, "info = ", info)
            return addr,  info
    return None, None

class MapleBreakpoint(gdb.Breakpoint):
    """
    MapleBreakpoint class creates an instance of Maple breakpoint using gdb python API,
    and provides Maple breakpoint stop control logic.
    """

    def __init__(self, mtype=gdb.BP_BREAKPOINT, mqualified=True):
        """
        1, create a Maple breakpoint using gdb python api
        2, initialize a Maple symbol table that this breakpoint will stop on.
        3, disable unnecessary plt breakpoint.
        """
        super().__init__('maple::maple_invoke_method')
        disable_maple_invoke_bp_plt()
        self.mbp_table = {}
        self.bp_addr, self.bp_info = get_maple_invoke_bp_stop_addr()

    def stop(self):
        """
        provide stop control logic here.
        note, user will add one or more Maple symbol into mbp_table, only those qualified
        ones in mbp_table will be admitted to make breakpoint stop

        return True: to stop
               False: to bypass
        """

        # if the Maple breakpoint was created before .so is loaded, when the breakpoint is hit,
        # plt will be not disable. So we disable it
        if not is_maple_invoke_bp_plt_disabled():
            disable_maple_invoke_bp_plt()
            return False

        # if the Maple breakpoint was created before .so is loaded, bp_addr, bp_info (location)
        # if not available. But once hit, address and loc start to be available.
        if not self.bp_info or not self.bp_addr:
            self.bp_addr, self.bp_info = get_maple_invoke_bp_stop_addr()

        # once the Maple breakpoint is hit, check a Maple symbol is passed in
        args_addr, args_symbol = m_symbol.get_symbol_name_by_current_frame_args()
        if args_symbol is None:
            return False

        # match pattern is matched but symbol does not end with '_mirbin_info'
        match_pattern = args_symbol[:-12]
        if not match_pattern in self.mbp_table:
            return False

        # Maple symbol for the breakpoint is disabled
        if self.mbp_table[match_pattern]['disabled'] is True:
            return False
                
        match_pattern_addr = m_symbol.get_symbol_address(match_pattern + '_mirbin_info')
        if match_pattern_addr == args_addr:
            if self.mbp_table[match_pattern]['ignore_count'] == 0:
                self.mbp_table[match_pattern]['hit_count'] += 1

                # update the Maple gdb runtime metadata store.
                m_datastore.mgdb_rdata.update_gdb_runtime_data()
                return True
            else :
                self.mbp_table[match_pattern]['ignore_count'] -= 1
                return False
        else: 
            return False

    def get_mbp_table(self):
        return self.mbp_table

    def set_bp_attr(self, symbol, key, value):
        self.mbp_table[symbol][key] = value

    def add_one_symbol(self, symbol,value):
        self.mbp_table[symbol] = value

    def clear_one_symbol(self, symbol):
        self.mbp_table.pop(symbol)

    def clear_all_symbol(self):
        self.mbp_table.clear()

    def disaply_mbp_table(self):
        for mspec in self.mbp_table:
            print ('mapleBreakpoint object[', mspec, ']: ', self.mbp_table[mspec])

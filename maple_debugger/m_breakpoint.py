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
import m_symbol
import m_datastore
import m_util
from m_util import gdb_print
import m_debug

def is_mbp_existed():
    """ determine where a Maple breakpoint exists """
    blist = gdb.breakpoints()
    for b in blist:
        if m_debug.Debug: m_debug.dbg_print("b.type=", b.type, "b.location=", b.location, "b.thread=", b.thread, "b.enabled=", b.enabled,\
                        "b.is_valid()=", b.is_valid())
        if 'maple::maple_invoke_method' in b.location and b.is_valid():
            return True, b
    return False, None

def get_mbp_id():
    """ get a Maple breakpoint gdb.Breakpoint object id """
    blist = gdb.breakpoints()
    for b in blist:
        if m_debug.Debug: m_debug.dbg_print("b.type=", b.type, "b.location=", b.location, "b.thread=", b.thread, "b.enabled=", b.enabled,\
                        "b.is_valid()=", b.is_valid())
        if 'maple::maple_invoke_method' in b.location and b.is_valid():
            return b
    return None

def is_maple_invoke_bp_plt_disabled(buf):
    """
        determine where Maple breakpoint plt disable or not

        params:
          buf: a string output of m_util.gdb_exec_to_str("info b")
    """
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

def disable_maple_invoke_bp_plt(buf):
    """
        disable a Maple breakpoint plt

        params:
          buf: a string output of m_util.gdb_exec_to_str("info b")
    """
    match_pattern = "<maple::maple_invoke_method(maple::method_header_t const*, maple::MFunction const*)@plt>"
    buf = buf.split('\n')
    for line in buf:
        if match_pattern in line:
            cmd = 'disable ' + line.split()[0]
            m_util.gdb_exec(cmd)

def update_maple_invoke_bp(buf, op_enable = True):
    """
        update maple::maple_invoke_method breakpoint
        when number of enabled maple breakpoints is 0, disable maple::maple_invoke_method
        when number of enabled maple breakpoints changes from 0 to non-0, enable maple::maple_invoke_method
        but if maple::maple_invoke_method is pending, then do nothing.

        params:
          buf: a string output of m_util.gdb_exec_to_str("info b")
          op_enable = True, to enable the maple::maple_invoke_method
          op_enable = False, to disable the maple::maple_invoke_method
    """
    match_pattern = "in maple::maple_invoke_method(maple::method_header_t const*, maple::MFunction const*)"
    buf = buf.split('\n')
    for line in buf:
        if match_pattern in line:
            cmd = 'disable ' if op_enable is False else 'enable '
            cmd += line.split()[0]
            m_util.gdb_exec(cmd)

def get_maple_invoke_bp_stop_addr(buf):
    """
    get Maple breakpoint address and its coresponding information

    params:
      buf: a string output of m_util.gdb_exec_to_str("info b")

    Return:
      None on address and None on breakpoint information if no information found, or
      1, addr: int. breakpoint stop address
      2, info: additional breakpoint information stating breakpoint source code and line

    Return data example:
      addr = 0x00007ffff5b5f061
      info = at /home/test/gitee/maple_engine/maple_engine/src/invoke_method.cpp:151
    """

    match_pattern = "in maple::maple_invoke_method(maple::method_header_t const*, maple::MFunction const*)"
    buf = buf.split('\n')
    for line in buf:
        if m_debug.Debug: m_debug.dbg_print("line=", line)
        if match_pattern in line:
            x = line.split(match_pattern)
            addr = x[0].split()[-1]
            try:
                addr = int(addr, 16)
            except:
                return None, None

            info = match_pattern + " ".join(x[1:])
            if m_debug.Debug: m_debug.dbg_print("addr = ", addr, "info = ", info)
            return addr,  info
    return None, None

class MapleBreakpoint(gdb.Breakpoint):
    """
    MapleBreakpoint class creates an instance of Maple breakpoint using gdb python API,
    and provides Maple breakpoint stop control logic.
    """

    def __init__(self, mtype=gdb.BP_BREAKPOINT, mqualified=True):
        """
        1, Creates a Maple breakpoint using gdb python api
        2, Initializes a Maple symbol table that this breakpoint will stop on.
        3, Disables unnecessary plt breakpoint.
        """
        super().__init__('maple::maple_invoke_method')
        buf = m_util.gdb_exec_to_str("info b")
        disable_maple_invoke_bp_plt(buf)
        self.mbp_table = {} # the symbols here are NOT mirbin_info symbols
        self.bp_addr, self.bp_info = get_maple_invoke_bp_stop_addr(buf)

        self.mbp_addr_sym_table = {} # a symbol address keyed table for fast stop logic
        self.load_objfiles = len(gdb.objfiles()) # initial value

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
        buf = m_util.gdb_exec_to_str("info b")
        if not is_maple_invoke_bp_plt_disabled(buf):
            disable_maple_invoke_bp_plt(buf)
            return False

        # if the Maple breakpoint was created before .so is loaded, bp_addr, bp_info (location)
        # if not available. But once hit, address and loc start to be available.
        if not self.bp_info or not self.bp_addr:
            self.bp_addr, self.bp_info = get_maple_invoke_bp_stop_addr(buf)

        # determine whether need to look up pending symbol's address. If the number of loaded libraries
        # is not eaual to the one saved, then libs might have been loaded or unloaded, so we try to update
        if len(gdb.objfiles()) != self.load_objfiles:
            self.load_objfiles = len(gdb.objfiles())
            pending_addr_symbol_list = self.get_pending_addr_symbol_list()
            if len(pending_addr_symbol_list) > 0:
                for sym in pending_addr_symbol_list:
                    self.update_pending_addr_symbol(sym)

        # NOTE!!! here we are getting the current stack's mir_header's address
        args_addr = m_symbol.get_symbol_addr_by_current_frame_args()
        if not args_addr or not args_addr in self.mbp_addr_sym_table:
            return False

        # mir_header address matches one breakpoint address list
        table_args_addr = self.mbp_addr_sym_table[args_addr]
        args_symbol = table_args_addr['mir_symbol']
        match_pattern = table_args_addr['symbol']

        # it is possible that match pattern is in mbp_addr_sym_table for address check,
        # but the self.mbp_addr_sym_table[args_addr]['symbol'] could have been cleared
        # in self.mbp_table by user's mb -clear command
        if not match_pattern in self.mbp_table:
            return False

        # Maple symbol for the breakpoint is disabled
        table_match_pattern = self.mbp_table[match_pattern]
        if table_match_pattern['disabled'] is True:
            return False

        if table_match_pattern['ignore_count'] > 0:
            table_match_pattern['ignore_count'] -= 1
            return False

        table_match_pattern['hit_count'] += 1

        # update the Maple gdb runtime metadata store.
        m_datastore.mgdb_rdata.update_gdb_runtime_data()
        # update the Maple frame change count
        m_datastore.mgdb_rdata.update_frame_change_counter()
        return True

    def clear_one_symbol(self, symbol):
        self.mbp_table.pop(symbol)

    def clear_all_symbol(self):
        self.mbp_table.clear()

    def display_mbp_table(self):
        for mspec in self.mbp_table:
            buf = 'mapleBreakpoint object mbp_table[' + str(mspec) + ']: ' + str(self.mbp_table[mspec])
            gdb_print(buf)

    def display_mbp_addr_sym_table(self):
        for mspec in self.mbp_addr_sym_table:
            buf = 'mapleBreakpoint object mbp_addr_sym_table[' + str(mspec) + ']: ' + str(self.mbp_addr_sym_table[mspec])
            gdb_print(buf)

    def get_pending_addr_symbol_list(self):
        return [ k for k, v in self.mbp_table.items() if not v['hex_addr'] ]

    def add_known_addr_symbol_into_addr_sym_table(self, symbol):
        """
        params:
          symbol: string. This is a regular symbol with NO _mirbin_info
        """
        if not symbol in self.mbp_table:
            return

        if self.mbp_table[symbol]['hex_addr'] and self.mbp_table[symbol]['address'] != 0:
            mirbin_addr = self.mbp_table[symbol]['address'] + 4
            mirbin_symbol = symbol + '_mirbin_info'
            self.mbp_addr_sym_table[hex(mirbin_addr)] = {}
            self.mbp_addr_sym_table[hex(mirbin_addr)]['mir_symbol']  = mirbin_symbol
            self.mbp_addr_sym_table[hex(mirbin_addr)]['symbol']   = symbol
        return

    def update_pending_addr_symbol(self, symbol):
        """
        try to get the address of a symbol. If available, update mbp_table[symbol], AND add a new item
        into mbp_addr_sym_table[addr+4] = symbol + '_mirbin_info'

        params:
          symbol: string. This is a NOT a mirbin symbol, it is a regular symbol user set in mb cmd.
        """
        addr = m_symbol.get_symbol_address(symbol)
        if not addr:
            return
        self.mbp_table[symbol]['hex_addr'] = addr
        self.mbp_table[symbol]['address'] = int(addr,16)

        mirbin_addr = self.mbp_table[symbol]['address'] + 4
        mirbin_symbol = symbol + '_mirbin_info'
        self.mbp_addr_sym_table[hex(mirbin_addr)] = {}
        self.mbp_addr_sym_table[hex(mirbin_addr)]['mir_symbol']  = mirbin_symbol
        self.mbp_addr_sym_table[hex(mirbin_addr)]['symbol']   = symbol

    def get_enabled_mbp_number(self):
        return len([ k for k, v in self.mbp_table.items() if not v['disabled'] ])

    def update_mbp(self):
        """
        when enabled maple breakpoints reach to 0, we disable the breakpint of maple::maple_invoke_method
        for better performance. When enabled maple breakpoints changes from 0 to 1 or more, we enable the
        breakpoint maple::maple_invoke_method.
        However, if the maple::maple_invoke_method is pending, we do not do anything since it is not 
        activated yet

        However, the beter way to do this is to use mbp_id.enabled = True or False to enable or disable
        the maple::maple_invoke_method. We chose not to do it this way, because our regression test suite
        needs an output pattern that can be checked easily. changing enabled=True or False does not make
        this easier.
        """

        mbp_id = get_mbp_id()
        if not mbp_id:
            return
        # if maple:maple_invoke_method breakpoint is pending, we do not have to do anything.
        if mbp_id.pending:
            return

        # get the maple breakpoint number for those are enabled
        mbp_num = self.get_enabled_mbp_number()
        if m_debug.Debug: m_debug.dbg_print("mbp_num returned =", mbp_num)
        # disable maple:maple_invoke_method breakpoint
        buf = m_util.gdb_exec_to_str("info b")
        if mbp_num == 0:
            # disable this breakpoint
            update_maple_invoke_bp(buf, False)
        else:
            # enable this breakpoint
            update_maple_invoke_bp(buf, True)

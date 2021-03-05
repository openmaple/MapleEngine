#
# Copyright (C) [2020-2021] Futurewei Technologies, Inc. All rights reserved.
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

import lldb
import os
import copy
import re
import sys
import pickle
from mlldb import m_config_lldb as m_debug
from shared import m_util
from shared.m_util import MColors
from shared.m_util import mdb_print
from  mdata import m_symbol
from  mdata import m_datastore
import m_set

mbp_glo_table = {}

def does_mbp_exist():

    """ determine where a Maple breakpoint exists """
    target = lldb.debugger.GetSelectedTarget()
    #namelist = lldb.SBStringList()
    #namelist.AppendString('maple::maple_invoke_method')
    #print(dir(namelist))
    #namelist.AppendString('maple::maple_invoke_method')
    #print ("namelist.GetSize()=", namelist.GetSize())
    #print ("namelist.GetStringAtIndex(0)=", namelist.GetStringAtIndex(0))
    #print (dir(target.GetBreakpointNames(namelist)))
    #print (target.GetBreakpointNames(namelist))
    #print (dir(namelist))
    #print ("namelist.IsValid()=", namelist.IsValid())
    #print ("namelist.GetSize()=", namelist.GetSize())
    #print ("namelist.GetStringAtIndex(0)=", namelist.GetStringAtIndex(0))

    num_bp = target.GetNumBreakpoints()
    if m_debug.Debug: m_debug.dbg_print("does_mbp_exist: num_bp=", num_bp)
    for i in range(num_bp):
        bp = target.GetBreakpointAtIndex(i)
        if m_debug.Debug: m_debug.dbg_print("\nbp=", bp)
        enabled_bool = bp.IsEnabled()
        if m_debug.Debug: m_debug.dbg_print("id=", bp.GetID(), "enabled_bool=", enabled_bool, "isValid()=", bp.IsValid())
        if m_debug.Debug: m_debug.dbg_print("num_location=", bp.GetNumLocations(), "num_resolved_loc=", bp.GetNumResolvedLocations())
        #print("bp.GetThreadName()=", bp.GetThreadName(), "bp.GetQueueName()=", bp.GetQueueName())
        #print("bp.MatchesName(maple::maple_invoke_method)=", bp.MatchesName('maple::maple_invoke_method'))

        stream = lldb.SBStream()
        bp.GetDescription(stream)
        if m_debug.Debug: m_debug.dbg_print ("selected bp desc=", stream.GetData())
        bp_name =  stream.GetData().split("name = ")[1].split(",")[0].replace("'","")
        if m_debug.Debug: m_debug.dbg_print ("bp_name=", bp_name)

        if bp_name == 'maple::maple_invoke_method':
            return True, bp


        #__getattr__ = lambda self, name: _swig_getattr(self, SBBreakpointLocation, name)
        #print ("__getattr__=", __getattr)

        # using GetLocatioAtIndex() could help to get the breakpoint name Once we run the
        # the program.
        '''
        bl = bp.GetLocationAtIndex(0)
        print ("bl=", bl)
        if not bl:
            continue
        print("bl.GetAddress()=", bl.GetAddress())
        print("bl.GetAddress().GetSymbol().GetName()=", bl.GetAddress().GetSymbol().GetName())
        bp_name = bl.GetAddress().GetSymbol().GetName()
        print("bl.GetLoadAddress()=", bl.GetLoadAddress())
        '''

        '''
        for bl in bp:
            print("bl=", bl)
            bl_load_addr = bl.GetLoadAddress()
            bl_addr = bl.GetAddress()
            print("bp", bp.GetID(), "loc addr=", bl_addr, "loc_load_addr=", loc_load_addr)
            if 'maple::maple_invoke_method' in bl_addr.GetSymbol().GetName() and enabled_bool:
                print("found match")
                return True, bp
            else:
                print("no match")
        '''

    return False, None


def get_mbp_id():
    """ get a Maple breakpoint lldb.Breakpoint object id """
    """ we probably do not need it, we can always get breakpoint object id from lldb.SBBreakpoint instace we created
        for maple::maple_invoke_method"""
    return None

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

    #match_pattern = "in maple::maple_invoke_method(maple::method_header_t const*, maple::MFunction const*)"
    #match_pattern = "libmplre.so`maple::maple_invoke_method(maple::method_header_t const*, maple::MFunction const*)"
    match_pattern = "maple::maple_invoke_method"
    buf = buf.split('\n')

    target = lldb.debugger.GetTargetAtIndex(0)
    bp_count = target.GetNumBreakpoints()
    stream = lldb.SBStream()

    #for line in buf:
    for i in range(0, bp_count):
        target.GetBreakpointAtIndex(i).GetDescription(stream)
        if m_debug.Debug: print ("selected bp desc=", stream.GetData())
        line = stream.GetData()
        if m_debug.Debug: m_debug.dbg_print("line=", line)
        if match_pattern in line or match_pattern == line:
            #x = line.split(match_pattern)
            #addr = x[0].split()[-1]
            info = target.GetBreakpointAtIndex(i).GetLocationAtIndex(0).GetAddress()
            #info.GetDescription(stream)
            #x = "  at " + stream.GetData().split(" at ")[1]
            addr = target.GetBreakpointAtIndex(i).GetLocationAtIndex(0).GetLoadAddress()
            #print("x: ", x)
            #print("Addr Info: ", info)
            #print("Addr: ", hex(addr))
            """
            try:
                addr = int(addr, 16)
            except:
                print("Issue Parsing Addr: ", addr, iaddr)
                return None, None

            info = match_pattern + " ".join(x[1:])
            """
            if m_debug.Debug: m_debug.dbg_print("addr = ", addr, "info = ", info)
            return addr,  info
    return None, None


class MapleBreakpoint():

    def __init__(self):
        """
        first, create a lldb native breakpoint on maple::maple_invoke_method function
        """
        target = lldb.debugger.GetSelectedTarget()
        breakpoint = target.BreakpointCreateByName('maple::maple_invoke_method')

        # add a callback to the breakpoint
        breakpoint.SetScriptCallbackFunction('m_breakpoint.MapleBreakpoint.stop')

        self.mbp_id = breakpoint
        global mbp_glo_table

        self.mbp_table = mbp_glo_table # the symbols here are NOT mirbin_info symbols
        #self.bp_addr, self.bp_info = get_maple_invoke_bp_stop_addr(buf)

        self.mbp_addr_sym_table = {} # a symbol address keyed table for fast stop logic

    @staticmethod
    def stop(bp_frame, bp_loc, bp_dict):
        '''
        1. from current frame. find out the mir_header address
        2. compare this mir_header address to maple breakpoint symbol's address
          2.1 maple::maple_invoke_method belongs to libmplr.so
          2.2 maple breakpoint symbols belong to libcore.so or application.so
          2.3 because of 2.1 and 2.2, we have to search target.modules, to look up
              the maple breakpoint's address or names...
          2.4 I was looking for a API, for a given symbol's address, return its name,
              or for a given symbol name, returns its address in the stack
        '''
        from  mdata import m_datastore
        global mbp_glo_table

        #Only load the dict once
        if not bool(mbp_glo_table):
            with open('mbp_glo_table.pkl', 'rb') as handle:
                mbp_glo_table = pickle.loads(handle.read())
                #if m_debug.Debug: m_debug.dbg_print("Loaded glo dict:",mbp_glo_table)

        #if m_debug.Debug: m_debug.dbg_print ("selected target = ", lldb.debugger.GetSelectedTarget())
        target = lldb.debugger.GetSelectedTarget()

        # lldb somehow disable system level profiling. we enable it if trace is set to on
        if m_set.msettings['trace'] == 'on':
            sys.setprofile(m_set.trace_maple_debugger)

        # fv is an object of SBValue class
        fv = bp_frame.FindVariable("mir_header")
        #if m_debug.Debug: m_debug.dbg_print ("mir_header target = ", fv)

        if not fv.IsValid():
            return False

        # mir_header_addr
        mir_header_addr = fv.GetValueAsSigned()
        ### get frame module
        sbaddr = target.ResolveLoadAddress(mir_header_addr)
        resolved_sym_name = sbaddr.GetSymbol().GetName()
        #if m_debug.Debug: m_debug.dbg_print("Resolved_sym_name: ", resolved_sym_name)

        #if m_debug.Debug: m_debug.dbg_print("Global Table: ", mbp_glo_table.items())
        blist = [{k:v} for k, v in sorted(mbp_glo_table.items(), key=(lambda x:x[1]['index']))]
        #if m_debug.Debug: m_debug.dbg_print("Blist: ", blist)

        for v in blist:
            symbol_regex_match = False
            key = [*v][0]

            #if a regular expression test match
            if mbp_glo_table[key]['is_regex']:
                # TODO: precompile the regexes before the loop for speed
                symbol_regex_match = re.match(key,resolved_sym_name)

            if resolved_sym_name == key + "_mirbin_info" or symbol_regex_match:
                mbp_glo_table[key]['hit_count'] = int(mbp_glo_table[key]['hit_count']) + 1
                if int(mbp_glo_table[key]['ignore_count']) > 0:
                    mbp_glo_table[key]['ignore_count'] = int(mbp_glo_table[key]['ignore_count']) - 1
                    return False

                #If disabled return
                if mbp_glo_table[key]['disabled']:
                    return False

                # update the Maple mdb runtime metadata store.
                #m_datastore.mlldb_rdata.update_mdb_runtime_data() #LGD
                # update the Maple frame change count
                m_datastore.mlldb_rdata.update_frame_change_counter()

                if mbp_glo_table[key]['debug']:
                    print ("frame                          = ", bp_frame)
                    print ("bp_loc                         = ", bp_loc)
                    print ("fv-mir_header                  = ", fv)
                    print ("fv isValid                     = ", fv.IsValid())
                    print ("fv type name                   = ", fv.GetTypeName())
                    print ("fv display type name           = ", fv.GetDisplayTypeName())
                    print ("fv name                        = ", fv.GetName())
                    print ("fv summary in string           = ", fv.GetSummary())
                    print ("fv value                       = ", fv.GetValue())
                    print ("fv value assigned              = ", fv.GetValueAsSigned())
                    print ("fv value unassigned            = ", fv.GetValueAsUnsigned())
                    print ("sbaddr                         = ", sbaddr)
                    print ("sbaddr.GetSymbol()             = ", sbaddr.GetSymbol())
                    print ("sbaddr.GetSymbol().GetName()   = ", sbaddr.GetSymbol().GetName())
                    print ("resolved_sym_name              = ", resolved_sym_name)
                    print ("mbp items                      = ", mbp_glo_table)
                return True

        #print ("resolved_sym_name = ", resolved_sym_name)

        # NOw we can do comparison. resolved_sym_name can compare the maple breakppint name that
        # user specified in mbreak command.
        # here is just an example to compare to a hardcoded maple breakpoint name of Ljava_2Flang_2FObject_3B_7C_3Cclinit_3E_7C_28_29V_
        #if resolved_sym_name == 'Ljava_2Flang_2FObject_3B_7C_3Cclinit_3E_7C_28_29V_mirbin_info':
            # 1. now you should check if Ljava_2Flang_2FObject_3B_7C_3Cclinit_3E_7C_28_29V is enabled or disabled. if disabled, return False
            # 2. you also check if ignore count for Ljava_2Flang_2FObject_3B_7C_3Cclinit_3E_7C_28_29V is 0 or not. if not 0, decrement it, return False
            # 3. you could call m_datastore.update_gdb_runtime_data() to update the cache.
        #    return True

        return False

    def clear_one_symbol(self, symbol):
        self.mbp_table.pop(symbol)

    def clear_all_symbol(self):
        self.mbp_table.clear()

    def display_mbp_table(self):
        for mspec in self.mbp_table:
            buf = 'mapleBreakpoint object mbp_table[' + str(mspec) + ']: ' + str(self.mbp_table[mspec])
            print(buf)

    def update_mbp(self):
        """
        when enabled maple breakpoints reach 0, we disable the breakpoint of maple::maple_invoke_method
        for better performance. When enabled maple breakpoints changes from 0 to 1 or more, we enable the
        breakpoint maple::maple_invoke_method.
        However, if the maple::maple_invoke_method is pending, we do not do anything since it is not
        activated yet

        However, the better way to do this is to use mbp_id.enabled = True or False to enable or disable
        the maple::maple_invoke_method. We chose not to do it this way, because our regression test suite
        needs an output pattern that can be checked easily. changing enabled=True or False does not make
        this easier.
        """

        with open('mbp_glo_table.pkl', 'wb') as handle:
            pickle.dump(self.mbp_table, handle)

        #mbp_id = get_mbp_id()
        mbp_id = self.mbp_id
        #if m_debug.Debug: m_debug.dbg_print("mbp_id", mbp_id)
        if not mbp_id:
            return
        # if maple:maple_invoke_method breakpoint is pending, we do not have to do anything.
        #if mbp_id.pending:
        #    return
        return

        # get the maple breakpoint number for those are enabled
        #mbp_num = self.get_enabled_mbp_number()
        #if m_debug.Debug: m_debug.dbg_print("mbp_num returned =", mbp_num)
        ## disable maple:maple_invoke_method breakpoint
        #buf = m_util.gdb_exec_to_str("info b")
        #if mbp_num == 0:
        #    # disable this breakpoint
        #    update_maple_invoke_bp(buf, False)
        #else:
        #    # enable this breakpoint
        #    update_maple_invoke_bp(buf, True)

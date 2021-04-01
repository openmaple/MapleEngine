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

#import gdb
import lldb
from mlldb import m_info
from mlldb import m_breakpoint
from shared.m_util import mdb_print
from shared import m_util
import m_config_lldb as m_debug
import traceback
import sys
import platform

def get_newest_frame():
    """
    get a gdb.Frame object representing the innermost frame on stack
    """

    try:
        #frame = lldb.newest_frame()
        #target = lldb.GetSelectedTarget()
        target = lldb.debugger.GetTargetAtIndex(0)
        process = target.GetProcess()
        thread = process.GetSelectedThread()
        frame = thread.GetFrameAtIndex(0)
    except:
        mdb_print("Trouble accessing newest LLDB Frame")
        return None

    #if not frame or not frame.is_valid():
    if not frame or not frame.IsValid():
        return None
    #print("MFrame: ",frame)
    return frame

def get_selected_frame():
    """
    get a gdb.Frame object representing the selected frame in current
    stack if it is valid
    """

    try:
        #frame = lldb.frame()
        frame = lldb.debugger.GetTargetAtIndex(0).GetProcess().GetSelectedThread().GetSelectedFrame()
        process = lldb.debugger.GetTargetAtIndex(0).GetProcess()
        for t in process:
            #print("Thread= ", t.GetThreadID(), t.GetIndexID(), t.GetName(), t.GetStopReason())
            if(t.GetStopReason() == 3):  # Get the stopped Thread
                frame = t.GetSelectedFrame()
        #frame = lldb.debugger.GetTargetAtIndex(0).GetProcess().GetThreadByID(0).GetSelectedFrame()
    except:
        mdb_print("-"*60)
        mdb_print("Trouble accessing selected LLDB Frame:")
        traceback.print_stack(file=sys.stdout)
        mdb_print("-"*60)
        traceback.print_exc(file=sys.stdout)
        mdb_print("-"*60)
        return None

    #if not frame or not frame.is_valid():
    if not frame or not frame.IsValid():
        return None
    return frame

def get_next_older_frame(frame):
    """
    get a gdb.Frame object representing the next older frame on stack
    """

    #if not frame or not frame.is_valid():
    if not frame or not frame.IsValid():
        mdb_print("Trouble accessing older LLDB Frame")
        return None
    else:
        return frame.get_parent_frame()
        #return frame.older()

    #target = debugger.GetSelectedTarget()
    #process = target.GetProcess()
    #thread = process.GetSelectedThread()
    #for sframe in thread:

def get_next_newer_frame(frame):
    """
    get a lldb.Frame object representing the next newer frame on stack
    """

    if not frame or not frame.is_valid():
        mdb_print("Trouble accessing newer LLDB Frame")
        return None
    else:
        #return frame.newer()
        mark = 0
        target = lldb.debugger.GetSelectedTarget()
        process = target.GetProcess()
        thread = process.GetSelectedThread()
        for sframe in thread:
            if mark == 1 : return sframe
            if frame == sframe: mark = 1

def is_maple_frame(frame):
    #mdb_print("LLDB Frame func name:  " + frame.GetSymbol().GetName())
    #mdb_print("LLDB Frame func dname: " + frame.GetSymbol().GetDisplayName())
    #return frame.GetSymbol().GetDisplayName() == "maple::maple_invoke_method"
    return "maple::maple_invoke_method" in frame.GetSymbol().GetDisplayName()

def get_frame_info(frame):
    """
    return a string buffer containing selected frame info
    """

    if not frame or not frame.IsValid():
        return  None, None, None

    #info_buffer = "{} in {}".format(hex(frame.pc()), frame.name())
    info_buffer = "{} in {}".format(frame.GetPCAddress().__hex__, frame.GetDisplayFunctionName())

    #frame_symbol = frame.function()
    #frame_sal = frame.find_sal()
    #frame_symbol = frame.GetDisplayFunctionName()
    frame_symbol = frame.GetFunction()
    frame_sal = frame.GetSymbol()

    """
    if frame_symbol:
        if m_debug.Debug:
            m_debug.dbg_print("frame symbol name: ", frame_symbol.name)
            m_debug.dbg_print("frame symbol line: ", frame_symbol.line)
            m_debug.dbg_print("frame symbol linkage_name: ", frame_symbol.linkage_name)
            m_debug.dbg_print("frame symbol print_name: ", frame_symbol.print_name)
            m_debug.dbg_print("frame symbol value: ", frame_symbol.value())
            m_debug.dbg_print("frame symbol address: ", frame_symbol.addr_class)

    if frame_sal:
        if m_debug.Debug:
            m_debug.dbg_print("frame sal pc: ", hex(frame_sal.pc) if frame_sal.pc != None else "None")
            m_debug.dbg_print("frame sal last: ", hex(frame_sal.last) if frame_sal.last != None else "None")
            m_debug.dbg_print("frame sal line: ", frame_sal.line)
            m_debug.dbg_print("frame sal.symtab filename: ", frame_sal.symtab.filename)
            m_debug.dbg_print("frame sal.symtab full filename: ", frame_sal.symtab.fullname())
    """

    #print("Frame Info: ", info_buffer, "\nFrame symbol:", frame_symbol, "\nFrame st:", frame_sal)
    return info_buffer, frame_symbol, frame_sal

def get_current_frame_rip(frame):
    """
    for a given frame, get its register "pc" value, and use the pc value to get the first
    instruction's memory address, return it as frame's rip

    params:
      frame: a gdb.Frame object

    returns:
      rip address, or frame's first instruction address: an int
    """

    if not frame:
        return None

    #arch = frame.architecture() #TODO: figure out api for this
    arch = platform.machine()
    #current_pc = frame.read_register("pc")
    current_pc = frame.GetPCAddress()
    if m_debug.Debug: m_debug.dbg_print("current_pc obj=", current_pc)
    current_pc = int(current_pc)

    # arch.disassemble(start_pc) returns a list of disassembled instructions from start_pc.
    # each element of the list is a dict contains 'addr', 'asm', 'length' which addr is the
    # memory address of the instruction.
    # see https://sourceware.org/gdb/onlinedocs/gdb/Architectures-In-Python.html#Architectures-In-Python
    disa = arch.disassemble(current_pc)[0]
    if m_debug.Debug: m_debug.dbg_print("disa=", disa)

    return disa['addr']

def get_newest_maple_frame_func_lib_info():
    """
    starting from currently selected frame, find out the newest Maple frame, and then
    call get_maple_frame_func_lib_info() to return func header offset, library .so path,
    library's coresponding assembly file path, func_header address, and the Maple frame itself

    returns:
      1, func_header_offset: in string.
      2, so_path: string. so library file full path.
      3, asm_path: string. asm file full path.
      4, func_header: int. address of function header.
      5, mirmpl_path: string. .mpl.mir.mpl file full path

      or all None if no valid Maple frame found
    """

    frame = get_selected_frame()

    if not frame:
        mdb_print('Selected Maple frame is invalid')
        return None, None, None, None, None

    mdb_print('Maple frame located: ')

    index = 0
    info_buffer = None
    frame_symbol = None
    frame_sal = None
    while frame:
        #print('Maple frame: ', frame)
        if not is_maple_frame(frame):
            frame = get_next_older_frame(frame)
            print('Getting older frame: ', frame)
            index += 1
            continue

        info_buffer, frame_symbol, frame_sal = get_frame_info(frame)
        if not frame_sal or not frame_symbol:
            mdb_print('Unable to locate Maple frame data')
            return None, None, None, None, None

        return get_maple_frame_func_lib_info(frame)

    return None, None, None, None, None

def get_maple_frame_func_lib_info(frame):
    """
    for a given Maple frame, find and return function related frame information

    params:
      frame: a mdb.Frame object.

    returns:
      1, func_header_offset: in string.
      2, so_path: string. so library file full path.
      3, asm_path: string. asm file full path.
      4, func_header: int. address of function header.
      5, mirmpl_path: in string. .mpl.mir.mpl file full path
    """

    frame_rip = int(frame.FindRegister("rip").value, 0)
    #frame_rip = get_current_frame_rip(frame)
    #buf = m_util.mdb_exec_to_str("info b")
    #buf = m_util.mdb_exec_to_str("break list")
    buf = "" # Not used currently
    bp_addr, bp_info = m_breakpoint.get_maple_invoke_bp_stop_addr(buf)
    if m_debug.Debug: m_debug.dbg_print("frame_rip =", frame_rip, "bp_addr=", bp_addr)

    # if breakpoint address is not same to frame_rip (first instruction address of the frame),
    # this means program already passed first instruction of the function
    if bp_addr != frame_rip and False:
        if m_debug.Debug: print("bp_addr != rip")
        func_addr_offset = m_info.get_initialized_maple_func_addrs()
        # func_addr_offset in string
        if not func_addr_offset:
            func_addr_offset = "0x1158:0078:"
            if m_debug.Debug: m_debug.dbg_print("func_addr_offset=None")
            #return None, None, None, None, None

        func_header = m_info.get_initialized_maple_func_attr('func.header')
        start_addr, so_path, asm_path, mirmpl_path = m_info.get_lib_so_info(func_header)
        if m_debug.Debug: m_debug.dbg_print("func_addr_offset=", func_addr_offset, "so_path=", so_path, \
                                            "asm_path=", asm_path, "func_header=", func_header, "mirmpl_path=", mirmpl_path)
        if m_debug.Debug: print("not equal")
        return func_addr_offset, so_path, asm_path, func_header, mirmpl_path

    # the breakpoint address is same to frame.rip (first instruction address in the memory),
    # this means program just stops at the first instruction of the function, so the func is NOT
    # initialized yet
    func_addr_offset, so_path, asm_path, func_header, mirmpl_path  = m_info.get_uninitialized_maple_func_addrs()
    if m_debug.Debug:
        print("func_addr_offset",func_addr_offset)
        print("so_path",so_path)
        print("asm_path",asm_path)
        print("func_header",func_header)
        print("mirmpl_path",mirmpl_path)

    # func_addr_offset in string , format of func_addr:offset:
    if not func_addr_offset or not so_path or not asm_path or not func_header or not mirmpl_path:
        if m_debug.Debug: m_debug.dbg_print("func_addr_offset=", func_addr_offset, "so_path=", so_path, "asm_path=", asm_path)
        if m_debug.Debug: m_debug.dbg_print("func_header=", func_header, "mirmpl_path=",mirmpl_path)
        return None, None, None, None, None
    return func_addr_offset, so_path, asm_path,func_header, mirmpl_path

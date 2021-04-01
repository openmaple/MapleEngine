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
import os
import re
import m_set
import m_util
from m_util import gdb_print
import m_debug
import m_symbol

def get_current_thread():
    return gdb.selected_thread()

num_objfiles = 0
cached_proc_mapping = ""
def get_info_proc_mapping():
    """
    wrapper of 'info proc mapping' cmd with cached mapping info
    """
    global num_objfiles, cached_proc_mapping
    num = len(gdb.objfiles())
    if num_objfiles != num:
        num_objfiles = num
        cached_proc_mapping = m_util.gdb_exec_to_str('info proc mapping')
    return cached_proc_mapping

def get_maple_frame_addr():
    """
    get current Maple frame addr via 'info args' gdb command.

    returns:
        address in string. e.g. 0x7fff6021c308
        or None if not found.
    """
    buf = m_util.gdb_exec_to_str('info args')
    if buf.find('_mirbin_info') != -1 and buf.find('mir_header') != -1:
        x = buf.split()
        return x[2]
    elif buf.find('_mirbin_info') != -1 and buf.find('_mirbin_code') != -1 and buf.find('header = ') != -1: #for dynamic language
        return buf.split('header = ')[1].split()[0]
    else:
        return None


def get_so_base_addr(name):
    """
    for a given name of library, return its base address via 'info proc mapping' cmd

    returns:
      base address of specified so file in string format, or
      None if not found
    """

    if not name:
        return None
    buf = get_info_proc_mapping()
    if not buf:
        return None

    buf = buf.split('\n')
    for line in buf:
        if not name in line:
            continue
        x = line.split()
        for i in range(len(x)):
            if name in x[i]:
                if x[i-1] == '0x0' or x[i-1] == '0x00':
                    return x[0]

    return None

def get_initialized_maple_func_attr(name):
    """
    for a given attribute name, return its value via 'print' command in string format.
    """

    if name and name in ['func.header', 'func.lib_addr', 'func.pc']:
        cmd = 'p/x *(long long*)&' + name
    else:
        return None

    try:
        buf = m_util.gdb_exec_to_str(cmd)
    except:
        return None

    if buf and buf[0] == '$' and ' = 0x' in buf:
        return buf.split(' = ')[1].rstrip()
    return None

def get_initialized_maple_func_addrs():
    """
    get an initialized Maple frame func header address

    returns:
      func header address in string format of xxxxxx:yyyyyy: format where
      xxxxxx means (header - lib_base_addr),
      yyyyyy means (pc - header - header_size)
    """

    header = get_initialized_maple_func_attr('func.header')
    pc = get_initialized_maple_func_attr('func.pc')

    if m_debug.Debug: m_debug.dbg_print("header=", header, "pc=", pc)
    if not header or not pc:
        return None

    try:
        header = int(header, 16)
    except:
        return None
    try:
        pc = int(pc, 16)
    except:
        return None

    lib_addr = get_lib_addr_from_proc_mapping(header)
    if m_debug.Debug: m_debug.dbg_print("header=", header, "lib_addr=",lib_addr, "pc=", pc)
    if not lib_addr:
        return None
    try:
        buf = m_util.gdb_exec_to_str('x/1xw ' + str(header))
    except:
        return None

    if not buf:
        return None
    header_size = int(buf.split(':')[1],16)

    if (header < lib_addr):
        gdb_print ("Warning: The header address is lower than lib_addr.")
        return None
    if (pc < header):
        gdb_print ("Warning: The pc address is lower than the header address.")
        return None

    xxxxxx = header - lib_addr
    yyyyyy = pc - header - header_size

    return hex(xxxxxx) + ":" + "{:04x}".format(yyyyyy)+":"

#####################################################
### function section related to 'info proc mapping'
#####################################################
proc_mapping_re = re.compile(r'(0x[0-9a-f]+)[ \t]+(0x[0-9a-f]+)[ \t]+0x[0-9a-f]+[ \t]+0x[0-9a-f]+[ \t]+(/\S+)', re.M)
def get_lib_addr_from_proc_mapping(addr):
    """
    for a given Maple method address, find out the base address of the .so lib
    this address belongs to

    params:
      addr: address of a method in int

    returns:
      lib so base address in int
    """

    buf = get_info_proc_mapping()
    start = None
    start_min = None
    end = None
    end_max = None
    path = None
    so_path = None
    asm_path = None
    for (hexstart, hexend, path) in re.findall(proc_mapping_re, buf):
        try:
            start = int(hexstart, 16)
        except:
            start = None
        try:
            end = int(hexend, 16)
        except:
            end = None
        if not start or not end or not path:
            continue

        if addr >= start and addr <= end:
            if not start_min:
                start_min = start
            else:
                start_min = start if start < start_min else start

            if not end_max:
                end_max = end
            else:
                end_max = end if end > end_max else end

            if m_debug.Debug: m_debug.dbg_print("start = ", hex(start), " start_min = ", hex(start_min), " end = ", hex(end), "end_max = ", hex(end_max), " addr = ", hex(addr))

            return start

    if m_debug.Debug: m_debug.dbg_print("return lib addr None")
    return None

def get_lib_so_info(addr):
    """
    for a given Maple method address, look up the lib so file, and get informations
    about the so library and its co-responding asm file info.

    params:
      addr: address of a method in hex string with prefix '0x', i.e 0x7fff6021c308

    returns:
      1, so lib start address: int
      2, lib so file full path in string
      3, lib asm file full path in string
      4, lib mpl.mir.mpl file full path in string
    """
    a = int(addr, 0)

    buf = get_info_proc_mapping()
    start = None
    end = None
    path = None
    so_path = None
    asm_path = None
    mirmpl_path = None
    for (hexstart, hexend, path) in re.findall(proc_mapping_re, buf):
        try:
            start = int(hexstart, 16)
        except:
            start = None
        try:
            end = int(hexend, 16)
        except:
            end = None
        if not start or not end or not path:
            continue

        if a >= start and a <= end:
            if not path.rstrip().lower().endswith('.so'):
                if m_debug.Debug: m_debug.dbg_print("path does not end with .so, path = ", path)
                return None, None, None, None
            so_path = path.rstrip().replace('/./','/')
            if m_debug.Debug: m_debug.dbg_print("gdb.solib_name(addr)=", gdb.solib_name(a))
            mirmpl_path = so_path[:-3] + '.mpl.mir.mpl'
            mirmpl_path = os.path.realpath(mirmpl_path)
            asm_path = so_path[:-3] + '.VtableImpl.s'
            asm_path = os.path.realpath(asm_path)
            if os.path.exists(so_path) and os.path.exists(asm_path) and os.path.exists(mirmpl_path):
                # both .so file and co-responding .s file exist in same directory
                if m_debug.Debug: m_debug.dbg_print("return lib info: start=", start, "so_path=",so_path,\
                                                    "asm_path=", asm_path, "mirmpl_path=", mirmpl_path)
                return start, so_path, asm_path, mirmpl_path

            asm_path = so_path[:-3] + '.s'
            asm_path = os.path.realpath(asm_path)
            mirmpl_path = so_path[:-3] + '.mpl.mir.mpl'
            mirmpl_path = os.path.realpath(mirmpl_path)
            if os.path.exists(so_path) and os.path.exists(asm_path) and os.path.exists(mirmpl_path):
                # both .so file and co-responding .s file exist in same directory
                if m_debug.Debug: m_debug.dbg_print("return lib info: start=", start, "so_path=",so_path, \
                                                    "asm_path=", asm_path, "mirmpl_path=", mirmpl_path)
                return start, so_path, asm_path, mirmpl_path

            so_path_short_name = so_path.split('/')[-1][:-3]

            base = so_path.split('/')
            asm_path = ''
            for i in range(len(base) - 4):
                asm_path += '/' + base[i]
            mirmpl_path = asm_path + '/maple_build/out/x86_64/'+ so_path_short_name + '.mpl.mir.mpl'
            mirmpl_path = os.path.realpath(mirmpl_path)
            asm_path += '/maple_build/out/x86_64/'+ so_path_short_name + '.VtableImpl.s'
            asm_path = os.path.realpath(asm_path)
            if os.path.exists(so_path) and os.path.exists(asm_path) and os.path.exists(mirmpl_path):
                # special case where .so and .VtableImpl.s are in such a different folders
                if m_debug.Debug:
                    m_debug.dbg_print("return lib info: start=", start, "so_path=",so_path, \
                                      "asm_path=", asm_path, "mirmpl_path=", mirmpl_path)
                return start, so_path, asm_path, mirmpl_path

            if not 'maple_lib_asm_path' in m_set.msettings:
                return None, None, None, None
            for v in m_set.msettings['maple_lib_asm_path']:
                mirmpl_path = v + '/'+ so_path_short_name + '.mpl.mir.mpl'
                mirmpl_path = os.path.realpath(mirmpl_path)
                asm_path = v + '/' + so_path_short_name + '.VtableImpl.s'
                asm_path = os.path.realpath(asm_path)
                #asm_path = path.split('maple/out')[0] + 'maple/out/common/share/' + so_path_short_name + '.VtableImpl.s'
                if os.path.exists(so_path) and os.path.exists(asm_path) and os.path.exists(mirmpl_path):
                    # .s file is in the search path list
                    if m_debug.Debug:
                        m_debug.dbg_print("return lib info: start=", start, "so_path=",so_path, \
                                          "asm_path=", asm_path, "mirmpl_path", mirmpl_path)
                    return start, so_path, asm_path, mirmpl_path

            return None, None, None, None
    return None, None, None, None

def get_dync_lib_so_info(addr):
    """
    for a given Maple method address, look up the lib so file, and get informations
    about the so library and its co-responding asm file info.

    params:
      addr: address of a method in hex string with prefix '0x', i.e 0x7fff6021c308

    returns:
      1, so lib start address: int
      2, lib so file full path in string
      3, lib asm file full path in string
      4, lib mpl.mir.mpl file full path in string
    """
    a = int(addr, 0)

    buf = get_info_proc_mapping()
    start = None
    end = None
    path = None
    so_path = None
    asm_path = None
    mirmpl_path = None
    for (hexstart, hexend, path) in re.findall(proc_mapping_re, buf):
        try:
            start = int(hexstart, 16)
        except:
            start = None
        try:
            end = int(hexend, 16)
        except:
            end = None
        if not start or not end or not path:
            continue

        if a >= start and a <= end:
            if not path.rstrip().lower().endswith('.so'):
                if m_debug.Debug: m_debug.dbg_print("path does not end with .so, path = ", path)
                return None, None, None, None
            so_path = path.rstrip().replace('/./','/')
            if m_debug.Debug: m_debug.dbg_print("gdb.solib_name(addr)=", gdb.solib_name(a))
            mmpl_path = so_path[:-3] + '.mmpl'
            mmpl_path = os.path.realpath(mmpl_path)
            asm_path = so_path[:-3] + '.s'
            asm_path = os.path.realpath(asm_path)
            if os.path.exists(so_path) and os.path.exists(asm_path) and os.path.exists(mmpl_path):
                # both .so file and co-responding .s file exist in same directory
                if m_debug.Debug: m_debug.dbg_print("return lib info: start=", start, "so_path=",so_path,\
                                                    "asm_path=", asm_path, "mmpl_path=", mmpl_path)
                return start, so_path, asm_path, mmpl_path

            so_path_short_name = so_path.split('/')[-1][:-3]

            if not 'maple_lib_asm_path' in m_set.msettings:
                return None, None, None, None
            for v in m_set.msettings['maple_lib_asm_path']:
                mmpl_path = v + '/'+ so_path_short_name + '.mmpl'
                mmpl_path = os.path.realpath(mirmpl_path)
                asm_path = v + '/' + so_path_short_name + '.s'
                asm_path = os.path.realpath(asm_path)
                #asm_path = path.split('maple/out')[0] + 'maple/out/common/share/' + so_path_short_name + '.VtableImpl.s'
                if os.path.exists(so_path) and os.path.exists(asm_path) and os.path.exists(mmpl_path):
                    # .s file is in the search path list
                    if m_debug.Debug:
                        m_debug.dbg_print("return lib info: start=", start, "so_path=",so_path, \
                                          "asm_path=", asm_path, "mmpl_path", mmpl_path)
                    return start, so_path, asm_path, mmpl_path

            return None, None, None, None
    return None, None, None, None

exclude_lib_path = ['/lib/', '/lib64/', '/usr/']
def lib_file_filter(path):
    if path[0] != '/':
        return False
    if not path.rstrip().lower().endswith('.so'):
        return False
    for item in exclude_lib_path:
        if path.startswith(item):
            return False
    return True

def get_loaded_lib_asm_path():
    asm_path_list = []
    objfiles = gdb.objfiles()
    for objfile in objfiles:
        if not lib_file_filter(objfile.filename):
            continue

        so_path = os.path.realpath(objfile.filename)
        if m_debug.Debug: m_debug.dbg_print("realpath so_path=",so_path)
        asm_path = so_path[:-3] + '.VtableImpl.s'
        asm_path = os.path.realpath(asm_path)
        if os.path.exists(so_path) and os.path.exists(asm_path):
            # both .so file and co-responding .s file exist in same directory
            if m_debug.Debug: m_debug.dbg_print("return lib info: so_path=",so_path, "asm_path=", asm_path)
            asm_path_list.append(asm_path)
            continue

        asm_path = so_path[:-3] + '.s'
        asm_path = os.path.realpath(asm_path)
        if os.path.exists(so_path) and os.path.exists(asm_path):
            # both .so file and co-responding .s file exist in same directory
            if m_debug.Debug: m_debug.dbg_print("return lib info: so_path=",so_path, "asm_path=", asm_path)
            asm_path_list.append(asm_path)
            continue

        so_path_short_name = so_path.split('/')[-1][:-3]

        base = so_path.split('/')
        asm_path = ''
        for i in range(len(base) - 4):
            asm_path += '/' + base[i]
        asm_path += '/maple_build/out/x86_64/'+ so_path_short_name + '.VtableImpl.s'
        asm_path = os.path.realpath(asm_path)
        if os.path.exists(so_path) and os.path.exists(asm_path):
            # special case where .so and .VtableImpl.s are in such a different folders
            if m_debug.Debug:
                m_debug.dbg_print("return lib info: so_path=",so_path, "asm_path=", asm_path)
            asm_path_list.append(asm_path)
            continue

        if not 'maple_lib_asm_path' in m_set.msettings:
            continue
        for v in m_set.msettings['maple_lib_asm_path']:
            asm_path = v + '/' + so_path_short_name + '.VtableImpl.s'
            asm_path = os.path.realpath(asm_path)
            #asm_path = path.split('maple/out')[0] + 'maple/out/common/share/' + so_path_short_name + '.VtableImpl.s'
            if os.path.exists(so_path) and os.path.exists(asm_path):
                # .s file is in the search path list
                if m_debug.Debug:
                    m_debug.dbg_print("return lib info: so_path=",so_path, "asm_path=", asm_path)
                asm_path_list.append(asm_path)
                continue

    if m_debug.Debug:
       m_debug.dbg_print("returning asm_path_list length", len(asm_path_list))
       m_debug.dbg_print("returning asm_path_list ", asm_path_list)

    return asm_path_list

def get_uninitialized_maple_func_addrs():
    """
    get func addr information from an uninitialized Maple frame.
    NOTE: call this when Maple func is NOT initialized yet.

    returns:
      1, func header address in string format of xxxxxx:yyyyyy: format where
         xxxxxx means (header - lib_base_addr),
         yyyyyy means (pc - header - header_size)
      2, so_path: in string. so library full path.
      3, asm_path: in string. asm (.s) file full path
      4, func_header: func header address in string.
      5, mirmpl_path: in string. .mpl.mir.mpl file full path
    """

    func_header = get_maple_frame_addr()
    if not func_header:
        return None, None, None, None
    if m_debug.Debug: m_debug.dbg_print("func_header=", func_header)

    func_lib_addr, so_path, asm_path, mirmpl_path = get_lib_so_info(func_header)
    if not func_lib_addr or not so_path or not asm_path or not mirmpl_path:
        return None, None, None, None, None
    if m_debug.Debug: m_debug.dbg_print("func_lib_addr=", func_lib_addr)

    header = int(func_header, 16)
    if (header < func_lib_addr):
        #gdb_print ("Warning: The header address is lower than lib_addr.")
        if m_debug.Debug: m_debug.dbg_print("Warning: The header address is lower than lib_addr.")
        return None, None, None, None, None

    offset = header - func_lib_addr

    # when a func in a frame is not initialized yet, pc value is 0000
    pc = '0000'

    return hex(offset) + ":" + pc + ":", so_path, asm_path, func_header, mirmpl_path

def get_uninitialized_dync_maple_func_addrs():
    """
    get func addr information from an uninitialized Maple frame.
    NOTE: call this when Maple func is NOT initialized yet.

    returns:
      1, func header address in string format of xxxxxx:yyyyyy: format where
         xxxxxx means (header - lib_base_addr),
         yyyyyy means (pc - header - header_size)
      2, so_path: in string. so library full path.
      3, asm_path: in string. asm (.s) file full path
      4, func_header: func header address in string.
      5, mmpl_path: in string. .mpl.mir.mpl file full path
    """

    #func_header = get_maple_frame_addr()
    func_header = m_symbol.get_dynamic_symbol_addr_by_current_frame()
    if not func_header:
        return None, None, None, None, None
    if m_debug.Debug: m_debug.dbg_print("func_header=", func_header)

    func_lib_addr, so_path, asm_path, mmpl_path = get_dync_lib_so_info(func_header)
    if not func_lib_addr or not so_path or not asm_path or not mmpl_path:
        return None, None, None, None, None
    if m_debug.Debug: m_debug.dbg_print("func_lib_addr=", func_lib_addr)

    header = int(func_header, 16)
    if (header < func_lib_addr):
        #gdb_print ("Warning: The header address is lower than lib_addr.")
        if m_debug.Debug: m_debug.dbg_print("Warning: The header address is lower than lib_addr.")
        return None, None, None, None, None

    offset = header - func_lib_addr

    # when a func in a frame is not initialized yet, pc value is 0000
    pc = '0000'

    return hex(offset) + ":" + pc + ":", so_path, asm_path, func_header, mmpl_path

#######################################################################
####  API section for retrieving Maple frame caller information
######################################################################
"""
use these api calls to get the current frame's caller's information. Due to 
the fact that at the time we call it the frame is in the stack, it's 
caller's information must also be available.
"""

def get_maple_caller_sp():
    try:
        buffer = m_util.gdb_exec_to_str('p caller.sp')
    except:
        return None

    if 'No symbol "caller" in current context.' in buffer:
        return None
    if ' = ' in buffer:
        return int(buffer.split(' = ')[-1])
    else:
        return None

def get_maple_caller_full():
    try:
        buffer = m_util.gdb_exec_to_str('p *caller')
    except:
        return None

    if 'No symbol "caller" in current context.' in buffer:
        return None
    else:
        return buffer

def get_maple_caller_argument_value(arg_idx, arg_num, mtype):
    """
    for a given Maple caller's arg_idx, arg_num and type,
    get its value from caller's operand_stack

    params:
      arg_idx: the argument index in a Maple method.
               e.g. method1(int a, long b, string c), for
               argument "a", its arg index is 0, c is 2.
      arg_num: number of argument a method has
      mtype  : a string. definition in m_asm.py
               for "a", it could be 'i32', 'i64'
    returns:
       the argument value in string.
    """

    if m_debug.Debug: m_debug.dbg_print("pass in arg_idx=", arg_idx, "arg_num=", arg_num, "mtype=", mtype)

    # get the caller.sp first
    try:
        buffer = m_util.gdb_exec_to_str('p caller.sp')
    except:
        return None

    if 'No symbol "caller" in current context.' in buffer:
        return None
    if ' = ' in buffer:
        sp = int(buffer.split(' = ')[-1])
    else:
        return None

    if  arg_idx > sp: #sp must be >= num of args
        return None
    if m_debug.Debug: m_debug.dbg_print("sp=", sp)

    # get the caller.operand_stack length
    try:
        buffer = m_util.gdb_exec_to_str('p caller.operand_stack.size()')
    except:
        return None

    if buffer[0] != '$':
        return None
    if ' = ' in buffer:
        try:
            length = int(buffer.split(' = ')[1])
        except:
            return None
    else:
        return None
    if m_debug.Debug: m_debug.dbg_print("length=", length)

    if length < arg_idx:
        return None

    # get the caller.operand_stack[arg_idx]
    idx = sp - arg_num + arg_idx + 1
    try:
        buffer = m_util.gdb_exec_to_str('p caller.operand_stack[' + str(idx) + ']')
    except:
        return None

    vline = buffer.split('x = ')[1][1:].split('}')[0]
    v = vline.split(mtype+' = ')[1].split(',')[0] if mtype in vline else None
    return v

def get_maple_a64_pointer_type(arg_value):
    """
    params:
       arg_value: a string. a value returned from caller's operand_stack value
                  by get_maple_caller_argument_value().
                  a example will be like '0x11058 "\320w\241\366\377\177"'

    returns:
      demangled a64 pointer type in string
    """

    addr = arg_value.split()[0]
    try:
        buf = m_util.gdb_exec_to_str('x/2xw ' + addr)
    except:
        return None
    addr = buf.split(':')[1].split()
    addr0 = addr[0]
    addr1 = addr[1]
    addr = addr1 + addr0[2:]

    try:
        buf = m_util.gdb_exec_to_str('x ' + addr)
    except:
        return None
    if not addr0[2:] in buf:
        return None
    s = buf.split()[1][9:-2]

    s = s.replace('_7C', '')
    s = s.replace('_3B', '')
    s = s.replace('_2F', '.')
    s = s.replace('_28', '')
    s = s.replace('_29', '')
    s = s.replace('_3C', '')
    s = s.replace('_3E', '')
    s = s.replace('_24', '$')

    return s


#######################################################################
####  API section for retrieving Maple frame operand_stack information
######################################################################
def get_maple_frame_stack_sp_index(func_header_name):
    """
    for a given func header name, find out its sp value reported in operand_stack
    """

    # get the func.operand_stack length
    try:
        buffer = m_util.gdb_exec_to_str('p func')
    except:
        return None

    if 'No symbol "func" in current context.' in buffer:
        return None

    stack_header_name = buffer.split('header = ')[1].split()[1][1:-2]
    if stack_header_name != func_header_name:
        if m_debug.Debug: m_debug.dbg_print("func_header_name=", func_header_name, "stack_header_name=", stack_header_name)
        return None

    sp_str = buffer.split('sp = ')[1].split()[0][:-1]
    if not sp_str:
        return None

    try:
        sp = int(sp_str)
    except:
        return None

    return sp

def get_one_frame_stack_local(index, sp, name, ltype):
    """
    for given indexi, sp vale and local or format name in operand_stack,
    return its value.

    params:
      index: int. index of locals or formals in func.operand_stack
      sp : int.  sp value in func.operand_stack
      name: string. local variable name
      ltype: string. Original type found for the local variable name.

    returns:
      type: echo original type if local vaiable name is not %%thrownval,
            or different type if it is %%thrownval
      value: this index's value in string.
    """

    # get the func.operand_stack length
    try:
        buffer = m_util.gdb_exec_to_str('p func')
    except:
        return None,None

    if 'No symbol "func" in current context.' in buffer:
        return None,None

    if index >= sp:
        return None,None

    # get the caller.operand_stack[index]
    try:
        buffer = m_util.gdb_exec_to_str('p func.operand_stack[' + str(index) + ']')
    except:
        return None,None

    '''
    $11 = {x = {u1 = 0 '\000', i8 = 0 '\000', i16 = 0, u16 = 0, i32 = 0, i64 = 0, f32 = 0, f64 = 0, a64 = 0x0, u8 = 0 '\000', u32 = 0,
    u64 = 0}, ptyp = maple::PTY_void}
    '''
    vline = buffer.split('x = ')[1][1:].split('}')[0]
    v = vline.split(ltype+' = ')[1].split(',')[0] if ltype in vline else None

    if name == "%%thrownval":
        try:
            maple_type = m_util.gdb_exec_to_str('p func.operand_stack[' + str(index) + '].ptyp')
        except:
            return None,None
        maple_type = maple_type.split(' = ')[1].split('maple::PTY_')[1].rstrip()
        if maple_type == 'a64': # run time becomes type 0xe which is a64
            v = vline.split(maple_type+' = ')[1].split(',')[0] if maple_type in vline else None
        else:
            maple_type = "none"
            v = "none"
        return maple_type, v

    return ltype, v

def get_one_frame_stack_dynamic(sp, idx):
    """
    for a given sp and index number in a dynamic caller operand_stack data, return its
    data type and value.

    Note, at runtime, caller.operand_stack is dynamic, sp, idx, types and values are all
    changing during the run time.
    """
    if idx > sp:
        return None, None

    # get the caller.operand_stack length
    length = None
    try:
        buffer = m_util.gdb_exec_to_str('p func.operand_stack.size()')
    except:
        return None, None

    if buffer[0] != '$':
        return None, None
    if ' = ' in buffer:
        try:
            length = int(buffer.split(' = ')[1])
        except:
            return None, None
    else:
        return None, None
    if m_debug.Debug: m_debug.dbg_print("length=", length)


    if length <= sp or length < idx:
        return None, None

    try:
        maple_type = m_util.gdb_exec_to_str('p func.operand_stack[' + str(idx) + '].ptyp')
    except:
        return None, None
    maple_type = maple_type.split(' = ')[1].split('maple::PTY_')[1].rstrip()

    try:
        buffer = m_util.gdb_exec_to_str('p func.operand_stack[' + str(idx) + ']')
    except:
        return None, None

    value = buffer.split('x = ')[1][1:].split('}')[0]
    if maple_type in value:
        v = value.split(maple_type + ' = ')[1].split(',')[0]
    else:
        return None, None

    if m_debug.Debug: m_debug.dbg_print("return maple_type=", maple_type, "v=", v)
    return maple_type, v

################################################################################
### API to get dynamic language data, e.g JS support.
### APIs include how to get all JS properties data, name, value, object index etc
################################################################################
# jstype from jsvalue.h
jstype_dict = {
  "JSTYPE_NONE",
  "JSTYPE_NULL",
  "JSTYPE_BOOLEAN",
  "JSTYPE_STRING",
  "JSTYPE_NUMBER",
  "JSTYPE_OBJECT",
  "JSTYPE_ENV",
  "JSTYPE_UNKNOWN",
  "JSTYPE_UNDEFINED",
  "JSTYPE_DOUBLE",
  "JSTYPE_NAN",
  "JSTYPE_INFINITY"
}

def get_current_thisbinding():
    cmd = "p __js_ThisBinding"
    buf = gdb.execute(cmd, to_string=True)
    # example of output: $38 = {asbits = 21474836481, s = {payload = {i32 = 1, u32 = 1, boo = 1, str = 1, obj = 1, ptr = 1}, tag = JSTYPE_OBJECT}}
    buf = buf.rstrip()

    if buf[0] != '$' or 'tag = JSTYPE' not in buf:
        return None, None
    tag = buf.split("tag = ")[-1][:-2]
    if tag == "JSTYPE_OBJECT":
        val = buf.split("obj = ")[1].split(',')[0]
    elif tag == "JSTYPE_NUMBER":
        val = buf.split("i32 = ")[1].split(',')[0]
    elif tag == "JSTYPE_BOOLEAN":
        val = buf.split("boo = ")[1].split(',')[0]
    elif tag == "JSTYPE_STRING":
        val = buf.split("str = ")[1].split(',')[0]
    else:
        val = None

    if m_debug.Debug: m_debug.dbg_print("tag = ", tag, "val = ", val)
    return tag, val

def get_realAddr_by_index(index):
    cmd = "p memory_manager->GetRealAddr(" + str(index) + ")"
    buf = gdb.execute(cmd, to_string=True)
    # example of a command to issue "p memory_manager->GetRealAddr(1)"
    buf = buf.rstrip()
    # example of returning output: "$39 = (void *) 0x7ffff63d4014"

    if buf[0] != '$' or ' = ' not in buf:
        return None
    addr = buf.split()[4]
    return addr

def get_realAddr_data(addr, jstype):
    if jstype == "JSTYPE_OBJECT":
        return get_obj_prop_list_head(addr)
    else:
        return None

def get_obj_prop_list_head(addr):
    cmd = "p *(struct __jsobject *)" + str(addr)
    # example of output could be:
    # 1. when prop_list is not available meaning nothing should be interpreted.
    #   "$40 = {prop_list = 0x0, prototype = {obj = 0x2, id = JSBUILTIN_OBJECTPROTOTYPE}, extensible = 1 '\001', object_class = JSGLOBAL, object_type = 0 '\000',
    #   is_builtin = 1 '\001', proto_is_builtin = 1 '\001', builtin_id = JSBUILTIN_GLOBALOBJECT, shared = {fast_props = 0x0, array_props = 0x0, fun = 0x0,
    #   prim_string = 0x0, prim_regexp = 0x0, prim_number = 0, prim_bool = 0, arr = 0x0, primDouble = 0}}
    #
    # 2. when prop_list is available, we should get the list and traverse the prop list
    #    $85 = {prop_list = 0x7ffff63d4178, prototype = {obj = 0x2, id = JSBUILTIN_OBJECTPROTOTYPE}, extensible = 1 '\001', object_class = JSGLOBAL, object_type = 0 '\000'    #    , is_builtin = 1 '\001', proto_is_builtin = 1 '\001', builtin_id = JSBUILTIN_GLOBALOBJECT, shared = {fast_props = 0x0, array_props = 0x0, fun = 0x0,
    #    prim_string = 0x0, prim_regexp = 0x0, prim_number = 0, prim_bool = 0, arr = 0x0, primDouble = 0}}

    buf = gdb.execute(cmd, to_string = True)
    buf = buf.rstrip()
    if m_debug.Debug: m_debug.dbg_print("get_obj_prop_list_head buf = ", buf)

    if buf[0] != '$' or 'prop_list = ' not in buf:
        return None
    prop_list = buf.split("prop_list = ")[1].split(',')[0]
    try:
        prop_list_addr = int(prop_list, 16)
    except:
        return None

    if m_debug.Debug: m_debug.dbg_print("prop_list_addr=", prop_list_addr)
    if prop_list_addr == 0:
        return None
    else:
        return prop_list

def get_prop_name_value(addr):
    # input example: 0x55555576d1a8
    cmd = "p *(__jsstring *)" + str(addr)
    buf = gdb.execute(cmd, to_string = True)
    buf = buf.rstrip()
    if buf[0] != '$' or 'length = ' not in buf or 'ascii = ' not in buf:
        return None,None
    # example of command output:
    # $87 = {kind = (unknown: 0), builtin = JSBUILTIN_STRING_ARGUMENTS, length = 2, x = {ascii = 0x55555576d1ac "v1", utf16 = 0x55555576d1ac u"ㅶ"}}
    if m_debug.Debug: m_debug.dbg_print("get_prop_name_value buf=", buf)

    length_str = buf.split('length = ')[1].split(',')[0]
    if m_debug.Debug: m_debug.dbg_print("get_prop_name_value length_str=", length_str)
    try:
        length = int(length_str, 10)
    except:
        return None,None
    if length == 0:
        return None,None

    ascii_name = buf.split('x = {ascii = ')[1].split(',')[0].split()[1][1:-1]
    utf16_name = buf.split('utf16 = ')[1].split()[1][2:-3]
    return ascii_name, utf16_name

def get_string_value_from_index(index):
    addr = get_realAddr_by_index(str(index))
    if not addr:
        return None, None
    return get_prop_name_value(addr)

def get_prop_list_node_data(node_addr):
    # input: node_addr is the address of node, a pointer value to 'struct __jsprop'
    # output: return node data in dict format.
    # dict:
    # {'tag': 'JSTYPE_NUMBER'
    #  'ascii_name': 'var'
    #  'utf16_name : 'ㅶ'
    #  'next': 0x12323434
    #  'name_addr': 0x55555576d1a8
    #  'value':  6 (if tag JSTYPE_NUMBER)
    #         :  hello world (if tag = JSTYPE_STRING
    #         :  index (if tag = JSTYPE_OBJECT)
    #         :  None: if tag = JSTYPE_UNDEFINED or JSTYPE_NONE or JSTYPE_NULL
    #
    cmd = "p *(struct __jsprop *)" + node_addr
    if m_debug.Debug: m_debug.dbg_print("cmd=", cmd)
    buf = gdb.execute(cmd, to_string = True)
    buf = buf.rstrip()
    if m_debug.Debug: m_debug.dbg_print("get_prop_list_data buf=", buf)

    # example of output:
    # "$86 = {n = {index = 1433850280, name = 0x55555576d1a8}, next = 0x0, desc = {{named_data_property = {value = {asbits = 34359738368, s = {payload = {i32 = 0,u32 = 0, boo = 0, str = 0, obj = 0, ptr = 0}, tag = JSTYPE_UNDEFINED}}}, named_accessor_property = {get = 0x800000000, set = 0x0}}, {s = {attr_writable = 3 '\003', attr_enumerable = 3 '\003', attr_configurable = 2 '\002', fields = 0 '\000'}, attrs = 131843}}, isIndex = false}"
    if buf[0] != '$' or 'next = ' not in buf or 'desc = ' not in buf:
        return data
    prop_next = buf.split("next = ")[1].split(',')[0]
    prop_name_addr = buf.split("name = ")[1].split(',')[0][:-1]
    tag = buf.split("tag = ")[1].split('}}}')[0]

    if m_debug.Debug: m_debug.dbg_print("prop_next=", prop_next, "prop_name_addr=", prop_name_addr, "tag=", tag)
    ascii_name, utf16_name = get_prop_name_value(prop_name_addr)
    if m_debug.Debug: m_debug.dbg_print("ascii_name=", ascii_name, "utf16_name=", utf16_name)
    element = {}
    element['tag'] = tag
    element['ascii_name'] = ascii_name
    element['utf16_name'] = utf16_name
    element['next'] = prop_next
    element['name_addr'] = prop_name_addr


    if tag == 'JSTYPE_UNDEFINED' or tag == 'JSTYPE_NONE' or tag == 'JSTYPE_NULL':
        element['value'] = ""
    elif tag == 'JSTYPE_NUMBER':
        value = buf.split('payload = {i32 = ')[1].split(',')[0]
        element['value'] = value
    elif tag == 'JSTYPE_BOOLEAN':
        value = buf.split('boo = ')[1].split(',')[0]
        element['value'] = value
    elif tag == 'JSTYPE_STRING':
        index = buf.split('str = ')[1].split(',')[0]
        ascii_v, utf16_v = get_string_value_from_index(index)
        element['value'] = ascii_v
    else:
        element['value'] = ""

    if m_debug.Debug: m_debug.dbg_print("element=", element)
    return element

def get_prop_list_data(prop_list_head):
    # input: prop_list_head is the address of the head node address in struct __jsprop
    #
    # output: a list contains each property's data in dict format
    # dict:
    # {'tag': 'JSTYPE_NUMBER'
    #  'ascii_name': 'var'
    #  'utf16_name : 'ㅶ'
    #  'next': 0x12323434
    #  'name_addr': 0x55555576d1a8
    #  'value':  6 (if tag JSTYPE_NUMBER)
    #         :  hello world (if tag = JSTYPE_STRING
    #         :  index (if tag = JSTYPE_OBJECT)
    #         :  None: if tag = JSTYPE_UNDEFINED or JSTYPE_NONE or JSTYPE_NULL
    #
    data = []

    node_addr = prop_list_head
    while True:
        element = get_prop_list_node_data(node_addr)
        if len(element) == 0:
            return data
        data.append(element)
        try:
            next_node_addr_num = int(element['next'], 16)
        except:
            break

        if next_node_addr_num == 0:
            # if no more nodes
            break
        node_addr = element['next']

    return data

def get_all_dync_properties_data():
    #output: a list contains elements, each element is a dict representing one node of property list
    tag,val = get_current_thisbinding()
    if m_debug.Debug: m_debug.dbg_print("tag=", tag, "val=", val)
    if tag and val:
        addr = get_realAddr_by_index(val)
        if m_debug.Debug: m_debug.dbg_print("real addr=", addr)

        #only when prop_list returns non-0 address, could the prop_list starts
        # to have meaningful data filled in.
        # also nite, prop_list_head is in struct of struct __jsprop. reference this
        # structure, we can get the list node data immediately
        prop_list_head = get_realAddr_data(addr, tag)
        if m_debug.Debug: m_debug.dbg_print("prop_list_head=", prop_list_head)

        if not prop_list_head:
            gdb_print("prop_list no data filled in yet")
            return None
        else:
            # get all the node data from property list
            data =  get_prop_list_data(prop_list_head)
            if m_debug.Debug: m_debug.dbg_print("data=", data)
            return data

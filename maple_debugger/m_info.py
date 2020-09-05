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
import os
import re
import m_set
import m_util
from m_util import gdb_print
import m_debug

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
        cached_proc_mapping = m_util.gdb_exec_to_string('info proc mapping')
    return cached_proc_mapping

def get_maple_frame_addr():
    """
    get current Maple frame addr via 'info args' gdb command.

    returns:
        address in string. e.g 0x7fff6021c308
        or None if not found.
    """
    buf = m_util.gdb_exec_to_string('info args')
    if buf.find('_mirbin_info') != -1 and buf.find('mir_header') != -1:
        x = buf.split()
        return x[2]
    else:
        return None

def get_so_base_addr(name):
    """
    For a given name of library, return its base address via 'info proc mapping' cmd

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
    For a given attribute name, return its value via 'print' command in string format.
    """

    if name and name in ['func.header', 'func.lib_addr', 'func.pc']:
        cmd = 'p/x *(long long*)&' + name
    else:
        return None

    try:
        buf = m_util.gdb_exec_to_string(cmd)
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
        buf = m_util.gdb_exec_to_string('x/1xw ' + str(header))
    except:
        return None

    if not buf:
        return None
    header_size = int(buf.split(':')[1],16)

    if (header < lib_addr):
        gdb_print ("header address is found less than lib_addr, something is wrong")
        return None
    if (pc < header):
        gdb_print ("pc address is found less than header, something is wrong")
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
    For a given Maple method address, find out the base address of the .so lib
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
    For a given Maple method address, look up the lib so file, and get informations
    about the so library and its co-responding asm file info.

    params:
      addr: address of a method in hex string with prefix '0x', i.e 0x7fff6021c308

    returns:
      1. so lib start address: int
      2. lib so file full path in string
      3. lib asm file full path in string
    """
    a = int(addr, 0)

    buf = get_info_proc_mapping()
    start = None
    end = None
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

        if a >= start and a <= end:
            if not path.rstrip().lower().endswith('.so'):
                if m_debug.Debug: m_debug.dbg_print("path does not end with .so, path = ", path)
                return None, None, None
            so_path = path.rstrip().replace('/./','/')
            if m_debug.Debug: m_debug.dbg_print("gdb.solib_name(addr)=", gdb.solib_name(a))
            asm_path = so_path[:-3] + '.VtableImpl.s'
            asm_path = os.path.realpath(asm_path)
            if os.path.exists(so_path) and os.path.exists(asm_path):
                # both .so file and co-responding .s file exist in same directory
                if m_debug.Debug: m_debug.dbg_print("return lib info: start=", start, "so_path=",so_path, "asm_path=", asm_path)
                return start, so_path, asm_path

            asm_path = so_path[:-3] + '.s'
            asm_path = os.path.realpath(asm_path)
            if os.path.exists(so_path) and os.path.exists(asm_path):
                # both .so file and co-responding .s file exist in same directory
                if m_debug.Debug: m_debug.dbg_print("return lib info: start=", start, "so_path=",so_path, "asm_path=", asm_path)
                return start, so_path, asm_path

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
                    m_debug.dbg_print("return lib info: start=", start, "so_path=",so_path, "asm_path=", asm_path)
                return start, so_path, asm_path

            if not 'maple_lib_asm_path' in m_set.msettings:
                return None,None, None
            for v in m_set.msettings['maple_lib_asm_path']:
                asm_path = v + '/' + so_path_short_name + '.VtableImpl.s'
                asm_path = os.path.realpath(asm_path)
                #asm_path = path.split('maple/out')[0] + 'maple/out/common/share/' + so_path_short_name + '.VtableImpl.s'
                if os.path.exists(so_path) and os.path.exists(asm_path):
                    # .s file is in the search path list
                    if m_debug.Debug:
                        m_debug.dbg_print("return lib info: start=", start, "so_path=",so_path, "asm_path=", asm_path)
                    return start, so_path, asm_path

            return None, None, None
    return None, None, None


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
    """

    func_header = get_maple_frame_addr()
    if not func_header:
        return None, None, None, None
    if m_debug.Debug: m_debug.dbg_print("func_header=", func_header)

    func_lib_addr, so_path, asm_path = get_lib_so_info(func_header)
    if not func_lib_addr or not so_path or not asm_path:
        return None, None, None, None
    if m_debug.Debug: m_debug.dbg_print("func_lib_addr=", func_lib_addr)

    header = int(func_header, 16)
    if (header < func_lib_addr):
        #gdb_print ("header address is found less than lib_addr, something is wrong")
        if m_debug.Debug: m_debug.dbg_print("header address is found less than lib_addr, something is wrong")
        return None,None,None, None

    offset = header - func_lib_addr

    # when a func in a frame is not initialized yet, pc value is 0000
    pc = '0000'

    return hex(offset) + ":" + pc + ":", so_path, asm_path, func_header

#######################################################################
####  API section for retrieving Maple frame caller information
######################################################################
"""
use these api to get the current frame's caller's information. Because
at the time we call it, the frame is in stack, so its caller's information
must be available.
"""

def get_maple_caller_sp():
    try:
        buffer = m_util.gdb_exec_to_string('p caller.sp')
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
        buffer = m_util.gdb_exec_to_string('p *caller')
    except:
        return None

    if 'No symbol "caller" in current context.' in buffer:
        return None
    else:
        return buffer

def get_maple_caller_argument_value(arg_idx, arg_num, mtype):
    """
    For given Maple caller's arg_idx, arg_num and type,
    get its value from caller's operand_stack

    params:
      arg_idx: the argument index in a Maple method.
               e.g method1(int a, long b, string c), for
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
        buffer = m_util.gdb_exec_to_string('p caller.sp')
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
        buffer = m_util.gdb_exec_to_string('p caller.operand_stack.size()')
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
        buffer = m_util.gdb_exec_to_string('p caller.operand_stack[' + str(idx) + ']')
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
        buf = m_util.gdb_exec_to_string('x/2xw ' + addr)
    except:
        return None
    addr = buf.split(':')[1].split()
    addr0 = addr[0]
    addr1 = addr[1]
    addr = addr1 + addr0[2:]

    try:
        buf = m_util.gdb_exec_to_string('x ' + addr)
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
    For a given func header name, find out its sp value reported in operand_stack
    """

    # get the func.operand_stack length
    try:
        buffer = m_util.gdb_exec_to_string('p func')
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
    For given indexi, sp vale and local or format name in operand_stack,
    return its value.

    params:
      index: int. index of locals or formals in caller.operand_stack
      sp : int.  sp value in caller.operand_stack
      name: string. local variable name
      ltype: string. Original type found for the local variable name.

    returns:
      type: echo original type if local vaiable name is not %%thrownval,
            or different type if it is %%thrownval
      value: this index's value in string.
    """

    # get the func.operand_stack length
    try:
        buffer = m_util.gdb_exec_to_string('p func')
    except:
        return None,None

    if 'No symbol "func" in current context.' in buffer:
        return None,None

    if index >= sp:
        return None,None

    # get the caller.operand_stack[index]
    try:
        buffer = m_util.gdb_exec_to_string('p func.operand_stack[' + str(index) + ']')
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
            maple_type = m_util.gdb_exec_to_string('p func.operand_stack[' + str(index) + '].ptyp')
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
    For a given sp and index number in a dynamic caller operand_stack data, return its
    data type and value.

    Note, at runtime, caller.operand_stack is dynamic, sp, idx, types and values are all
    changing during the run time.
    """

    if idx > sp:
        return None, None

    # get the caller.operand_stack length
    length = None
    try:
        buffer = m_util.gdb_exec_to_string('p caller.operand_stack.size()')
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
        maple_type = m_util.gdb_exec_to_string('p func.operand_stack[' + str(idx) + '].ptyp')
    except:
        return None, None
    maple_type = maple_type.split(' = ')[1].split('maple::PTY_')[1].rstrip()

    try:
        buffer = m_util.gdb_exec_to_string('p func.operand_stack[' + str(idx) + ']')
    except:
        return None, None

    value = buffer.split('x = ')[1][1:].split('}')[0]
    if maple_type in value:
        v = value.split(maple_type + ' = ')[1].split(',')[0]
    else:
        return None, None

    if m_debug.Debug: m_debug.dbg_print("return maple_type=", maple_type, "v=", v)
    return maple_type, v

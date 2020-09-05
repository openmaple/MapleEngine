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

import m_util
import m_debug

def get_symbol_address(symbol):
    """
    For a given Maple symbol, returns its address in hex string via gdb p command,
    or None if not found
    """

    cmd = 'p ' + symbol
    try:
        result  = m_util.gdb_exec_to_string(cmd)
    except:
        return None

    match_pattern = '<' + symbol + '>'

    if result.find(match_pattern) is -1:
        return None
    else:
        x = result.split()
        pattern_index = x.index(match_pattern)
        return x[pattern_index - 1]

def get_symbol_name_by_current_func_args():
    """
    get the 'func' symbol name and its address in currect frame stack
    """

    cmd = 'p func'
    try:
        result = m_util.gdb_exec_to_string(cmd)
    except:
        return None, None
    if m_debug.Debug: m_debug.dbg_print("result=", result)


    if 'header = ' in result and '_mirbin_info>' in result:
        s = result.split('header = ')
        address = s[1].split()[0]
        name = s[1].split()[1].split('>')[0][1:]
        caller = result.split('caller = ')[1].split(',')[0]
        if m_debug.Debug: m_debug.dbg_print("address=", address, "name=", name, "caller=", caller)
        return address, name
    else:
        return None, None

def get_symbol_addr_by_current_frame_args():
    """
    get the Maple symbol address in currect frame stack via gdb print command
    """
    cmd = 'p/x *(long long*)&mir_header'
    try:
        result = m_util.gdb_exec_to_string(cmd)
    except:
        return None

    if result[0] == '$' and ' = 0x' in result:
        return result.split(' = ')[1].rstrip()
    else:
        return None

def get_mirheader_name_by_mirheader_addr(addr):
    cmd = 'x ' + addr
    try:
        result = m_util.gdb_exec_to_string(cmd)
    except:
        return None
    result = result.rstrip()
    if m_debug.Debug: m_debug.dbg_print("result=", result, "addr=", addr)

    if addr in result:
        return result.split()[1][1:-2]
    else:
        return None

def get_symbol_name_by_current_frame_args():
    """
    get the Maple symbol name and its address in currect frame stack via 'info args' command
    """

    cmd = 'info args'
    try:
        result  = m_util.gdb_exec_to_string(cmd)
    except:
        return None, None
    if m_debug.Debug: m_debug.dbg_print("result=", result)

    offset = result.find('_mirbin_info>')
    if offset is -1: #no Maple metadata pattern
        return None, None
    x = result.split()
    for i in range(len(x)):
        if x[i].find('_mirbin_info>') != -1:
            return x[i-1], x[i][1:-1]
    return None, None


def check_symbol_in_current_frame(symbol):
    """
    check if Maple symbol is in current stack frame
    """

    cmd = 'info frame '
    try:
        result  = m_util.gdb_exec_to_string(cmd)
    except:
        return False

    offset = result.find(symbol)
    if offset is -1: # symbol not found in current frame
        return False
    else:
        return True

def get_variable_value(variable):
    """
    return a variable value. i.e
    (gdb) p __opcode_cnt
    $4 = 0
    """

    cmd = 'p ' + variable
    try:
        result  = m_util.gdb_exec_to_string(cmd)
    except:
        return None

    offset = result.find("=")
    if offset is -1: # not found
        return None
    x = result.split()
    return x[2]

def get_demangled_maple_symbol(symbol, to_filename=False):
    """
    for a given Maple symbol, convert it to demangled symbol name
    """

    if not '_7C' in symbol:
        return None
    s = symbol
    s = s.replace('_7C', '') # use the jni format
    s = s.replace('_3B', ';')
    s = s.replace('_2F', '.')
    s = s.replace('_28', '(')
    s = s.replace('_29', ')')
    s = s.replace('_3C', '<')
    s = s.replace('_3E', '>')
    s = s.replace('_24', '$')

    if to_filename == False:
        return s

    # to here, label becomes something like "Ljava.lang.Class;|getComponentType|()Ljava.lang.Class;"
    s = s.split('|')[0]
    s = s.replace(';', '')
    s = s[1:]
    f = s.replace('.', '/')
    f = f + '.java'
    if m_debug.Debug: m_debug.dbg_print("s=", s, "f=", f)

    return f

#######################################################################
### Maple Data Related
#######################################################################
java_primitive_type_dict = {
    '__pinf_Z': 'boolean',
    '__pinf_B': 'byte',
    '__pinf_C': 'char',
    '__pinf_S': 'short',
    '__pinf_I': 'int',
    '__pinf_J': 'long',
    '__pinf_F': 'float',
    '__pinf_D': 'double',
    'Z': 'boolean',
    'B': 'byte',
    'C': 'char',
    'S': 'short',
    'I': 'int',
    'J': 'long',
    'F': 'float',
    'D': 'double',
}

maple_symbol_type_dict = {
    '__cinf_Z': 'boolean',
    '__cinf_B': 'byte',
    '__cinf_C': 'char',
    '__cinf_S': 'short',
    '__cinf_I': 'int',
    '__cinf_J': 'long',
    '__cinf_F': 'float',
    '__cinf_D': 'double',
    '__cinf_L': 'class'
}

java_type_to_size_dict = {
    'Z': 1, # boolean
    'B': 1, # byte
    'C': 2, # char
    'S': 2, # short
    'I': 4, # int
    'J': 8, # long
    'F': 4, # float
    'D': 8, # double
    'L': 8  # class
}

def get_java_primitive_symbol_name(symbol):
    """
    for a symbol that presents a native java primitive type,
    return the symbol name.
    """

    if symbol[0:8] in java_primitive_type_dict:
        return symbol[8:]
    else:
        if '__pinf_A' in symbol:
            if symbol[-1] in java_primitive_type_dict:
                return symbol[-1]
        return None

def get_maple_symbol_name(symbol):
    """
    for a symbol that presents a Maple object type,
    return the symbol name.
    e.g __cinf_Ljava_2Flang_2FString_3B, return Ljava_2Flang_2FString_3B
    """

    offset = symbol.find('L')
    if offset == -1:
        ### check if it is a java primitive type
        return get_java_primitive_symbol_name(symbol)

    ### this is a Maple class object
    return symbol[offset:]


def get_maple_symbol_full_syntax(symbol):
    """
    For a given Maple symbol, return its full syntax, also known as its
    symbol description.

    params
      symbol: string.  symbol we can see in gdb stack data.
            e.g __cinf_Ljava_2Flang_2FString_3B,
                __cinf_ALjava_2Flang_2FString_3B
                __cinf_ZLsun_xxxxxxxxxxxxxxxxxxxx
                __cinf_JLjavax_yyyyyyyyyyyyyyy
                __cinf_FLsun_xxxxxxxxxxxxxxxxx

    return:
      type_size: int. object size
      full syntax in string.
      examples of full syntax:
        array int my_array[]
        boolean my_var
    """

    if m_debug.Debug: m_debug.dbg_print("symbol=", symbol)
    # check a special symbol for Java native array string
    if symbol is '__cinf_Ljava_2Flang_2FString_3B':
        return 'array string', 0

    # if it is class object, not primitive nor array
    if symbol[:8] in maple_symbol_type_dict:
        if symbol[7] == 'L': # if it is a Maple java object
            full_syntax = maple_symbol_type_dict[symbol[:8]]
        else:
            full_syntax = maple_symbol_type_dict[symbol[:8]] + ' ' + symbol[8:]
        if symbol[7] in java_type_to_size_dict:
            type_size = java_type_to_size_dict[symbol[7]]
        else:
            type_size = 1
        if m_debug.Debug: m_debug.dbg_print("full_syntax=", full_syntax, "type_size=", type_size)
        return full_syntax, type_size

    # to check if it is array of class, or array of primitive type.
    array_type_prefix = symbol[3:8]
    if array_type_prefix == 'inf_A':
        dimension = 0
        while array_type_prefix in symbol:
            dimension += 1
            array_type_prefix += 'A'
        array_type_prefix = 'inf_' + 'A' * dimension
        name = symbol.split(array_type_prefix)[1]
        return 'array ' + name + '[]'*dimension, java_type_to_size_dict[name[0]]
    else:
        return 'unknown ' + symbol, 0

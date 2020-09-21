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

""" Maple types defined in Maple Engine source code """
MAPLE_TYPES = [
    'Invalid',      #index 0x0
    'void',         #index 0x1
    'i8',           #index 0x2
    'i16',          #index 0x3
    'i32',          #index 0x4
    'i64',          #index 0x5
    'u8',           #index 0x6
    'u16',          #index 0x7
    'u32',          #index 0x8
    'u64',          #index 0x9
    'u1',           #index 0xa
    'ptr',          #index 0xb
    'ref',          #index 0xc
    'a32',          #index 0xd
    'a64',          #index 0xe
    'f32',          #index 0xf
    'f64',          #index 0x10
    'f128',         #index 0x11
    'c64',          #index 0x12
    'c128',         #index 0x13
    'simplestr',    #index 0x14
    'simpleobj',    #index 0x15
    'dynany',       #index 0x16
    'dynundef',     #index 0x17
    'dynnull',      #index 0x18
    'dynbool',      #index 0x19
    'dyni32',       #index 0x1a
    'dynstr',       #index 0x1b
    'dynobj',       #index 0x1c
    'dynf64',       #index 0x1d
    'dynf32',       #index 0x1e
    'dynnone',      #index 0x1f
    'constStr',     #index 0x20
    'gen',          #index 0x21
    'agg',          #index 0x22
    'v2i64',        #index 0x23
    'v4i32',        #index 0x24
    'v8i16',        #index 0x25
    'v16i8',        #index 0x26
    'v2f64',        #index 0x27
    'v4f32',        #index 0x28
    'unknown',      #index 0x29
    'Derived'       #index 0x2a
]

def create_asm_mirbin_label_cache(asm_path, proc_saved = {}):
    """ Create a dictionary for a given asm file
        When it is called the first time with a given asm file, it starts
        a subprocess to handle the asm file and returns None;
        When called subsequently using the same asm file, it will wait for the
        first subprocess to finish and return a dictionary created from its output.
    """
    if asm_path in proc_saved:
        d = m_util.proc_communicate(proc_saved.pop(asm_path))
        return eval(d)
    cmd="bash -c \"paste <(grep -bn _mirbin_info: '" + asm_path \
        + "') <(grep -Fb .cfi_endproc '" + asm_path \
        + "')\" | awk -F: -v a='\"' -v b='\":(' -v c=',' -v d='),' " \
        + "'BEGIN { print \"{\" } { print a$3b$1c$2c$4d } END { print \"}\" }'"
    proc_saved[asm_path] = m_util.proc_popen(cmd)
    return None

def look_up_src_file_info(asm_path, label_asm_ln, start_offset, end_offset, func_pc_offset):
    """
      for a given Maple symbol block a given func_pc_offset, get the corresponding
      source code information.

      params:
        asm_path: string. asm (.s) file full path.
        label_asm_ln: int. starting line num of Maple symbol block in asm file
        start_offset: int. starting offset of Maple symbol block in asm file
        end_offset:   int. ending offset of Maple symbol block in asm file
        func_pc_offset: string. func_pc_offset to searched in the symbol block

      return:
        None if source code infomration is not found, or
        a dict that contains information including source file name, source code line,
        corresponding symbol block line number and file offset in asm file.
        {
            'short_src_file_name': string. source code short file name.
            'short_src_file_line': int. source code line number. -1 means MPI_CLINIT_CHECK line
            'asm_line': int. line num where the source code info was found in asm file
            'asm_offset': int. offset of the asm file were source code info was found.
        }
    """
    if m_debug.Debug: m_debug.dbg_print("func_pc_offset=", func_pc_offset, " asm_path=", asm_path, \
                       "label_asm_ln=", label_asm_ln, "start_offset=", start_offset, \
                       "end_offset=", end_offset)
    try:
        func_pc_offset_int = int(func_pc_offset, 16)
        if m_debug.Debug: m_debug.dbg_print("func_pc_offset_int=", func_pc_offset_int)
    except:
        return None

    f = open(asm_path, "r")
    f.seek(start_offset)
    offset = start_offset
    line_num = label_asm_ln
    src_info_line = None
    short_src_file_name = None
    short_src_file_linenum = None
    last_opcode_line_pc = None
    this_line_pc = 0
    stop_on_next_byte_opcode = False
    while offset < end_offset:
        line = f.readline()
        if "// LINE " in line:
            src_info_line = line
            if m_debug.Debug: m_debug.dbg_print("src_info_line=", src_info_line, " line_num=", line_num )
        elif func_pc_offset in line:
            ''' this is the line that matches the func_pc_offset '''
            if m_debug.Debug: m_debug.dbg_print("found func_pc_offset = ", func_pc_offset, " line_num = ", line_num)
            if not src_info_line: #there is no co-responding source code file
                short_src_file_name = None
                if "MPL_CLINIT_CHECK" in line:
                    # special signature -1 to indicate MPI_CLINT_CHECK line
                    short_src_file_linenum = -1
                else:
                    short_src_file_linenum = 0
            else:
                x = src_info_line.split()
                short_src_file_name = x[2]
                short_src_file_linenum = int(x[4][:-1])

            f.close()
            d = {}
            d["short_src_file_name"] = short_src_file_name
            d["short_src_file_line"] = short_src_file_linenum
            d["asm_line"] = line_num
            d["asm_offset"] = offset
            if m_debug.Debug: m_debug.dbg_print("result in dict ", d)
            return d
        elif ".byte OP_" in line:
            if stop_on_next_byte_opcode == True:
                if m_debug.Debug: m_debug.dbg_print("found func_pc_offset = ", func_pc_offset, " line_num = ", line_num)
                if not src_info_line: #there is no co-responding source code file
                    short_src_file_name = None
                    if "MPL_CLINIT_CHECK" in line:
                        # special signature -1 to indicate MPI_CLINT_CHECK line
                        short_src_file_linenum = -1
                    else:
                        short_src_file_linenum = 0
                else:
                    x = src_info_line.split()
                    short_src_file_name = x[2]
                    short_src_file_linenum = int(x[4][:-1])

                f.close()
                d = {}
                d["short_src_file_name"] = short_src_file_name
                d["short_src_file_line"] = short_src_file_linenum
                d["asm_line"] = line_num
                d["asm_offset"] = offset
                if m_debug.Debug: m_debug.dbg_print("result in dict ", d)
                return d

            try:
                this_line_pc = int(line.split("// ")[1][:4],16)
            except:
                # in case of no pc offset shown in .byte Op line, the offset is same to previous byte OP line's offset
                if m_debug.Debug: m_debug.dbg_print("byte opcode line cannot retrieve pc value, line_num=", line_num, "line=", line)
                offset += len(line)
                line_num += 1
                continue
            last_opcode_line_pc = this_line_pc
            if m_debug.Debug: m_debug.dbg_print("last_opcode_line_pc=", last_opcode_line_pc)
        elif line.strip().startswith(".long") or line.strip().startswith(".quad"):
            if not last_opcode_line_pc:
                # Opcode not started yet.
                offset += len(line)
                line_num += 1
                continue

            this_line_pc = last_opcode_line_pc + 4
            if func_pc_offset_int == this_line_pc:
                stop_on_next_byte_opcode = True

        elif ".cfi_endproc" in line:
            break

        offset += len(line)
        line_num += 1
    f.close()
    return None

def look_up_next_src_file_info(asm_path, asm_line, asm_offset):
    """
      for a given asm file and asm line number and offset of the asm line number, return the next
      source file name and line.

      params:
        asm_path: string. asm (.s) file full path.
        asm_line: int
        asm_offset: int

      return:
        source code short file name or None if not found
        source code line number or 0 if not found
    """
    if m_debug.Debug: m_debug.dbg_print("asm_path = ", asm_path, "asm_line = ", asm_line, "asm_offset=", asm_offset)

    f = open(asm_path, "r")
    f.seek(asm_offset)
    short_src_file_name = None
    short_src_file_linenum = None

    additional_lines = 0
    while additional_lines < 5:
        line = f.readline()
        if "// LINE " in line:
            x = line.split()
            short_src_file_name = x[2]
            short_src_file_linenum = int(x[4][:-1])
            if m_debug.Debug: m_debug.dbg_print("short_src_file_name = ", short_src_file_name, "short_src_file_linenum", short_src_file_linenum,\
                               "found in asm line_num =", asm_line + additional_lines )
            f.close()
            return short_src_file_name, short_src_file_linenum
        else:
            additional_lines += 1

    f.close()
    return None, None

def look_up_next_opcode(asm_path, asm_line, asm_offset):
    """
      for a given asm file and asm line number and offset of the asm line number, return whether next opcode
      is going to jump to different source code line (e.g. Java code). If True, return next opcode's short 
      source file name and line.

      params:
        asm_path: string. asm (.s) file full path.
        asm_line: int
        asm_offset: int. start offset in asm file line

      return:
        True if msi cmd should stop at next opcodd, and source code short file name and source line
        False if msi cmd should not stop. None as source name, None as source line
    """
    if m_debug.Debug:
        m_debug.dbg_print("asm_path = ", asm_path, "asm_line = ", asm_line, "asm_offset=", asm_offset)

    f = open(asm_path, "r")
    f.seek(asm_offset)
    line = f.readline()

    short_src_file_name = None
    short_src_file_linenum = None
    additional_lines = 0
    while True:
        line = f.readline()
        if "// LINE " in line:
            x = line.split()
            short_src_file_name = x[2]
            short_src_file_linenum = int(x[4][:-1])
            if m_debug.Debug:
                m_debug.dbg_print("short_src_file_name = ", short_src_file_name,\
                                  "short_src_file_linenum", short_src_file_linenum,\
                                  "found in asm line_num =", asm_line + additional_lines)
            f.close()
            return True, short_src_file_name, short_src_file_linenum
        elif ".byte " in line:
            f.close()
            return False, None, None
        elif ".cfi_endproc" in line:
            break
        else:
            continue

    f.close()
    return False,None, None

def get_func_arguments(asm_path, mirbin_label, offset):
    """
    for a given asm (.s) file and a Maple symbol name, return this Maple symbol's
    formal (function argument) info and local (function local variable) info.

    params:
      asm_path: string. asm (.s) file full path.
      mirbin_label: string. Maple symbol name.
      offset: int. offset of asm file where Maple symbol block starts from

    returns:
      None if nothing found, or
      the result in dictionary format that contains
      1. list of formal types
      2. list of formal names
      3. list of local variable types
      4. list of local variable names

      result dictionary:
      {
        'formals_type': [...]
        'formals_name': [...]
        'locals_type' : [...]
        'locals_name' : [...]
      }
    """

    f = open(asm_path, "r")
    f.seek(offset)
    pre_line = None
    formals_type = []
    formals_name = []
    locals_type = []
    locals_name = []
    formals_num = None
    locals_num = None
    eval_depth = None
    method_flags = None

    line = f.readline()
    if m_debug.Debug: m_debug.dbg_print("line=", line, "mirbin_label=", mirbin_label)
    if mirbin_label in line:
        pre_line = line
    else:
        if m_debug.Debug: m_debug.dbg_print()
        f.close()
        return None
    if m_debug.Debug: m_debug.dbg_print()

    line = f.readline()
    if ".long" in line and mirbin_label in pre_line:
        pre_line = line
    else:
        if m_debug.Debug: m_debug.dbg_print()
        f.close()
        return None
    if m_debug.Debug: m_debug.dbg_print()

    line = f.readline()
    if "// func storage info" in line and ".long" in pre_line:
        x = line.split()
        formals_num = int(x[1][:-1])
        locals_num = int(x[2][:-1])
        eval_depth = int(x[3][:-1])
        method_flags = int(x[4])
        if m_debug.Debug: m_debug.dbg_print("formals_num=", formals_num,\
                          "locals_num=", locals_num, "eval_depth=", eval_depth, "method_flags=", method_flags)
        pre_line = line
    else:
        if m_debug.Debug: m_debug.dbg_print()
        f.close()
        return None
    if m_debug.Debug: m_debug.dbg_print()

    if formals_num > 0:
        line = f.readline()
        if "// PrimType of formal arguments" in line:
            pre_line = line
            f_num = formals_num
            while f_num != 0:
                line = f.readline()
                if m_debug.Debug: m_debug.dbg_print("line=", line)
                s = line.split()
                if s[1][-1] == ",":
                    formal_index = int(line.split()[1][:-1],16)
                else:
                    formal_index = int(line.split()[1],16)
                formals_type.append(MAPLE_TYPES[formal_index])
                f_num -= 1
                pre_line = line
        else:
            if m_debug.Debug: m_debug.dbg_print()
            f.close()
            return None
        if m_debug.Debug: m_debug.dbg_print()

    if locals_num > 0:
        line = f.readline()
        if "// PrimType of automatic variables" in line:
            pre_line = line
            l_num = locals_num
            while l_num != 0:
                line = f.readline()
                s = line.split()
                if s[1][-1] == ",":
                    local_index = int(line.split()[1][:-1],16)
                else:
                    local_index = int(line.split()[1],16)
                locals_type.append(MAPLE_TYPES[local_index])
                l_num -= 1
                pre_line = line
        else:
            if m_debug.Debug: m_debug.dbg_print()
            f.close()
            return None
        if m_debug.Debug: m_debug.dbg_print()

    if formals_num > 0:
        line = f.readline()
        if "// Name of formal arguments" in line:
            pre_line = line
            f_num = formals_num
            while f_num != 0:
                line = f.readline()
                formals_name.append(line.split()[1].split("\\0")[0][1:])
                f_num -= 1
                pre_line = line
        else:
            if m_debug.Debug: m_debug.dbg_print()
            f.close()
            return None
        if m_debug.Debug: m_debug.dbg_print()

    if locals_num > 0:
        line = f.readline()
        if "// Name of automatic variables" in line:
            pre_line = line
            l_num = locals_num
            while l_num != 0:
                line = f.readline()
                locals_name.append(line.split()[1].split('\\0')[0][1:])
                l_num -= 1
                pre_line = line
        else:
            if m_debug.Debug: m_debug.dbg_print()
            f.close()
            return None
        if m_debug.Debug: m_debug.dbg_print()

    line = f.readline()
    if ".p2align" in line:
        pass

    f.close()

    d = {}
    d["formals_type"] = formals_type
    d["formals_name"] = formals_name
    d["locals_type"] = locals_type
    d["locals_name"] = locals_name
    return d

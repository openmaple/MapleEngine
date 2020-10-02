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
import m_datastore
import m_frame
import m_list
import m_info
import m_util
from m_util import MColors
from m_util import gdb_print
import m_debug

MBT_FORMAT_SRC = 0
MBT_FORMAT_ASM = 1
MBT_FORMAT_MIR = 2

def print_maple_frame(frame, index, mbt_format):
    """
    prints one Maple backtrace frame.

    params:
      frame: a gdb.Frame object
      index: a index number of the frame.
      mbt_format: print Maple backtrace in specified format, MBT_FORMAT_SRC or MBT_FORMAT_ASM or MBT_FORMAT_MIR
    """

    data = m_datastore.get_stack_frame_data(frame)
    if not data:
        gdb_print('#%i no info' % (index))
        return

    so_path = data['frame_func_header_info']['so_path']
    asm_path = data['frame_func_header_info']['asm_path']
    mirmpl_path = data['frame_func_header_info']['mirmpl_path']
    func_addr_offset = data['frame_func_header_info']['func_addr_offset']
    func_name = data['frame_func_header_info']['func_name']

    src_file_short_name = data['frame_func_src_info']['short_src_file_name']
    asm_line_num = data['frame_func_src_info']['asm_line']
    mirmpl_line_num = data['frame_func_src_info']['mirmpl_line']
    src_file_line = data['frame_func_src_info']['short_src_file_line']

    func_argus_locals = data['func_argus_locals']

    if src_file_short_name:
        file_full_path = None
        for source_path in m_list.maple_source_path_list:
            file_full_path = m_list.find_one_file(src_file_short_name, source_path)
            if not file_full_path:
                continue
            else:
                break
        if not file_full_path: file_full_path = "unknown"
    else:
        file_full_path = 'unknown'

    # buffer format
    # index func_offset func_symbol (type argu=value, ...) at source-file-full-path:line_num
    args_buffer = ""
    arg_num = len(func_argus_locals['formals_name'])
    for i in range(arg_num):
        arg_value = m_info.get_maple_caller_argument_value(i, arg_num, func_argus_locals['formals_type'][i])
        if arg_value:
            if func_argus_locals['formals_type'][i] == 'a64':
                mtype = m_info.get_maple_a64_pointer_type(arg_value)
                if not mtype:
                    mtype = ""
            else:
                mtype = ""
        else:
            arg_value = '...'
            mtype = ""
        args_buffer += func_argus_locals['formals_type'][i]
        args_buffer += '<' + mtype + '>'
        args_buffer += ' '
        args_buffer += MColors.BT_ARGNAME + func_argus_locals['formals_name'][i] + MColors.ENDC
        #args_buffer += func_argus_locals['formals_name'][i]
        args_buffer += '='
        args_buffer += arg_value
        args_buffer += ', '
    if arg_num > 0:
        args_buffer = args_buffer[:-2]
    if m_debug.Debug: m_debug.dbg_print("arg_num=", arg_num, " args_buffer=", args_buffer)

    if mbt_format == MBT_FORMAT_ASM:
        buffer = '#%i %s:%s %s(%s) at %s:%s' % \
           (index, MColors.BT_ADDR + so_path.split('/')[-1] + MColors.ENDC,MColors.BT_ADDR + func_addr_offset + MColors.ENDC,\
             m_util.color_symbol(MColors.BT_FNNAME, func_name), args_buffer, MColors.BT_SRC + asm_path + MColors.ENDC, asm_line_num)
    elif mbt_format == MBT_FORMAT_SRC:
        buffer = '#%i %s:%s %s(%s) at %s:%s' % \
           (index, MColors.BT_ADDR + so_path.split('/')[-1] + MColors.ENDC, MColors.BT_ADDR + func_addr_offset + MColors.ENDC,\
            m_util.color_symbol(MColors.BT_FNNAME, func_name), args_buffer, MColors.BT_SRC + file_full_path + MColors.ENDC, src_file_line)
    else:
        buffer = '#%i %s:%s %s(%s) at %s:%s' % \
           (index, MColors.BT_ADDR + so_path.split('/')[-1] + MColors.ENDC,MColors.BT_ADDR + func_addr_offset + MColors.ENDC,\
             m_util.color_symbol(MColors.BT_FNNAME, func_name), args_buffer, MColors.BT_SRC + mirmpl_path + MColors.ENDC, mirmpl_line_num)
    gdb_print(buffer)

def print_gdb_frame(frame, index):
    """ print one gdb native backtrace frame """

    s   = m_util.gdb_exec_to_str('bt')
    bt  = s.split('#' + str(index))
    bt  = bt[1].split('#')[0][:-1]
    bt  = '#' + str(index) + bt
    gdb_print(bt)

class MapleBacktrace(gdb.Command):
    """displays Maple backtrace in multiple modes
    mbt is the alias of mbacktrace command
    mbacktrace: prints backtrace of Maple frames
    mbacktrace -asm: prints backtrace of Maple frames in assembly format
    mbacktrace -mir: prints backtrace of Maple frames in Maple IR format
    mbacktrace -full: prints backtrace of mixed gdb native frames and Maple frames
    """

    def __init__(self):
        gdb.Command.__init__ (self,
                              "mbacktrace",
                              gdb.COMMAND_STACK,
                              gdb.COMPLETE_NONE)

        m_util.gdb_exec('alias mbt = mbacktrace')

    def invoke(self, args, from_tty):
        self.mbt_func(args, from_tty)

    def mbt_func(self, args, from_tty):
        s = str(args)
        mbt_format = MBT_FORMAT_SRC
        full = False
        if s == "-full" or s == "full":
            full = True
        elif s == "-asm":
            mbt_format = MBT_FORMAT_ASM
        elif s == "-mir":
            mbt_format = MBT_FORMAT_MIR

        selected_frame = m_frame.get_selected_frame()
        newest_frame = m_frame.get_newest_frame()
        if not selected_frame or not newest_frame:
            gdb_print('Unable to locate Maple frame')
            return

        # walk through from innermost frame to selected frame
        index = 0
        frame = newest_frame
        while frame != selected_frame and frame:
            frame = m_frame.get_next_older_frame(frame)
            index += 1

        if not frame:
            gdb_print('No valid frames found')
            return

        start_level = index
        gdb_print('Maple Traceback (most recent call first):')
        while frame:
            if m_frame.is_maple_frame(frame):
                print_maple_frame(frame, index, mbt_format)
            elif full:
                print_gdb_frame(frame, index)
            index += 1
            frame = m_frame.get_next_older_frame(frame)
            if frame:
                m_util.gdb_exec_to_null('up-silently')

        # move frame back to the original stack level
        m_util.gdb_exec_to_null('down-silently ' + str(index - start_level - 1))


class MapleUpCmd(gdb.Command):
    """selects and prints Maple stack frame that called this one
    mup: moves up one Maple frame to caller of selected Maple frame
    mup -n: moves up n Maple frames from currently selected Maple frame
    """
    def __init__(self):
        gdb.Command.__init__ (self,
                              "mup",
                              gdb.COMMAND_STACK,
                              gdb.COMPLETE_NONE)

    def invoke(self, args, from_tty):
        self.mup_func(args, from_tty)

    def usage(self):
        gdb_print("mup : Moves Maple frame up one level")
        gdb_print("mup -n: Moves Maple frame up n levels")

    def mup_func(self, args, from_tty):
        steps = 1
        s = str(args).split()
        if len(s) is 1:
            try:
                steps = int(s[0])
            except:
                pass
        elif len(s) > 1:
            self.usage()
            return

        selected_frame = m_frame.get_selected_frame()
        newest_frame = m_frame.get_newest_frame()
        if not selected_frame or not newest_frame:
            gdb_print('Unable to locate Maple frame')
            return

        # walk through from innermost frame to selected frame
        # to get the level number from newest frame to selected frame
        index = 0
        frame = newest_frame
        while frame != selected_frame and frame:
            frame = m_frame.get_next_older_frame(frame)
            index += 1
        if not frame:
            gdb_print('No valid frames found')
            return

        prev_maple_frame = frame
        prev_maple_frame_index = index
        while frame and steps > 0:
            frame = m_frame.get_next_older_frame(frame)
            if frame:
                m_util.gdb_exec_to_null('up-silently')
                index += 1
            else:
                frame = prev_maple_frame
                m_util.gdb_exec_to_null('down-silently ' + str(index - prev_maple_frame_index))
                index = prev_maple_frame_index
                break

            if m_frame.is_maple_frame(frame):
                prev_maple_frame = frame
                prev_maple_frame_index = index
                steps -= 1

        print_maple_frame(frame, index, MBT_FORMAT_SRC)
        m_datastore.mgdb_rdata.update_frame_change_counter()

class MapleDownCmd(gdb.Command):
    """selects and prints Maple stack frame called by this one
    mdown: moves down one Maple frame called by selected Maple frame
    mdown -n: moves down n Maple frames called from currently selected Maple frame
    """
    def __init__(self):
        gdb.Command.__init__ (self,
                              "mdown",
                              gdb.COMMAND_STACK,
                              gdb.COMPLETE_NONE)

    def invoke(self, args, from_tty):
        self.mdown_func(args, from_tty)

    def usage(self):
        gdb_print("mdown : Moves Maple frame down one level")
        gdb_print("mdown -n: Moves Maple frame down n levels")

    def mdown_func(self, args, from_tty):
        steps = 1
        s = str(args).split()
        if len(s) is 1:
            try:
                steps = int(s[0])
            except:
                pass
        elif len(s) > 1:
            self.usage()
            return

        selected_frame = m_frame.get_selected_frame()
        newest_frame = m_frame.get_newest_frame()
        if not selected_frame or not newest_frame:
            gdb_print('Unable to locate Maple frame')
            return

        # walk through from innermost frame to selected frame
        # to get the level number from newest frame to selected frame.
        index = 0
        frame = newest_frame
        while frame != selected_frame and frame:
            frame = m_frame.get_next_older_frame(frame)
            index += 1

        if not frame:
            gdb_print('No valid frames found')
            return

        prev_maple_frame = frame
        prev_maple_frame_index = index
        while frame and steps > 0:
            frame = m_frame.get_next_newer_frame(frame)
            if frame:
                m_util.gdb_exec_to_null('down-silently')
                index -= 1
            else:
                frame = prev_maple_frame
                m_util.gdb_exec_to_null('up-silently ' + str(index - prev_maple_frame_index))
                index = prev_maple_frame_index
                break

            if m_frame.is_maple_frame(frame):
                prev_maple_frame = frame
                prev_maple_frame_index = index
                steps -= 1

        print_maple_frame(frame, index, MBT_FORMAT_SRC)
        m_datastore.mgdb_rdata.update_frame_change_counter()

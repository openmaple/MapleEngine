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
import m_datastore
import m_frame
import m_list
import m_info
from inspect import currentframe, getframeinfo
import m_util

def print_maple_frame(frame, index, asm_format):
    """
    print one Maple backtrace frame.

    params:
      frame: a gdb.Frame object
      index: a index number of the frame.
      asm_format: print Maple backtrace in asm format if set
    """

    data = m_datastore.get_stack_frame_data(frame)
    if not data:
        sys.stdout.write('#%i no info\n' % (index))
        return
    
    so_path = data['frame_func_header_info']['so_path']
    asm_path = data['frame_func_header_info']['asm_path']
    func_addr_offset = data['frame_func_header_info']['func_addr_offset']
    func_name = data['frame_func_header_info']['func_name']

    src_file_short_name = data['frame_func_src_info']['short_src_file_name']
    asm_line_num = data['frame_func_src_info']['asm_line']
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
        args_buffer += func_argus_locals['formals_name'][i]
        args_buffer += '='
        args_buffer += arg_value
        args_buffer += ', '
    if arg_num > 0:
        args_buffer = args_buffer[:-2]
    m_util.debug_print(__file__, getframeinfo(currentframe()).lineno, print_maple_frame,\
                        "arg_num=", arg_num, " args_buffer=", args_buffer)
        
    if asm_format:
        buffer = '#%i %s:%s %s(%s) at %s:%s\n' % (index, so_path.split('/')[-1],func_addr_offset, func_name, args_buffer, asm_path, asm_line_num)
    else:
        buffer = '#%i %s:%s %s(%s) at %s:%s\n' % (index, so_path.split('/')[-1],func_addr_offset, func_name, args_buffer, file_full_path, src_file_line)
    sys.stdout.write(buffer)

def print_gdb_frame(frame, index):
    """ print one gdb native backtrace frame """

    s   = gdb.execute('bt', to_string = True)
    bt  = s.split('#' + str(index))
    bt  = bt[1].split('#')[0][:-1]
    bt  = '#' + str(index) + bt
    print(bt)

class MapleBacktrace(gdb.Command):
    """
    This class defines a Maple backtrace command, to print stack frames in calling sequence.
    """

    def __init__(self):
        gdb.Command.__init__ (self,
                              "mbacktrace",
                              gdb.COMMAND_STACK,
                              gdb.COMPLETE_NONE)

        gdb.execute('alias mbt = mbacktrace')

    def invoke(self, args, from_tty):
        gdb.execute('set pagination off', to_string = True)
        self.mbt_func(args, from_tty)
        gdb.execute('set pagination on', to_string = True)

    def mbt_func(self, args, from_tty):
        s = str(args)
        asm = False
        full = False
        if s == "-full" or s == "full":
            full = True
        elif s == "-asm":
            asm = True
        
        selected_frame = m_frame.get_selected_frame()
        newest_frame = m_frame.get_newest_frame()
        if not selected_frame or not newest_frame:
            print('Unable to locate Maple frame')
            return

        # walk through from innermost frame to selected frame
        index = 0
        frame = newest_frame
        while frame != selected_frame and frame:
            frame = m_frame.get_next_older_frame(frame)
            index += 1

        if not frame:
            print('No valid frames found')
            return

        start_level = index
        sys.stdout.write('Maple Traceback (most recent call first):\n')
        while frame:
            if m_frame.is_maple_frame(frame):
                print_maple_frame(frame, index, asm)
            elif full:
                print_gdb_frame(frame, index)
            index += 1
            frame = m_frame.get_next_older_frame(frame)
            if frame:
                gdb.execute('up-silently ', to_string = True)

        # move frame back to the original stack level
        gdb.execute('down-silently ' + str(index - start_level - 1), to_string = True)


class MapleUpCmd(gdb.Command):
    """
    This class defines Maple command mup, to move up the Maple frame in stack.
    """
    def __init__(self):
        gdb.Command.__init__ (self,
                              "mup",
                              gdb.COMMAND_STACK,
                              gdb.COMPLETE_NONE)

    def invoke(self, args, from_tty):
        gdb.execute('set pagination off', to_string = True)
        self.mup_func(args, from_tty)
        gdb.execute('set pagination on', to_string = True)

    def usage(self):
        print("mup : move Maple frame up one level")
        print("mup -n: move Maple frame up n levels")

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
            print('Unable to locate Maple frame')
            return

        # walk through from innermost frame to selected frame
        # to get the level number from newest frame to selected frame
        index = 0
        frame = newest_frame
        while frame != selected_frame and frame:
            frame = m_frame.get_next_older_frame(frame)
            index += 1
        if not frame:
            print('No valid frames found')
            return

        prev_maple_frame = frame
        prev_maple_frame_index = index
        while frame and steps > 0:
            frame = m_frame.get_next_older_frame(frame)
            if frame:
                gdb.execute('up-silently ', to_string = True)
                index += 1
            else:
                frame = prev_maple_frame
                gdb.execute('down-silently ' + str(index - prev_maple_frame_index), to_string = True)
                index = prev_maple_frame_index
                break
            
            if m_frame.is_maple_frame(frame):
                prev_maple_frame = frame
                prev_maple_frame_index = index
                steps -= 1

        asm_format = False
        print_maple_frame(frame, index, asm_format)

class MapleDownCmd(gdb.Command):
    """
    This class defines Maple command mdown, to move down the Maple frame in stack.
    """
    def __init__(self):
        gdb.Command.__init__ (self,
                              "mdown",
                              gdb.COMMAND_STACK,
                              gdb.COMPLETE_NONE)

    def invoke(self, args, from_tty):
        gdb.execute('set pagination off', to_string = True)
        self.mdown_func(args, from_tty)
        gdb.execute('set pagination on', to_string = True)

    def usage(self):
        print("mdown : move Maple frame down one level")
        print("mdown -n: move Maple frame down n levels")

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
            print('Unable to locate Maple frame')
            return

        # walk through from innermost frame to selected frame
        # to get the level number from newest frame to selected frame.
        index = 0
        frame = newest_frame
        while frame != selected_frame and frame:
            frame = m_frame.get_next_older_frame(frame)
            index += 1

        if not frame:
            print('No valid frames found')
            return

        prev_maple_frame = frame
        prev_maple_frame_index = index
        while frame and steps > 0:
            frame = m_frame.get_next_newer_frame(frame)
            if frame:
                gdb.execute('down-silently ', to_string = True)
                index -= 1
            else:
                frame = prev_maple_frame
                gdb.execute('up-silently ' + str(index - prev_maple_frame_index), to_string = True)
                index = prev_maple_frame_index
                break
            
            if m_frame.is_maple_frame(frame):
                prev_maple_frame = frame
                prev_maple_frame_index = index
                steps -= 1

        asm_format = False
        print_maple_frame(frame, index, asm_format)

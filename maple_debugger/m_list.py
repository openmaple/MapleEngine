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
import os
import sys
import m_frame
import m_datastore
from inspect import currentframe, getframeinfo
import m_util

maple_source_path_list = []

def find_one_file(name, path):
    """ Find the file with specified name and given base path """
    for root, dirs, files in os.walk(path):
        if name in files:
            return os.path.join(root, name)

class MapleSourcePathCmd(gdb.Command):
    """
    This class defines msrcpath command, to manage the paths used for searching
    Maple application source files
    """

    def __init__(self):
        gdb.Command.__init__ (self,
                              "msrcpath",
                              gdb.COMMAND_FILES,
                              gdb.COMPLETE_NONE)

        global maple_source_path_list

        # add current working directory at the top
        line = gdb.execute('pwd', to_string=True)
        if line:
            s = line.split()
            line = s[2][:-1] if len(s) is 3 else None
            if line:
                #maple_source_path_list.append(line)
                maple_source_path_list.append(os.path.expanduser(line))
        '''
        if len(maple_source_path_list) == 1:
            print("Maple application source code search path only has one path as following:")
            print("  ", maple_source_path_list[0])
            print("Please add more search paths using msrcpath -add <path> command")
        '''

        gdb.execute('alias msp = msrcpath')

    def invoke(self, args, from_tty):
        self.msrcpath_func(args, from_tty)

    def usage(self):
        print ("  msrcpath : ")
        print ("    msrcpath -show or msrcpath: display all the source code search paths")
        print ("    msrcpath -add <path>:  add one path to the top of the list")
        print ("    msrcpath -del <path>:  delete specified path from the list")
        print ("    msrcpath -help:        print this message")
        print ("\n")

    def msrcpath_func(self, args, from_tty):
        s = str(args)
        if len(s) is 0: #default to msrc -show
            self.show_path()
            return

        x = s.split()
        if len(x) == 1: # src -show
            if x[0] == '-show':
                self.show_path()
            elif x[0] == '-help':
                self.usage()
            else:
                self.usage()
                return
        elif len(x) is 2: # add one path
            if x[0] == '-add':
                self.add_path(x[1])
            elif x[0] == '-del':
                self.del_path(x[1])
            else:
                self.usage()
                return
 
    def show_path(self):
        print ("Maple source path list: --")
        if len(maple_source_path_list) is 0:
            print ("none")
            return
        for path in maple_source_path_list:
            print (path)

    def add_path(self, path):
        # we add path into the top of the list
        global maple_source_path_list
        if path in maple_source_path_list:
            maple_source_path_list.remove(path)
        maple_source_path_list = [os.path.expandvars(os.path.expanduser(path))] + maple_source_path_list

    def del_path(self, path):
        # if path is in maple_source_path_list, delete it.
        global maple_source_path_list
        if path in maple_source_path_list:
            maple_source_path_list.remove(path)

class MapleListCmd(gdb.Command):
    """
    This class defines Maple command mlist, to display the proper lines of Maple
    application source file or its compiled asm file
    """

    def __init__(self):
        gdb.Command.__init__ (self,
                              "mlist",
                              gdb.COMMAND_FILES,
                              gdb.COMPLETE_NONE)

        self.short_src_file_name = None
        self.src_file_line = 1
        self.asm_file_full_path = None
        self.asm_file_line = 1
        self.func_header_name = None
        self.asm_func_header_name = None

    def invoke(self, args, from_tty):
        gdb.execute('set pagination off', to_string = True)
        self.mlist_func(args, from_tty)
        gdb.execute('set pagination on', to_string = True)

    def usage(self):
        print ("  mlist      : list source code associated with current Maple frame")
        print ("  mlist -asm : list assemble instructions associated with current Maple frame")


    def mlist_func(self, args, from_tty):
        s = str(args)
        if len(s) is 0: #mlist current source code file
            self.mlist_source_file_func()
            return

        x = s.split()
        if len(x) == 1: # mlist co-responding asm file
            if x[0] == '-asm':
                self.mlist_asm_file_func()
        return

    def mlist_asm_file_func(self):
        frame = m_frame.get_selected_frame()
        if not frame:
            return

        ds = m_datastore.get_stack_frame_data(frame)
        if not ds:
            return

        m_util.debug_print(__file__, getframeinfo(currentframe()).lineno, self.mlist_asm_file_func, "ds=", ds)
        m_util.debug_print(__file__, getframeinfo(currentframe()).lineno, self.mlist_asm_file_func,\
                        "asm_file_full_path=", ds['frame_func_header_info']['asm_path'])
        m_util.debug_print(__file__, getframeinfo(currentframe()).lineno, self.mlist_asm_file_func,\
                        "asm_file_line=", ds['frame_func_src_info']['asm_line'])
        m_util.debug_print(__file__, getframeinfo(currentframe()).lineno, self.mlist_asm_file_func,\
                        "asm_func_header_name=", ds['frame_func_header_info']['func_header_name'])
        m_util.debug_print(__file__, getframeinfo(currentframe()).lineno, self.mlist_asm_file_func,\
                        "self.asm_file_full_name: ", self.asm_file_full_path)
        m_util.debug_print(__file__, getframeinfo(currentframe()).lineno, self.mlist_asm_file_func,\
                        "self.asm_file_line: ", self.asm_file_line)
        m_util.debug_print(__file__, getframeinfo(currentframe()).lineno, self.mlist_asm_file_func,\
                        "self.asm_func_header_name: ", self.asm_func_header_name)

        if not self.asm_file_full_path:
            m_util.debug_print(__file__, getframeinfo(currentframe()).lineno, self.mlist_asm_file_func)
            self.asm_file_full_path = ds['frame_func_header_info']['asm_path']
            self.asm_file_line = ds['frame_func_src_info']['asm_line']
            self.asm_func_header_name = ds['frame_func_header_info']['func_header_name']
        else:
            if self.asm_file_full_path != ds['frame_func_header_info']['asm_path']:
                m_util.debug_print(__file__, getframeinfo(currentframe()).lineno, self.mlist_asm_file_func)
                self.asm_file_full_path = ds['frame_func_header_info']['asm_path']
                self.asm_file_line = ds['frame_func_src_info']['asm_line']
                self.asm_func_header_name = ds['frame_func_header_info']['func_header_name']
            else:
                if self.asm_func_header_name == ds['frame_func_header_info']['func_header_name']:
                    m_util.debug_print(__file__, getframeinfo(currentframe()).lineno, self.mlist_asm_file_func)
                    pass
                else:
                    m_util.debug_print(__file__, getframeinfo(currentframe()).lineno, self.mlist_asm_file_func)
                    self.asm_file_line = ds['frame_func_src_info']['asm_line']
                    self.asm_func_header_name = ds['frame_func_header_info']['func_header_name']

        self.display_asm_file_lines(self.asm_file_full_path, self.asm_file_line, ds['frame_func_header_info']['func_header_asm_tuple'])

    def mlist_source_file_func(self):
        frame = m_frame.get_selected_frame()
        if not frame:
            return

        ds = m_datastore.get_stack_frame_data(frame)
        if not ds:
            return

        m_util.debug_print(__file__, getframeinfo(currentframe()).lineno, self.mlist_source_file_func, "ds=", ds)
        m_util.debug_print(__file__, getframeinfo(currentframe()).lineno, self.mlist_source_file_func,\
                        "src_file_short_name: ", ds['frame_func_src_info']['short_src_file_name'])
        m_util.debug_print(__file__, getframeinfo(currentframe()).lineno, self.mlist_source_file_func,\
                        "src_file_line: ", ds['frame_func_src_info']['short_src_file_line'])
        m_util.debug_print(__file__, getframeinfo(currentframe()).lineno, self.mlist_source_file_func,\
                        "func_header_name: ", ds['frame_func_header_info']['func_header_name'])
        m_util.debug_print(__file__, getframeinfo(currentframe()).lineno, self.mlist_source_file_func,\
                        "self.src_file_short_name: ", self.short_src_file_name)
        m_util.debug_print(__file__, getframeinfo(currentframe()).lineno, self.mlist_source_file_func,\
                        "self.src_file_line: ", self.src_file_line)
        m_util.debug_print(__file__, getframeinfo(currentframe()).lineno, self.mlist_source_file_func,\
                        "self.func_header_name: ", self.func_header_name)

        if not self.short_src_file_name:
            self.short_src_file_name = ds['frame_func_src_info']['short_src_file_name']
            self.src_file_line = ds['frame_func_src_info']['short_src_file_line']
            self.func_header_name = ds['frame_func_header_info']['func_header_name']
        else:
            if self.short_src_file_name != ds['frame_func_src_info']['short_src_file_name']:
                self.short_src_file_name = ds['frame_func_src_info']['short_src_file_name']
                self.src_file_line = ds['frame_func_src_info']['short_src_file_line']
                self.func_header_name = ds['frame_func_header_info']['func_header_name']
            else:
                if self.func_header_name == ds['frame_func_header_info']['func_header_name']:
                    pass
                else:
                    self.src_file_line = ds['frame_func_src_info']['short_src_file_line']
                    self.func_header_name = ds['frame_func_header_info']['func_header_name']

        file_full_path = None
        for source_path in maple_source_path_list:
            file_full_path = find_one_file(self.short_src_file_name, source_path)
            if not file_full_path:
                continue
            else:
                break
        
        if not file_full_path:
            print ("source code file ", self.short_src_file_name, " not found in any path")
            return
        self.display_src_lines(file_full_path, self.src_file_line)


    def display_src_lines(self, filename, line_num):
        if line_num <= 5:
            start_line = 1
        else:
            start_line = line_num - 4

        print ("file: ", filename, " line: ", line_num)
        f = open(filename, 'r')
        count = 1
        for line in f:
            if count < start_line:
                count += 1
                continue
            elif count >= start_line and count <= line_num + 4:
                print(count, "   ", line.rstrip())
                count += 1
            else:
                self.src_file_line = count + 4
                break
        print ("\n")
        f.close()

    

    def display_asm_file_lines(self, filename, line_num, asm_tuple):
        """
        params:
          filename: string. asm file full path
          line_num: int. line number of this asm file
          asm_tuple: (maple_symbol_block_start_line, block_start_offset, block_end_offset)
        """

        if line_num <= 5:
            start_line = 1
            end_line = 9
        else:
            start_line = line_num - 4
            end_line = line_num + 4
 
        block_line = asm_tuple[0] #this offset is the label's starting line number
        block_offset = asm_tuple[1]

        
        f = open(filename, 'r')
        f.seek(block_offset)
        ln = block_line
        offset = asm_tuple[1]
        while start_line > ln:
            line = f.readline()
            offset += len(line)
            ln += 1

        print ("file: ", filename, " line: ", line_num)
        count = start_line
        for count in range(start_line, end_line):
            line = f.readline()
            if not line:
                break
            else:
                print(count, "   ", line.rstrip())
        f.close()
        print ("\n")
    
        self.asm_file_line = end_line + 4


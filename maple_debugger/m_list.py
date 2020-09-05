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
import m_frame
import m_datastore
import m_util
import m_set
import m_debug
from m_util import MColors
from m_util import gdb_print

maple_source_path_list = []

def find_one_file(name, path):
    """ Find the file with specified name and given base path """
    if not name:
        return None
    exist = m_datastore.mgdb_rdata.in_fullpath_cache(name, path)
    if exist:
        return m_datastore.mgdb_rdata.get_fullpath_cache(name, path)
    res = None
    for root, dirs, files in os.walk(path):
        if name in files:
            res = os.path.join(root, name)
            if not res:
                if m_debug.Debug: m_debug.dbg_print("fine_one_file found file with None fullpath")
            break
    if res:
        m_datastore.mgdb_rdata.add_fullpath_cache(name, path, res)
    return res

class MapleSourcePathCmd(gdb.Command):
    """Add and manage the Maple source code search paths
    msp is the alias of msrcpath command
    msrcpath: display all the search paths of Maple application and library source code
    msrcpath -show: same to msrcpath command without arugment
    msrcpath -add <path>:  add one path to the top of the list
    msrcpath -del <path>:  delete specified path from the list
    """

    def __init__(self):
        gdb.Command.__init__ (self,
                              "msrcpath",
                              gdb.COMMAND_FILES,
                              gdb.COMPLETE_NONE)

        global maple_source_path_list

        # add current working directory at the top
        line = m_util.gdb_exec_to_string('pwd')
        if line:
            s = line.split()
            line = s[2][:-1] if len(s) is 3 else None
            if line:
                maple_source_path_list.append(os.path.expandvars(os.path.expanduser(line)))
        '''
        if len(maple_source_path_list) == 1:
            gdb_print("Maple application source code search path only has one path as following:")
            gdb_print("  ", maple_source_path_list[0])
            gdb_print("Please add more search paths using msrcpath -add <path> command")
        '''

        m_util.gdb_exec('alias msp = msrcpath')

    def invoke(self, args, from_tty):
        self.msrcpath_func(args, from_tty)

    def usage(self):
        gdb_print ("  msrcpath : ")
        gdb_print ("    msrcpath -show or msrcpath: display all the source code search paths")
        gdb_print ("    msrcpath -add <path>:  add one path to the top of the list")
        gdb_print ("    msrcpath -del <path>:  delete specified path from the list")
        gdb_print ("    msrcpath -help:        print this message")
        gdb_print ("\n")

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
        gdb_print ("Maple source path list: --")
        if len(maple_source_path_list) is 0:
            gdb_print ("none")
            return
        for path in maple_source_path_list:
            gdb_print (path)
        if m_debug.Debug: m_datastore.mgdb_rdata.show_file_cache()

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
            m_datastore.mgdb_rdata.del_fullpath_cache()
            m_datastore.mgdb_rdata.del_src_lines()
            if m_debug.Debug: m_datastore.mgdb_rdata.show_file_cache()

class MapleListCmd(gdb.Command):
    """List source code in multiple modes
    mlist: list source code associated with current Maple frame
    mlist -asm: list assemble instructions associated with current Maple frame
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
        self.mlist_func(args, from_tty)

    def usage(self):
        gdb_print ("  mlist      : list source code associated with current Maple frame")
        gdb_print ("  mlist -asm : list assemble instructions associated with current Maple frame")


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

        if m_debug.Debug:
            m_debug.dbg_print("ds=", ds)
            m_debug.dbg_print("asm_file_full_path=", ds['frame_func_header_info']['asm_path'])
            m_debug.dbg_print("asm_file_line=", ds['frame_func_src_info']['asm_line'])
            m_debug.dbg_print("asm_func_header_name=", ds['frame_func_header_info']['func_header_name'])
            m_debug.dbg_print("self.asm_file_full_name: ", self.asm_file_full_path)
            m_debug.dbg_print("self.asm_file_line: ", self.asm_file_line)
            m_debug.dbg_print("self.asm_func_header_name: ", self.asm_func_header_name)

        if not self.asm_file_full_path:
            if m_debug.Debug: m_debug.dbg_print()
            self.asm_file_full_path = ds['frame_func_header_info']['asm_path']
            self.asm_file_line = ds['frame_func_src_info']['asm_line']
            self.asm_func_header_name = ds['frame_func_header_info']['func_header_name']
        else:
            if self.asm_file_full_path != ds['frame_func_header_info']['asm_path']:
                if m_debug.Debug: m_debug.dbg_print()
                self.asm_file_full_path = ds['frame_func_header_info']['asm_path']
                self.asm_file_line = ds['frame_func_src_info']['asm_line']
                self.asm_func_header_name = ds['frame_func_header_info']['func_header_name']
            else:
                if self.asm_func_header_name == ds['frame_func_header_info']['func_header_name']:
                    if m_debug.Debug: m_debug.dbg_print()
                    pass
                else:
                    if m_debug.Debug: m_debug.dbg_print()
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

        if m_debug.Debug:
            m_debug.dbg_print("ds=", ds)
            m_debug.dbg_print("src_file_short_name: ", ds['frame_func_src_info']['short_src_file_name'])
            m_debug.dbg_print("src_file_line: ", ds['frame_func_src_info']['short_src_file_line'])
            m_debug.dbg_print("func_header_name: ", ds['frame_func_header_info']['func_header_name'])
            m_debug.dbg_print("self.src_file_short_name: ", self.short_src_file_name)
            m_debug.dbg_print("self.src_file_line: ", self.src_file_line)
            m_debug.dbg_print("self.func_header_name: ", self.func_header_name)

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
            if self.short_src_file_name:
                gdb_print ("source code file " + self.short_src_file_name + " not found in any path")
            else:
                gdb_print ("source code file not found. try 'mlist -asm' instead")
            return
        self.display_src_lines(file_full_path, self.src_file_line)


    def display_src_lines(self, filename, line_num):
        exist = m_datastore.mgdb_rdata.in_src_lines(filename)
        if exist:
            total = m_datastore.mgdb_rdata.get_src_lines(filename)
        else:
            with open (filename) as fp:
                total = len(fp.readlines())
            m_datastore.mgdb_rdata.add_src_lines(filename, total)
        try:
            count = int(m_set.msettings["linecount"])
            if count < 5:
                count = 10 if count != 0 else total << 1
        except:
            count = 10
        half = (count - 1) >> 1
        s_line = str(line_num - half) if line_num > half + 1 else '1'
        half = count - half - 1
        e_line = str(line_num + half) if line_num + half <= total else str(total)
        gdb_print("src file: %s%s%s line: %s" % (MColors.BT_SRC, filename, MColors.ENDC, str(line_num)))
        if m_util.Highlighted:
            buf =m_util.shell_cmd('highlight -O xterm256 --force -s %s %s | sed -n %s,%sp |'
                ' nl -ba -s" " -w8 -v%s | sed "s/^  \\( *%s \\)/=>\\1/"' \
                % (m_util.HighlightStyle, filename, s_line, e_line, s_line, str(line_num)))
        else:
            buf = m_util.shell_cmd('sed -n %s,%sp %s | nl -ba -s" " -w8 -v%s | sed "s/^  \\( *%s \\)/=>\\1/"' \
                % (s_line, e_line, filename, s_line, str(line_num)))
        m_util.gdb_write(buf)
        self.src_file_line = line_num + count if line_num + count <= total else total


    def display_asm_file_lines(self, filename, line_num, asm_tuple):
        """
        params:
          filename: string. asm file full path
          line_num: int. line number of this asm file
          asm_tuple: (maple_symbol_block_start_line, block_start_offset, block_end_offset)
        """
        exist = m_datastore.mgdb_rdata.in_asmblock_lines(filename, asm_tuple[0])
        if exist:
            total = m_datastore.mgdb_rdata.get_asmblock_lines(filename, asm_tuple[0])
        else:
            with open(filename) as fp:
                offset = asm_tuple[1]
                fp.seek(offset)
                total = 0
                while offset < asm_tuple[2]:
                    line = fp.readline()
                    offset += len(line)
                    total += 1
            m_datastore.mgdb_rdata.add_asmblock_lines(filename, asm_tuple[0], total)
        try:
            count = int(m_set.msettings["linecount"])
            if count < 9:
                count = 20 if count != 0 else total << 1
        except:
            count = 20
        if line_num < asm_tuple[0] or line_num > asm_tuple[0] + total:
            return
        curr_num = line_num - asm_tuple[0] + 1
        half = (count - 1) >> 1
        s_line = curr_num - half if curr_num > half + 1 else 1
        half = count - half - 1
        e_line = curr_num + half if curr_num + half <= total else total
        gdb_print("asm file: %s%s%s line: %s" % (MColors.BT_SRC, filename, MColors.ENDC, str(line_num)))
        ss_line = asm_tuple[0] + s_line - 1
        if m_util.Highlighted:
            buf =m_util.shell_cmd('dd skip=%s bs=1 count=%s if=%s 2> /dev/null |'
                ' highlight -O xterm256 -S asm -s %s | sed -n %s,%sp |'
                ' nl -ba -s" " -w12 -v%s | sed "s/^  \\( *%s \\)/=>\\1/"' \
                % (str(asm_tuple[1]), str(asm_tuple[2] - asm_tuple[1]), filename, \
                   m_util.HighlightStyle, str(s_line), str(e_line), str(ss_line), str(line_num)))
        else:
            buf =m_util.shell_cmd('dd skip=%s bs=1 count=%s if=%s 2> /dev/null |'
                ' sed -n %s,%sp | nl -ba -s" " -w12 -v%s | sed "s/^  \\( *%s \\)/=>\\1/"' \
                % (str(asm_tuple[1]), str(asm_tuple[2] - asm_tuple[1]), filename, \
                   str(s_line), str(e_line), str(ss_line), str(line_num)))
        m_util.gdb_write(buf)
        self.asm_file_line = line_num + count if line_num + count < asm_tuple[0] + total else asm_tuple[0] + total - 1

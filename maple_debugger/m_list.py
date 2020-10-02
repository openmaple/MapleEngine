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

default_src_display_line_count = 10
default_asm_display_line_count = 20
default_mir_display_line_count = 20

asm_lang = os.path.expandvars('$MAPLE_DEBUGGER_ROOT/highlight/asm.lang')
mpl_lang = os.path.expandvars('$MAPLE_DEBUGGER_ROOT/highlight/mpl.lang')

MLIST_MODE_SRC = 0
MLIST_MODE_ASM = 1
MLIST_MODE_MIR = 2

def find_one_file(name, path):
    """ find the file with specified name and given base path """
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
                if m_debug.Debug: m_debug.dbg_print("find_one_file found file with None fullpath")
            break
    m_datastore.mgdb_rdata.add_fullpath_cache(name, path, res)
    return res

def display_src_file_lines(filename, line_num):
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
            count = default_src_display_line_count if count != 0 else total << 1
    except:
        count = default_src_display_line_count

    line_num = line_num if line_num < total else total

    half = (count - 1) >> 1
    s_line = line_num - half if line_num > half + 1 else 1
    half = count - half - 1
    e_line = line_num + half if line_num + half <= total else total
    gdb_print("src file: %s%s%s line: %d" % (MColors.BT_SRC, filename, MColors.ENDC, line_num))
    if m_util.Highlighted:
        buf =m_util.shell_cmd('highlight -O xterm256 --force -s %s %s | sed -n %d,%dp |'
            ' nl -ba -s" " -w8 -v%d | sed "s/^  \\( *%d \\)/=>\\1/"' \
            % (m_util.HighlightStyle, filename, s_line, e_line, s_line, line_num))
    else:
        buf = m_util.shell_cmd('sed -n %d,%dp %s | nl -ba -s" " -w8 -v%d | sed "s/^  \\( *%d \\)/=>\\1/"' \
            % (s_line, e_line, filename, s_line, line_num))
    m_util.gdb_write(buf)

    new_line_num = line_num + count if line_num + count <= total else total
    return new_line_num

class MapleSourcePathCmd(gdb.Command):
    """add and manage the Maple source code search paths
    msp is the alias of msrcpath command
    msrcpath: display all the search paths of Maple application and library source code
    msrcpath -show: same to msrcpath command without argument
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
        line = m_util.gdb_exec_to_str('pwd')
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
        gdb_print("  msrcpath :\n"
                  "    msrcpath -show or msrcpath: Displays all the source code search paths\n"
                  "    msrcpath -add <path>:  Adds one path to the top of the list\n"
                  "    msrcpath -del <path>:  Deletes specified path from the list\n"
                  "    msrcpath -help:        Prints this message")

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
        if not os.path.exists(os.path.expandvars(os.path.expanduser(path))):
            buffer = "%s specified but not found, please verify: " % (path)
            gdb_print(buffer)
        else:
            maple_source_path_list = [os.path.expandvars(os.path.expanduser(path))] + maple_source_path_list
        return

    def del_path(self, path):
        # if path is in maple_source_path_list, delete it.
        global maple_source_path_list
        if path in maple_source_path_list:
            maple_source_path_list.remove(path)
            m_datastore.mgdb_rdata.del_fullpath_cache()
            m_datastore.mgdb_rdata.del_src_lines()
            if m_debug.Debug: m_datastore.mgdb_rdata.show_file_cache()

class MapleListCmd(gdb.Command):
    """list source code in multiple modes
    mlist: list source code associated with current Maple frame
    mlist -asm: list assembly instructions associated with current Maple frame
    mlist -mir: list Maple IR associated with current Maple frame
    """

    def __init__(self):
        gdb.Command.__init__ (self,
                              "mlist",
                              gdb.COMMAND_FILES,
                              gdb.COMPLETE_NONE)

        self.frame_change_count = 0
        self.frame_data = {}

        self.prev_src_file_line = 0
        self.prev_asm_file_line = 0
        self.prev_mir_file_line = 0

    def invoke(self, args, from_tty):
        self.mlist_func(args, from_tty)

    def usage(self):
        gdb_print("  mlist      : Lists source code associated with current Maple frame\n"
                  "  mlist -asm : Lists assembly instructions associated with current Maple frame\n"
                  "  mlist . | mlist -asm:. | mlist -mir:. : Lists code located by the filename and line number of current Maple frame\n"
                  "  mlist line-num : Lists current source code file at line of [line-num]\n"
                  "  mlist filename:line-num : Lists specified source code file at line of [line-num]\n"
                  "  mlist +|-[num]: Lists current source code file offsetting from previous listed line, offset can be + or -\n"
                  "  mlist -asm:+|-[num]: Lists current assembly instructions offsetting from previous listed line. offset can be + or -)\n"
                  "  mlist -mir : Lists Maple IR associated with current Maple frame\n"
                  "  mlist -mir:+|-[num]: Lists current Maple IR offsetting from previous listed line. offset can be + or -")

    def mlist_func(self, args, from_tty):
        s = str(args)
        if len(s) is 0: #mlist current source code file
            self.mlist_source_file_func()
            return

        mlist_mode = MLIST_MODE_SRC
        line_offset = 0
        src_line_num = 0
        src_file_name = None

        if len(s.split()) != 1:
            self.usage()
            return

        if s == "-asm":
            mlist_mode = MLIST_MODE_ASM
        elif "-asm:-" == s:
            mlist_mode = MLIST_MODE_ASM
            line_offset = default_asm_display_line_count * -1
        elif "-asm:-" == s[0:6]:
            mlist_mode = MLIST_MODE_ASM
            if s[6:].isdigit():
                line_offset = int(s[6:]) * -1
        elif "-asm:+" == s:
            mlist_mode = MLIST_MODE_ASM
            line_offset = default_asm_display_line_count
        elif "-asm:+" == s[0:6]:
            mlist_mode = MLIST_MODE_ASM
            if s[6:].isdigit():
                line_offset = int(s[6:])
            else:
                self.usage()
                return
        elif "-asm:" == s[0:5]:
            mlist_mode = MLIST_MODE_ASM
            if s[5:].isdigit():
                line_offset = int(s[5:])
            elif "." == s[5:]:
                m_datastore.mgdb_rdata.update_frame_change_counter()
            else:
                self.usage()
                return

        elif s == "-mir":
            mlist_mode = MLIST_MODE_MIR
        elif "-mir:-" == s:
            mlist_mode = MLIST_MODE_MIR
            line_offset = default_mir_display_line_count * -1
        elif "-mir:-" == s[0:6]:
            mlist_mode = MLIST_MODE_MIR
            if s[6:].isdigit():
                line_offset = int(s[6:]) * -1
        elif "-mir:+" == s:
            mlist_mode = MLIST_MODE_MIR
            line_offset = default_mir_display_line_count
        elif "-mir:+" == s[0:6]:
            mlist_mode = MLIST_MODE_MIR
            if s[6:].isdigit():
                line_offset = int(s[6:])
            else:
                self.usage()
                return
        elif "-mir:" == s[0:5]:
            mlist_mode = MLIST_MODE_MIR
            if s[5:].isdigit():
                line_offset = int(s[5:])
            elif "." == s[5:]:
                m_datastore.mgdb_rdata.update_frame_change_counter()
            else:
                self.usage()
                return

        elif "." == s:
            m_datastore.mgdb_rdata.update_frame_change_counter()
        elif s.isdigit():
            src_line_num = int(s)
        elif ':' in s:
            x = s.split(':')
            if x[1].isdigit():
                src_file_name = x[0]
                src_line_num = int(x[1])
            else:
                self.usage()
                return
        elif s[0] == '+' and s[1:].isdigit():
            line_offset = int(s[1:])
        elif s == "+":
            line_offset = default_src_display_line_count
        elif s[0] == '-' and s[1:].isdigit():
            line_offset = int(s[1:]) * -1
        elif s == "-":
            line_offset = default_src_display_line_count * -1
        else:
            self.usage()
            return

        if mlist_mode == MLIST_MODE_ASM:
            self.mlist_asm_file_func(line_offset)
        elif mlist_mode == MLIST_MODE_SRC:
            self.mlist_source_file_func(filename = src_file_name, line = src_line_num, offset = line_offset)
        else:
            self.mlist_mir_file_func(line_offset)

    def mlist_asm_file_func(self, offset):
        frame_change_count = m_datastore.mgdb_rdata.read_frame_change_counter()

        # stack selected frame changed by other cmds or breakpoints
        if self.frame_change_count < frame_change_count:
            frame = m_frame.get_selected_frame()
            if not frame:
                return

            ds = m_datastore.get_stack_frame_data(frame)
            if not ds:
                return

            self.frame_data = ds
            self.frame_change_count = frame_change_count

        if offset != 0:
            new_line = self.prev_asm_file_line + offset
            self.frame_data['frame_func_src_info']['asm_line'] = new_line if new_line > 1 else 1

        self.display_asm_file_lines(self.frame_data['frame_func_header_info']['asm_path'], \
                                    self.frame_data['frame_func_src_info']['asm_line'],\
                                    self.frame_data['frame_func_header_info']['func_header_asm_tuple'])

    def mlist_mir_file_func(self, offset):
        frame_change_count = m_datastore.mgdb_rdata.read_frame_change_counter()

        # stack selected frame changed by other cmds or breakpoints
        if self.frame_change_count < frame_change_count:
            frame = m_frame.get_selected_frame()
            if not frame:
                return

            ds = m_datastore.get_stack_frame_data(frame)
            if not ds:
                return

            self.frame_data = ds
            self.frame_change_count = frame_change_count

        if offset != 0:
            new_line = self.prev_mir_file_line + offset
            self.frame_data['frame_func_src_info']['mirmpl_line'] = new_line if new_line > 1 else 1

        self.display_mir_file_lines(self.frame_data['frame_func_header_info']['mirmpl_path'], \
                                    self.frame_data['frame_func_src_info']['mirmpl_line'],\
                                    self.frame_data['frame_func_header_info']['func_header_mirmpl_tuple'])

    def mlist_source_file_func(self, filename = None, line = 0, offset = 0):
        """
        params: line = 0 and offset = 0 allowed
                line = 0 and offset != 0 allowed
                line != 0 and offset = 0 allowed
                line != 0 and offset != 0 NOT allowed
        """
        if line != 0 and offset != 0:
            return

        frame_change_count = m_datastore.mgdb_rdata.read_frame_change_counter()

        # stack selected frame changed by other cmds or breakpoints
        if self.frame_change_count < frame_change_count:
            frame = m_frame.get_selected_frame()
            if not frame:
                return

            ds = m_datastore.get_stack_frame_data(frame)
            if not ds:
                return

            self.frame_data = ds
            self.frame_change_count = frame_change_count

        if filename:
            self.frame_data['frame_func_src_info']['short_src_file_name'] = filename
        if line != 0:
            self.frame_data['frame_func_src_info']['short_src_file_line'] = line
        if offset != 0:
            new_line = self.prev_src_file_line + offset
            self.frame_data['frame_func_src_info']['short_src_file_line'] = new_line if new_line > 1 else 1

        file_full_path = None
        for source_path in maple_source_path_list:
            file_full_path = find_one_file(self.frame_data['frame_func_src_info']['short_src_file_name'], source_path)
            if not file_full_path:
                continue
            else:
                break

        if not file_full_path:
            if self.frame_data['frame_func_src_info']['short_src_file_name']:
                gdb_print ("Warning: Source code file " + self.frame_data['frame_func_src_info']['short_src_file_name'] + " not found in any path")
            else:
                gdb_print ("Warning: Source code file not found. try 'mlist -asm' instead")
            return
        self.display_src_lines(file_full_path, self.frame_data['frame_func_src_info']['short_src_file_line'])

    def display_src_lines(self, filename, line_num):
        """
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
                count = self.default_src_display_line_count if count != 0 else total << 1
        except:
            count = self.default_src_display_line_count

        line_num = line_num if line_num < total else total

        half = (count - 1) >> 1
        s_line = line_num - half if line_num > half + 1 else 1
        half = count - half - 1
        e_line = line_num + half if line_num + half <= total else total
        gdb_print("src file: %s%s%s line: %d" % (MColors.BT_SRC, filename, MColors.ENDC, line_num))
        if m_util.Highlighted:
            buf =m_util.shell_cmd('highlight -O xterm256 --force -s %s %s | sed -n %d,%dp |'
                ' nl -ba -s" " -w8 -v%d | sed "s/^  \\( *%d \\)/=>\\1/"' \
                % (m_util.HighlightStyle, filename, s_line, e_line, s_line, line_num))
        else:
            buf = m_util.shell_cmd('sed -n %d,%dp %s | nl -ba -s" " -w8 -v%d | sed "s/^  \\( *%d \\)/=>\\1/"' \
                % (s_line, e_line, filename, s_line, line_num))
        m_util.gdb_write(buf)
        """
        new_line_num = display_src_file_lines(filename, line_num)

        self.prev_src_file_line = line_num
        self.frame_data ['frame_func_src_info']['short_src_file_line'] = new_line_num

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
                count = default_asm_display_line_count if count != 0 else total << 1
        except:
            count = default_asm_display_line_count

        if line_num < asm_tuple[0]:
            line_num = asm_tuple[0]
        if line_num > asm_tuple[0] + total:
            line_num = asm_tuple[0] + total - 1

        curr_num = line_num - asm_tuple[0] + 1
        half = (count - 1) >> 1
        s_line = curr_num - half if curr_num > half + 1 else 1
        half = count - half - 1
        e_line = curr_num + half if curr_num + half <= total else total
        gdb_print("asm file: %s%s%s line: %d" % (MColors.BT_SRC, filename, MColors.ENDC, line_num))
        ss_line = asm_tuple[0] + s_line - 1
        if m_util.Highlighted:
            buf =m_util.shell_cmd('dd skip=%d bs=1 count=%d if=%s 2> /dev/null |'
                ' highlight -O xterm256 --config-file=%s -s %s | sed -n %d,%dp |'
                ' nl -ba -s" " -w12 -v%d | sed "s/^  \\( *%d \\)/=>\\1/"' \
                % (asm_tuple[1], asm_tuple[2] - asm_tuple[1], filename, asm_lang, \
                   m_util.HighlightStyle, s_line, e_line, ss_line, line_num))
        else:
            buf =m_util.shell_cmd('dd skip=%d bs=1 count=%d if=%s 2> /dev/null |'
                ' sed -n %d,%dp | nl -ba -s" " -w12 -v%d | sed "s/^  \\( *%d \\)/=>\\1/"' \
                % (asm_tuple[1], asm_tuple[2] - asm_tuple[1], filename, \
                   s_line, e_line, ss_line, line_num))
        m_util.gdb_write(buf)

        self.prev_asm_file_line = line_num
        self.frame_data['frame_func_src_info']['asm_line'] = line_num + count if line_num + count < asm_tuple[0] + total else asm_tuple[0] + total - 1

    def display_mir_file_lines(self, filename, line_num, mir_tuple):
        """
        params:
          filename: string. asm file full path
          line_num: int. line number of this asm file
          mir_tuple: (maple_symbol_block_start_line, block_start_offset, block_end_offset) in mir.mpl file
        """
        exist = m_datastore.mgdb_rdata.in_mirblock_lines(filename, mir_tuple[0])
        if exist:
            total = m_datastore.mgdb_rdata.get_mirblock_lines(filename, mir_tuple[0])
        else:
            with open(filename) as fp:
                offset = mir_tuple[1]
                fp.seek(offset)
                total = 0
                while offset < mir_tuple[2]:
                    line = fp.readline()
                    offset += len(line)
                    total += 1
            m_datastore.mgdb_rdata.add_mirblock_lines(filename, mir_tuple[0], total)
        try:
            count = int(m_set.msettings["linecount"])
            if count < 9:
                count = default_mir_display_line_count if count != 0 else total << 1
        except:
            count = default_mir_display_line_count

        if line_num < mir_tuple[0]:
            line_num = mir_tuple[0]
        if line_num > mir_tuple[0] + total:
            line_num = mir_tuple[0] + total - 1

        curr_num = line_num - mir_tuple[0] + 1
        half = (count - 1) >> 1
        s_line = curr_num - half if curr_num > half + 1 else 1
        half = count - half - 1
        e_line = curr_num + half if curr_num + half <= total else total
        gdb_print("asm file: %s%s%s line: %d" % (MColors.BT_SRC, filename, MColors.ENDC, line_num))
        ss_line = mir_tuple[0] + s_line - 1
        if m_util.Highlighted:
            buf =m_util.shell_cmd('dd skip=%d bs=1 count=%d if=%s 2> /dev/null |'
                ' highlight -O xterm256 --config-file=%s -s %s | sed -n %d,%dp |'
                ' nl -ba -s" " -w12 -v%d | sed "s/^  \\( *%d \\)/=>\\1/"' \
                % (mir_tuple[1], mir_tuple[2] - mir_tuple[1], filename, mpl_lang, \
                   m_util.HighlightStyle, s_line, e_line, ss_line, line_num))
        else:
            buf =m_util.shell_cmd('dd skip=%d bs=1 count=%d if=%s 2> /dev/null |'
                ' sed -n %d,%dp | nl -ba -s" " -w12 -v%d | sed "s/^  \\( *%d \\)/=>\\1/"' \
                % (mir_tuple[1], mir_tuple[2] - mir_tuple[1], filename, \
                   s_line, e_line, ss_line, line_num))
        m_util.gdb_write(buf)

        self.prev_mir_file_line = line_num
        self.frame_data['frame_func_src_info']['mirmpl_line'] = line_num + count if line_num + count < mir_tuple[0] + total else mir_tuple[0] + total - 1

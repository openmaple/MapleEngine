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

# ---------------------------------------------------------------------
# Be sure to add the python path that points to the LLDB shared library.
#
# # To use this in the embedded python interpreter using "lldb" just
# import it with the full path using the "command script import"
# command
#   (lldb) command script import /path/to/cmdtemplate.py
# ---------------------------------------------------------------------

from __future__ import print_function

import inspect
import lldb
import traceback
import optparse
import shlex
import sys
import os
import re
import m_config_lldb

from mlldb import m_frame
from mdata import m_datastore
from mlldb import m_set
from shared import m_util
from shared.m_util import mdb_print
from shared.m_util import MColors
from mlldb import m_config_lldb as m_debug

maple_source_path_list = []

default_src_display_line_count = 10
default_asm_display_line_count = 20
default_mir_display_line_count = 20

asm_lang = os.path.expandvars('$MAPLE_DEBUGGER_ROOT/highlight/asm.lang')
mpl_lang = os.path.expandvars('$MAPLE_DEBUGGER_ROOT/highlight/mpl.lang')

MLIST_MODE_SRC = 0
MLIST_MODE_ASM = 1
MLIST_MODE_MIR = 2

maple_source_path_list = ["."]

#def find_one_file(src_file_short_name, source_path):
    #print("Finding the file:", src_file_short_name, "in path:", source_path)
#    return ("./"+src_file_short_name)

def find_one_file(name, path):
    """ find the file with specified name and given base path """
    if not name:
        return None
    exist = m_datastore.mlldb_rdata.in_fullpath_cache(name, path)
    if exist:
        return m_datastore.mlldb_rdata.get_fullpath_cache(name, path)
    res = None
    for root, dirs, files in os.walk(path):
        if name in files:
            res = os.path.join(root, name)
            if not res:
                if m_debug.Debug: m_debug.dbg_print("find_one_file found file with None fullpath")
            break
    m_datastore.mlldb_rdata.add_fullpath_cache(name, path, res)
    if res == None:
        return(".../"+name)
    return res

def display_src_file_lines(filename, line_num):
    exist = m_datastore.mlldb_rdata.in_src_lines(filename)
    if exist:
        total = m_datastore.mlldb_rdata.get_src_lines(filename)
    else:
        with open (filename) as fp:
            total = len(fp.readlines())
        m_datastore.mlldb_rdata.add_src_lines(filename, total)
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
    mdb_print("src file: %s%s%s line: %d" % (MColors.BT_SRC, filename, MColors.ENDC, line_num))
    if m_util.Highlighted:
        buf =m_util.shell_cmd('highlight -O xterm256 --force -s %s %s | sed -n %d,%dp |'
            ' nl -ba -s" " -w8 -v%d | sed "s/^  \\( *%d \\)/=>\\1/"' \
            % (m_util.HighlightStyle, filename, s_line, e_line, s_line, line_num))
    else:
        buf = m_util.shell_cmd('sed -n %d,%dp %s | nl -ba -s" " -w8 -v%d | sed "s/^  \\( *%d \\)/=>\\1/"' \
            % (s_line, e_line, filename, s_line, line_num))
    m_util.mdb_write(buf)

    new_line_num = line_num + count if line_num + count <= total else total
    return new_line_num


class MListCommand:
    program = 'mlist'

    frame_change_count = 0
    frame_data = {}
    prev_src_file_line = 0
    prev_asm_file_line = 0
    prev_mir_file_line = 0

    @classmethod
    def register_lldb_command(cls, debugger, module_name):
        parser = cls.create_options()
        cls.__doc__ = parser.format_help()
        # Add any commands contained in this module to LLDB
        command = 'command script add -c %s.%s %s' % (module_name,
                                                      cls.__name__,
                                                      cls.program)
        debugger.HandleCommand(command)
        if m_config_lldb.debug:
            print('The "{0}" command has been installed, type "help {0}" or "{0} '
                  '--help" for detailed help.'.format(cls.program))

    @classmethod
    def create_options(cls):

        usage = "usage: %prog [options] arg"
        description = ('Basically:  mlist : Lists source code associated with current Maple frame in multiple modes')

        """list source code in multiple modes
        mlist: list source code associated with current Maple frame
        mlist --asm: list assemble instructions associated with current Maple frame
        mlist --mir: list Maple IR associated with current Maple frame
        mlist --line-num : Lists current source code file at line of [line-num]
        """

        # Pass add_help_option = False, since this keeps the command in line
        #  with lldb commands, and we wire up "help command" to work by
        # providing the long & short help methods below.
        parser = optparse.OptionParser(
            description=description,
            prog=cls.program,
            usage=usage,
            add_help_option=False)

        parser.add_option(
            '-a',
            '--asm',
            action='store_true',
            dest='mlist_asm',
            help=': Lists assembly instructions associated with current Maple frame',
            default=False)

        parser.add_option(
            '-m',
            '--mir',
            action='store_true',
            dest='mlist_mir',
            help=': Lists Maple IR associated with current Maple frame',
            default=False)

        parser.add_option(
            '-l',
            '--line-num',
            action='store_true',
            dest='mlist_line',
            help=': Lists current source code file at line of [line-num]',
            default=False)

        parser.add_option(
            '-p',
            '--path',
            action='store_true',
            dest='mlist_path',
            help=': Lists code located by the filename and line number of current Maple frame',
            default=False)

        parser.add_option(
            '-f',
            '--file',
            action='store_true',
            dest='mlist_file',
            help='Lists specified source code file at line of [line-num]',
            default=False)

        parser.add_option(
            '-h',
            '--help',
            action='store_true',
            dest='mlist_help',
            help=description,
            default=False)

        return parser

    def get_short_help(self):
        return "Lists source code associated with current Maple frame"

    def get_long_help(self):
        return self.help_string

    @classmethod
    def __init__(self, debugger, unused):
        self.parser = self.create_options()
        self.help_string = self.parser.format_help()
        self.frame_change_count = 0
        self.frame_data = {}
        self.prev_src_file_line = 0
        self.prev_asm_file_line = 0
        self.prev_mir_file_line = 0

    def __call__(self, debugger, command, exe_ctx, result):
        # Use the Shell Lexer to properly parse up command options just like a
        # shell would
        command_args = shlex.split(command)
        run_it = False
        ml_file = None
        ml_path = None
        ml_format = MLIST_MODE_SRC
        ml_line = 0

        try:
            (options, args) = self.parser.parse_args(command_args)
            if m_debug.Debug:
                print(options)
                print(args)
            if options.mlist_help:
                print("mlist : Lists source code associated with current Maple frame\n"
                      "    mlist -asm : Lists assembly instructions associated with current Maple frame\n"
                      "    mlist . | mlist -asm:. : Lists code located by the filename and line number of current Maple frame\n"
                      "    mlist line-num : Lists current source code file at line of [line-num]\n"
                      "    mlist filename:line-num : Lists specified source code file at line of [line-num]\n"
                      "    mlist +|-[num]: Lists current source code file offsetting from previous listed line, offset can be + or -\n"
                      "    mlist -asm:+|-[num]: Lists current assembly instructions offsetting from previous listed line. offset can be + or -")
            elif options.mlist_file:
                run_it = True
                ml_file = True

            elif options.mlist_path:
                run_it = True
                ml_path = True

            elif options.mlist_line:
                run_it = True
                ml_line = True

            elif options.mlist_asm:
                run_it = True
                ml_format = MLIST_MODE_ASM

            elif options.mlist_mir:
                run_it = True
                ml_format = MLIST_MODE_MIR

            if run_it:
                self.do_list(ml_file,ml_path,ml_format,ml_line)
            else:
                ml_format = MLIST_MODE_SRC
                self.do_list(ml_file,ml_path,ml_format,ml_line)

        except Exception:
            # if you don't handle exceptions, passing an incorrect argument to
            # the OptionParser will cause LLDB to exit (courtesy of OptParse
            # dealing with argument errors by throwing SystemExit)
            result.SetError("option parsing failed")
            print("-"*60)
            print("Exception caught in mlist code:")
            traceback.print_stack(file=sys.stdout)
            print("-"*60)
            traceback.print_exc(file=sys.stdout)
            print("-"*60)
            return
        # not returning anything is akin to returning success

    def do_list(self, ml_file, ml_path, ml_format,ml_line):
        line_offset = 1
        src_file_name = ml_file
        src_line_num = ml_line

        mdb_print("Listing")
        MColors.init_maple_colors()
        mlist_mode = ml_format
        if mlist_mode == MLIST_MODE_ASM:
            self.mlist_asm_file_func(line_offset)
        elif mlist_mode == MLIST_MODE_SRC:
            self.mlist_source_file_func(filename = src_file_name, line = src_line_num, offset = line_offset)
        else:
            self.mlist_mir_file_func(line_offset)

    def mlist_asm_file_func(self, offset):
        frame_change_count = m_datastore.mlldb_rdata.read_frame_change_counter()

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
        frame_change_count = m_datastore.mlldb_rdata.read_frame_change_counter()

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

        frame_change_count = m_datastore.mlldb_rdata.read_frame_change_counter()

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
                mdb_print ("Warning: Source code file " + self.frame_data['frame_func_src_info']['short_src_file_name'] + " not found in any path")
            else:
                mdb_print ("Warning: Source code file not found. try 'mlist -asm' instead")
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
        exist = m_datastore.mlldb_rdata.in_asmblock_lines(filename, asm_tuple[0])
        if exist:
            total = m_datastore.mlldb_rdata.get_asmblock_lines(filename, asm_tuple[0])
        else:
            with open(filename) as fp:
                offset = asm_tuple[1]
                fp.seek(offset)
                total = 0
                while offset < asm_tuple[2]:
                    line = fp.readline()
                    offset += len(line)
                    total += 1
            m_datastore.mlldb_rdata.add_asmblock_lines(filename, asm_tuple[0], total)
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
        mdb_print("asm file: %s%s%s line: %d" % (MColors.BT_SRC, filename, MColors.ENDC, line_num))
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
        #buf = buf.replace(".long",MColors.magenta(".long"))
        #buf = buf.replace(".byte",MColors.magenta(".byte"))
        #buf = buf.replace(".word",MColors.magenta(".word"))
        #buf = re.sub(r"\/\/", MColors.cyan("//"), buf,flags=re.MULTILINE)
        buf = re.sub(r'\/\/\ ((%|\w+).*)', MColors.cyan('// \\1').strip(), buf, flags=re.MULTILINE)
        buf = re.sub(r'\.(\w+)', MColors.magenta('.\\1').strip(), buf, flags=re.MULTILINE)

        m_util.mdb_write(buf)

        self.prev_asm_file_line = line_num
        self.frame_data['frame_func_src_info']['asm_line'] = line_num + count if line_num + count < asm_tuple[0] + total else asm_tuple[0] + total - 1

    def display_mir_file_lines(self, filename, line_num, mir_tuple):
        """
        params:
          filename: string. asm file full path
          line_num: int. line number of this asm file
          mir_tuple: (maple_symbol_block_start_line, block_start_offset, block_end_offset) in mir.mpl file
        """
        exist = m_datastore.mlldb_rdata.in_mirblock_lines(filename, mir_tuple[0])
        if exist:
            total = m_datastore.mlldb_rdata.get_mirblock_lines(filename, mir_tuple[0])
        else:
            with open(filename) as fp:
                offset = mir_tuple[1]
                fp.seek(offset)
                total = 0
                while offset < mir_tuple[2]:
                    line = fp.readline()
                    offset += len(line)
                    total += 1
            m_datastore.mlldb_rdata.add_mirblock_lines(filename, mir_tuple[0], total)
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
        mdb_print("mir file: %s%s%s line: %d" % (MColors.BT_SRC, filename, MColors.ENDC, line_num))
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

        buf = re.sub(r"\#(\w+.*)", MColors.lblue('#\\1').strip(), buf, flags=re.MULTILINE)
        buf = re.sub(r'\$(\w+)', MColors.lyellow('$\\1').strip(), buf, flags=re.MULTILINE)
        buf = re.sub(r'\&(\w+\s)', MColors.lmagenta('&\\1').strip(), buf, flags=re.MULTILINE)
        m_util.mdb_write(buf)

        self.prev_mir_file_line = line_num
        self.frame_data['frame_func_src_info']['mirmpl_line'] = line_num + count if line_num + count < mir_tuple[0] + total else mir_tuple[0] + total - 1


class MSrcPathCommand:
    program = 'msrcpath'

    @classmethod
    def register_lldb_command(cls, debugger, module_name):
        parser = cls.create_options()
        cls.__doc__ = parser.format_help()
        # Add any commands contained in this module to LLDB
        command = 'command script add -c %s.%s %s' % (module_name,
                                                      cls.__name__,
                                                      cls.program)
        debugger.HandleCommand(command)
        if m_config_lldb.debug:
            print('The "{0}" command has been installed, type "help {0}" or "{0} '
                  '--help" for detailed help.'.format(cls.program))

        global maple_source_path_list

        # add current working directory at the top
        line = m_util.mdb_exec_to_str('pwd')
        if line:
            s = line.split()
            line = s[2][:-1] if len(s) is 3 else None
            if line:
                maple_source_path_list.append(os.path.expandvars(os.path.expanduser(line)))
        '''
        if len(maple_source_path_list) == 1:
            mdb_print("Maple application source code search path only has one path as following:")
            mdb_print("  ", maple_source_path_list[0])
            mdb_print("Please add more search paths using msrcpath -add <path> command")
        '''

        #m_util.gdb_exec('alias msp = msrcpath')
        command = 'command alias msp msrcpath'
        debugger.HandleCommand(command)


    @classmethod
    def create_options(cls):

        usage = "usage: %prog [options] arg"
        description = ('Basically:  msrcpath <options> arg: Displays and manages all Maple source code search paths')

        """add and manage the Maple source code search paths
        msp is the alias of msrcpath command
        msrcpath: display all the search paths of Maple application and library source code
        msrcpath -show: same to msrcpath command without argument
        msrcpath -add <path>:  add one path to the top of the list
        msrcpath -del <path>:  delete specified path from the list
        """

        # Pass add_help_option = False, since this keeps the command in line
        #  with lldb commands, and we wire up "help command" to work by
        # providing the long & short help methods below.
        parser = optparse.OptionParser(
            description=description,
            prog=cls.program,
            usage=usage,
            add_help_option=False)

        parser.add_option(
            '-s',
            '--show',
            action='store_true',
            dest='show_path',
            help='or msrcpath: Displays all the source code search paths',
            default=False)

        parser.add_option(
            '-a',
            '--add',
            action='store_true',
            dest='add_path',
            help='<path>:  Adds one path to the top of the list',
            default=False)

        parser.add_option(
            '-d',
            '--del',
            action='store_true',
            dest='del_path',
            help='<path>:  Deletes specified path from the list',
            default=False)

        parser.add_option(
            '-h',
            '--help',
            action='store_true',
            dest='help_msrcpath',
            help=description,
            default=False)

        return parser

    def get_short_help(self):
        return "Displays and manages all Maple source code search paths"

    def get_long_help(self):
        return self.help_string

    def __init__(self, debugger, unused):
        self.parser = self.create_options()
        self.help_string = self.parser.format_help()

    def __call__(self, debugger, command, exe_ctx, result):
        # Use the Shell Lexer to properly parse up command options just like a
        # shell would
        command_args = shlex.split(command)

        try:
            (options, args) = self.parser.parse_args(command_args)
            print(options)
            print(args)
            if options.msrcpath_help:
                print("msrcpath : Displays and manages all Maple source code search paths\n"
                      "    msrcpath -show or msrcpath: Displays all the source code search paths\n"
                      "    msrcpath -add <path>:  Adds one path to the top of the list\n"
                      "    msrcpath -del <path>:  Deletes specified path from the list\n"
                      "    msrcpath -help:        Prints this message")
            else:
                print("Executing command")

        except Exception:
            # if you don't handle exceptions, passing an incorrect argument to
            # the OptionParser will cause LLDB to exit (courtesy of OptParse
            # dealing with argument errors by throwing SystemExit)
            result.SetError("option parsing failed")
            print("-"*60)
            print("Exception caught in msrcpath code:")
            traceback.print_stack(file=sys.stdout)
            print("-"*60)
            traceback.print_exc(file=sys.stdout)
            print("-"*60)
            return
        # not returning anything is akin to returning success

    def show_path(self):
        mdb_print ("Maple source path list: --")
        if len(maple_source_path_list) is 0:
            mdb_print ("none")
            return
        for path in maple_source_path_list:
            mdb_print (path)
        if m_debug.Debug: m_datastore.mlldb_rdata.show_file_cache()

    def add_path(self, path):
        # we add path into the top of the list
        global maple_source_path_list
        if path in maple_source_path_list:
            maple_source_path_list.remove(path)
        if not os.path.exists(os.path.expandvars(os.path.expanduser(path))):
            buffer = "%s specified but not found, please verify: " % (path)
            mdb_print(buffer)
        else:
            maple_source_path_list = [os.path.expandvars(os.path.expanduser(path))] + maple_source_path_list
        return

    def del_path(self, path):
        # if path is in maple_source_path_list, delete it.
        global maple_source_path_list
        if path in maple_source_path_list:
            maple_source_path_list.remove(path)
            m_datastore.mlldb_rdata.del_fullpath_cache()
            m_datastore.mlldb_rdata.del_src_lines()
            if m_debug.Debug: m_datastore.mlldb_rdata.show_file_cache()


def __lldb_init_module(debugger, dict):
    # Register all classes that have a register_lldb_command method
    for _name, cls in inspect.getmembers(sys.modules[__name__]):
        if inspect.isclass(cls) and callable(getattr(cls,
                                                     "register_lldb_command",
                                                     None)):
            cls.register_lldb_command(debugger, __name__)



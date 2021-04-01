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

from __future__ import print_function

import inspect
import lldb
import traceback
import optparse
import shlex
import sys
from shared.m_util import mdb_print
import m_config_lldb as m_debug
from shared.m_util import MColors
from shared.m_util import mdb_print
from shared import m_util
from mlldb import m_list
from mdata import m_datastore
from mlldb import m_info
from mlldb import m_frame

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
        #mdb_print('#%i no info' % (index))
        data = frame.register["rdi"].value
        #data = frame.register["rdi"].Dereference().GetValue()
        if m_debug.Debug: m_debug.dbg_print("RDI: ", data)
        ivar = frame.FindVariable("mir_header")
        if m_debug.Debug: m_debug.dbg_print("Variable: ", ivar)

        #for ivar in lldb.frame.variables:
            #print("Variable: ", ivar)

        mdb_print(('#%i  %s' % (index, data)).replace("\n  * frame", ""))
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
    MColors.init_maple_colors()
    for i in range(arg_num):
        arg_value = m_info.get_maple_caller_argument_value(i, arg_num, func_argus_locals['formals_type'][i])
        if m_debug.Debug: m_debug.dbg_print("arg_num=", arg_num, " arg_value=", arg_value)
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
        buffer = '#%2i %s:%s %s(%s) at %s:%s' % \
           (index, MColors.bt_addr( so_path.split('/')[-1]), MColors.bt_addr(func_addr_offset),\
             m_util.color_symbol(MColors.BT_FNNAME, func_name), args_buffer, MColors.bt_src(asm_path.strip()), asm_line_num)
    elif mbt_format == MBT_FORMAT_SRC:
        buffer = '#%2i %s:%s %s(%s) at %s:%s' % \
           (index, MColors.bt_addr(so_path.split('/')[-1]), MColors.bt_addr(func_addr_offset),\
            m_util.color_symbol(MColors.BT_FNNAME, func_name), args_buffer, MColors.bt_src(file_full_path.strip()), src_file_line)
    else:
        buffer = '#%2i %s:%s %s(%s) at %s:%s' % \
           (index, MColors.bt_addr(so_path.split('/')[-1]),MColors.bt_addr(func_addr_offset),\
             m_util.color_symbol(MColors.BT_FNNAME, func_name), args_buffer, MColors.bt_src(mirmpl_path.strip()), mirmpl_line_num)
    #mdb_print(buffer)
    mdb_print(buffer.replace("\n    frame", ""))
    #mdb_print(buffer.replace("\n  * frame", ""))


def print_mdb_frame(frame, index):
    """ print one mdb native backtrace frame """
    hpc = hex(frame.GetPC()).strip()
    hpc = MColors.italic(MColors.yellow(hpc).strip()).strip()
    funct = str(frame.GetFunctionName()).strip()
    module = str(frame.GetModule()).strip().replace("(x86_64) ","")
    buf=str(frame.GetLineEntry())
    if(buf == ':4294967295'): #if the file is not available
        buf=""
        funct = (MColors.lblue("in")+funct+MColors.lblue(" from")).strip()
    else:
        module = ""
        funct = (MColors.lblue("in")+funct+MColors.lgreen(" at")).strip()
    #print("#"+str(index),hex(frame.GetPC()), "in", frame.GetFunctionName(), "from", frame.GetModule(),buf)
    mdb_print(('#%2i%s %s%s %s' % (index, hpc, funct, module, buf)))

    #s   = m_util.mdb_exec_to_str('bt')
    #bt  = s.split('#' + str(index))
    #if(index == 0):
    #    mdb_print(bt[0].replace("\n   * frame", ""))
    #bt  = bt[1].split('#')[0][:-1]
    ##bt  = '#' + str(index) + ' ' + bt
    #
    #mdb_print(('#%i  %s' % (index, bt)).replace("\n    frame", ""))
    #mdb_print(bt)
    #mdb_print(bt.replace("\n    frame", ""))


class MBackTraceCommand:
    program = 'mbacktrace'

    @classmethod
    def register_lldb_command(cls, debugger, module_name):
        parser = cls.create_options()
        cls.__doc__ = parser.format_help()
        # Add any commands contained in this module to LLDB
        command = 'command script add -c %s.%s %s' % (module_name,
                                                      cls.__name__,
                                                      cls.program)
        debugger.HandleCommand(command)

        if m_debug.debug:
            print('The "{0}" command has been installed, type "help {0}" or "{0} '
                  '--help" for detailed help.'.format(cls.program))

        command = 'command alias mbt mbacktrace'
        debugger.HandleCommand(command)

    @classmethod
    def create_options(cls):

        usage = "usage: %prog [options] "
        description = ('Basically:  mbacktrace : displays Maple backtrace in multiple modes')

        """displays Maple backtrace in multiple modes
        mbt is the alias of mbacktrace command
        mbacktrace: prints backtrace of Maple frames
        mbacktrace -asm: prints backtrace of Maple frames in assembly format
        mbacktrace -full: prints backtrace of mixed mdb native frames and Maple frames
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
            dest='backtrace_asm',
            help=description,
            default=False)

        parser.add_option(
            '-m',
            '--mir',
            action='store_true',
            dest='backtrace_mir',
            help=description,
            default=False)

        parser.add_option(
            '-f',
            '--full',
            action='store_true',
            dest='backtrace_full',
            help=description,
            default=False)

        parser.add_option(
            '-h',
            '--help',
            action='store_true',
            dest='backtrace_help',
            help=description,
            default=False)

        return parser

    def get_short_help(self):
        return "Displays full or partial Maple backtrace information as assembly, maple and native"

    def get_long_help(self):
        return self.help_string

    def __init__(self, debugger, unused):
        self.parser = self.create_options()
        self.help_string = self.parser.format_help()

    def __call__(self, debugger, command, exe_ctx, result):
        # Use the Shell Lexer to properly parse up command options just like a
        # shell would
        command_args = shlex.split(command)
        full = False
        run_it = True
        mbt_format = MBT_FORMAT_SRC

        try:
            (options, args) = self.parser.parse_args(command_args)

            if m_debug.debug:
                print(options)
                print(args)

            if options.backtrace_help:
                run_it = False
                print("Displays full or partial Maple backtrace information as assembly, maple and native\n"
                      "    mbt is the alias of mbacktrace command \n"
                      "    mbacktrace: prints backtrace of Maple frames \n"
                      "    mbacktrace -asm: prints backtrace of Maple frames in assembly format \n"
                      "    mbacktrace -full: prints backtrace of mixed mdb native frames and Maple frames ")

            elif options.backtrace_full:
                run_it = True
                full = True

            elif options.backtrace_asm:
                run_it = True
                mbt_format = MBT_FORMAT_ASM

            elif options.backtrace_mir:
                run_it = True
                mbt_format = MBT_FORMAT_MIR

            if run_it:
                self.do_backtrace(full,mbt_format)

        except Exception:
            # if you don't handle exceptions, passing an incorrect argument to
            # the OptionParser will cause LLDB to exit (courtesy of OptParse
            # dealing with argument errors by throwing SystemExit)
            result.SetError("option parsing failed")
            print("-"*60)
            print("Exception caught in mbacktrace code:")
            traceback.print_stack(file=sys.stdout)
            print("-"*60)
            traceback.print_exc(file=sys.stdout)
            print("-"*60)
            return
        # not returning anything is akin to returning success

    def do_backtrace(self, full, mbt_format):
        selected_frame = m_frame.get_selected_frame()
        newest_frame = m_frame.get_newest_frame()
        if not selected_frame or not newest_frame:
            mdb_print('Unable to locate Maple frame')
            return

        # walk through from innermost frame to selected frame
        index = 0
        frame = newest_frame
        while frame != selected_frame and frame:
            print("Frame is:",frame)
            frame = m_frame.get_next_older_frame(frame)
            index += 1

        if not frame:
            mdb_print('No valid frames found')
            return

        start_level = index
        mdb_print('Maple Traceback (most recent call first):')

        while frame:
            if m_frame.is_maple_frame(frame):
                print_maple_frame(frame, index, mbt_format)
            elif full:
                print_mdb_frame(frame, index)
            #else:
            #    print("not maple frame")

            index += 1
            frame = m_frame.get_next_older_frame(frame)
            #if m_debug.Debug: m_debug.dbg_print("frame=", frame, " index=", index)

            if frame:
                m_util.mdb_exec_to_null('up')
                #m_util.mdb_exec_to_null('frame select'+str(index))

        # move frame back to the original stack level
        #m_util.mdb_exec_to_null('down-silently ' + str(index - start_level - 1))
        m_util.mdb_exec_to_null('down ' + str(index - start_level - 1))


class MUpCommand:
    program = 'mup'

    @classmethod
    def register_lldb_command(cls, debugger, module_name):
        parser = cls.create_options()
        cls.__doc__ = parser.format_help()
        # Add any commands contained in this module to LLDB
        command = 'command script add -c %s.%s %s' % (module_name,
                                                      cls.__name__,
                                                      cls.program)
        debugger.HandleCommand(command)
        if m_debug.debug:
            print('The "{0}" command has been installed, type "help {0}" or "{0} '
                  '--help" for detailed help.'.format(cls.program))

    @classmethod
    def create_options(cls):

        usage = "usage: %prog [options] arg"
        description = ('Basically:  mup : Selects and prints Maple stack frame that called this one')

        # Pass add_help_option = False, since this keeps the command in line
        #  with lldb commands, and we wire up "help command" to work by
        # providing the long & short help methods below.
        parser = optparse.OptionParser(
            description=description,
            prog=cls.program,
            usage=usage,
            add_help_option=False)

        """selects and prints Maple stack frame that called this one
        mup: moves up one Maple frame to caller of selected Maple frame
        mup -n: moves up n Maple frames from currently selected Maple frame
        """

        parser.add_option(
            '-n',
            '--nlevels',
            action='store_true',
            dest='ignore',
            help=': moves up n Maple frames from currently selected Maple frame',
            default=False)

        parser.add_option(
            '-h',
            '--help',
            action='store_true',
            dest='mup_help',
            help=description,
            default=False)

        return parser

    def get_short_help(self):
        return "Selects and prints the calling Maple stack frame of current"

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
            if options.mup_help:
                print("mup : Moves Maple frame up one level")
                print("mup -n: Moves Maple frame up n levels")
            else:
                print("Executing command")

        except Exception:
            # if you don't handle exceptions, passing an incorrect argument to
            # the OptionParser will cause LLDB to exit (courtesy of OptParse
            # dealing with argument errors by throwing SystemExit)
            result.SetError("option parsing failed")
            print("-"*60)
            print("Exception caught in mup code:")
            traceback.print_stack(file=sys.stdout)
            print("-"*60)
            traceback.print_exc(file=sys.stdout)
            print("-"*60)
            return
        # not returning anything is akin to returning success

class MDownCommand:
    program = 'mdown'

    @classmethod
    def register_lldb_command(cls, debugger, module_name):
        parser = cls.create_options()
        cls.__doc__ = parser.format_help()
        # Add any commands contained in this module to LLDB
        command = 'command script add -c %s.%s %s' % (module_name,
                                                      cls.__name__,
                                                      cls.program)
        debugger.HandleCommand(command)
        if m_debug.debug:
            print('The "{0}" command has been installed, type "help {0}" or "{0} '
                  '--help" for detailed help.'.format(cls.program))

    @classmethod
    def create_options(cls):

        usage = "usage: %prog [options] arg"
        description = ('Basically:  mup : Selects and prints Maple stack frame called by this one')

        # Pass add_help_option = False, since this keeps the command in line
        #  with lldb commands, and we wire up "help command" to work by
        # providing the long & short help methods below.
        parser = optparse.OptionParser(
            description=description,
            prog=cls.program,
            usage=usage,
            add_help_option=False)

        """selects and prints Maple stack frame called by this one
        mdown: moves down one Maple frame called by selected Maple frame
        mdown -n: moves down n Maple frames called from currently selected Maple frame
        """

        parser.add_option(
            '-n',
            '--nlevels',
            action='store_true',
            dest='ignore',
            help=': moves down n Maple frames called from currently selected Maple frame',
            default=False)

        parser.add_option(
            '-h',
            '--help',
            action='store_true',
            dest='mdown_help',
            help=description,
            default=False)

        return parser

    def get_short_help(self):
        return "Selects and prints the called Maple stack frame from current"

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
            if options.mdown_help:
                print("mdown : Moves Maple frame down one level")
                print("mdown -n: Moves Maple frame down n levels")
            else:
                print("Executing command")

        except Exception:
            # if you don't handle exceptions, passing an incorrect argument to
            # the OptionParser will cause LLDB to exit (courtesy of OptParse
            # dealing with argument errors by throwing SystemExit)
            result.SetError("option parsing failed")
            print("-"*60)
            print("Exception caught in mdown code:")
            traceback.print_stack(file=sys.stdout)
            print("-"*60)
            traceback.print_exc(file=sys.stdout)
            print("-"*60)
            return
        # not returning anything is akin to returning success

class MLocalCommand:
    program = 'mlocal'

    @classmethod
    def register_lldb_command(cls, debugger, module_name):
        parser = cls.create_options()
        cls.__doc__ = parser.format_help()
        # Add any commands contained in this module to LLDB
        command = 'command script add -c %s.%s %s' % (module_name,
                                                      cls.__name__,
                                                      cls.program)
        debugger.HandleCommand(command)
        if m_debug.debug:
            print('The "{0}" command has been installed, type "help {0}" or "{0} '
                  '--help" for detailed help.'.format(cls.program))

    @classmethod
    def create_options(cls):

        usage = "usage: %prog [options] arg"
        description = ('Basically:  mlocal : Displays selected Maple frame arguments, local variables and stack dynamic data')

        # Pass add_help_option = False, since this keeps the command in line
        #  with lldb commands, and we wire up "help command" to work by
        # providing the long & short help methods below.
        parser = optparse.OptionParser(
            description=description,
            prog=cls.program,
            usage=usage,
            add_help_option=False)

        """displays selected Maple frame arguments, local variables and stack dynamic data
        mlocal: displays function local variabls of currently selected Maple frame
        mlocal [-s|-stack]: displays runtime operand stack changes of current selected Maple frame
        """

        parser.add_option(
            '-s',
            '--stack',
            action='store_true',
            dest='ignore',
            help='[-s|-stack]: displays runtime operand stack changes of current selected Maple frame',
            default=False)

        parser.add_option(
            '-h',
            '--help',
            action='store_true',
            dest='mlocal_help',
            help=description,
            default=False)

        return parser

    def get_short_help(self):
        return "Displays selected Maple frame arguments, local variables and stack dynamic data"

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
            if options.mlocal_help:
                print("mlocal: Displays function local variabls of currently selected Maple frame"
                    "    mlocal [-s|-stack]: Displays runtime operand stack changes of currently selected Maple frame")
            else:
                print("Executing command")

        except Exception:
            # if you don't handle exceptions, passing an incorrect argument to
            # the OptionParser will cause LLDB to exit (courtesy of OptParse
            # dealing with argument errors by throwing SystemExit)
            result.SetError("option parsing failed")
            print("-"*60)
            print("Exception caught in mlocal code:")
            traceback.print_stack(file=sys.stdout)
            print("-"*60)
            traceback.print_exc(file=sys.stdout)
            print("-"*60)
            return
        # not returning anything is akin to returning success


def __lldb_init_module(debugger, dict):
    # Register all classes that have a register_lldb_command method
    for _name, cls in inspect.getmembers(sys.modules[__name__]):
        if inspect.isclass(cls) and callable(getattr(cls,
                                                     "register_lldb_command",
                                                     None)):
            cls.register_lldb_command(debugger, __name__)



#
# Copyright (C) [2020-2021] Futurewei Technologies, Inc. All rights reverved.
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
import optparse
import shlex
import sys
import m_config_lldb


class MStepiCommand:
    program = 'mstepi'

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
        command = 'command alias msi mstepi'
        debugger.HandleCommand(command)

    @classmethod
    def create_options(cls):

        usage = "usage: %prog [options] arg"
        description = ('Basically:  mstepi [n]: Steps a specified number Maple instruction')

        """steps specified number of Maple instructions
        msi is an alias of mstepi command
        mstepi: steps in next Maple instruction
        mstepi [n]: steps in to next nth Maple instruction
        mstepi -abs [n]: steps in specified index of Maple instruction
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
            '--abs',
            action='store_true',
            dest='maple_stepi',
            help='[n]: steps in specified index of Maple instruction',
            default=False)

        parser.add_option(
            '-c',
            '--count',
            action='store_true',
            dest='count_stepi',
            help=': Shows current Maple opcode count',
            default=False)

        parser.add_option(
            '-h',
            '--help',
            action='store_true',
            dest='stepi_help',
            help=description,
            default=False)

        return parser

    def get_short_help(self):
        return "Steps one or a specified Maple instructions "

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
            if options.stepi_help:
                print("mstepi : Steps into specified index n of Maple instruction\n"
                    "    msi is an alias of mstepi command\n"
                    "    mstepi : Steps into next Maple instruction\n"
                    "    mstepi [n]: Steps into next n Maple instructions\n"
                    "    mstepi -abs [n]: Steps into specified index n of Maple instruction\n"
                    "    mstepi -c: Shows current Maple opcode count\n"
                    "    mstepi --help: Displays this message")
            else:
                print("Executing command")

        except Exception:
            # if you don't handle exceptions, passing an incorrect argument to
            # the OptionParser will cause LLDB to exit (courtesy of OptParse
            # dealing with argument errors by throwing SystemExit)
            result.SetError("option parsing failed")
            print("-"*60)
            print("Exception caught in mstepi code:")
            traceback.print_stack(file=sys.stdout)
            print("-"*60)
            traceback.print_exc(file=sys.stdout)
            print("-"*60)
            return
        # not returning anything is akin to returning success

class MStepCommand:
    program = 'mstep'

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

        command = 'command alias ms mstep'
        debugger.HandleCommand(command)

        command = "command regex mss 's/(.+)/thread step-scripted -C m_stepi.MapleSimpleStep /'"
        debugger.HandleCommand(command)
        command = "command regex msp 's/(.+)/thread step-scripted -C m_stepi.MaplePlanStep /'"
        debugger.HandleCommand(command)
        command = "command regex msc 's/(.+)/thread step-scripted -C m_stepi.MapleConditionStep /'"
        debugger.HandleCommand(command)

    @classmethod
    def create_options(cls):

        usage = "usage: %prog [options] arg"
        description = ('Basically:  mstep : Steps a program one Maple instruction')

        """steps program until it reaches a different source line of Maple Application
        Steps program until it reaches a different source line of Maple Application
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
            '-h',
            '--help',
            action='store_true',
            dest='step_help',
            help=description,
            default=False)

        return parser

    def get_short_help(self):
        return "Steps program until it reaches a different source line of Maple Application"

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
            if options.step_help:
                print(" ms is an alias of mstep command\n"
                    "  ms : Steps program until it reaches a different source line of Maple Application\n"
                    "  ms --help: Displays this message")
            else:
                print("Executing command")

        except Exception:
            # if you don't handle exceptions, passing an incorrect argument to
            # the OptionParser will cause LLDB to exit (courtesy of OptParse
            # dealing with argument errors by throwing SystemExit)
            result.SetError("option parsing failed")
            print("-"*60)
            print("Exception caught in mstep code:")
            traceback.print_stack(file=sys.stdout)
            print("-"*60)
            traceback.print_exc(file=sys.stdout)
            print("-"*60)
            return
        # not returning anything is akin to returning success

class MFinishCommand:
    program = 'mfinish'

    @classmethod
    def register_lldb_command(cls, debugger, module_name):
        parser = cls.create_options()
        cls.__doc__ = parser.format_help()
        # Add any commands contained in this module to LLDB
        command = 'command script add -c %s.%s %s' % (module_name,
                                                      cls.__name__,
                                                      cls.program)
        debugger.HandleCommand(command)

        command = "command regex mf 's/(.+)/thread step-scripted -C m_stepi.MapleFinishPrintCont /'"
        debugger.HandleCommand(command)

        if m_config_lldb.debug:
            print('The "{0}" command has been installed, type "help {0}" or "{0} '
                  '--help" for detailed help.'.format(cls.program))

    @classmethod
    def create_options(cls):

        usage = "usage: %prog [options] arg"
        description = ('Basically:  mfinish : Executes until selected Maple stack frame returns')

        # Pass add_help_option = False, since this keeps the command in line
        #  with lldb commands, and we wire up "help command" to work by
        # providing the long & short help methods below.
        parser = optparse.OptionParser(
            description=description,
            prog=cls.program,
            usage=usage,
            add_help_option=False)

        """execute until selected Maple stack frame returns
        Execute until selected Maple stack frame returns
        """
        parser.add_option(
            '-h',
            '--help',
            action='store_true',
            dest='mfinish_help',
            help=description,
            default=False)

        return parser

    def get_short_help(self):
        return "Executes until selected Maple stack frame returns"

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
            if options.mfinish_help:
                print("mfinish: Executes until selected Maple stack frame returns")
            else:
                print("Executing command")

        except Exception:
            # if you don't handle exceptions, passing an incorrect argument to
            # the OptionParser will cause LLDB to exit (courtesy of OptParse
            # dealing with argument errors by throwing SystemExit)
            result.SetError("option parsing failed")
            print("-"*60)
            print("Exception caught in mfinish code:")
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



class MapleSimpStep:

    def __init__(self, thread_plan, dict):
        self.thread_plan = thread_plan
        self.start_address = thread_plan.GetThread().GetFrameAtIndex(0).GetPC()

    def explains_stop(self, event):
        # We are stepping, so if we stop for any other reason, it isn't
        # because of us.
        if self.thread_plan.GetThread().GetStopReason() == lldb.eStopReasonTrace:
            return True
        else:
            return False

    def should_stop(self, event):
        cur_pc = self.thread_plan.GetThread().GetFrameAtIndex(0).GetPC()

        if cur_pc < self.start_address or cur_pc >= self.start_address + 20:
            self.thread_plan.SetPlanComplete(True)
            return True
        else:
            return False

    def should_step(self):
        return True


class MaplePlanStep:

    def __init__(self, thread_plan, dict):
        self.thread_plan = thread_plan
        self.start_address = thread_plan.GetThread().GetFrameAtIndex(0).GetPCAddress()
        self.step_thread_plan = thread_plan.QueueThreadPlanForStepOverRange(
            self.start_address, 20)

    def explains_stop(self, event):
        # Since all I'm doing is running a plan, I will only ever get asked this
        # if myplan doesn't explain the stop, and in that case I don't either.
        return False

    def should_stop(self, event):
        if self.step_thread_plan.IsPlanComplete():
            self.thread_plan.SetPlanComplete(True)
            return True
        else:
            return False

    def should_step(self):
        return False

# MapleConditionStep does "step over" through the current function,
# and when it stops at each line, it checks some condition (in this example the
# value of a variable) and stops if that condition is true.


class MapleConditionStep:

    def __init__(self, thread_plan, dict):
        self.thread_plan = thread_plan
        self.start_frame = thread_plan.GetThread().GetFrameAtIndex(0)
        self.queue_next_plan()

    def queue_next_plan(self):
        cur_frame = self.thread_plan.GetThread().GetFrameAtIndex(0)
        cur_line_entry = cur_frame.GetLineEntry()
        start_address = cur_line_entry.GetStartAddress()
        end_address = cur_line_entry.GetEndAddress()
        line_range = end_address.GetFileAddress() - start_address.GetFileAddress()
        self.step_thread_plan = self.thread_plan.QueueThreadPlanForStepOverRange(
            start_address, line_range)

    def explains_stop(self, event):
        # We are stepping, so if we stop for any other reason, it isn't
        # because of us.
        return False

    def should_stop(self, event):
        if not self.step_thread_plan.IsPlanComplete():
            return False

        frame = self.thread_plan.GetThread().GetFrameAtIndex(0)
        if not self.start_frame.IsEqual(frame):
            self.thread_plan.SetPlanComplete(True)
            return True

        # This part checks the condition.  In this case we are expecting
        # some integer variable called "a", and will stop when it is 20.
        a_var = frame.FindVariable("a")

        if not a_var.IsValid():
            print("A was not valid.")
            return True

        error = lldb.SBError()
        a_value = a_var.GetValueAsSigned(error)
        if not error.Success():
            print("A value was not good.")
            return True

        if a_value == 20:
            self.thread_plan.SetPlanComplete(True)
            return True
        else:
            self.queue_next_plan()
            return False

    def should_step(self):
        return True

# MapleFinishPrintCont steps out of the current frame, gathers some information
# and then continues.  The information in this case is rax.  Currently the thread
# plans do not seem to be a  safe place to call lldb command-line commands, so
# information is gathered through SB API calls. This is meant to gther Maple Data
# as the Data tree is expanded.


class MapleFinishPrintCont:

    def __init__(self, thread_plan, dict):
        self.thread_plan = thread_plan
        self.step_out_thread_plan = thread_plan.QueueThreadPlanForStepOut(
            0, True)
        self.thread = self.thread_plan.GetThread()

    def is_stale(self):
        if self.step_out_thread_plan.IsPlanStale():
            self.do_print()
            return True
        else:
            return False

    def explains_stop(self, event):
        return False

    def should_stop(self, event):
        if self.step_out_thread_plan.IsPlanComplete():
            self.do_print()
            self.thread_plan.SetPlanComplete(True)
        return False

    def do_print(self):
        frame_0 = self.thread.frames[0]
        rax_value = frame_0.FindRegister("rax")
        if rax_value.GetError().Success():
            print("RAX on exit: ", rax_value.GetValue())
        else:
            print("Couldn't get rax value:", rax_value.GetError().GetCString())

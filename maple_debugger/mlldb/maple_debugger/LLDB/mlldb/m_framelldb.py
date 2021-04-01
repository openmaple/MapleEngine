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


# Use this app call hierarchy
#debugger.GetSelectedTarget()
#debugger.GetSelectedTarget().GetProcess()
#debugger.GetSelectedTarget().GetProcess().GetThread()
#debugger.GetSelectedTarget().GetProcess().GetThread().GetSelectedFrame()


def disthis(debugger, command, *args):
    """Usage: disthis
Disables the breakpoint the currently selected thread is stopped at."""

    target = lldb.SBQueue()      # some random object that will be invalid
    thread = lldb.SBQueue()      # some random object that will be invalid

    if len(args) == 2:
        # Old lldb invocation style
        result = args[0]
        if debugger and debugger.GetSelectedTarget() and debugger.GetSelectedTarget().GetProcess():
            target = debugger.GetSelectedTarget()
            if not target:
                print("invalid target", file=result)
                return
            process = target.GetProcess()
            if not process:
                print("invalid process", file=result)
                return
            thread = process.GetSelectedThread()
            if not thread:
                print("invalid thread", file=result)
                return
            frame = thread.GetSelectedFrame()
            if not frame:
                print("invalid frame", file=result)
                return
    elif len(args) == 3:
        # New (2015 & later) lldb invocation style where we're given the execution context
        exe_ctx = args[0]
        result = args[1]
        target = exe_ctx.GetTarget()
        thread = exe_ctx.GetThread()

    if thread.IsValid() != True:
        print >>result, "error: process is not paused."
        result.SetStatus (lldb.eReturnStatusFailed)
        return

def __lldb_init_module (debugger, dict):
    debugger.HandleCommand('command script add -f %s.disthis disthis' % __name__)



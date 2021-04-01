#
# Copyright (C) [2021] Futurewei Technologies, Inc. All rights reserved.
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

import gdb
import os
import sys
import time
import m_debug
import m_util
from m_util import gdb_print
import m_datastore

msettings = {}

mexcluded = {"m_debug.py", "m_set.py", "m_gdb.py", "m_help.py"}
mtrclmts  = os.getenv('MAPLE_TRACE_LIMIT','')
mtrclimit = int(mtrclmts) if mtrclmts.isdigit() else 80

def trace_maple_debugger(frame, event, arg, mtrace_data = [0]):
    """implements the trace function of Maple debugger
    The trace output includes the elapsed time of each target function for performance profiling

    params:
       frame: the frame to be traced
       event: event which occurs, both "call" and "return" are needed
       arg:   Return value for "return" and None for "call"

    returns:
       trace_maple_debugger itself
    """
    if event == "call":
        co = frame.f_code
        filename = co.co_filename.split('/')[-1]
        if filename.startswith("m_") and filename not in mexcluded:
            funcname, lineno, caller = co.co_name, frame.f_lineno, frame.f_back
            callsite = caller.f_code.co_filename.split('/')[-1] + ':' + str(caller.f_lineno) if caller else "None"
            if co.co_argcount == 0:
                argstr = 'arg#0'
            else:
                argstr = 'arg#%d, %s' % (co.co_argcount,
                        ', '.join(str(frame.f_locals[v]) for v in co.co_varnames[:co.co_argcount]))
                argstr = (argstr[:mtrclimit] + '...' if len(argstr) > mtrclimit \
                        else argstr).replace('\n', '\\n').replace('\033', '\\033')
            mtrace_data[0] += 1
            gdb_print("+ " * mtrace_data[0] + "==> %s:%d, %s, @(%s), {%s}" % \
                    (filename, lineno, funcname, callsite, argstr), gdb.STDERR)
            mtrace_data.append(time.time())
    elif event == "return" and mtrace_data[0] > 0:
        co = frame.f_code
        filename = co.co_filename.split('/')[-1]
        if filename.startswith("m_") and filename not in mexcluded:
            etime = int((time.time() - mtrace_data.pop()) * 1000000)
            funcname, lineno, retstr = co.co_name, frame.f_lineno, str(arg)
            retstr = (retstr[:mtrclimit] + '...' if len(retstr) > mtrclimit \
                    else retstr).replace('\n', '\\n').replace('\033', '\\033')
            gdb_print("- " * mtrace_data[0] + "<-- %s:%d, %s, %sus, ret: %s" % \
                    (filename, lineno, funcname, etime, retstr), gdb.STDERR)
            mtrace_data[0] -= 1
    return trace_maple_debugger

def register_event_handlers(action, event_handlers = {}):
    """register event handlers with action(either 'connect' or 'disconnect')
    Ignore all event handlers defined in m_util.py with names like event_%s_handler
    """
    template = "def trc_event_{0}(e=None): gdb.write('@@@Event name: {0}, type: %s\\n' % type(e).__name__)"
    msg = 'En' if action == 'connect' else 'Dis'
    for e in sorted(gdb.events.__dict__.keys()):
        if e.startswith("__"): continue
        if e not in event_handlers:
            exec(template.format(e))
            exec('event_handlers[e] = trc_event_' + e)
        exec('gdb.events.{0}.{1}(event_handlers[e])'.format(e, action))
        gdb.write('  %sabled tracing for event "%s"\n' % (msg, e))

class MapleSetCmd(gdb.Command):
    """sets and displays Maple debugger settings
    mset <name> <value>: sets Maple debugger environment settings
    mset verbose on|off: enables/disables Maple debugger verbose mode
    mset -add  <key name of list> <list item value>: adds new key/value into a list
      example: mset -add maple_lib_asm_path ~/gitee/maple_engine/maple_build/out/x864
    mset -del  <key name of list> <list item value>: deletes one key/value from a list
      example: maple_lib_asm_path ~/gitee/maple_engine/maple_build/out/x864
    mset -show: displays current settings
    """

    def __init__(self):
        gdb.Command.__init__ (self,
                              "mset",
                              gdb.COMMAND_DATA,
                              gdb.COMPLETE_NONE)
        global msettings
        msettings['verbose'] = 'off'
        msettings['event'] = 'disconnect'
        msettings['opcode'] = 'off' # for msi, ms, mfinish, mni, mn command to show instruction info
        msettings['stack'] = 'off' # for msi and mni to show evaluation stack info
        m_debug.Debug = False
        msettings['maple_lib_asm_path'] = []

    def invoke(self, args, from_tty):
        self.mset_func(args, from_tty)

    def usage(self):
        gdb_print("mset:\n"
                  "  -To set Maple debugger environment:\n"
                  "    syntax: mset <name> <value>\n"
                  "    example: mset verbose on\n"
                  "  -To add/del items in list:\n"
                  "    syntax:  mset -add <key name of list> <list item value>\n"
                  "    example: mset -add maple_lib_asm_path ~/gitee/maple_engine/maple_build/out/x864\n"
                  "             mset -del maple_lib_asm_path ~/gitee/maple_engine/maple_build/out/x864\n"
                  "  -To display current settings:\n"
                  "    syntax:  mset -show\n"
                  "  -To load user defined configuration file in json format:\n"
                  "    syntax:  mset -lconfig <configuration json file>")

    def mset_func(self, args, from_tty):
        s = str(args).split()
        global msettings
        if len(s) == 1:
            if s[0] == '-show':
                self.mshow_settings()
                if m_debug.Debug: m_datastore.mgdb_rdata.show_file_cache()
                return
            self.usage()
            return
        elif len(s) == 2:
            if s[0] == 'trace':
                if s[1].isdigit():
                    global mtrclimit
                    mtrclimit = int(s[1])
                else:
                    sys.setprofile(trace_maple_debugger if s[1] == 'on' else None)
            elif s[0] == 'verbose':
                m_debug.Debug = False if s[1] == 'off' else True
                msettings[s[0]] = 'on' if m_debug.Debug else 'off'
            elif s[0] == 'event':
                action = 'connect' if s[1] == 'on' else 'disconnect'
                if action != msettings[s[0]]:
                    register_event_handlers(action)
                    msettings[s[0]] = action
            elif s[0] == 'opcode':
                msettings[s[0]] = 'on' if s[1] == 'on' else 'off'
            elif s[0] == 'stack':
                msettings[s[0]] = 'on' if s[1] == 'on' else 'off'
            else:
                msettings[s[0]] = s[1]
                return
        elif len(s) == 3:
            if s[0] == '-add':
                k = s[1]
                v = s[2]
                if k == 'maple_lib_asm_path':
                    msettings[k].append(os.path.expandvars(os.path.expanduser(v)))
                return
            elif s[0] == '-del':
                k = s[1]
                v = s[2]
                if k == 'maple_lib_asm_path':
                    try:
                        msettings[k].remove(v)
                    except:
                        pass
                m_datastore.mgdb_rdata.del_asmblock_lines()
                if m_debug.Debug: m_datastore.mgdb_rdata.show_file_cache()
                return
        else:
            self.usage()
            return

    def mget_a_setting(self, key):
        global msettings
        if key in msettings:
            return msettings[key]
        else:
            return None

    def mshow_settings(self):
        global msettings
        gdb_print(str(msettings))

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
import m_info
import m_frame
import m_datastore
import m_debug
from m_util import gdb_print
from m_util import MColors

class MapleLocalCmd(gdb.Command):
    """displays selected Maple frame arguments, local variables and stack dynamic data
    mlocal: displays function local variabls of currently selected Maple frame
    mlocal [-s|-stack]: displays runtime operand stack changes of current selected Maple frame
    """

    def __init__(self):
        gdb.Command.__init__ (self,
                              "mlocal",
                              gdb.COMMAND_DATA,
                              gdb.COMPLETE_NONE)


    def invoke(self, args, from_tty):
        self.mlocal_func(args, from_tty)

    def usage(self):
        gdb_print("mlocal: Displays function local variabls of currently selected Maple frame"
                  "mlocal [-s|-stack]: Displays runtime operand stack changes of currently selected Maple frame")

    def mlocal_func(self, args, from_tty):
        mode = 'local'
        s = args.split()
        if len(s) == 1 and s[0] == '-stack':
            mode = 'stack'
        elif len(s) == 1 and s[0] == '-s':
            mode = 'stack'

        frame = m_frame.get_selected_frame()
        if not frame:
            gdb_print("no valid frame found")
            return

        data = m_datastore.get_stack_frame_data(frame)
        if not data:
            gdb_print("no valid frame data found")
            return

        func_argus_locals = data['func_argus_locals']
        func_header_name = data['frame_func_header_info']['func_header_name']

        if m_debug.Debug:
            m_debug.dbg_print("func_argus_locals ======= ")
            m_debug.dbg_print("func_argus_locals=", func_argus_locals)
            m_debug.dbg_print("func_header_name=", func_header_name)

        if mode == 'local':
            self.display_locals(func_argus_locals, func_header_name)
        else:
            self.display_stack(func_argus_locals, func_header_name)

        return

    def display_locals(self, func_argus_locals_dict, func_header_name):
        """
        displays function local variables and arguments.

        params:
          func_header_name: string. function name
          func_argus_locals_dict: function local/argument data in dict
            {
                'locals_type': a list of local variable type in string, e.g. ['void', 'v2i64'],
                'locals_name': a list of local variable name in string. e.g. ['%%retval', '%%thrownval'],
                'formals_type': a list of argument type in string, e.g. ['a64'],
                'formals_name': a list of argument name in string, e.g. ['%1']
            }
        """
        local_num = len(func_argus_locals_dict['locals_name'])
        sp = m_info.get_maple_frame_stack_sp_index(func_header_name)
        if not sp:
            gdb_print("no valid operand stack data found")
            return
        if m_debug.Debug: m_debug.dbg_print("sp=", sp)

        for i in range(local_num):
            name = func_argus_locals_dict['locals_name'][i]
            ltype = func_argus_locals_dict['locals_type'][i]
            if m_debug.Debug: m_debug.dbg_print("local #", i, "name=", name, "ltype=", ltype)
            if name == "%%retval":
                continue

            rtype, local_value = m_info.get_one_frame_stack_local(i, sp, name, ltype)
            if not local_value:
                local_value = "none"

            line = "local #" +  str(i) + ": name=" + MColors.BT_ARGNAME + name + MColors.ENDC + \
                    " type=" + rtype + " value=" + str(local_value)
            gdb_print(line)

        return

    def display_stack(self, func_argus_locals_dict, func_header_name):
        """
        displays function dynamic stack data.

        params:
          func_header_name: string. function name
          func_argus_locals_dict: function local/argument data in dict
            {
                'locals_type': a list of local variable type in string, e.g. ['void', 'v2i64'],
                'locals_name': a list of local variable name in string. e.g. ['%%retval', '%%thrownval'],
                'formals_type': a list of argument type in string, e.g. ['a64'],
                'formals_name': a list of argument name in string, e.g. ['%1']
            }
        """
        sp = m_info.get_maple_frame_stack_sp_index(func_header_name)
        if not sp:
            gdb_print("no valid operand stack data found")
            return
        local_num = len(func_argus_locals_dict['locals_name'])

        idx = local_num
        count = 0
        report_count = 0
        while idx <= sp:
            mtype, v = m_info.get_one_frame_stack_dynamic(sp, idx)
            if m_debug.Debug: m_debug.dbg_print("idx=", idx, "mtype=", mtype, "v=", v)
            if not mtype or not v:
                count += 1
                idx += 1
                continue

            line = "sp=" + str(count) + ": type=" + str(mtype) + " value=" + str(v)
            gdb_print(line)
            count += 1
            idx += 1
            report_count += 1

        if report_count == 0:
            gdb_print("evaluated stack: empty")
        return

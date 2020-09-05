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
import m_frame
import m_datastore
import m_util
from m_util import gdb_print
import m_debug

#def get_current_stack_level():
#    """ get the stack level from the newest frame """
#    frame = m_frame.get_newest_frame()
#    if not frame:
#        return 0
#
#    level = 0
#    while frame:
#        level += 1
#        frame = m_frame.get_next_older_frame(frame)
#
#    return level

class MapleNextiCmd(gdb.Command):
    """Step one Maple instruction, but proceed through subroutine calls
    mnexti: Step one Maple instruction, but proceed through subroutine calls
    """

    def __init__(self):
        gdb.Command.__init__ (self,
                              "mnexti",
                              gdb.COMMAND_RUNNING,
                              gdb.COMPLETE_NONE)
        m_util.gdb_exec('alias mni = mnexti')

    def invoke(self, args, from_tty):
        self.mni_func(args, from_tty)

    def usage(self):
        gdb_print ("mnexti: Step one Maple instruction, but proceed through subroutine calls")

    def mni_func(self, args, from_tty):
        s = args.split()
        if len(s) > 0: # msi into next Maple instruction
            self.usage()
            return

        #start_level_num = get_current_stack_level()
        prev_frame = m_frame.get_newest_frame()
        m_util.gdb_exec_to_null('msi -internal')
        new_frame = m_frame.get_newest_frame()
        #end_level_num   = get_current_stack_level()

        if new_frame != prev_frame and prev_frame.is_valid():
            #assert start_level_num < end_level_num
            m_util.gdb_exec_to_null('finish')
            m_util.gdb_exec_to_null('msi -internal')
        #else:
        #    assert start_level_num >= end_level_num

        # a trigger point to update m_datastore caches
        m_datastore.mgdb_rdata.update_gdb_runtime_data()

        # to display the latest instructions will be executed after this command
        frame = m_frame.get_selected_frame()
        if not frame:
            return
        ds = m_datastore.get_stack_frame_data(frame)
        if not ds:
            return
        if m_debug.Debug: m_debug.dbg_print("retrieved ds=", ds)

        asm_path = ds['frame_func_header_info']['asm_path']
        asm_line = ds['frame_func_src_info']['asm_line']
        asm_offset = ds['frame_func_src_info']['asm_offset']

        f = open(asm_path, 'r')
        f.seek(asm_offset)
        line = f.readline()
        gdb_print(str(asm_line) + " : " + line.rstrip())
        line = f.readline()
        gdb_print(str(asm_line+1) + " : " + line.rstrip())
        line = f.readline()
        gdb_print(str(asm_line+2) + " : " + line.rstrip())
        f.close()

        return

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
import sys
import m_frame
import m_datastore
from inspect import currentframe, getframeinfo
import m_util

def get_current_stack_level():
    """ get the stack level from selected frame """
    frame = m_frame.get_selected_frame()
    if not frame:
        return 0

    level = 0
    while frame:
        level += 1
        frame = m_frame.get_next_older_frame(frame)

    return level

class MapleNextiCmd(gdb.Command):
    """
    This class defines Maple command mnexti, to perform a Maple next instruction
    operation.
    """

    def __init__(self):
        gdb.Command.__init__ (self,
                              "mnexti",
                              gdb.COMMAND_RUNNING,
                              gdb.COMPLETE_NONE)
        gdb.execute('alias mni = mnexti')

    def invoke(self, args, from_tty):
        gdb.execute('set pagination off', to_string = True)
        self.mni_func(args, from_tty)
        gdb.execute('set pagination on', to_string = True)

    def usage(self):
        print ("mnexti: Step one Maple instruction, but proceed through subroutine calls")

    def mni_func(self, args, from_tty):
        s = args.split()
        if len(s) > 0: # msi into next Maple instruction
            self.usage()
            return
        
        start_level_num = get_current_stack_level()
        m_util.debug_print(__file__, getframeinfo(currentframe()).lineno, self.mni_func, "start_level_num", start_level_num)
        buf = gdb.execute('msi', to_string = True)
        end_level_num   = get_current_stack_level()
        m_util.debug_print(__file__, getframeinfo(currentframe()).lineno, self.mni_func, "end_level_num", end_level_num)

        if start_level_num < end_level_num:
            buf = gdb.execute('finish', to_string = True)
            buf = gdb.execute('msi', to_string = True)

        # a trigger point to update m_datastore caches
        m_datastore.mgdb_rdata.update_gdb_runtime_data()

        # to display the latest instructions will be executed after this command
        frame = m_frame.get_selected_frame()
        if not frame:
            return
        ds = m_datastore.get_stack_frame_data(frame)
        if not ds:
            return
        m_util.debug_print(__file__, getframeinfo(currentframe()).lineno, self.mni_func,\
                            "retrieved ds=", ds)

        asm_path = ds['frame_func_header_info']['asm_path']
        asm_line = ds['frame_func_src_info']['asm_line']
        asm_offset = ds['frame_func_src_info']['asm_offset']

        f = open(asm_path, 'r')
        f.seek(asm_offset)
        line = f.readline()
        print(asm_line, " : ", line.rstrip())
        line = f.readline()
        print(asm_line+1, " : ", line.rstrip())
        line = f.readline()
        print(asm_line+2, " : ", line.rstrip())
        f.close()

        return

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
from m_util import MColors
from m_util import gdb_print
import m_debug
import m_asm
import m_list
import m_stepi

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

def mni_common():
    #start_level_num = get_current_stack_level()
    prev_frame = m_frame.get_newest_frame()
    m_util.gdb_exec('msi -internal')
    new_frame = m_frame.get_newest_frame()
    #end_level_num   = get_current_stack_level()

    if new_frame != prev_frame and prev_frame.is_valid():
        #assert start_level_num < end_level_num
        m_stepi.silent_finish()
        m_util.gdb_exec('msi -internal')
    #else:
    #    assert start_level_num >= end_level_num

    # a trigger point to update m_datastore caches
    m_datastore.mgdb_rdata.update_gdb_runtime_data()
    m_datastore.mgdb_rdata.update_frame_change_counter()

def mni_display_last_opcodes():
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

    gdb_print("asm file: %s%s%s" % (MColors.BT_SRC, asm_path, MColors.ENDC))
    f = open(asm_path, 'r')
    f.seek(asm_offset)
    line = f.readline()
    gdb_print(str(asm_line) + " : " + line.rstrip())
    line = f.readline()
    gdb_print(str(asm_line+1) + " : " + line.rstrip())
    line = f.readline()
    gdb_print(str(asm_line+2) + " : " + line.rstrip())
    f.close()

class MapleNextiCmd(gdb.Command):
    """step one Maple instruction, but proceed through subroutine calls
    mnexti: step one Maple instruction, but proceed through subroutine calls
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
        gdb_print ("mnexti: Steps one Maple instruction, but proceeds through subroutine calls")

    def mni_func(self, args, from_tty):
        s = args.split()
        if len(s) > 0: # msi into next Maple instruction
            self.usage()
            return
        mni_common()
        mni_display_last_opcodes()
        m_util.gdb_exec('display')

        return

class MapleNextCmd(gdb.Command):
    """Step program, proceeding through subroutine calls of Maple application
    Step program, proceeding through subroutine calls of Maple application
    """

    def __init__(self):
        gdb.Command.__init__ (self,
                              "mnext",
                              gdb.COMMAND_RUNNING,
                              gdb.COMPLETE_NONE)
        m_util.gdb_exec('alias mn = mnext')

    def invoke(self, args, from_tty):
        self.mnext_func(args, from_tty)

    def usage(self):
        gdb_print ("mnext: Step program, proceeding through subroutine calls of Maple application")

    def mnext_func(self, args, from_tty):
        """
        mni_common()
        m_util.gdb_exec("mstep")
        """

        frame = m_frame.get_selected_frame()
        ds = m_datastore.get_stack_frame_data(frame)
        if not ds:
            return
        if m_debug.Debug: m_debug.dbg_print("retrieved ds=", ds)
        asm_path = ds['frame_func_header_info']['asm_path']
        asm_line = ds['frame_func_src_info']['asm_line']
        asm_offset = ds['frame_func_src_info']['asm_offset']

        stop = False
        short_src_file_name = None
        short_src_file_line = None
        count = 0
        while not stop:
            # before we execute msi command, we must know should we stop keep executing msi after the current msi
            # is executed. If current msi command is executed and found it should stop, then we need the source file
            # information after stop.
            stop, short_src_file_name, short_src_file_line = m_asm.look_up_next_opcode(asm_path, asm_line, asm_offset)
            if m_debug.Debug:
                m_debug.dbg_print("stop =", stop, "short_src_file_name=", short_src_file_name, "short_src_file_line=",short_src_file_line)
            mni_common()
            count += 1
            if m_debug.Debug:
                m_debug.dbg_print("executed %d opcodes", count)
            # retrieve the new frame data since one opcode can change to a different frame
            frame = m_frame.get_selected_frame()
            ds = m_datastore.get_stack_frame_data(frame)
            if not ds:
                gdb_print("Warning: Failed to get new stack frame to continue")
                return
            if m_debug.Debug: m_debug.dbg_print("retrieved ds=", ds)
            asm_path = ds['frame_func_header_info']['asm_path']
            asm_line = ds['frame_func_src_info']['asm_line']
            asm_offset = ds['frame_func_src_info']['asm_offset']

        gdb_print("Info: executed %d %s" % (count, 'opcode' if count == 1 else 'opcodes'))
        if m_debug.Debug:
            m_debug.dbg_print ("short_src_file_name = ", short_src_file_name, "short_src_file_line=", short_src_file_line)

        file_full_path = None
        for source_path in m_list.maple_source_path_list:
            file_full_path = m_list.find_one_file(short_src_file_name, source_path)
            if not file_full_path:
                continue
            else:
                break

        if not file_full_path:
            gdb_print("Warning: source file %s not found" % (short_src_file_name))
        else:
            m_list.display_src_file_lines(file_full_path, short_src_file_line)
        return

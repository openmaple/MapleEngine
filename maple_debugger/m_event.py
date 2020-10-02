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
import m_asm
import m_mirmpl
import m_util
from m_util import gdb_print
from m_util import MColors
import m_datastore
import m_inf
import m_info

def create_libcore_label_cache(base_path):
    asm_path = base_path + '/libcore.VtableImpl.s'
    if not os.path.exists(asm_path):
        return False
    m_asm.create_asm_mirbin_label_cache(asm_path, True)
    mirmpl_path = base_path + '/libcore.mpl.mir.mpl'
    if os.path.exists(mirmpl_path):
        m_mirmpl.create_mirmpl_label_cache(mirmpl_path, True)
    return True

def event_new_objfile_handler(e):
    """event handler for new_objfile
    it calls m_asm.create_asm_mirbin_label_cache() and m_mirmpl.create_mirmpl_label_cache()
    to handle asm and Maple IR labels in background processes
    """
    path_list = e.new_objfile.filename.split('/')
    if path_list[-1] == "libcore.so" and \
      not create_libcore_label_cache(os.path.realpath('/'.join(path_list[:-1]))) and \
      len(path_list) > 4:
        out_path = '/'.join(path_list[:-4]) + '/maple_build/out/x86_64'
        create_libcore_label_cache(os.path.realpath(out_path))

def event_exited_handler(e):
    """event handler to do cleanup
    """
    m_mirmpl.create_mirmpl_label_cache(None)
    m_asm.create_asm_mirbin_label_cache(None)

def event_breakpoint_created_handler(bp):
    """event handler to handle breakpoint creation
       if a native breakpoint is created in Maple shared library program space,
       delete this breakpoint or disable it
    """
    if bp.location == "maple::maple_invoke_method" or bp.location == "__inc_opcode_cnt":
        return

    if not m_inf.is_inferior_running():
        return

    try:
        result_value = gdb.parse_and_eval(bp.location)
    except:
        # breakpoint symbol not found in program space, so it is not a valid breakpoint
        return

    so_name = gdb.solib_name(int(result_value.address))
    so_name = os.path.realpath(so_name)
    if not so_name: # this happens to be at very beginning before all the shared libraries are loaded
        return

    so_name = os.path.realpath(so_name)
    asm_name_1 = so_name[:-3] + '.s'
    asm_name_2 = so_name[:-3] + '.VtableImpl.s'

    asm_list = m_info.get_loaded_lib_asm_path()
    if asm_name_1 in asm_list or asm_name_2 in asm_list:
        bploc = '%s%s%s' % (MColors.BP_ADDR, bp.location, MColors.ENDC)
        # this means this breakpoint's address is in Maple Engine loaded shared library
        gdb_print("Warning: It is not allowed to set a gdb breakpoint at %s in Maple shared library %s%s%s" % \
            (bploc, MColors.BT_SRC, so_name.split('/')[-1], MColors.ENDC))
        bp.delete()
        gdb_print("This gdb breakpoint at %s was deleted. Please set a Maple breakpoint with a 'mbreak' command" % bploc)

def event_breakpoint_modified_handler(bp):
    """event handler to handle breakpoint modification
       if a native breakpoint is modified in Maple shared library program space,
       delete this breakpoint or disable it
    """
    if bp.location == "maple::maple_invoke_method" or bp.location == "__inc_opcode_cnt":
        return

    if not m_inf.is_inferior_running():
        return

    try:
        result_value = gdb.parse_and_eval(bp.location)
    except:
        # breakpoint symbol not found in program space, so it is not a valid breakpoint
        return

    so_name = gdb.solib_name(int(result_value.address))
    so_name = os.path.realpath(so_name)
    if not so_name: # this happens to be at very beginning before all the shared libraries are loaded
        return

    so_name = os.path.realpath(so_name)
    asm_name_1 = so_name[:-3] + '.s'
    asm_name_2 = so_name[:-3] + '.VtableImpl.s'

    asm_list = m_info.get_loaded_lib_asm_path()
    if asm_name_1 in asm_list or asm_name_2 in asm_list:
        bploc = '%s%s%s' % (MColors.BP_ADDR, bp.location, MColors.ENDC)
        # this means this breakpoint's address is in Maple Engine loaded shared library
        gdb_print("Warning: It is not allowed to set a gdb breakpoint at %s in Maple shared library %s%s%s" % \
            (bploc, MColors.BT_SRC, so_name.split('/')[-1], MColors.ENDC))
        bp.delete()
        gdb_print("This gdb breakpoint at %s was deleted. Please set a Maple breakpoint with a 'mbreak' command" % bploc)

def init_event_handlers():
    """initializes event handlers
    """
    gdb.events.new_objfile.connect(event_new_objfile_handler)
    gdb.events.exited.connect(event_exited_handler)
    gdb.events.breakpoint_created.connect(event_breakpoint_created_handler)
    gdb.events.breakpoint_modified.connect(event_breakpoint_modified_handler)

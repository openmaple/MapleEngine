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
import sys
import m_symbol
import m_info
import m_frame
import m_datastore
import m_util
from m_util import MColors
from m_util import gdb_print
import m_set
import m_debug
import m_list
import m_asm

def is_msi_bp_existed():
    """ determines if the msi breakpoints exist or not """
    blist = gdb.breakpoints()
    for b in blist:
        if m_debug.Debug:
             m_debug.dbg_print("b.type=", b.type, "b.location=", b.location, "b.thread=", \
                     b.thread, "b.enabled=", b.enabled, "b.is_valid()=", b.is_valid())
        if '__inc_opcode_cnt' == b.location and b.is_valid():
            return True, b
    return False, None

def get_msi_bp_id():
    """ gets an msi breakpoint gdb.Breakpoint object id """
    blist = gdb.breakpoints()
    for b in blist:
        if m_debug.Debug:
             m_debug.dbg_print("b.type=", b.type, "b.location=", b.location, "b.thread=", \
                     b.thread, "b.enabled=", b.enabled, "b.is_valid()=", b.is_valid())
        if '__inc_opcode_cnt' == b.location and b.is_valid():
            return b
    return None

def enable_msi_bp():
    blist = gdb.breakpoints()
    for b in blist:
        if '__inc_opcode_cnt' == b.location and b.is_valid():
            b.enabled = True
            break
    return

def disable_msi_bp():
    blist = gdb.breakpoints()
    for b in blist:
        if '__inc_opcode_cnt' == b.location and b.is_valid():
            b.enabled = False
            break
    return

class MapleStepiBreakpoint(gdb.Breakpoint):
    """
    this class defines a Maple msi breakpoint. msi breakpoint is a breakpoint
    at symbol of __int_opcode_cnt, and can be set along with a specified running
    thread
    """

    def __init__(self, threadno, count = 1, mtype=gdb.BP_BREAKPOINT, mqualified=False):
        """
        creates an msi breakpoint with specified thread
        """
        self.settings = {}
        self.settings['symbol'] = '__inc_opcode_cnt'
        self.settings['variable'] = '__opcode_cnt'
        self.settings['count'] = count
        self.settings['thread'] = threadno

        spec = self.settings['symbol'] + ' thread ' + str(threadno)
        super().__init__(spec)
        self.msi_bp_object_id = get_msi_bp_id()

    def set_bp_attr(self, key, value):
        self.settings[key] = value

    def get_bp_attr(self, key):
        return self.settings[key]

    def get_bp_id(self):
        return self.msi_bp_object_id

    def display_settings(self):
        gdb_print ("msi break point settings ---- ")
        gdb_print (str(self.settings))

class MapleStepiCmd(gdb.Command):
    """steps specified number of Maple instructions
    msi is an alias of mstepi command
    mstepi: steps in next Maple instruction
    mstepi [n]: steps in to next nth Maple instruction
    mstepi -abs [n]: steps in specified index of Maple instruction
    """

    def __init__(self):
        gdb.Command.__init__ (self,
                              "mstepi",
                              gdb.COMMAND_RUNNING,
                              gdb.COMPLETE_NONE)

        self.mbp_object = None
        self.msi_bp_id = None
        self.mbp_dync_object = None
        self.msi_bp_dync_id = None
        m_util.gdb_exec('alias msi = mstepi')

        self.msi_mode_default = 0
        self.msi_mode_internal = 1

    def init_gdb_breakpoint(self, threadno, count, bp_on_static=True, bp_on_dync=True):
        # create a msi breakpoint object on a specified thread
        if bp_on_static:
            self.mbp_object = MapleStepiBreakpoint(threadno, count)
            self.mbp_object.thread = threadno
            self.mbp_object.silent = True
            self.msi_bp_id = self.mbp_object.msi_bp_object_id

        if bp_on_dync:
            self.mbp_dync_object = MapleStepiDyncBreakpoint(threadno, count)
            self.mbp_dync_object.thread = threadno
            self.mbp_dync_object.silent = True
            self.msi_bp_dync_id = self.mbp_dync_object.msi_bp_object_id

    def invoke(self, args, from_tty):
        self.msi_func(args, from_tty)

    def usage(self):
        gdb_print("  msi : Steps into next Maple instruction\n"
                  "  msi [n]: Steps into next n Maple instructions\n"
                  "  msi -abs [n]: Steps into specified index n of Maple instruction\n"
                  "  msi -c: Shows current Maple opcode count\n"
                  "  msi help: Displays usage")

    def msi_func(self, args, from_tty):
        tobject= m_info.get_current_thread()
        if not tobject:
            gdb_print ("current thread info not valid or program not started yet")
            return
        tidx = tobject.num

        s = args.split()
        if len(s) == 0: # msi into next Maple instruction
            count = 1
            self.set_mstepi(tidx, count, self.msi_mode_default)
        elif len(s) == 1: # msi [n]
            if s[0] == '-debug':
                self.debug()
            elif s[0].isdigit() and int(s[0]) > 0:
                self.set_mstepi(tidx, int(s[0]), self.msi_mode_default)
            elif s[0] == '-count' or s[0] == '-c':
                self.show_current_opcode_cnt()
            elif s[0] == '-internal':
                self.set_mstepi(tidx, 1, self.msi_mode_internal)
            else:
                self.usage()
        elif len(s) == 2: # msi -abs n
            if s[0] == '-abs' and s[1].isdigit() :
                delta_count = self.delta_from_current_opcode_cnt(int(s[1]))
                if not delta_count:
                    self.usage()
                elif delta_count < 0:
                    gdb_print("current opcode count is already ahead of what you specified " + s[1])
                else:
                    self.set_mstepi(tidx, delta_count, self.msi_mode_default)
            else:
                self.usage()
        else:
            self.usage()

    # determine current frame is static or dynamic, and then return current opcode count
    def get_current_opcode_cnt(self):
        if m_frame.is_current_maple_frame_dync():
            return self.get_current_opcode_cnt_dync()
        else:
            return self.get_current_opcode_cnt_static()

    def get_current_opcode_cnt_static(self):
        """returns: string as the current opcode cnt"""
        current_opcode_cnt = m_symbol.get_variable_value('__opcode_cnt')
        if not current_opcode_cnt:
            return None
        return current_opcode_cnt

    def get_current_opcode_cnt_dync(self):
        """returns: string as the current opcode cnt"""
        current_opcode_cnt_dync = m_symbol.get_variable_value('__opcode_cnt_dyn')
        if not current_opcode_cnt_dync:
            return None
        return current_opcode_cnt_dync

    # determine current frame is static or dynamic, and then display the current opcode count
    def show_current_opcode_cnt(self):
        current_opcode_cnt = self.get_current_opcode_cnt()
        tobject= m_info.get_current_thread()
        if not current_opcode_cnt:
            gdb_print ("Thread %d: current opcode count not found" % (tobject.num))
        else:
            gdb_print ("Thread %d: current opcode count is %s" % (tobject.num, current_opcode_cnt))
        return

    def delta_from_current_opcode_cnt(self, target_opcode_cnt):
        """ for given int target_opcode_cnt, calculate the delta count from current opcode cnt
            returns: None if current_opcode_cnt is not valid
                    target_opcode_cnt - current_opcode_cnt
        """
        current_opcode_cnt_str = self.get_current_opcode_cnt()
        if not current_opcode_cnt_str:
            return None
        try:
            current_opcode_cnt = int(current_opcode_cnt_str)
        except:
            return None
        return target_opcode_cnt - current_opcode_cnt

    def mstepi_common(self, threadno, count):
        """returns: selected frame object in stack right after msi breakpoint is hit
                    None: if something is wrong or no frame available in stack
        """
        # check if there is existing msi point, it could be created by msi command
        # previously, or it could be created by gdb b command.
        if m_frame.is_current_maple_frame_dync():
            return self.mstepi_common_dync(threadno, count)
        else:
            result, frame = m_frame.is_closest_older_maple_frame_dync()
            if not result and not frame:
                #return None
                return self.mstepi_common_static(threadno, count)

            elif result and frame:
                return self.mstepi_common_dync(threadno, count)
            elif not result and frame:
                return self.mstepi_common_static(threadno, count)
            else:
                return self.mstepi_common_static(threadno, count)
            #return None

    def mstepi_common_static(self, threadno, count):
        msi_bp_exist, msi_bp_id = is_msi_bp_existed()
        #print ("msi_bp_exist=", msi_bp_exist, "msi_bp_id=", msi_bp_id, "self.msi_bp_id=", self.msi_bp_id)
        if not msi_bp_exist:
            # there is no msi bp exist, so just create a new msi breakpoint
            self.init_gdb_breakpoint(threadno, count, True, False)
        else:
            if msi_bp_id != self.msi_bp_id:
                # if existing msi bp id does not match to self.msi_bp_id
                gdb_print("There are one or more breakpints already created at __inc_opcode_cnt\n"
                          "In order to use mstepi command, please delete those breakpoints first\n")
                return None
            else:
                # the existing msi bp id matches to self.msi_bp_id, it was created
                # by mstepi command previously.
                if self.msi_bp_id.enabled is False:
                    enable_msi_bp()

        self.mbp_object.set_bp_attr('thread', threadno)
        self.mbp_object.set_bp_attr('count', count)
        assert count > 0
        if self.mbp_object.ignore_count != count - 1:
            self.mbp_object.ignore_count = count - 1

        hitcnt = self.mbp_object.hit_count
        # It is important to perform a continue command here for the msi breakpoint
        # to be reached.
        m_util.gdb_exec("continue")

        """
        if the msi breakpoint's count reaches to 1, it will return True to cause gdb
        stop at msi breakpoint by previous 'continue' command.
        Once gdb stops here, a gdb 'finish' command shall be called. However,
        'finish' command must NOT be called inside of msi breakpoint stop() logic
        """
        frame = None
        if self.mbp_object.hit_count - hitcnt == count:
            """
            if gdb stops here, it must have hit one breakpoint. However. the breakpoint
            it hits may not be the msi breakpoint, it could be another breakpoint the user set up.
            In this case, we check if it is a Maple frame. If it is, then the breakpoint is
            ours. Otherwise just stop here and return because it hit a user's other stop.
            """
            frame = m_frame.get_selected_frame()
            if not frame:
                return None
            if not m_frame.is_maple_frame(frame):
                if m_set.msettings['opcode'] == 'on':
                    m_util.gdb_exec("finish")
                else:
                    silent_finish()
                # now get the new selected frame
                frame = m_frame.get_selected_frame()
                if not frame:
                    return None

        # this will update the Maple gdb runtime metadata store.
        m_datastore.mgdb_rdata.update_gdb_runtime_data()
        m_datastore.mgdb_rdata.update_frame_change_counter()

        # always disable msi breakpoint after msi is excuted
        disable_msi_bp()
        return frame

    def mstepi_common_dync(self, threadno, count):
        msi_bp_dync_exist, msi_bp_dync_id = is_msi_bp_dync_existed()
        if not msi_bp_dync_exist:
            # there is no msi bp exist, so just create a new msi breakpoint
            self.init_gdb_breakpoint(threadno, count, False, True)
        else:
            if msi_bp_dync_id != self.msi_bp_dync_id:
                # if existing msi bp id does not match to self.msi_bp_id
                gdb_print("There are one or more breakpints already created at __inc_opcode_cnt_dyn\n"
                          "In order to use mstepi command, please delete those breakpoints first\n")
                return None
            else:
                # the existing msi bp id matches to self.msi_bp_id, it was created
                # by mstepi command previously.
                if self.msi_bp_dync_id.enabled is False:
                    enable_msi_bp_dync()

        self.mbp_dync_object.set_bp_attr('thread', threadno)
        self.mbp_dync_object.set_bp_attr('count', count)
        assert count > 0
        if self.mbp_dync_object.ignore_count != count - 1:
            self.mbp_dync_object.ignore_count = count - 1

        hitcnt = self.mbp_dync_object.hit_count
        # It is important to perform a continue command here for the msi breakpoint
        # to be reached.
        m_util.gdb_exec("continue")

        """
        if the msi breakpoint's count reaches to 1, it will return True to cause gdb
        stop at msi breakpoint by previous 'continue' command.
        Once gdb stops here, a gdb 'finish' command shall be called. However,
        'finish' command must NOT be called inside of msi breakpoint stop() logic
        """
        frame = None
        if self.mbp_dync_object.hit_count - hitcnt == count:
            """
            if gdb stops here, it must have hit one breakpoint. However. the breakpoint
            it hits may not be the msi breakpoint, it could be another breakpoint the user set up.
            In this case, we check if it is a Maple frame. If it is, then the breakpoint is
            ours. Otherwise just stop here and return because it hit a user's other stop.
            """
            frame = m_frame.get_selected_frame()
            if not frame:
                return None
            if not m_frame.is_maple_frame(frame):
                if m_set.msettings['opcode'] == 'on':
                    m_util.gdb_exec("finish")
                else:
                    silent_finish()
                # now get the new selected frame
                frame = m_frame.get_selected_frame()
                if not frame:
                    return None

        # this will update the Maple gdb runtime metadata store.
        m_datastore.mgdb_rdata.update_gdb_runtime_data()
        m_datastore.mgdb_rdata.update_frame_change_counter()

        # always disable msi breakpoint after msi is excuted
        disable_msi_bp_dync()
        return frame

    def mstepi_update_and_display(self, frame):
        # this will update the Maple gdb runtime metadata store.
        m_datastore.mgdb_rdata.update_gdb_runtime_data()
        m_datastore.mgdb_rdata.update_frame_change_counter()

        ds = m_datastore.get_stack_frame_data(frame)
        if not ds:
            return
        if m_debug.Debug: m_debug.dbg_print("retrieved ds=", ds)

        if m_set.msettings['stack'] == 'on':
            m_util.gdb_exec('mlocal -stack')

        asm_path = ds['frame_func_header_info']['asm_path']
        asm_line = ds['frame_func_src_info']['asm_line']
        asm_offset = ds['frame_func_src_info']['asm_offset']

        gdb_print("asm file: %s%s%s" % (MColors.BT_SRC, asm_path, MColors.ENDC))
        f = open(asm_path, 'r')
        f.seek(asm_offset)
        gdb_print("=> %d : %s" % (asm_line, f.readline().rstrip()))
        gdb_print("   %d : %s" % (asm_line + 1, f.readline().rstrip()))
        gdb_print("   %d : %s" % (asm_line + 2, f.readline().rstrip()))
        f.close()
        m_util.gdb_exec('display')

    def set_mstepi(self, threadno, count, msi_mode):
        frame = self.mstepi_common(threadno, count)
        if not frame:
            return

        if msi_mode == self.msi_mode_default:
            self.mstepi_update_and_display(frame)
        elif msi_mode == self.msi_mode_internal:
            return
        else:
            return

    def debug(self):
        if self.mbp_object:
            self.mbp_object.display_settings()
        else:
            gdb_print("nothing\n")

class MapleFinishBreakpoint (gdb.FinishBreakpoint):
    def __init__(self):
        """ creates a finish breakpoint which is invisible to user
        """
        super().__init__(internal = True)
        self.silent = True

    def stop(self):
        """ stop when hits finish breakpoint
        """
        return True

def silent_finish():
    fbp = MapleFinishBreakpoint()
    m_util.gdb_exec("continue")

class MapleFinishCmd(gdb.Command):
    """execute until selected Maple stack frame returns
    Execute until selected Maple stack frame returns
    """

    def __init__(self):
        gdb.Command.__init__ (self,
                              "mfinish",
                              gdb.COMMAND_RUNNING,
                              gdb.COMPLETE_NONE)

    def invoke(self, args, from_tty):
        self.mfinish_func(args)

    def mfinish_func(self, args):
        frame = m_frame.get_newest_frame()
        if not frame:
            return
        silent_finish()

        frame = m_frame.get_selected_frame()
        if m_debug.Debug: m_debug.dbg_print("frame.name()=", frame.name())
        m_util.gdb_exec("msi")
        m_util.gdb_exec("mlist")
        m_datastore.mgdb_rdata.update_frame_change_counter()

class MapleStepCmd(gdb.Command):
    """step program until it reaches a different source line of Maple Application
    Step program until it reaches a different source line of Maple Application
    """

    def __init__(self):
        gdb.Command.__init__ (self,
                              "mstep",
                              gdb.COMMAND_RUNNING,
                              gdb.COMPLETE_NONE)
        m_util.gdb_exec('alias ms = mstep')

    def invoke(self, args, from_tty):
        self.mstep_func(args)

    def mstep_func(self, args):
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
            m_util.gdb_exec("msi -internal")
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

#################################################################################
### Dynamic maple stepi 
#################################################################################
def is_msi_bp_dync_existed():
    """ determines if the msi breakpoints exist or not """
    blist = gdb.breakpoints()
    for b in blist:
        if m_debug.Debug:
             m_debug.dbg_print("b.type=", b.type, "b.location=", b.location, "b.thread=", \
                     b.thread, "b.enabled=", b.enabled, "b.is_valid()=", b.is_valid())
        if '__inc_opcode_cnt_dyn' == b.location and b.is_valid():
            return True, b
    return False, None

def get_msi_bp_dync_id():
    """ gets an msi breakpoint gdb.Breakpoint object id """
    blist = gdb.breakpoints()
    for b in blist:
        if m_debug.Debug:
             m_debug.dbg_print("b.type=", b.type, "b.location=", b.location, "b.thread=", \
                     b.thread, "b.enabled=", b.enabled, "b.is_valid()=", b.is_valid())
        if '__inc_opcode_cnt_dyn' == b.location and b.is_valid():
            return b
    return None

def enable_msi_bp_dync():
    blist = gdb.breakpoints()
    for b in blist:
        if '__inc_opcode_cnt_dyn' == b.location and b.is_valid():
            b.enabled = True
            break
    return

def disable_msi_bp_dync():
    blist = gdb.breakpoints()
    for b in blist:
        if '__inc_opcode_cnt_dyn' == b.location and b.is_valid():
            b.enabled = False
            break
    return

class MapleStepiDyncBreakpoint(gdb.Breakpoint):
    """
    this class defines a Maple msi breakpoint. msi breakpoint is a breakpoint
    at symbol of __int_opcode_cnt, and can be set along with a specified running
    thread
    """

    def __init__(self, threadno, count = 1, mtype=gdb.BP_BREAKPOINT, mqualified=False):
        """
        creates an msi breakpoint with specified thread
        """
        self.settings = {}
        self.settings['symbol'] = '__inc_opcode_cnt_dyn'
        self.settings['variable'] = '__opcode_cnt_dyn'
        self.settings['count'] = count
        self.settings['thread'] = threadno

        spec = self.settings['symbol'] + ' thread ' + str(threadno)
        super().__init__(spec)
        self.msi_bp_object_id = get_msi_bp_dync_id()

    def set_bp_attr(self, key, value):
        self.settings[key] = value

    def get_bp_attr(self, key):
        return self.settings[key]

    def get_bp_id(self):
        return self.msi_bp_object_id

    def display_settings(self):
        gdb_print ("msi dync break point settings ---- ")
        gdb_print (str(self.settings))

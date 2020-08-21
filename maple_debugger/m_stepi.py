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
import m_symbol
import m_info
import m_frame
import m_datastore
from inspect import currentframe, getframeinfo
import m_util

def is_msi_bp_existed():
    """ determine if the msi breakpoints exist or not """
    blist = gdb.breakpoints()
    for b in blist:
        m_util.debug_print(__file__, getframeinfo(currentframe()).lineno, is_msi_bp_existed,\
                        "b.type=", b.type, "b.location=", b.location, "b.thread=", b.thread, "b.enabled=", b.enabled,\
                        "b.is_valid()=", b.is_valid())
        if '__inc_opcode_cnt' in b.location and b.is_valid():
            return True, b
    return False, None

def get_msi_bp_id():
    """ get a msi breakpoint gdb.Breakpoint object id """
    blist = gdb.breakpoints()
    for b in blist:
        m_util.debug_print(__file__, getframeinfo(currentframe()).lineno, get_msi_bp_id,\
                        "b.type=", b.type, "b.location=", b.location, "b.thread=", b.thread, "b.enabled=", b.enabled,\
                        "b.is_valid()=", b.is_valid())
        if '__inc_opcode_cnt' in b.location and b.is_valid():
            return b
    return None

def enable_msi_bp():
    blist = gdb.breakpoints()
    for b in blist:
        if '__inc_opcode_cnt' in b.location and b.is_valid():
            b.enabled = True
            break
    return

def disable_msi_bp():
    blist = gdb.breakpoints()
    for b in blist:
        if '__inc_opcode_cnt' in b.location and b.is_valid():
            b.enabled = False
            break
    return

class MapleStepiBreakpoint(gdb.Breakpoint):
    """
    This class defines a Maple msi breakpoint. msi breakpoint is a breakpoint
    at symbol of __int_opcode_cnt, and can be set along with a specified running
    thread
    """

    def __init__(self, threadno, count = 1, mtype=gdb.BP_BREAKPOINT, mqualified=False):
        """
        create a msi breakpoint with specified thread
        """
        self.settings = {}
        self.settings['symbol'] = '__inc_opcode_cnt'
        self.settings['variable'] = '__opcode_cnt'
        self.settings['count'] = count
        self.settings['thread'] = threadno

        spec = self.settings['symbol'] + ' thread ' + str(threadno)
        super().__init__(spec)
        self.msi_bp_object_id = get_msi_bp_id()

    def stop(self):
        # check if current frame symbol is "__inc_opcode_cnt"
        match = m_symbol.check_symbol_in_current_frame(self.settings['symbol'])
        if not match:
            return False

        # check current thread matches to what msi breakpoint is looking for
        thread_object = m_info.get_current_thread()
        if not thread_object:
            m_util.debug_print(__file__, getframeinfo(currentframe()).lineno, self.stop, "thread not valid")
            return False
        if thread_object.num != self.settings['thread']:
            m_util.debug_print(__file__, getframeinfo(currentframe()).lineno, self.stop, "thread id not match")
            return False
        m_util.debug_print(__file__, getframeinfo(currentframe()).lineno, self.stop, "thread_object=", thread_object)

        if self.settings['count'] is 1:
            m_util.debug_print(__file__, getframeinfo(currentframe()).lineno, self.stop, "count=1, stop here")
            # this will update the Maple gdb runtime metadata store.
            m_datastore.mgdb_rdata.update_gdb_runtime_data()
            return True
        else:
            m_util.debug_print(__file__, getframeinfo(currentframe()).lineno, self.stop, "count=", self.settings['count'])
            self.settings['count'] -= 1
            return False

    def set_bp_attr(self, key, value):
        self.settings[key] = value

    def get_bp_attr(self, key):
        return self.settings[key]

    def get_bp_id(self):
        return self.msi_bp_object_id

    def display_settings(self):
        print ("msi break point settings ---- ")
        for k,v in self.settings.items():
            print (k, ': ', v)


class MapleStepiCmd(gdb.Command):
    """
    This class defines Maple debugger command mstepi, alias as msi
    """

    def __init__(self):
        gdb.Command.__init__ (self,
                              "mstepi",
                              gdb.COMMAND_RUNNING,
                              gdb.COMPLETE_NONE)

        self.mbp_object = None
        self.msi_bp_id = None
        gdb.execute('alias msi = mstepi')

    def init_gdb_breakpoint(self, threadno, count):
        # create a msi breakpoint object on a specified thread
        self.mbp_object = MapleStepiBreakpoint(threadno, count)
        self.mbp_object.thread = threadno
        self.msi_bp_id = self.mbp_object.msi_bp_object_id


    def invoke(self, args, from_tty):
        gdb.execute('set pagination off', to_string = True)
        self.msi_func(args, from_tty)
        gdb.execute('set pagination on', to_string = True)

    def usage(self):
        print ("  msi : step in next Maple instruction")
        print ("  msi [n]: step in to next n Maple instructions")
        print ("  msi -abs [n]: step in to specified index n of Maple instruction")
        print ("  msi help: usuage")

    def msi_func(self, args, from_tty):
        tobject= m_info.get_current_thread()
        if not tobject:
            print ("current thread info not valid or program not started yet")
            return
        tidx = tobject.num

        s = args.split()
        if len(s) == 0: # msi into next Maple instruction
            count = 1
            self.set_mstepi(tidx, count)
        elif len(s) == 1: # msi [n]
            if s[0] == '-debug':
                self.debug()
                return
            elif s[0].isnumeric() is True:
                self.set_mstepi(tidx, int(s[0]))
            else:
                self.usage()
                return
        elif len(s) == 2: # msi -abs n
            if s[0] == '-abs' and s[1].isnumeric() :
                self.set_mstepi(tidx, int(s[1]), absOption=True)
            else:
                self.usage()
                return
        else:
            self.usage()
            return

    def set_mstepi(self, threadno, count, absOption=False):

        # check if there is existing msi point, it could be created by msi command
        # previously, or it could be created by gdb b command.
        msi_bp_exist, msi_bp_id = is_msi_bp_existed()
        if not msi_bp_exist:
            # there is no msi bp exist, so just create a new msi breakpoint
            self.init_gdb_breakpoint(threadno, count)
        else:
            if msi_bp_id != self.msi_bp_id:
                # if existing msi bp id does not match to self.msi_bp_id
                print("There are one or more breakpints already created at __inc_opcode_cnt")
                print("In order to use mstepi command, please delete those breakpoints first")
                print("")
                return
            else:
                # the existing msi bp id matches to self.msi_bp_id, it was created
                # by mstepi command previously.
                if self.msi_bp_id.enabled is False:
                    enable_msi_bp()

        self.mbp_object.set_bp_attr('thread', threadno)
        # if msi -abs option specifed, setup the correct count value for stop control
        if absOption is True:
            current_opcode_cnt = m_symbol.get_variable_value('__opcode_cnt')
            if current_opcode_cnt is None:
                print ("__opcode_cnt value not found")
                return
            else:
                current_opcode_cnt = int(current_opcode_cnt)
                if count > current_opcode_cnt:
                    count = count - current_opcode_cnt
                else:
                    count = 1
        self.mbp_object.set_bp_attr('count', count)

        # It is important to perform a continue command here for the msi breakpoint
        # to be reached.
        gdb.execute("continue")


        """
        if the msi breakpoint's count reaches to 1, it will return True to cause gdb
        stop at msi breakpoint by previous 'continue' command. 
        Once gdb stops here, a gdb 'finish' command shall be called. However,
        'finish' command must NOT be called inside of msi breakpoint stop() logic
        """
        frame = None
        if self.mbp_object.get_bp_attr('count') is 1:
            """
            If gdb stops here, it must have hit one breakpoint. However. the breakpoint
            it hits might not be the msi breakpoint, it could be the other breakpoint user set up. 
            In this case, we better check if it is a Maple frame. If it is, then the breakpoint is
            what we want, otherwise, just stop here and we just return because it hit what user
            means to stop. 
            """
            frame = m_frame.get_selected_frame()
            if not frame:
                return
            if not m_frame.is_maple_frame(frame):
                gdb.execute("finish")
                # now get the new selected frame
                frame = m_frame.get_selected_frame()
                if not frame:
                    return

        # always disable msi breakpoint after msi is excuted 
        disable_msi_bp()

        ds = m_datastore.get_stack_frame_data(frame)
        if not ds:
            return
        m_util.debug_print(__file__, getframeinfo(currentframe()).lineno, self.set_mstepi,"retrieved ds=", ds)
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

    def debug(self):
        if self.mbp_object:
            self.mbp_object.display_settings()
        else:
            print("nothing\n")

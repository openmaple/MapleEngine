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
import os
import gdb
from inspect import currentframe, getframeinfo
import m_util
import copy

msettings = {}

class MapleSetCmd(gdb.Command):
    """
    This class defines Maple command m_set to set Maple debugger's environments, and other
    settings that will be used by the Maple debugger commands.
    """

    def __init__(self):
        gdb.Command.__init__ (self,
                              "mset",
                              gdb.COMMAND_DATA,
                              gdb.COMPLETE_NONE)
        global msettings
        msettings['verbose'] = 'off'
        msettings['maple_lib_asm_path'] = []
        
    def invoke(self, args, from_tty):
        self.mset_func(args, from_tty)

    def usage(self):
        print("mset:")
        print("  -to set Maple debugger environment:")
        print("    syntax: mset <name> <value>")
        print("    example: mset verbose on")
        print("  -to add/del items in list:")
        print("    syntax:  mset -add <key name of list> <list item value>")
        print("    example: mset -add maple_lib_asm_path /home/carl/gitee/maple_engine/maple_build/out/x864")
        print("             mset -del maple_lib_asm_path /home/carl/gitee/maple_engine/maple_build/out/x864")
        print("  -to view current settings:")
        print("    syntax:  mset -show")
        print("  -to load user defined configuration file in json format:")
        print("    syntax:  mset -lconfig <configuration json file>")

    def mset_func(self, args, from_tty):
        s = str(args).split()
        global msettings
        if len(s) == 1:
            if s[0] == '-show':
                self.mshow_settings()
                return
            self.usage()
            return
        elif len(s) == 2:
            if s[0] == '-lconfig':
                data = m_util.load_maple_debugger_config(s[1])
                if data:
                    msettings = copy.deepcopy(dict(data))
                return
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
        print(msettings)

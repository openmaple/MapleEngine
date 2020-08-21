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
import os
import sys

mgdb_path=os.path.dirname(__file__)
sys.path.append(os.path.expanduser(mgdb_path))

from m_stack_cmds import *
from m_breakpoints_cmds import *
from m_symbol import *
from m_stepi import *
from m_list import *
from m_stack_data import *
from m_print import *
from m_nexti import *
from m_set import *
from m_type import *
from m_help import *

def init_alert():
    print("")
    print("Before start to use Maple debugger, please make sure to setup two sets of search paths:")
    print("1, The search paths of user's program and library source code.")
    print("   This is required to display source code of your application and library")
    print("   Use msrcpath command to show/add/del source code search paths")
    print("2, The search paths of the generated assembly files of user's program and library.")
    print("   This is required by many of Maple debugger commands")
    print("   Use mset command to show/add/del libaray asm file search paths")
    print("")
    return

def init_gdb():
    gdb.execute('set auto-load python-scripts off')

def main():
    mbt =  MapleBacktrace()
    mbreak = MapleBreakpointCmd()
    mstepi = MapleStepiCmd()
    msource = MapleSourcePathCmd()
    mlist = MapleListCmd()
    mup = MapleUpCmd()
    mdown = MapleDownCmd()
    mlocal = MapleLocalCmd()
    mprint = MaplePrintCmd()
    mnexti = MapleNextiCmd()
    mset   = MapleSetCmd()
    mtype  = MapleTypeCmd()
    mhelp  = MapleHelpCmd()

    init_alert()
    init_gdb()

if __name__ == "__main__":
    main()

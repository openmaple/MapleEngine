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
import sys

mgdb_path=os.path.dirname(__file__)
sys.path.append(os.path.expanduser(mgdb_path))

from m_stack_cmds import MapleBacktrace, MapleUpCmd, MapleDownCmd
from m_bp_cmds import MapleBreakpointCmd
from m_stepi import MapleStepiCmd,MapleFinishCmd
from m_list import MapleListCmd, MapleSourcePathCmd
from m_stack_data import MapleLocalCmd
from m_print import MaplePrintCmd
from m_nexti import MapleNextiCmd
from m_set import MapleSetCmd
from m_type import MapleTypeCmd, MapleSymbolCmd
from m_help import MapleHelpCmd
import m_util
from m_util import gdb_print

def init_alert():
    gdb_print("")
    gdb_print("Before start to use Maple debugger, please make sure to setup two sets of search paths:")
    gdb_print("1, The search paths of user's program and library source code.")
    gdb_print("   This is required to display source code of your application and library")
    gdb_print("   Use msrcpath command to show/add/del source code search paths")
    gdb_print("2, The search paths of the generated assembly files of user's program and library.")
    gdb_print("   This is required by many of Maple debugger commands")
    gdb_print("   Use mset command to show/add/del libaray asm file search paths")
    gdb_print("")
    return

def init_gdb():
    m_util.gdb_exec('set auto-load python-scripts off')
    m_util.enable_color_output(True)

def main():
    # Register commands for Maple debugger
    MapleBacktrace()
    MapleBreakpointCmd()
    MapleStepiCmd()
    MapleSourcePathCmd()
    MapleListCmd()
    MapleUpCmd()
    MapleDownCmd()
    MapleLocalCmd()
    MaplePrintCmd()
    MapleNextiCmd()
    MapleSetCmd()
    MapleTypeCmd()
    MapleHelpCmd()
    MapleSymbolCmd()
    MapleFinishCmd()

    init_alert()
    init_gdb()

if __name__ == "__main__":
    main()

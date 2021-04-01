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
import os
import sys

mgdb_path=os.path.dirname(__file__)
sys.path.append(os.path.expanduser(mgdb_path))

from m_stack_cmds import MapleBacktrace, MapleUpCmd, MapleDownCmd
from m_bp_cmds import MapleBreakpointCmd
from m_stepi import MapleStepiCmd,MapleFinishCmd,MapleStepCmd
from m_list import MapleListCmd, MapleSourcePathCmd
from m_stack_data import MapleLocalCmd
from m_print import MaplePrintCmd
from m_nexti import MapleNextiCmd,MapleNextCmd
from m_set import MapleSetCmd
from m_type import MapleTypeCmd, MapleSymbolCmd
from m_help import MapleHelpCmd
import m_util
import m_set
import m_event

def init_alert():
    m_util.gdb_print("\n"
        "Before using the Maple debugger, please setup two sets of search paths:\n"
        "1, Search path for the user's program and library source code.\n"
        "   This is required to display source code of your application and library\n"
        "   Use the 'msrcpath' command to show/add/del source code search paths\n"
        "2, Search path of the generated assembly files of user's program and library.\n"
        "   This is required by many of the Maple debugger commands\n"
        "   Use the 'mset' command to show/add/del library asm file search paths\n")

def debugger_exception_handler(etype, evalue, etraceback):
    """ Exception handler for Maple debugger
    """
    import traceback
    if type(evalue) is KeyboardInterrupt:
        gdb.write('Interrupted by user.\n', gdb.STDERR)
    elif type(evalue) is gdb.error:
        gdb.write('gdb.error occured.\n', gdb.STDERR)
    else:
        gdb.write('Warning, an uncaught exception occured. '
                  'Enable trace with "mset trace on" for more details.\n', gdb.STDERR)
        traceback.print_exception(etype, evalue, etraceback)
        if not m_util.is_interactive():
            m_util.gdb_exec('quit')
    # Resume trace setting if it has been set
    if sys.getprofile() == m_set.trace_maple_debugger:
        sys.setprofile(m_set.trace_maple_debugger)

def init_gdb():
    m_util.gdb_exec('set auto-load python-scripts off')
    m_util.enable_color_output(m_util.is_interactive())
    sys.excepthook = debugger_exception_handler
    m_event.init_event_handlers()

def main():
    ver = gdb.VERSION.split('.')
    if int(ver[0]) * 100 + int(ver[1]) < 801:
        m_util.gdb_print( \
            "Current GDB version is %s.\n"
            "Maple debugger requires GDB version 8.1 and later.\n"
            "Warning: Maple debugger is not enabled due to unsupported GDB version."
            % gdb.VERSION)
        return

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
    MapleStepCmd()
    MapleNextCmd()

    init_alert()
    init_gdb()

if __name__ == "__main__":
    main()

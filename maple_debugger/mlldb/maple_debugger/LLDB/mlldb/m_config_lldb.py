#
# Copyright (C) [2020-2021] Futurewei Technologies, Inc. All rights reserved.
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

from shared import m_util
import sys
import lldb

x = 0            # Default value of the 'x' configuration setting
Debug = False    # Default value for debugging
debug = False    # Default value for debugging
flags = [0, 1, 0, 1, 0, 1, 0]
comms = ""

# For use in registration - command script import ...
plugins = [ 'm_break', 'm_help', 'm_list', 'm_breakpoint', 'm_set', 'm_type', 'm_stack_cmds', 'lldb_module_utils']
plugdbug = [ 'False',  'False',  'False',  'False',        'False', 'True',   'True',         'False']
#plugins = [ 'lldb_module_utils', 'm_stack_cmds', 'm_break', 'm_list', 'm_nexti', 'm_set', 'm_print', 'm_help', 'm_stepi', 'm_type']
#pdbg = [ 'm_breakpointcmd.py', 'm_listcmd.py', 'm_sourcepathcmd.py', 'm_nextcmd.py', 'm_nexticmd.py', 'm_setcmd.py', 'm_printcmd.py', 'm_upcmd.py', 'm_helpcmd.py', 'm_downcmd.py', 'm_localcmd.py', 'm_stepcmd.py', 'm_stepicmd.py', 'm_finishcmd.py', 'm_typecmd.py', 'm_symbolcmd.py']


def dbg_print(*args, **kwargs):
    """
    Maple debugger's verbose mode print, aka the debugger prints of Maple debugger
    """
    if not Debug:
        return

    #filename = "filen"
    #func = "funcn"
    #line = "line#"
    #callerframe = lldb.frame

    callerframe = sys._getframe(1)
    co = callerframe.f_code
    filename = co.co_filename.split('/')[-1]
    func = co.co_name
    line = callerframe.f_lineno

    #Iterate over all args, convert them to str, and join them
    args_str = ' '.join(map(str,args))

    #Iterater over all kwargs, convert them into k=v and join them
    kwargs_str = ' '.join('{}={}'.format(k,v) for k,v in kwargs.items())

    #Form the final representation by adding func name
    buffer = "{}:{}():{}: {}".format(filename, func, line, ' '.join([args_str, kwargs_str]))
    #m_util.mdb_print(buffer)
    print(buffer)

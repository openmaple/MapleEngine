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

import sys
from m_util import gdb_print

Debug = False

def dbg_print(*args, **kwargs):
    """
    Maple debugger's verbose mode print, aka the debugger prints of Maple debugger
    """
    if not Debug:
        return

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
    gdb_print(buffer)

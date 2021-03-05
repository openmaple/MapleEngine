#
# Copyright (C) [2021] Futurewei Technologies, Inc. All rights reserved.
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
import m_debug

def is_inferior_running():
    inf = gdb.selected_inferior()
    """
    if m_debug.Debug:
        m_debug.dbg_print("inf.is_valid()=", inf.is_valid())
        m_debug.dbg_print("inf.num=", inf.num)
        m_debug.dbg_print("inf.pid=", inf.pid)
        m_debug.dbg_print("inf.was_attached=", inf.was_attached)
        m_debug.dbg_print("inf.threads()=", inf.threads(), "len(theads()=", len(inf.threads()))
    """

    if inf.pid != 0 and len(inf.threads()) > 0:
        return True
    else:
        return False

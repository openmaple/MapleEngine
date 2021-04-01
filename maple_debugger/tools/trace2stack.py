#!/usr/bin/env python3
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

import sys
import os.path

def generate_trace_stack(filename):
    """
    Generate data for trace stack which can be used to get flame graph

    Trace format:
    + ==> m_breakpoint.py:128, stop, @(<string>:6), {arg#1, <m_breakpoint.MapleBreakpoint obj...>}
    + + ==> m_util.py:31, gdb_exec_to_str, @(m_breakpoint.py:140), {arg#1, info b}
    - - <-- m_util.py:32, gdb_exec_to_str, 66us, ret: Num...\n
    + + ==> m_breakpoint.py:41, is_maple_invoke_bp_plt_disabled, @(m_breakpoint.py:141), {arg#1, Num...\n}
    - - <-- m_breakpoint.py:56, is_maple_invoke_bp_plt_disabled, 7us, ret: True
    + + ==> m_symbol.py:62, get_symbol_addr_by_current_frame_args, @(m_breakpoint.py:160), {arg#0}
    + + + ==> m_util.py:31, gdb_exec_to_str, @(m_symbol.py:68), {arg#1, p/x *(long long*)&mir_header}
    - - - <-- m_util.py:32, gdb_exec_to_str, 57us, ret: $105 = 0x7fffea8cae28\n
    - - <-- m_symbol.py:73, get_symbol_addr_by_current_frame_args, 85us, ret: 0x7fffea8cae28
    - <-- m_breakpoint.py:162, stop, 242us, ret: False
    """
    with open(filename, 'r') as trace_file:
        trace_lines = trace_file.readlines()

    print("# git clone https://github.com/brendangregg/FlameGraph.git")
    print("# cat this-file | FlameGraph/stackcollapse.pl > trace.folded")
    print("# FlameGraph/flamegraph.pl trace.folded > trace.svg")
    fstack = [('bottom', '')]
    etime = [-1]
    for line in trace_lines:
        buf = line.replace('\n',' ').split(' ')
        if len(buf) < 4:
            continue
        if buf[0] == "+":
            for idx in range(len(buf)):
                if buf[idx] == "==>":
                    func_name = buf[idx + 2][0:-1]
                    gdbarg = ' '.join(buf[idx + 4:]) if func_name.startswith('gdb_e') else ''
                    fstack.append((buf[idx + 1] + " " + func_name, gdbarg))
                    etime.append(0)
                    ontop = True
                    break
        if buf[0] == "-":
            for idx in range(len(buf)):
                if buf[idx] == "<--":
                    if idx >= len(fstack):
                        sys.stderr.write("Corrupted trace output.")
                        return
                    total_time = int(buf[idx + 3][0:-3], 10)
                    if idx > 1:
                        etime[idx - 1] += total_time
                    ontop = None if ontop else print("_.self._")
                    for i in range(idx, 0, -1):
                        print(' '.join(fstack[i]))
                    print(total_time - etime.pop())
                    fstack.pop()
                    break

if __name__ == "__main__":

    if len(sys.argv) < 2:
        print("Usage: " + sys.argv[0] + " <trace-file>")
        print("       -- Generate the data for flame graph")
        sys.exit(1)

    if not os.path.isfile(sys.argv[1]):
        print("File " + sys.argv[1] + " does not exists")
        sys.exit(1)

    generate_trace_stack(sys.argv[1])

#
# Copyright (C) [2020-2021] Futurewei Technologies, Inc. All rights reserved.
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

import pickle
from shared import m_util
from mlldb import m_config_lldb as m_debug

def create_mirmpl_label_cache(mir_path, popen_only = False, proc_saved = {}):
    """ Create a dictionary for a given mir file
        When it is called the first time with a given mir file, it starts
        a subprocess to handle the mir file and returns None
        When called subsequently using the same mir file, it will wait for the
        first subprocess to finish and return a dictionary created from its output
    """
    if not mir_path:
        for p in proc_saved.values():
            m_util.proc_communicate(p)
        return None
    if mir_path not in proc_saved:
        cmd="bash -c \"paste <(grep -bn '^func &' '" + mir_path \
            + "' | cut -d' ' -f1,2 | sed -e 's/func &//' -e 's/$/:/') <(grep -b '^}' '" + mir_path \
            + "' | cut -d: -f1)\" | awk -F: -v a='\"' -v b='\":(' -v c=',' -v d='),' " \
              "'BEGIN { print \"import pickle\\nimport sys\\nd={\" } " \
              "{ print a$3b$1c$2c$4d } END { print \"}\\no=pickle.dumps(d,3)\\n" \
              "sys.stdout.write(o.decode(\\\"latin1\\\"))\" }' | python3"
        proc_saved[mir_path] = m_util.proc_popen(cmd)
    if popen_only: return None
    d = m_util.proc_communicate(proc_saved.pop(mir_path))
    return pickle.loads(d)

def lookup_mirmpl_info(mirmpl_path, mirmpl_line_tuple, mir_instidx):
    """ in file mirmpl_path, and mirmpl_line_tuple,  for a given func_addr_offset_pc, lookup the
        matching line's line-num and its offset
        params: mirmpl: string. mirmpl file full path.
                mirmpl_line_tuple: a tuple for a Maple symbol.
                                    tuple[0] = mirmpl symbol block line number
                                    tuple[1] = mirmpl symbol block start offset
                                    tuple[2] = mirmpl symbol block end offset
                mir_instidx: string. mir_instidx offset_pc e.g. 0008

        return:
            a dict {'mirmpl_line': int, 'mirmpl_line_offset': int}
            where mirmple_line: func_addr_offset_pc found at this line, and at this mirmpl file offset
    """
    if m_debug.Debug: m_debug.dbg_print("mirmpl_path=", mirmpl_path, "mirmpl_line_tuple=", mirmpl_line_tuple, "mir_instidx=", mir_instidx)

    offset = mirmpl_line_tuple[1]
    line_num = mirmpl_line_tuple[0]
    end_offset = mirmpl_line_tuple[2]

    if not mir_instidx:
        # return this symbol block's first address
        return {'mirmpl_line': line_num, 'mirmpl_line_offset': offset}

    f = open(mirmpl_path, "r")
    f.seek(offset)
    while offset < end_offset:
        line = f.readline()
        if mir_instidx in line:
            if m_debug.Debug: m_debug.dbg_print("found ", mir_instidx, " at line", line_num, "line=", line)
            f.close()
            return {'mirmpl_line': line_num, 'mirmpl_line_offset': offset}
        else:
            line_num += 1
            offset += len(line)

    f.close()

    return None

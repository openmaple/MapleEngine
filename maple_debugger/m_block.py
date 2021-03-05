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

def get_block(frame):
    '''
    :param frame: a gdb.Frame object
    :returns: a gdb.Block object or None
    '''
    if frame is None:
        return None
    if not frame.is_valid():
        return None

    try:
        block = frame.block()
    except :
        return None
    else:
        pass

    if not block.is_valid():
        return None
    else :
        return block

def get_block_from_pc(pc):
    '''
    :param pc: frame's resume address
    :type pc: gdb.Frame.pc()
    :returns: a gdb.Block object or None
    '''
    return gdb.block_for_pc(pc)


def get_block_info(frame):
    '''
    :description: get block info for specified gdb frame
    :param frame:  a gdb.Frame object
    :type frame: a gdb.Frame object
    :returns: a string with block info including block start, block end, block func
    '''
    block = get_block(frame)
    if block is None:
        return "block info: None"
    buf = "block info: start 0x%x, end 0x%x, func %s" % (block.start, block.end, block.function)
    return buf


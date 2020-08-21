#!/usr/bin/env python3
import gdb
from inspect import currentframe, getframeinfo
import m_util

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


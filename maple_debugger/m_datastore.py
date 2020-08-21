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
import copy
import m_frame
import m_asm_interpret
import m_symbol
from inspect import currentframe, getframeinfo
import m_util

class MapleClassDefInfo():
    """
    class MapleClassDefInfo creates a cache of class inheritance hierarchy, and provides
    management of this cache, so that they can be referenced very quickly.

    The cache is created based on libraries' coresponding .macros.def files, and will
    only be created once at few trigger points
    """

    def __init__(self):
        """
        initializes two internal data structures.
        1, maple_class_def_cache as a dict
        2, .macros.def files this object reads class inheritance hierarchy from. a list

        dict of maple_class_def_cache:
          key: a string, class name.
          value: a dict, class details
            class detail dict:
              {
                'size': int. class size
                'base_class': string. the base class it is derived from.
                'fields': a list, items of the list is a dict too.
                  {
                    'name': string.
                    'offset': int. field offset in the class
                    'length': int. field length.
                  }
              }
        """
        self.maple_class_def_cache = {}
        self.def_paths = []

    def create_maple_class_def_cache(self, def_full_path):
        f = open(def_full_path, 'r')
        class_name = None
        base_class_name = None
        class_size = None
        field_name = None
        filed_offset = None
        filed_length = None
        filed_dict = None
        class_count = 0
        class_field_count = 0
        line = None
        for line in f:
            if '__MRT_CLASS(' in line:
                buf = line.split(', ')
                class_name = buf[0][12:]
                class_size = int(buf[1])
                base_class_name = buf[-1].rstrip()[:-1]
                self.maple_class_def_cache[class_name] = {}
                self.maple_class_def_cache[class_name]['size'] = class_size
                self.maple_class_def_cache[class_name]['base_class'] = base_class_name
                self.maple_class_def_cache[class_name]['fields'] = []
                class_count += 1
            elif '__MRT_CLASS_FIELD(' in line:
                buf = line.split(', ')
                class_name = buf[0][18:]
                field_name = buf[1]
                field_offset = int(buf[2])
                field_length = int(buf[-1].rstrip()[:-1])
                if class_name in self.maple_class_def_cache:
                    field_dict = {'name': field_name, 'offset': field_offset, 'length': field_length}
                    self.maple_class_def_cache[class_name]['fields'].append(field_dict)
                    class_field_count += 1

        f.close()

        self.def_paths.append(def_full_path)

    def add_one_maple_class_def(self, class_name, value):
        self.maple_class_def_cache[class_name] = value

    def get_maple_class_def(self, class_name):
        if class_name in self.maple_class_def_cache:
            m_util.debug_print(__file__, getframeinfo(currentframe()).lineno, self.get_maple_class_def, \
                                "class_name", class_name, "found")
            m_util.debug_print(__file__, getframeinfo(currentframe()).lineno, self.get_maple_class_def,\
                         "return maple_class_def_cache[class_name]:", self.maple_class_def_cache[class_name])
            return self.maple_class_def_cache[class_name]

        m_util.debug_print(__file__, getframeinfo(currentframe()).lineno, self.get_maple_class_def, \
                        "class_name", class_name, "Not found")
        return None

    def get_maple_class_def_list(self):
        """ return a list of class names in the cache """
        return [*self.maple_class_def_cache]


class MapleMirbinInfoIndex():
    """
    class MapleMirbinInfoIndex creates a cache of all Maple symbol block information
    for quick reference.

    The cache is created based on libraries' coresponding .s files or .VtableImplt.s files
    The cache will only be created once at few trigger points whichever happens first
    """

    def __init__(self):
        """
        initialize mirbin_info_cache as a empty dict.
        Key: .s file full path name or .VtableImpl.s file full path name coresponding to .so file used.
        Value: a dict with following format
          key: Maple symbol
          value: a tuple, (symbol-line-num, symbol-block-offset, symbol-block-end-offset).

        example of mirbin_info_cache:
          mirbin_info_cache['libcore.s'] =
          {
            'Ljava_2Flang_2FClass_3B_7CisArray_7C_28_29Z_mirbin_info' : (100, 5397, 5409)
            'Ljava_2Flang_2FStringBuffer_3B_7CsetLength_7C_28I_29V_mirbin_info': (200, 6000,7000)
          }
          mirbin_info_cache['MyApplication.s'] =
          {
            'LTypeTest_3B_7CshowLongList_7C_28AJ_29V_mirbin_info': (300, 6088,7020)
          }

        """
        self.mirbin_info_cache = {}
        
    def init_new_asm(self, asm):
        if not asm in self.mirbin_info_cache:
            self.mirbin_info_cache[asm] = {}

    def add_new_asm_item(self, asm, k, v):
        self.mirbin_info_cache[asm][k] = v

    def assign_new_asm(self, asm, d):
        self.init_new_asm(asm)
        self.mirbin_info_cache[asm] = copy.deepcopy(dict(d))

    def has_key(self, asm):
        r = asm in self.mirbin_info_cache
        return r

    def get_asm_mirbin_item(self, asm, mirbin_name):
        if not asm in self.mirbin_info_cache:
            m_util.debug_print(__file__, getframeinfo(currentframe()).lineno, self.get_asm_mirbin_item, \
                                "asm", asm, "not found in mirbin info cache ")
            return None
        if not mirbin_name in self.mirbin_info_cache[asm]:
            m_util.debug_print(__file__, getframeinfo(currentframe()).lineno, self.get_asm_mirbin_item, \
                                "Maple label", mirbin_name, "not found in mirbin info cache ")
            return None

        m_util.debug_print(__file__, getframeinfo(currentframe()).lineno, self.get_asm_mirbin_item, \
                            "return ", self.mirbin_info_cache[asm][mirbin_name])
        return self.mirbin_info_cache[asm][mirbin_name]


class MapleGDBRuntimeData():
    """
    class MapleGDBRuntimeData is an object at runtime, create and manage
    Maple symbol caches (mirbin_info_cache) and class inheritance hierarchy.

    This class provides api for other module to dynamically update the cache
    at runtime.
    """

    def __init__(self):
        """
        create two cache object instance, one for Maple symbol cache,
        one for class inheritance hierarchy
        """
        self.mirbin_info = MapleMirbinInfoIndex()
        self.class_def = MapleClassDefInfo()
        
    #####################################
    # api for mirbin_info_cache
    #####################################
    def create_mirbin_info_cache(self, asm_file):
        if self.mirbin_info_cache_has_key(asm_file):
            return

        self.mirbin_info.init_new_asm(asm_file)

        d = m_asm_interpret.create_asm_mirbin_label_cache(asm_file)
        self.mirbin_info_cache_assign(asm_file, d)

        count = len(self.mirbin_info.mirbin_info_cache[asm_file].keys())
        m_util.debug_print(__file__, getframeinfo(currentframe()).lineno, self.create_mirbin_info_cache, \
                            "asm file ", asm_file, count, "labels found")

    def get_one_label_mirbin_info_cache(self, asm, label):
        return self.mirbin_info.get_asm_mirbin_item(asm, label)
    
    def mirbin_info_cache_has_key(self, asm):
        return self.mirbin_info.has_key(asm)

    def mirbin_info_cache_assign(self, asm, d):
        self.mirbin_info.assign_new_asm(asm, dict(d))

    #####################################
    # api for class_def_cache
    #####################################
    def create_class_def(self, def_full_path):
        self.class_def.create_maple_class_def_cache(def_full_path)

    def add_class_def(self, class_name, value):
        self.class_def.add_one_maple_class_def(class_name, value)

    def get_class_def(self, class_name):
        return self.class_def.get_maple_class_def(class_name)

    def get_class_def_list(self):
        return self.class_def.get_maple_class_def_list()


    #####################################
    # advanced api for other module to use
    #####################################
    def update_gdb_runtime_data(self):

        addr_offset, so_path, asm_path, func_header, frame  = m_frame.get_closest_maple_frame_func_lib_info()
        m_util.debug_print(__file__, getframeinfo(currentframe()).lineno, self.update_gdb_runtime_data, "addr_offset", addr_offset)
        m_util.debug_print(__file__, getframeinfo(currentframe()).lineno, self.update_gdb_runtime_data, "so_path", so_path)
        m_util.debug_print(__file__, getframeinfo(currentframe()).lineno, self.update_gdb_runtime_data, "asm_path", asm_path)
        m_util.debug_print(__file__, getframeinfo(currentframe()).lineno, self.update_gdb_runtime_data, "func_header", func_header)
        if not addr_offset or not so_path or not asm_path or not func_header or not frame:
            return

        if not self.mirbin_info_cache_has_key(asm_path):
            print("create Maple symbol cache using file: ", asm_path)
            self.create_mirbin_info_cache(asm_path)

        def_file_path = asm_path[:-1] + 'macros.def' if 'VtableImpl' in asm_path else asm_path[:-1] + 'VtableImpl.macros.def'
        if not def_file_path in self.class_def.def_paths:
            print("create def cache using file: ", def_file_path)
            self.create_class_def(def_file_path)


mgdb_rdata =  MapleGDBRuntimeData()


def get_stack_frame_data(frame):
    """
    For a given Maple stack frame, get all finds of frame data including
    1, function header info
    2, function source code information
    3, function local variable and arugment informatiion.

    params:
      frame: a gdb.frame object. A SELECTED Maple frame from current stack.

    returns:
       a dict contains following keys and values
        'frame_func_src_info':  whatever m_asm_interpret.look_up_src_file_info() returns
                {'short_src_file_name': a string. source code short name
                 'short_src_file_line': a int. source code file line
                 'asm_line': a int. line number where func_header_name offset is found in asm file
                 'asm_offset': a int. offset in the asm file co-repsonding to asm_line
                }
        'frame_func_header_info':
                {'func_name': func_name, string.
                 'so_path': so_path full path, a string
                 'asm_path': asm_path full path,a string
                 'func_addr_offset': func_addr_offset, string in xxxx:yyyy: format
                 'func_header_name' : func_header_name, a string. e.g xxxxxxxxxxx_mirbin_info
                 'func_header_asm_tuple': func_header_name_block_asm_tuple, (asm line, asm_start_offset, asm_end_offset)
                }
        'func_argus_locals': same return as m_asm_interpret.get_func_arguments() returns
                {'locals_type': local variable type list, e.g ['void', 'v2i64'],
                 'locals_name': local variable name list['%%retval', '%%thrownval'],
                 'formals_type': func argument type list. e.g ['a64'],
                 'formals_name': func argument name list. e.g ['%1']
                }
    """
    m_util.debug_print(__file__, getframeinfo(currentframe()).lineno, get_stack_frame_data,\
                     "==== get_stack_frame_data =====")

    func_addr_offset, so_path, asm_path, func_header, frame = m_frame.get_maple_frame_func_lib_info(frame)
    m_util.debug_print(__file__, getframeinfo(currentframe()).lineno, get_stack_frame_data,\
                        "func_addr_offset =", func_addr_offset, "func_header=", func_header)
    m_util.debug_print(__file__, getframeinfo(currentframe()).lineno, get_stack_frame_data,\
                        "so_path =", so_path, "asm_path=", asm_path)
    if not func_addr_offset or not so_path or not asm_path or not func_header:
        return None

    ### get the function label of the frame
    label_addr, func_header_name = m_symbol.get_symbol_name_by_current_frame_args()
    m_util.debug_print(__file__, getframeinfo(currentframe()).lineno, get_stack_frame_data,\
                        "func_header_name=", func_header_name)
    if func_header_name:
        func_name = func_header_name[:-12]
        m_util.debug_print(__file__, getframeinfo(currentframe()).lineno, get_stack_frame_data,\
                             "func_name=", func_name)
    else:
        label_addr, func_header_name = m_symbol.get_symbol_name_by_current_func_args()
        func_name = func_header_name[:-12]
        m_util.debug_print(__file__, getframeinfo(currentframe()).lineno, get_stack_frame_data,\
                             "func_name=", func_name)
    
    m_util.debug_print(__file__, getframeinfo(currentframe()).lineno, get_stack_frame_data,\
                        "func_header_name=", func_header_name)
    if not func_header_name:
        return None

    # if mirbin_info_cache for this asm file is not created yet in m_datastore, create it
    # if not m_datastore.mgdb_rdata.mirbin_info_cache_has_key(asm_path):
    if not mgdb_rdata.mirbin_info_cache_has_key(asm_path):
        m_util.debug_print(__file__, getframeinfo(currentframe()).lineno, get_stack_frame_data,\
                         "create mirbin info cache for asm file", asm_path)
        mgdb_rdata.create_mirbin_info_cache(asm_path)
    else:
        m_util.debug_print(__file__, getframeinfo(currentframe()).lineno, get_stack_frame_data,\
                         "mirbin info cache for ", asm_path, "is already created")
        pass


    asm_line_tuple = mgdb_rdata.get_one_label_mirbin_info_cache(asm_path, func_header_name)
    if not asm_line_tuple:
        return None
    m_util.debug_print(__file__, getframeinfo(currentframe()).lineno, get_stack_frame_data,\
                        "asm_line_tuple=", asm_line_tuple)
    m_util.debug_print(__file__, getframeinfo(currentframe()).lineno, get_stack_frame_data,\
                        "func_addr_offset=", func_addr_offset)
    m_util.debug_print(__file__, getframeinfo(currentframe()).lineno, get_stack_frame_data,\
                        "func_addr_offset.split(':')[1]=", func_addr_offset.split(':')[1])
    func_addr_offset_pc = func_addr_offset.split(':')[1]
    d = m_asm_interpret.look_up_src_file_info(asm_path, asm_line_tuple[0], asm_line_tuple[1], asm_line_tuple[2],\
                                              func_addr_offset_pc)
    if not d:
        m_util.debug_print(__file__, getframeinfo(currentframe()).lineno, get_stack_frame_data,\
                            "look_up_src_file_info returns None")
        return None
    m_util.debug_print(__file__, getframeinfo(currentframe()).lineno, get_stack_frame_data,\
                        "first round returns dict d:", d)

    # In some cases, when func_addr_offset's pc partion is 0000, the source code file information can be
    # missing. In this case, we can check few more lines from its reported asm line ONLT for pc = '0000' situation
    if not d['short_src_file_name'] and func_addr_offset_pc == '0000' and d['asm_offset'] != 0 and d['asm_line'] != 0:
        m_util.debug_print(__file__, getframeinfo(currentframe()).lineno, get_stack_frame_data,\
                            "func offset pc is 0000, check short_src_file again")
        d['short_src_file_name'], d['short_src_file_line'] = \
            m_asm_interpret.look_up_next_src_file_info(asm_path, d['asm_line'], d['asm_offset'])
        m_util.debug_print(__file__, getframeinfo(currentframe()).lineno, get_stack_frame_data,\
                        "second round for func offset pc 0000 returns dict d:", d)

    asm_line_offset = asm_line_tuple[1]
    asm_line_num = d['asm_line']
    src_file_short_name = d['short_src_file_name']
    src_file_line = d['short_src_file_line']

    if not asm_line_offset:
        return None

    ### get the method arguments and local variables
    func_argus_locals = m_asm_interpret.get_func_arguments(asm_path, func_header_name, asm_line_offset)
    m_util.debug_print(__file__, getframeinfo(currentframe()).lineno, get_stack_frame_data,\
                        "func_argus_locals:", func_argus_locals)
    if not func_argus_locals:
        return None

    rdata = {}
    rdata['frame_func_src_info']    = d
    rdata['frame_func_header_info'] = { 'so_path': so_path, \
                                        'asm_path': asm_path, \
                                        'func_addr_offset': func_addr_offset, \
                                        'func_name': func_name, \
                                        'func_header_name' : func_header_name, \
                                        'func_header_asm_tuple': asm_line_tuple \
                                      }
    rdata['func_argus_locals']     =  func_argus_locals

    return rdata

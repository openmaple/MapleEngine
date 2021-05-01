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
import m_frame
import m_asm
import m_mirmpl
import m_symbol
import m_debug
from m_util import gdb_print

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
            if m_debug.Debug:
                m_debug.dbg_print("class_name", class_name, "found")
                m_debug.dbg_print("return maple_class_def_cache[class_name]:", self.maple_class_def_cache[class_name])
            return self.maple_class_def_cache[class_name]

        if m_debug.Debug: m_debug.dbg_print("class_name", class_name, "Not found")
        return None

    def get_maple_class_def_list(self):
        """ return a sorted list of class names in the cache """
        relist = [*self.maple_class_def_cache]
        relist.sort()
        return relist


class MapleMirbinInfoIndex():
    """
    class MapleMirbinInfoIndex creates a cache of all Maple symbol block information
    for quick reference.

    The cache is created based on libraries' coresponding .s files or .VtableImplt.s files
    The cache will only be created once, at a few trigger points, whichever happens first
    """

    def __init__(self):
        """
        initialize mirbin_info_cache as a empty dict.
        Key: .s file full path name or .VtableImpl.s file full path name coresponding to .so file used.
        Value: A dict with following format
          key: Maple symbol
          value: A tuple, (symbol-line-num, symbol-block-offset, symbol-block-end-offset).

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
        self.mirbin_info_cache[asm] = dict(d)

    def has_key(self, asm):
        r = asm in self.mirbin_info_cache
        return r

    def get_asm_mirbin_item(self, asm, mirbin_name):
        if not asm in self.mirbin_info_cache:
            if m_debug.Debug: m_debug.dbg_print("asm", asm, "not found in mirbin info cache ")
            return None
        if not mirbin_name in self.mirbin_info_cache[asm]:
            if m_debug.Debug: m_debug.dbg_print("Maple label", mirbin_name, "not found in mirbin info cache ")
            return None

        if m_debug.Debug: m_debug.dbg_print("return ", self.mirbin_info_cache[asm][mirbin_name])
        return self.mirbin_info_cache[asm][mirbin_name]

    def get_asm_list(self):
        relist = [*self.mirbin_info_cache]
        relist.sort()
        return relist

    def get_symbol_list(self, asm_path):
        relist = [*self.mirbin_info_cache[asm_path]]
        relist.sort()
        return relist

class MapleMirmplInfoIndex():
    """
    class MapleMirmplInfoIndex creates a cache of all Maple symbol block information
    for quick reference.

    The cache is created based on libraries' coresponding .mpl.mir.mpl files
    The cache will only be created once, at a few trigger points, whichever happens first
    """

    def __init__(self):
        """
        initialize mirmpl_info_cache as a empty dict.
        Key: .mpl.mir.mpl file full path name coresponding to .so file used.
        Value: A dict with following format
          key: Maple symbol
          value: A tuple, (symbol-line-num, symbol-block-offset, symbol-block-end-offset).

        example of mirmpl_info_cache:
          mirmpl_info_cache['libcore.mpl.mir.mpl'] =
          {
            'Ljava_2Flang_2FClass_3B_7CisArray_7C_28_29Z' : (100, 5397, 5409)
            'Ljava_2Flang_2FStringBuffer_3B_7CsetLength_7C_28I_29V': (200, 6000,7000)
          }
          mirmpl_info_cache['MyApplication.mpl.mir.mpl'] =
          {
            'LTypeTest_3B_7CshowLongList_7C_28AJ_29V': (300, 6088,7020)
          }

        """
        self.mirmpl_info_cache = {}

    def init_new_mirmpl(self, mirmpl):
        if not mirmpl in self.mirmpl_info_cache:
            self.mirmpl_info_cache[mirmpl] = {}

    def add_new_mirmpl_item(self, mirmpl, k, v):
        self.mirmpl_info_cache[mirmpl][k] = v

    def assign_new_mirmpl(self, mirmpl, d):
        self.init_new_mirmpl(mirmpl)
        self.mirmpl_info_cache[mirmpl] = dict(d)

    def has_key(self, mirmpl):
        return mirmpl in self.mirmpl_info_cache

    def get_mirmpl_item(self, mirmpl, symbol):
        if not mirmpl in self.mirmpl_info_cache:
            if m_debug.Debug: m_debug.dbg_print("mirmpl", mirmpl, "not found in mirmpl info cache ")
            return None
        if not symbol in self.mirmpl_info_cache[mirmpl]:
            if m_debug.Debug: m_debug.dbg_print("Maple label", symbol, "not found in mirmpl_info_cache[",mirmpl, "]" )
            return None

        if m_debug.Debug: m_debug.dbg_print("return ", self.mirmpl_info_cache[mirmpl][symbol])
        return self.mirmpl_info_cache[mirmpl][symbol]

    def get_mirmpl_list(self):
        relist = [*self.mirmpl_info_cache]
        relist.sort()
        return relist

    def get_symbol_list(self, mirmpl):
        relist = [*self.mirmpl_info_cache[mirmpl]]
        relist.sort()
        return relist

class MapleFileCache():
    def __init__(self):
        self.general_fullpath = {}
        self.src_file_lines   = {}
        self.asm_block_lines  = {}
        self.mir_block_lines  = {}

    # api for general file short name, path and its combined fullpath.
    def in_general_fullpath(self, name, path):
        return (name, path) in self.general_fullpath

    def add_general_fullpath(self, name, path, fullpath):
        self.general_fullpath[(name, path)] = fullpath

    def get_general_fullpath(self, name, path):
        return self.general_fullpath[(name, path)]

    def del_general_fullpath(self):
        self.general_fullpath.clear()

    def show_general_fullpath(self):
        gdb_print("mgdb_rdata.general_fullpath cache ==== ")
        gdb_print(str(self.general_fullpath))

    # api for source code file info cache
    def in_src_file_lines(self, name):
        return name in self.src_file_lines

    def add_src_file_lines(self, name, lines):
        self.src_file_lines[name] = lines

    def get_src_file_lines(self, name):
        return self.src_file_lines[name]

    def del_src_file_lines(self):
        self.src_file_lines.clear()

    def show_src_file_lines(self):
        gdb_print("mgdb_rdata.src_file_lines cache ==== ")
        gdb_print(str(self.src_file_lines))

    # api for asm block lines cache
    def in_asm_block_lines(self, name, start):
        return (name, start) in self.asm_block_lines

    def add_asm_block_lines(self, name, start, lines):
        self.asm_block_lines[(name, start)] = lines

    def get_asm_block_lines(self, name, start):
        return self.asm_block_lines[(name, start)]

    def del_asm_block_lines(self):
        self.asm_block_lines.clear()

    def show_asm_block_lines(self):
        gdb_print("mgdb_rdata.asm_block_lines cache ==== ")
        gdb_print(str(self.asm_block_lines))

    # api for mir block lines cache
    def in_mir_block_lines(self, name, start):
        return (name, start) in self.mir_block_lines

    def add_mir_block_lines(self, name, start, lines):
        self.mir_block_lines[(name, start)] = lines

    def get_mir_block_lines(self, name, start):
        return self.mir_block_lines[(name, start)]

    def del_mir_block_lines(self):
        self.mir_block_lines.clear()

    def show_mir_block_lines(self):
        gdb_print("mgdb_rdata.mir_block_lines cache ==== ")
        gdb_print(str(self.mir_block_lines))

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
        self.file_cache = MapleFileCache()
        self.mirmpl_info = MapleMirmplInfoIndex()
        self.frame_change_counter = 0

    def update_frame_change_counter(self):
        self.frame_change_counter += 1

    def read_frame_change_counter(self):
        return self.frame_change_counter

    #####################################
    # api for mirbin_info_cache
    #####################################
    def create_mirbin_info_cache(self, asm_file):
        if self.mirbin_info_cache_has_key(asm_file):
            return

        self.mirbin_info.init_new_asm(asm_file)

        d = m_asm.create_asm_mirbin_label_cache(asm_file)
        self.mirbin_info_cache_assign(asm_file, d)

        if m_debug.Debug:
            count = len(self.mirbin_info.mirbin_info_cache[asm_file].keys())
            m_debug.dbg_print("asm file ", asm_file, count, "labels found")

    def get_one_label_mirbin_info_cache(self, asm, label):
        return self.mirbin_info.get_asm_mirbin_item(asm, label)

    def mirbin_info_cache_has_key(self, asm):
        return self.mirbin_info.has_key(asm)

    def mirbin_info_cache_assign(self, asm, d):
        self.mirbin_info.assign_new_asm(asm, dict(d))

    def get_mirbin_cache_asm_list(self):
        return self.mirbin_info.get_asm_list()

    def get_mirbin_cache_symbol_list(self, asm_path):
        return self.mirbin_info.get_symbol_list(asm_path)

    #####################################
    # api for mirmpl_info_cache
    #####################################
    def create_mirmpl_info_cache(self, mirmpl_path):
        if self.mirmpl_info_cache_has_key(mirmpl_path):
            return

        self.mirmpl_info.init_new_mirmpl(mirmpl_path)

        d = m_mirmpl.create_mirmpl_label_cache(mirmpl_path)
        self.mirmpl_info_cache_assign(mirmpl_path, d)

        if m_debug.Debug:
            count = len(self.mirmpl_info.mirmpl_info_cache[mirmpl_path].keys())
            m_debug.dbg_print("mirmpl path ", mirmpl_path, count, "labels found")

    def get_one_label_mirmpl_info_cache(self, mirmpl, label):
        return self.mirmpl_info.get_mirmpl_item(mirmpl, label)

    def mirmpl_info_cache_has_key(self, mirmpl):
        return self.mirmpl_info.has_key(mirmpl)

    def mirmpl_info_cache_assign(self, mirmpl, d):
        self.mirmpl_info.assign_new_mirmpl(mirmpl, dict(d))

    def get_mirmpl_cache_mirmpl_list(self):
        return self.mirmpl_info.get_mirmpl_list()

    def get_mirmpl_cache_symbol_list(self, mir_path):
        return self.mirmpl_info.get_symbol_list(mir_path)

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
    # api for Maple File Cache
    #####################################
    def in_fullpath_cache(self, name, path):
        return self.file_cache.in_general_fullpath(name, path)

    def add_fullpath_cache(self, name, path, fullpath):
        self.file_cache.add_general_fullpath(name, path, fullpath)

    def get_fullpath_cache(self, name, path):
        return self.file_cache.get_general_fullpath(name, path)

    def del_fullpath_cache(self):
        self.file_cache.del_general_fullpath()

    # api for source code file info cache
    def in_src_lines(self, name):
        return self.file_cache.in_src_file_lines(name)

    def add_src_lines(self, name, lines):
        self.file_cache.add_src_file_lines(name, lines)

    def get_src_lines(self,name):
        return self.file_cache.get_src_file_lines(name)

    def del_src_lines(self):
        self.file_cache.del_src_file_lines()

    # api for asm block lines cache
    def in_asmblock_lines(self, name, start):
        return self.file_cache.in_asm_block_lines(name,start)

    def add_asmblock_lines(self, name, start, lines):
        self.file_cache.add_asm_block_lines(name, start, lines)

    def get_asmblock_lines(self, name, start):
        return self.file_cache.get_asm_block_lines(name, start)

    def del_asmblock_lines(self):
        self.file_cache.del_asm_block_lines()

    # api for mir block lines cache
    def in_mirblock_lines(self, name, start):
        return self.file_cache.in_mir_block_lines(name,start)

    def add_mirblock_lines(self, name, start, lines):
        self.file_cache.add_mir_block_lines(name, start, lines)

    def get_mirblock_lines(self, name, start):
        return self.file_cache.get_mir_block_lines(name, start)

    def del_mirblock_lines(self):
        self.file_cache.del_mir_block_lines()

    def show_file_cache(self):
        self.file_cache.show_asm_block_lines()
        self.file_cache.show_src_file_lines()
        self.file_cache.show_mir_block_lines()
        self.file_cache.show_general_fullpath()


    #####################################
    # advanced api for other module to use
    #####################################
    def update_gdb_runtime_data(self):

        addr_offset, so_path, asm_path, func_header, mirmpl_path = m_frame.get_newest_maple_frame_func_lib_info()
        if m_debug.Debug:
            m_debug.dbg_print("addr_offset=", addr_offset)
            m_debug.dbg_print("so_path=", so_path)
            m_debug.dbg_print("asm_path=", asm_path)
            m_debug.dbg_print("func_header=", func_header)
            m_debug.dbg_print("mirmpl_path=", mirmpl_path)
        if not addr_offset or not so_path or not asm_path or not func_header or not mirmpl_path:
            return

        if not self.mirmpl_info_cache_has_key(mirmpl_path):
            gdb_print("create Maple mir.mpl cache using file: " + mirmpl_path)
            self.create_mirmpl_info_cache(mirmpl_path)

        if not self.mirbin_info_cache_has_key(asm_path):
            gdb_print("create Maple symbol cache using file: " + asm_path)
            self.create_mirbin_info_cache(asm_path)

        def_file_path = asm_path[:-1] + 'macros.def' if 'VtableImpl' in asm_path else asm_path[:-1] + 'VtableImpl.macros.def'
        if not def_file_path in self.class_def.def_paths:
            gdb_print("create def cache using file: " + def_file_path)
            self.create_class_def(def_file_path)

    def update_gdb_runtime_data_dync(self):

        addr_offset, so_path, asm_path, func_header, mmpl_path = m_frame.get_newest_maple_frame_func_lib_info(True)
        if m_debug.Debug:
            m_debug.dbg_print("addr_offset=", addr_offset)
            m_debug.dbg_print("so_path=", so_path)
            m_debug.dbg_print("asm_path=", asm_path)
            m_debug.dbg_print("func_header=", func_header)
            m_debug.dbg_print("mmpl_path=", mmpl_path)
        if not addr_offset or not so_path or not asm_path or not func_header or not mmpl_path:
            return

        if not self.mirbin_info_cache_has_key(asm_path):
            gdb_print("create Maple symbol cache using file: " + asm_path)
            self.create_mirbin_info_cache(asm_path)

        '''
        if not self.mirmpl_info_cache_has_key(mirmpl_path):
            gdb_print("create Maple mir.mpl cache using file: " + mirmpl_path)
            self.create_mirmpl_info_cache(mirmpl_path)

        if not self.mirbin_info_cache_has_key(asm_path):
            gdb_print("create Maple symbol cache using file: " + asm_path)
            self.create_mirbin_info_cache(asm_path)

        def_file_path = asm_path[:-1] + 'macros.def' if 'VtableImpl' in asm_path else asm_path[:-1] + 'VtableImpl.macros.def'
        if not def_file_path in self.class_def.def_paths:
            gdb_print("create def cache using file: " + def_file_path)
            self.create_class_def(def_file_path)
        '''



mgdb_rdata =  MapleGDBRuntimeData()

def get_stack_frame_data(frame):
    #if frame.name() == 'maple::InvokeInterpretMethod':
    if m_frame.is_maple_frame_dync(frame):
        return get_stack_frame_data_dync(frame)
    else:
        return get_stack_frame_data_static(frame)

def get_stack_frame_data_dync(frame):
    func_addr_offset, so_path, asm_path, func_header, mmpl_path = m_frame.get_maple_frame_func_lib_info_dync(frame)
    if m_debug.Debug:
        m_debug.dbg_print("func_addr_offset =", func_addr_offset, "func_header=", func_header)
        m_debug.dbg_print("so_path =", so_path, "asm_path=", asm_path, "mmpl_path=", mmpl_path)
    if not func_addr_offset or not so_path or not asm_path or not func_header or not mmpl_path:
        return None

    ### get the function label of the frame
    label_addr, func_header_name = m_symbol.get_symbol_name_by_current_frame_args_dync()
    if m_debug.Debug: m_debug.dbg_print("func_header_name=", func_header_name)
    if func_header_name:
        func_name = func_header_name[:-12]
        if m_debug.Debug: m_debug.dbg_print("func_name=", func_name)
    '''
    else:
        label_addr, func_header_name = m_symbol.get_symbol_name_by_current_func_args()
        func_name = func_header_name[:-12]
        if m_debug.Debug: m_debug.dbg_print("func_name=", func_name)
    '''

    if m_debug.Debug: m_debug.dbg_print("func_header_name=", func_header_name)
    if m_debug.Debug: m_debug.dbg_print("func_name=", func_name)
    if not func_header_name:
        return None

    '''
    asm_cache = m_asm.create_asm_mirbin_label_cache(asm_path)
    if not asm_cache:
        asm_cache = m_asm.create_asm_mirbin_label_cache(asm_path)
    print ("asm_cache = ", asm_cache)
    '''

    asm_tuple = mgdb_rdata.get_one_label_mirbin_info_cache(asm_path, func_header_name)
    if not asm_tuple:
        return None

    rdata = {}
    rdata['frame_func_header_info'] = { 'so_path': so_path, \
                                        'asm_path': asm_path, \
                                        'mirmpl_path': None, \
                                        'mmpl_path': mmpl_path, \
                                        'func_addr_offset': func_addr_offset, \
                                        'func_name': func_name, \
                                        'func_header_name' : func_header_name, \
                                        'func_header_asm_tuple': asm_tuple, \
                                        'func_header_mirmpl_tuple': None \
                                      }

    ### TODO!!!
    ### 1, fill in data for js source file name, line, asm line, asm offset, mmple_line,
    ###    mmple_line_offset
    ### 2. local and formal data returns None for now for JS.
    func_addr_offset_pc = func_addr_offset.split(':')[1]
    d = m_asm.lookup_src_file_info_dync(asm_path, func_header_name, func_addr_offset_pc)
    if not d:
        if m_debug.Debug: m_debug.dbg_print("lookup_src_file_info_dync returns None")
        return None
    if m_debug.Debug: m_debug.dbg_print("first round returns dict d:", d)

    # In some cases, when func_addr_offset's pc partion is 0000, or func_addr_offset is followed by "MPL_CLINIT_CHECK",
    # the source code file information can be missing.
    # In this case, we can check few more lines from its reported asm line ONLY for pc = '0000' or "MPI_CLINIT_CHECK" situation
    if not d['short_src_file_name'] and (func_addr_offset_pc == '0000' or d['short_src_file_line'] == -1) and d['asm_offset'] != 0 and d['asm_line'] != 0:
        if m_debug.Debug: m_debug.dbg_print("func offset pc is 0000, check short_src_file again")
        d['short_src_file_name'], d['short_src_file_line'] = \
            m_asm.lookup_next_src_file_info(asm_path, d['asm_line'], d['asm_offset'])
        if m_debug.Debug: m_debug.dbg_print("second round for func offset pc 0000 returns dict d:", d)

    rdata['frame_func_src_info'] = d

    ### Get JS function formals and locals
    func_argus_locals_dict = m_asm.get_js_func_formals_locals(asm_path, func_header_name)
    if not func_argus_locals_dict:
        rdata['func_argus_locals'] = None
    else:
        rdata['func_argus_locals'] = func_argus_locals_dict

    #print ("rdata dync", rdata)
    return rdata



def get_stack_frame_data_static(frame):
    """
    For a given Maple stack frame, get all finds of frame data including
    1, function. Header info
    2, function. Source code information
    3, function. Local variable and argument information.

    params:
      frame: a gdb.frame object. A SELECTED Maple frame from current stack.

    returns:
       a dict contains following keys and values
        'frame_func_src_info':  whatever m_asm.lookup_src_file_info() returns
                {'short_src_file_name': a string. source code short name
                 'short_src_file_line': a int. source code file line
                 'asm_line': a int. line number where func_header_name offset is found in asm file
                 'asm_offset': a int. offset in the asm file co-repsonding to asm_line
                 'mirmpl_line': a int. symbol block line in mir.mpl file
                 'mirmpl_line_offset': a int. symbol block line offset in mir.mpl file
                 'mir_instidx': a string. symbol block mir instidx offset, e.g. 0008 in both .s and .s file
                }
        'frame_func_header_info':
                {'func_name': func_name, string.
                 'so_path': so_path full path, a string
                 'asm_path': asm_path full path,a string
                 'mirmpl_path': mir.mpl file full path. a string
                 'func_addr_offset': func_addr_offset, string in xxxx:yyyy: format
                 'func_header_name' : func_header_name, a string. e.g. xxxxxxxxxxx_mirbin_info
                 'func_header_asm_tuple': func_header_name_block_asm_tuple, (asm line, asm_start_offset, asm_end_offset)
                 'func_header_mirmpl_tuple': func_header_name_block_mir_tuple, (mir line, mir_start_offset, mir_end_offset)
                }
        'func_argus_locals': same return as m_asm.get_func_arguments() returns
                {'locals_type': local variable type list, e.g. ['void', 'v2i64'],
                 'locals_name': local variable name list['%%retval', '%%thrownval'],
                 'formals_type': func argument type list. e.g. ['a64'],
                 'formals_name': func argument name list. e.g. ['%1']
                }
    """
    if m_debug.Debug: m_debug.dbg_print("==== get_stack_frame_data =====")

    func_addr_offset, so_path, asm_path, func_header, mirmpl_path = m_frame.get_maple_frame_func_lib_info(frame)
    if m_debug.Debug:
        m_debug.dbg_print("func_addr_offset =", func_addr_offset, "func_header=", func_header)
        m_debug.dbg_print("so_path =", so_path, "asm_path=", asm_path, "mirmpl_path=", mirmpl_path)
    if not func_addr_offset or not so_path or not asm_path or not func_header or not mirmpl_path:
        return None

    ### get the function label of the frame
    label_addr, func_header_name = m_symbol.get_symbol_name_by_current_frame_args()
    if m_debug.Debug: m_debug.dbg_print("func_header_name=", func_header_name)
    if func_header_name:
        func_name = func_header_name[:-12]
        if m_debug.Debug: m_debug.dbg_print("func_name=", func_name)
    else:
        label_addr, func_header_name = m_symbol.get_symbol_name_by_current_func_args()
        func_name = func_header_name[:-12]
        if m_debug.Debug: m_debug.dbg_print("func_name=", func_name)

    if m_debug.Debug: m_debug.dbg_print("func_header_name=", func_header_name)
    if not func_header_name:
        return None

    # if mirbin_info_cache for this asm file is not created yet in m_datastore, create it
    # if not m_datastore.mgdb_rdata.mirbin_info_cache_has_key(asm_path):
    if not mgdb_rdata.mirbin_info_cache_has_key(asm_path):
        if m_debug.Debug: m_debug.dbg_print("create mirbin info cache for asm file", asm_path)
        mgdb_rdata.create_mirbin_info_cache(asm_path)

    # if mirmpl_info_cache for this mir.mpl file is not created yet in m_datastore, create it
    # if not m_datastore.mgdb_rdata.mirmpl_info_cache_has_key(mirmpl_path):
    if not mgdb_rdata.mirmpl_info_cache_has_key(mirmpl_path):
        if m_debug.Debug: m_debug.dbg_print("create mirmpl info cache for mirmpl file", mirmpl_path)
        mgdb_rdata.create_mirmpl_info_cache(mirmpl_path)

    asm_line_tuple = mgdb_rdata.get_one_label_mirbin_info_cache(asm_path, func_header_name)
    if not asm_line_tuple:
        return None
    if m_debug.Debug:
        m_debug.dbg_print("asm_line_tuple=", asm_line_tuple)
        m_debug.dbg_print("func_addr_offset=", func_addr_offset)
        m_debug.dbg_print("func_addr_offset.split(':')[1]=", func_addr_offset.split(':')[1])
    func_addr_offset_pc = func_addr_offset.split(':')[1]
    d = m_asm.lookup_src_file_info(asm_path, asm_line_tuple[0], asm_line_tuple[1], asm_line_tuple[2],\
                                              func_addr_offset_pc)
    if not d:
        if m_debug.Debug: m_debug.dbg_print("lookup_src_file_info returns None")
        return None
    if m_debug.Debug: m_debug.dbg_print("first round returns dict d:", d)

    # In some cases, when func_addr_offset's pc partion is 0000, or func_addr_offset is followed by "MPL_CLINIT_CHECK",
    # the source code file information can be missing.
    # In this case, we can check few more lines from its reported asm line ONLY for pc = '0000' or "MPI_CLINIT_CHECK" situation
    if not d['short_src_file_name'] and (func_addr_offset_pc == '0000' or d['short_src_file_line'] == -1) and d['asm_offset'] != 0 and d['asm_line'] != 0:
        if m_debug.Debug: m_debug.dbg_print("func offset pc is 0000, check short_src_file again")
        d['short_src_file_name'], d['short_src_file_line'] = \
            m_asm.lookup_next_src_file_info(asm_path, d['asm_line'], d['asm_offset'])
        if m_debug.Debug: m_debug.dbg_print("second round for func offset pc 0000 returns dict d:", d)

    # read the mpl.mir.mpl file, get the func_addr_offset_pc line and offset at .mpl.mir.mpl file
    # note, func_header_name ends with symbol_mirbin_info, while mir.mpl's symbol has no '_mirbin_info'
    #print("mirmpl_path=",mirmpl_path, "func_header_name[:-12]=",func_header_name[:-12])
    mirmpl_line_tuple = mgdb_rdata.get_one_label_mirmpl_info_cache(mirmpl_path, func_header_name[:-12])
    mir_instidx = d['mir_instidx']
    #print("mirmpl_line_tuple=",mirmpl_line_tuple, "mir_instidx=", mir_instidx)
    d_mirmpl = m_mirmpl.lookup_mirmpl_info(mirmpl_path, mirmpl_line_tuple, mir_instidx)
    #rint("d_mirmpl=",d_mirmpl)
    # dictionary of d_mirmpl: {'mirmpl_line': int, 'mirmpl_Line_offset': int}
    if not d_mirmpl:
        if m_debug.Debug: m_debug.dbg_print("lookup_mirmpl_info returns None")
        return None
    if m_debug.Debug: m_debug.dbg_print("m_mirmpl.lookup_mirmpl_info() returns dict d_mirmpl:", d_mirmpl)

    # merge two dictionaries
    d = {**d, **d_mirmpl}
    if m_debug.Debug: m_debug.dbg_print("After merging asm_d and mirmpl_d:", d)

    asm_line_offset = asm_line_tuple[1]
    asm_line_num = d['asm_line']
    mirmpl_line_num = d['mirmpl_line']
    mirmpl_line_offset = d['mirmpl_line_offset']
    src_file_short_name = d['short_src_file_name']
    src_file_line = d['short_src_file_line']

    if not asm_line_offset:
        return None

    ### get the method arguments and local variables
    func_argus_locals = m_asm.get_func_arguments(asm_path, func_header_name, asm_line_offset)
    if m_debug.Debug: m_debug.dbg_print("func_argus_locals:", func_argus_locals)
    if not func_argus_locals:
        return None

    rdata = {}
    rdata['frame_func_src_info']    = d
    rdata['frame_func_header_info'] = { 'so_path': so_path, \
                                        'asm_path': asm_path, \
                                        'mirmpl_path': mirmpl_path, \
                                        'func_addr_offset': func_addr_offset, \
                                        'func_name': func_name, \
                                        'func_header_name' : func_header_name, \
                                        'func_header_asm_tuple': asm_line_tuple, \
                                        'func_header_mirmpl_tuple': mirmpl_line_tuple \
                                      }
    rdata['func_argus_locals']     =  func_argus_locals

    return rdata

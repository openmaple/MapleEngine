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
import re
import m_debug
import m_datastore
import m_symbol
import m_asm
import m_list
import m_info
import m_frame
import m_util
from m_util import MColors
from m_util import gdb_print


class MapleTypeCmd(gdb.Command):
    """print a matching class and its inheritance hierarchy by a given search expression
    mtype: given a regex, searches and prints all matching class names if multiple are found,
    or print detailed information of class inheritance hierarchy if a single match is found
    mtype <regular-express>: e.g. mtype _2Fjava
    """

    def __init__(self):
        gdb.Command.__init__ (self,
                              "mtype",
                              gdb.COMMAND_DATA,
                              gdb.COMPLETE_NONE)


    def invoke(self, args, from_tty):
        self.mtype_func(args, from_tty)


    def usage(self):
        gdb_print("mtype <regex of a mangled Maple class name or a property name>")


    def mtype_func(self, args, from_tty):
        s = args.split()
        if len(s) == 0 or len(s) > 1:
            self.usage()
            return

        is_dync, frame = m_frame.is_closest_older_maple_frame_dync()
        if not is_dync:
            return self.mtype_func_static(s[0])
        else:
            return self.mtype_func_dync(s[0])

    def mtype_func_dync(self, regex):
        data = m_info.get_all_dync_properties_data()
        if not data or len(data) == 0:
            gdb_print("property data not ready yet")
            return

        reg = r"(%s)+" % regex
        try:
            pattern = re.compile(reg)
        except:
            return

        index = 1
        for element in data:
            m = pattern.search(element['name'])
            if not m:
                continue
            else:
                buf = "#" + str(index) + " " + MColors.TP_CNAME + "property name=" + MColors.ENDC + MColors.TP_STR_V + element['name'] + MColors.ENDC \
                      + MColors.TP_ANAME + ", node addr=" + MColors.ENDC + MColors.TP_STR_V + element['node_addr'] + MColors.ENDC \
                      + MColors.TP_ANAME + ", tag=" + MColors.ENDC + MColors.TP_STR_V + element['tag'] + MColors.ENDC \
                      + MColors.TP_ANAME + ", value=" + MColors.ENDC + MColors.TP_STR_V + element['value'] + MColors.ENDC
                gdb_print(buf)
            index += 1


    def mtype_func_static(self, regex):
        # create a re.recompile pattern
        reg = r"(%s)+" % regex
        pattern = re.compile(reg)

        # class_list is a sorted list that contains class names
        class_list = m_datastore.mgdb_rdata.get_class_def_list()
        if m_debug.Debug: m_debug.dbg_print("return class_list length:",len(class_list))
        if len(class_list) == 0:
            gdb_print("The class information table is not available yet")
            gdb_print("Retry this after running Maple breakpoint and Maple stack commands")
            return

        count = 0
        last_matching_class_name = None
        for class_name in class_list:
            m = pattern.search(class_name)
            if not m:
                continue
            count += 1
            buffer = '#{} class name: {}'.format(count, MColors.TP_CNAME + class_name + MColors.ENDC)
            gdb_print(buffer)
            last_matching_class_name = class_name

        if count > 1:
            return

        inheritance_list = self.get_class_inheritance_list(last_matching_class_name)
        if not inheritance_list:
            return
        if m_debug.Debug: m_debug.dbg_print("return inheritance_list length:",len(inheritance_list))
        self.display_class_list(inheritance_list)

        gdb_print("\n  Method list:")
        updated_asm_path_list = get_updated_lib_asm_path_list()
        symbol = "^" + last_matching_class_name + "_7C"
        print_prefix = ""
        search_display_symbol_list(symbol, 4, print_prefix, updated_asm_path_list)

    def get_class_inheritance_list(self, class_name):
        inherit_list = []
        name = class_name
        count = 0
        while True:
            obj_class_dict = m_datastore.mgdb_rdata.get_class_def(name)
            if m_debug.Debug: m_debug.dbg_print("count=", count, "obj_class_dict=", obj_class_dict)
            if not obj_class_dict:
                return  None
            inherit_list = [{'class_name': name, 'obj_class': obj_class_dict}] + inherit_list
            count += 1
            if obj_class_dict['base_class'] == 'THIS_IS_ROOT':
                break
            else:
                name = obj_class_dict['base_class']

        if m_debug.Debug: m_debug.dbg_print("count=", count, "obj_class_dict=", obj_class_dict)
        for i, v in enumerate(inherit_list):
            if m_debug.Debug: m_debug.dbg_print("  inherit_list #",i, v)
        if m_debug.Debug: m_debug.dbg_print()

        return inherit_list

    def display_class_list(self, class_list):
        if not class_list or len(class_list) is 0 :
            return

        level = len(class_list)
        field_count = 1
        for i in range(level):
            buffer = '  level {} {}: '.format(i+1, MColors.TP_CNAME + class_list[i]['class_name'] + MColors.ENDC)
            gdb_print(buffer)

            slist = sorted(class_list[i]['obj_class']['fields'], key=lambda x:x['offset'])
            for v in slist:
                buffer ='    #{0:d},off={1:2d},len={2:2d},"{3:<16s}"'.format(field_count, v['offset']\
                        ,v['length'],v['name'])
                gdb_print(buffer)
                field_count += 1

def get_updated_lib_asm_path_list():
    # get an asm path list for those libraries loaded at this point.
    loaded_asm_path_list = m_info.get_loaded_lib_asm_path()
    if m_debug.Debug: m_debug.dbg_print("loaded_lib_asm_path_list:",loaded_asm_path_list)

    # asm_path_list is a sorted list that contains asm paths
    asm_path_list = m_datastore.mgdb_rdata.get_mirbin_cache_asm_list()
    if m_debug.Debug: m_debug.dbg_print("return asm_path_list:",asm_path_list)

    # get the diff between currently loaded asm file and whats loaded by m_datastore.update_gdb_runtime_data
    diff_asm_path_list = list(set(loaded_asm_path_list) - set(asm_path_list))
    if m_debug.Debug: m_debug.dbg_print("diff_asm_path_list:",diff_asm_path_list)

    if len(diff_asm_path_list) == 0 and len(asm_path_list) == 0:
        gdb_print("The libraries that the symbol searched had not been loaded yet")
        gdb_print("Retry this after running Maple breakpoint and Maple stack commands")
        return None

    for asm_path in diff_asm_path_list:
        #gdb_print("New Maple symbols found, loading them from", asm_path)
        m_datastore.mgdb_rdata.create_mirbin_info_cache(asm_path)
        asm_path_list.append(asm_path)

    return asm_path_list

def search_display_symbol_list(sym_regexp, indent, print_prefix, asm_path_list):
    """
    search symbols using symb_regexp, print matching symbol name with given prefix.
    params:
      sym_regexp: string. regular expression for the symbol to be searched
      print_prefix: string. the prefix for print.
      asm_path_list: search with the asm paths in asm_path_list
      indent: int. number of space characters

    returns:
      count: number of matched symbol
      last_matchimg_symbol_name: string, or None if count = 0
      last_matching_symbol_path: string, or None if count = 0
    """

    # create a re.recompile pattern
    reg = r"(%s)+" % sym_regexp
    try:
        pattern = re.compile(reg)
    except:
        return 0, None, None

    count = 0
    last_matching_symbol_name = None
    last_matching_symbol_path = None
    asm_path = None
    for asm_path in asm_path_list:
        # asm_symbol_list is a sorted list contains all the symbols from asm_path
        asm_symbol_list = m_datastore.mgdb_rdata.get_mirbin_cache_symbol_list(asm_path)
        if m_debug.Debug: m_debug.dbg_print("asm=", asm_path, ", len(asm_symbol_list)=", len(asm_symbol_list))
        # !!! element in asm_symbol_list all contain '_mirbin_info' suffix, need to remove it before the pattern search
        asm_symbol_list_trim = [e[:-12] for e in asm_symbol_list]
        if m_debug.Debug: m_debug.dbg_print("asm=", asm_path, ", len(asm_symbol_list_trim)=", len(asm_symbol_list_trim))
        for symbol_name in asm_symbol_list_trim:
            m = pattern.search(symbol_name)
            if not m:
                continue
            count += 1
            indent_str = ' ' * indent
            if print_prefix:
                buffer = '{}#{} {}: {}'.format(indent_str, count, print_prefix, m_util.color_symbol(MColors.SP_SNAME, symbol_name))
            else:
                buffer = '{}#{} {}'.format(indent_str, count, m_util.color_symbol(MColors.TP_MNAME, symbol_name))
            gdb_print(buffer)
            last_matching_symbol_name = symbol_name
            last_matching_symbol_path = asm_path

    return count, last_matching_symbol_name, last_matching_symbol_path

def display_symbol_detail(short_symbol_name, symbol_asm_path):
    '''
    Input:
        short_symbol_name: short symbol name, e.g Add_5453f871. The real symbol name used for search
                           in Add_5453f871_mirbin_info. String
        symbol_asm_path; the assembly file name to search the symbol
    '''
    if m_debug.Debug: m_debug.dbg_print("symbol_asm_path=", symbol_asm_path, "short_symbol_name=", short_symbol_name)
    if not short_symbol_name or not symbol_asm_path:
        return

    frame = m_frame.get_selected_frame()
    if not frame:
        return
    is_dync = False
    if m_frame.is_maple_frame_dync(frame):
        is_dync = True

    # short_symbol_name does not contain suffix "_mirbin_info" which is the element name cached in m_datastore
    symbol_name = short_symbol_name + "_mirbin_info"
    data = m_datastore.mgdb_rdata.get_one_label_mirbin_info_cache(symbol_asm_path,symbol_name)
    if is_dync == True:
        d = m_asm.lookup_src_file_info_dync(symbol_asm_path, symbol_name, "0000")
    else:
        d = m_asm.lookup_src_file_info(symbol_asm_path, data[0], data[1], data[2], "0000")
    if not d:
        return

    short_src_file_name, short_src_file_line = m_asm.lookup_next_src_file_info(symbol_asm_path,\
                                                                                d["asm_line"], d['asm_offset'])

    file_full_path = None
    for source_path in m_list.maple_source_path_list:
        file_full_path = m_list.find_one_file(short_src_file_name, source_path)
        if not file_full_path:
            continue
        else:
            break

    gdb_print("assembly file : " + symbol_asm_path)
    if not file_full_path:
        gdb_print("source        : unknown")
    else:
        gdb_print("source        : " + file_full_path)
    if is_dync == True:
        return
    else:
        demangled_name = m_symbol.get_demangled_maple_symbol(short_symbol_name)
        if not demangled_name:
            gdb_print("demangled name: " + m_util.color_symbol(MColors.SP_SNAME, short_symbol_name))
        else:
            #gdb_print("demangled name: " + m_symbol.get_demangled_maple_symbol(m_util.color_symbol(MColors.SP_SNAME, symbol_name)))
            gdb_print("demangled name: " + m_util.color_symbol(MColors.SP_SNAME, demangled_name))


class MapleSymbolCmd(gdb.Command):
    """print a matching symbol list or it's detailed infomation
    msymbol: given a regex, searches and prints all matching symbol names if multiple are found,
    or print detailed information of the symbol if a single match is found
    msymbol <regular-express>: e.g. msymbol sun.*executor
    """

    def __init__(self):
        gdb.Command.__init__ (self,
                              "msymbol",
                              gdb.COMMAND_DATA,
                              gdb.COMPLETE_NONE)

    def invoke(self, args, from_tty):
        self.msymbol_func(args, from_tty)

    def usage(self):
        gdb_print("msymbol <regex of a symbol>")

    def msymbol_func(self, args, from_tty):
        s = args.split()
        if len(s) == 0 or len(s) > 1:
            self.usage()
            return

        updated_asm_path_list = get_updated_lib_asm_path_list()
        if not updated_asm_path_list:
            return

        count, last_matching_symbol_name, last_matching_symbol_path = search_display_symbol_list(s[0], 0, "symbol name", updated_asm_path_list)
        if m_debug.Debug: m_debug.dbg_print("last_matching_symbol_path=", last_matching_symbol_path, "last_matching_symbol_name=", last_matching_symbol_name)
        if count != 1:
            return

        display_symbol_detail(last_matching_symbol_name, last_matching_symbol_path)
        return

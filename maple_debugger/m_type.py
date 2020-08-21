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
from inspect import currentframe, getframeinfo
import m_util
import re
import m_datastore


class MapleTypeCmd(gdb.Command):
    """
    This class defines a Maple debugger command mtype, to show for a given class name expression,
    find the matching class names. And if only one matching is found, display its class inheritance
    hierarchy
    """

    def __init__(self):
        gdb.Command.__init__ (self,
                              "mtype",
                              gdb.COMMAND_DATA,
                              gdb.COMPLETE_NONE)
        

    def invoke(self, args, from_tty):
        gdb.execute('set pagination off', to_string = True)
        self.mtype_func(args, from_tty)
        gdb.execute('set pagination on', to_string = True)


    def usage(self):
        print("mtype <regex of a mangled Maple class name>")
        

    def mtype_func(self, args, from_tty):
        s = args.split()
        if len(s) == 0 or len(s) > 1:
            self.usage()
            return

        # create a re.recompile pattern
        reg = r"(%s)+" % s[0]
        pattern = re.compile(reg)
        
        class_list = m_datastore.mgdb_rdata.get_class_def_list()
        m_util.debug_print(__file__, getframeinfo(currentframe()).lineno, self.mtype_func,\
                        "return class_list length:",len(class_list))
        if len(class_list) == 0:
            print("class information table not available yet")
            print("Retry this after running Maple breakpoint and Maple stack commands")
            return

        count = 0
        last_matching_class_name = None
        for class_name in class_list:
            m = pattern.search(class_name)
            if not m:
                continue
            count += 1
            buffer = '#{} class name {}:'.format(count, class_name)
            print(buffer)
            last_matching_class_name = class_name
            
        if count > 1:
            return

        inheritance_list = self.get_class_inheritance_list(last_matching_class_name)
        if not inheritance_list:
            return
        m_util.debug_print(__file__, getframeinfo(currentframe()).lineno, self.mtype_func,\
                         "return inheritance_list length:",len(inheritance_list))
        self.display_class_list(inheritance_list)

    def get_class_inheritance_list(self, class_name):
        inherit_list = []
        name = class_name
        count = 0
        while True:
            obj_class_dict = m_datastore.mgdb_rdata.get_class_def(name)
            m_util.debug_print(__file__, getframeinfo(currentframe()).lineno, self.get_class_inheritance_list,\
                            "count=", count, "obj_class_dict=", obj_class_dict)
            if not obj_class_dict:
                return  None
            inherit_list = [{'class_name': name, 'obj_class': obj_class_dict}] + inherit_list
            count += 1
            if obj_class_dict['base_class'] == 'THIS_IS_ROOT':
                break
            else:
                name = obj_class_dict['base_class']

        m_util.debug_print(__file__, getframeinfo(currentframe()).lineno, self.get_class_inheritance_list,\
                            "count=", count, "obj_class_dict=", obj_class_dict)
        for i in range(len(inherit_list)):
            m_util.debug_print(__file__, getframeinfo(currentframe()).lineno, self.get_class_inheritance_list,\
                        "  inherit_list #",i, inherit_list[i])
        m_util.debug_print(__file__, getframeinfo(currentframe()).lineno, self.get_class_inheritance_list)

        return inherit_list

    def display_class_list(self, class_list):
        if not class_list or len(class_list) is 0 :
            return

        level = len(class_list)
        field_count = 1
        for i in range(level):
            buffer = '  level {} {}: '.format(i+1, class_list[i]['class_name'])
            print(buffer)

            slist = sorted(class_list[i]['obj_class']['fields'], key=lambda x:x['offset'])
            for field_idx in range(len(slist)):
                buffer ='    #{0:d},off={1:2d},len={2:2d},"{3:<16s}"'.format(field_count, slist[field_idx]['offset']\
                        ,slist[field_idx]['length'],slist[field_idx]['name'])
                print(buffer)
                field_count += 1

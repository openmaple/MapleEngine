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

import gdb
import m_datastore
import m_symbol
import m_util
from m_util import MColors
from m_util import gdb_print
import m_debug

class MaplePrintCmd(gdb.Command):
    """Print Maple runtime object data
    print a Maple object data:
    mprint <addr-in-hex>: e.g mprint 0x13085
    """

    def __init__(self):
        gdb.Command.__init__ (self,
                              "mprint",
                              gdb.COMMAND_DATA,
                              gdb.COMPLETE_NONE)

    def invoke(self, args, from_tty):
        self.mprint_func(args, from_tty)

    def usage(self):
        gdb_print ("  mprint      : print Maple object data")
        gdb_print ("  mprint <addr-in-hex>: e.g mprint 0x13085")

    def mprint_func(self, args, from_tty):
        s = str(args)

        x = s.split()
        if len(x) == 1: # address
            try:
                addr = int(x[0],16)
            except:
                self.usage()
                return
        else:
            self.usage()
            return

        class_name, full_syntax, type_size  = self.get_class_name_syntax(addr)
        if not full_syntax:
            return

        # if it is a array of class obj, or just non-array regular class obj
        if 'array' in full_syntax and class_name[0] == 'L' \
            or 'class' in full_syntax and class_name[0] == 'L' :
            class_list = self.get_class_list(class_name, full_syntax, type_size)

            if not class_list or len(class_list) is 0:
                gdb_print("no class data found")
                return

            self.display_class_data(class_list, addr)

        # if it is an array of primitive type
        elif 'array' in full_syntax and class_name in m_symbol.java_primitive_type_dict:
            ### array is a object that takes 12 bytes per java standard
            self.display_array_primitive_values(addr + 12, full_syntax, type_size)

        return

    def get_class_name_syntax(self, addr):
        """
        For a given stack address, get its class name, class full syntax (description), and
        the object size.

        params:
          addr: int. a stack address

        returns:
          1, class name: string
          2, full_syntax: string. a full description of the object at this address
          3, size: int. the object size
        """

        try:
            buffer = m_util.gdb_exec_to_string('x/gx ' + hex(addr))
        except:
            return None,None,None
        obj_addr = buffer.split(':')[1].strip(' ')

        try:
            buffer = m_util.gdb_exec_to_string('x ' + obj_addr)
        except:
            return None,None,None
        if not '<' in buffer or not '>' in buffer:
            return None,None,None

        buf = buffer.split(':')[0].split()[1]
        class_name = None
        dimension = None
        if buf[0] is '<' and buf[-1] is '>':
            class_name = m_symbol.get_maple_symbol_name(buf[1:-1])
            full_syntax, type_size = m_symbol.get_maple_symbol_full_syntax(buf[1:-1])
        else:
            return None,None,None

        if m_debug.Debug: m_debug.dbg_print("return class_name:", class_name, "full_syntax:", full_syntax, "type_size:", type_size)
        return class_name, full_syntax, type_size

    def get_class_list(self, class_name, full_syntax, type_size):
        """
        For a given class name and its full syntax, find out all the derived classes.
        return a list of class inheritance hierarchy starting from the most base class.

        params:
          class_name: string. class name
          full_syntax: string. description of the class object
          type_size: int. the size of the class object size.

        returns:
          a list of class inheritance hierarchy starting from the most base class
          The item in the list is a dict with the format of
            {
                'class_name': class name, string
                'full_syntax': class desciption. in string. e.g 'array int Ivar[]'
                'type_size': int.
                'obj_class': obj_class dict retrieved from m_datastore .macros.def cache.
                             This cache has a dict format:
                             {
                                'fields': list of {'name': field name,
                                                   'length': field length,
                                                   'offset': offset from the starting offset of the class.
                                                  }
                                'base_class': base class name. in string.
                                'size': this is actually not the size, it is endding-offset-of-the-class + 1,
                                    meaning next level obj should start from this offset.
                             }
            }
        """

        inherit_list = []
        name = class_name
        count = 0
        while True:
            obj_class_dict = m_datastore.mgdb_rdata.get_class_def(name)
            if m_debug.Debug: m_debug.dbg_print("count=", count, "obj_class_dict=", obj_class_dict)
            if not obj_class_dict:
                return  None
            if count is 0: # only the first class_name, we know its type at runtime
                mfull_syntax = full_syntax
                mtype_size = type_size
            else: # for all the base objects, we only know the name, but not runtime type
                mfull_syntax = 'class'
                mtype_size = 1
            inherit_list = [{'class_name': name, 'full_syntax': mfull_syntax, 'type_size': mtype_size, \
                            'obj_class': obj_class_dict}] + inherit_list
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

    def display_class_data(self, class_list, addr):
        level = len(class_list)
        if level is 0:
            gdb_print("no data found")
            return

        '''print head line before printing level'''
        if 'array' in class_list[level-1]['full_syntax']:
            buffer = 'object type: {}'.format(MColors.MP_FSYNTAX + class_list[level-1]['full_syntax'] + MColors.ENDC)
        else:
            buffer = 'object type: {} {}'.format(MColors.MP_FSYNTAX + class_list[level-1]['full_syntax'] + MColors.ENDC,\
                                                 MColors.MP_FSYNTAX + class_list[level-1]['class_name'] + MColors.ENDC)
        gdb_print(buffer)

        field_count = 1
        for i in range(level):
            if 'array' in class_list[i]['full_syntax']:
                if class_list[i]['obj_class']['base_class'] == "THIS_IS_ROOT":
                    arr_length = self.get_array_length(addr + class_list[i]['obj_class']['size'])
                    buffer = 'level {} {}: length={}'.format(i+1, MColors.MP_CNAME + class_list[i]['full_syntax'] + MColors.ENDC,arr_length)
                    gdb_print(buffer)
                    self.print_array_class_values(addr+class_list[i]['obj_class']['size'])
                else:
                    arr_length = self.get_array_length(addr + class_list[i-1]['obj_class']['size'])
                    buffer = 'level {} {}: length={}'.format(i+1, MColors.MP_CNAME + class_list[i]['full_syntax'] + MColors.ENDC,arr_length)
                    gdb_print(buffer)
                    self.print_array_class_values(addr+class_list[i-1]['obj_class']['size'])
            else:
                buffer = 'level {} {} {}: '.format(i+1, MColors.MP_FSYNTAX + class_list[i]['full_syntax'] + MColors.ENDC,\
                                                    MColors.MP_CNAME + class_list[i]['class_name'] + MColors.ENDC)
                gdb_print(buffer)

                slist = sorted(class_list[i]['obj_class']['fields'], key=lambda x:x['offset'])
                for v in slist:
                    value_string = self.get_value_from_memory(addr, v['offset'],v['length'])
                    if not value_string:
                        buffer ='  #{0:d},off={1:2d},len={2:2d},"{3:<16s}",value=None'.format(field_count, v['offset']\
                                ,v['length'],v['name'])
                    else:
                        buffer ='  #{0:d},off={1:2d},len={2:2d},"{3:<16s}",value={4}'.format(field_count, v['offset']\
                                ,v['length'],v['name'],value_string)
                    gdb_print(buffer)
                    field_count += 1

        return

    def get_array_length(self, addr):
        cmd = 'x/1xw ' + hex(addr)
        try:
            buffer = m_util.gdb_exec_to_string(cmd)
        except:
            return 0
        item_num = int(buffer.split(':')[1].strip(),16)
        if m_debug.Debug: m_debug.dbg_print("item_num=", item_num)
        return item_num

    def display_array_primitive_values(self, addr, full_syntax, type_size):
        buffer = 'Object Type: {}'.format(MColors.MP_FSYNTAX + full_syntax + MColors.ENDC)
        gdb_print(buffer)
        item_num = self.get_array_length(addr)
        buffer = 'Level 1 {}: length={}'.format(MColors.MP_FSYNTAX + full_syntax + MColors.ENDC, item_num)
        gdb_print(buffer)

        if item_num > 10:
            item_list = [0,1,2,3,4,5,6, item_num-3,item_num-2,item_num-1]
        else:
            item_list = [i for i in range(item_num)]

        steps = 0
        show_snip = True
        for i in item_list:
            obj_addr = addr + 4 + type_size * i  # class reference is a pointer, 8 bytes
            if type_size == 8:
                cmd = 'x/1gx ' + hex(obj_addr)
            elif type_size == 4:
                cmd = 'x/1xw ' + hex(obj_addr)
            elif type_size == 2:
                cmd = 'x/1xh ' + hex(obj_addr)
            elif type_size == 1:
                cmd = 'x/1xb ' + hex(obj_addr)
            else:
                return
            if m_debug.Debug: m_debug.dbg_print("cmd=", cmd)
            try:
                buffer = m_util.gdb_exec_to_string(cmd)
            except:
                buf = '  [{}] {}'.format(i,'no-data')
                gdb_print (buf)
                steps += 1
                continue

            steps += 1
            v = buffer.split(':')[1].strip()
            v = hex(int(v, 16)) #remove leading 0s. e.g 0x000123 to 0x123
            buf = '  [{}] {}'.format(i, v)
            gdb_print (buf)

            if item_num > 10 and steps > 6 and show_snip == True:
                gdb_print("  ...")
                show_snip = False

        return

    def print_array_class_values(self, addr):
        """ display 10 items if length is less than 10, or first 7 and last three if length is over 11 """

        item_num = self.get_array_length(addr)

        if item_num > 10:
            item_list = [0,1,2,3,4,5,6, item_num-3,item_num-2,item_num-1]
        else:
            item_list = [i for i in range(item_num)]

        steps = 0
        show_snip = True
        for i in item_list:
            obj_addr = addr + 4 + 8 * i  # class reference is a pointer, 8 bytes
            cmd = 'x/1gx ' + hex(obj_addr)
            if m_debug.Debug: m_debug.dbg_print("cmd=", cmd)
            try:
                buffer = m_util.gdb_exec_to_string(cmd)
            except:
                buf = '  [{}] {}'.format(i,'no-data')
                gdb_print (buf)
                steps += 1
                continue

            steps += 1
            v = buffer.split(':')[1].strip()
            v = hex(int(v, 16)) #remove leading 0s. e.g 0x000123 to 0x123
            buf = '  [{}] {}'.format(i, v)
            gdb_print (buf)

            if item_num > 10 and steps > 6 and show_snip == True:
                gdb_print("  ...")
                show_snip = False

        return


    def get_value_from_memory(self, addr, offset, length):
        """
        params:
          addr: int
          offset: int
          length: int

        returns:
          value in string
        """

        if m_debug.Debug: m_debug.dbg_print("addr=", addr, " offset=", offset, " length=", length)
        hex_string = None
        doulbe_string = None
        long_string = None
        int_string = None
        float_string = None
        word_string = None
        short_string = None
        byte_string = None
        ret = None
        if length == 8: # 8 byte, could be a long, a 8 byte address ptr, or a double
            cmd = 'x/1gx ' + hex(addr + offset) # cmd to get 8 byte address
            try:
                hex_string = m_util.gdb_exec_to_string(cmd)
            except:
                if m_debug.Debug: m_debug.dbg_print()
                return None
            hex_string = hex_string.split()[1]
            hex_string = hex(int(hex_string,16))

            cmd = 'x/1gf ' + hex(addr + offset) # cmd to get 8 byte double value
            try:
                double_string = m_util.gdb_exec_to_string(cmd)
            except:
                if m_debug.Debug: m_debug.dbg_print()
                return None
            double_string = double_string.split()[1]

            cmd = 'x/1dg ' + hex(addr + offset) # cmd to get 8 byte long value
            try:
                long_string = m_util.gdb_exec_to_string(cmd)
            except:
                if m_debug.Debug: m_debug.dbg_print()
                return None
            long_string = long_string.split()[1]

            ret = 'hex:' + hex_string + ',long:' + long_string + ',double:' + double_string
            if m_debug.Debug: m_debug.dbg_print("ret=", ret)
            return ret
        elif length == 4: # 4 byte,could be int, float, hex
            cmd = 'x/1xw ' + hex(addr + offset) # cmd to get 4 byte hex address
            try:
                hex_string = m_util.gdb_exec_to_string(cmd)
            except:
                if m_debug.Debug: m_debug.dbg_print()
                return None
            hex_string = hex_string.split()[1]

            cmd = 'x/1dw ' + hex(addr + offset) # cmd to get 4 byte int
            try:
                int_string = m_util.gdb_exec_to_string(cmd)
            except:
                if m_debug.Debug: m_debug.dbg_print()
                return None
            int_string = int_string.split()[1]

            cmd = 'x/1fw ' + hex(addr + offset) # cmd to get 4 byte float
            try:
                float_string = m_util.gdb_exec_to_string(cmd)
            except:
                if m_debug.Debug: m_debug.dbg_print()
                return None
            float_string = float_string.split()[1]

            ret = 'hex:'+hex_string+',int:'+int_string+',float:'+float_string
            if m_debug.Debug: m_debug.dbg_print("ret=", ret)
            return ret
        elif length == 2: # 2 byte, could be short, hex, 2 character
            cmd = 'x/1xh ' + hex(addr + offset) # cmd to get 2 byte hex address
            try:
                hex_string = m_util.gdb_exec_to_string(cmd)
            except:
                if m_debug.Debug: m_debug.dbg_print()
                return None
            hex_string = hex_string.split()[1]

            cmd = 'x/1dh ' + hex(addr + offset) # cmd to get 2 byte short int
            try:
                short_string = m_util.gdb_exec_to_string(cmd)
            except:
                if m_debug.Debug: m_debug.dbg_print()
                return None
            short_string = short_string.split()[1]

            cmd = 'x/2b ' + hex(addr + offset) # cmd to get 2 byte characters
            try:
                byte_string = m_util.gdb_exec_to_string(cmd)
            except:
                if m_debug.Debug: m_debug.dbg_print()
                return None
            byte_string = byte_string.split(':')[1].strip()

            ret = 'hex:'+hex_string+',short:'+short_string+',byte:'+byte_string
            if m_debug.Debug: m_debug.dbg_print("ret=", ret)
            return ret
        elif length == 1: # 1 byte. could be hex, c
            cmd = 'x/1xb ' + hex(addr + offset) # cmd to get 1 byte hex
            try:
                hex_string = m_util.gdb_exec_to_string(cmd)
            except:
                if m_debug.Debug: m_debug.dbg_print()
                return None
            hex_string = hex_string.split()[1]

            cmd = 'x/1cb ' + hex(addr + offset) # cmd to get 1 byte characters
            try:
                byte_string = m_util.gdb_exec_to_string(cmd)
            except:
                if m_debug.Debug: m_debug.dbg_print()
                return None
            byte_string = byte_string.split(':')[1].strip()

            ret = 'hex:'+hex_string+',byte:'+byte_string
            if m_debug.Debug: m_debug.dbg_print("ret=", ret)
            return ret
        else:
            if m_debug.Debug: m_debug.dbg_print()
            return None

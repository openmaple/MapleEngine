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
import os
import sys
import m_set
import json

def debug_print(filename, line, func, *args, **kwargs):
    """
    Maple debugger's verbose mode print, aka the debugger prints of Maple debugger
    """
   
    if not 'verbose' in m_set.msettings:
        return
    if m_set.msettings['verbose'] == 'off':
        return

    short_name = filename.split('/')[-1]

    #Iterate over all args, convert them to str, and join them
    args_str = ' '.join(map(str,args))

    #Iterater over all kwargs, convert them into k=v and join them
    kwargs_str = ' '.join('{}={}'.format(k,v) for k,v in kwargs.items())

    #Form the final representation by adding func name
    buffer = "{}:{}():{}: {}".format(short_name, func.__name__, line, ' '.join([args_str, kwargs_str]))
    print(buffer)

def load_maple_debugger_config(filename):
    with open(filename, 'r') as f:
        data_dict = json.load(f)
    if not data_dict:
        return None
    else:
        return data_dict

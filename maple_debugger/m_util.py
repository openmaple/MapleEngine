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
import sys
import os
import re
import subprocess

Colored = False
Highlighted = False
HighlightStyle = 'breeze'

class MColors:
    BRED = '\033[91m'
    BGREEN = '\033[92m'
    BYELLOW = '\033[93m'
    BBLUE = '\033[94m'
    BMAGENTA = '\033[95m'
    BCYAN = '\033[96m'
    DRED = '\033[31m'
    DGREEN = '\033[32m'
    DYELLOW = '\033[33m'
    DBLUE = '\033[34m'
    BOLD = '\033[1m'
    UNDERLINE = '\033[4m'

    ENDC = ''

    NM_SYMBOL = ''
    NM_METHOD = ''

    BT_ADDR = ''
    BT_FNNAME = ''
    BT_SRC = ''
    BT_ARGNAME = ''

    BP_ADDR = ''
    BP_ATTR = ''
    BP_SYMBOL = ''
    BP_STATE_GR = ''
    BP_STATE_RD = ''

    TP_CNAME = ''
    TP_MNAME = ''
    SP_SNAME = ''
    MP_FSYNTAX = ''
    MP_CNAME = ''

    def init_maple_colors(colored = True):
        # restore it in case disable color was called
        MColors.ENDC = '\033[0m' if colored else ''

        # For all symbols
        MColors.NM_SYMBOL = MColors.BCYAN if colored else ''
        MColors.NM_METHOD = MColors.BMAGENTA if colored else ''

        # Maple Colors used for trace frames.
        MColors.BT_ADDR = MColors.BBLUE if colored else ''
        MColors.BT_FNNAME = MColors.DYELLOW if colored else ''
        MColors.BT_SRC = MColors.DGREEN if colored else ''
        MColors.BT_ARGNAME = MColors.BBLUE if colored else ''

        # Maple Colors used for breakpoints
        MColors.BP_ADDR = MColors.BBLUE if colored else ''
        MColors.BP_ATTR = MColors.BMAGENTA if colored else ''
        MColors.BP_SYMBOL = MColors.NM_SYMBOL if colored else ''
        MColors.BP_STATE_GR = MColors.BGREEN if colored else ''
        MColors.BP_STATE_RD = MColors.BRED if colored else ''

        # Maple Colors used for mtype prints
        MColors.TP_CNAME = MColors.BBLUE if colored else '' # class name
        MColors.TP_MNAME = MColors.NM_SYMBOL  # method name

        # Maple Colors used for msymbol prints
        MColors.SP_SNAME = MColors.NM_SYMBOL  # symbol name

        # Maple Colors used for mprint
        MColors.MP_CNAME = MColors.BBLUE if colored else ''
        MColors.MP_FSYNTAX = MColors.BYELLOW if colored else ''

def color_symbol(c, sym):
    """ Color symbol
    """
    if not Colored:
        return sym;
    s = sym.split('_7C')
    if len(s) != 3:
        return sym
    return ''.join([c, s[0],'_7C', MColors.NM_METHOD, s[1], c, '_7C', s[2], MColors.ENDC])

def enable_color_output(c = True):
    """ Enable colored output if possible

    Args:
        c (boolean): True to enable colored output if possible
                     False to disable colored output

    Returns:
    """
    global Colored, Highlighted
    if c:
        stdout = os.readlink('/proc/%s/fd/1' % str(os.getpid()))
        c = True if stdout.startswith("/dev/pts/") else False
    Colored = c
    exe = '/usr/bin/highlight'
    Highlighted = True if c and os.path.isfile(exe) and os.access(exe, os.X_OK) else False
    MColors.init_maple_colors(colored = Colored)

def gdb_exec(cmd):
    """Execute a gdb command with gdb.execute()

    Args:
       cmd (string): gdb command to be executed

    Returns:
    """
    gdb.execute(cmd)

mretrace = re.compile(r'^- [- ]*<-- .*|^\+ [+ ]*==> .*', re.M)
def gdb_exec_to_null(cmd):
    """Execute a gdb command with gdb.execute()
    The output of gdb will be thrown away.
    If it is in the trace mode of Maple debugger, all trace output will be printed to stderr.

    Args:
       cmd (string): gdb command to be executed

    Returns:
    """
    if sys.getprofile():
        buf = gdb.execute(cmd, to_string=True)
        for m in re.findall(mretrace, buf):
            gdb.write(m +" @_@\n", gdb.STDERR)
    else:
        gdb.execute(cmd, to_string=True)

def gdb_exec_to_string(cmd):
    """Execute a gdb command with gdb.execute()
    The output of gdb will be put into a string and return it.
    It should not be used to execute a Maple debugger command.

    Args:
       cmd (string): gdb command to be executed

    Returns:
       A string which includes the output of the gdb command
    """
    return gdb.execute(cmd, to_string=True)

def gdb_echo_and_exec(cmd):
    """Echo a gdb command and execute it with gdb.execute()

    Args:
       cmd (string): gdb command to be echoed and executed

    Returns:
    """
    gdb.execute('echo (gdb_exec) ' + cmd + '\\n')
    gdb.execute(cmd)

def gdb_print(string, stream = gdb.STDOUT):
    """A wrapper function of gdb.write with '\n' to the end of string
    """
    gdb.write(string + '\n', stream)

def gdb_write(string, stream = gdb.STDOUT):
    """A wrapper function of gdb.write
    """
    gdb.write(string, stream)


def shell_cmd(cmd):
    """Execute a command through shell without stderr output
    """
    command = subprocess.Popen(cmd, stdout=subprocess.PIPE, shell=True)
    (output, err) = command.communicate()
    return output.decode("utf-8")

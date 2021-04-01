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

import lldb
import sys
import os
import re
import subprocess

#Colored = False
Colored = True
Highlighted = False
HighlightStyle = 'breeze'

# Constants
ESC = '\033['
ALLOFF = ' \033[0m'
# Attribute codes
acode = dict( [('alloff', 0),
              ('bold', 1),
              ('faint', 2),
              ('italic', 3),
              ('underline', 4),
              ('sblink', 5),
              ('fblink', 6),
              ('reverse', 7),
              ('strike', 9),
              ('normal', 22),
              ('black', 30),
              ('red', 31),
              ('green', 32),
              ('yellow', 33),
              ('blue', 34),
              ('magenta', 35),
              ('cyan', 36),
              ('white', 37),
              ('foreground', 38),
              ('background', 48),
              ('superscript', 73),
              ('subscript', 74),
              ('endoffset', 20),
              ('brightoffset', 60),
              ('backoffset', 10)])

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

    @staticmethod
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
        MColors.MP_STR_V = MColors.BGREEN if colored else ''

    @staticmethod
    def colorize(string, color, backgr, bright):
        if not Colored:
            return string
        set_color = acode[color]
        if(backgr):
            set_color += acode['backoffset']
        if(bright):
            set_color += acode['brightoffset']
        retstr = str(ESC+str(set_color)+"m"+str(string)+ALLOFF)
        return(retstr)

    @staticmethod
    def black(string):
        return MColors.colorize(string, 'black', False, False)
    @staticmethod
    def red(string):
        return MColors.colorize(string, 'red', False, False)
    @staticmethod
    def green(string):
        return MColors.colorize(string, 'green', False, False)
    @staticmethod
    def yellow(string):
        return MColors.colorize(string, 'yellow', False, False)
    @staticmethod
    def blue(string):
        return MColors.colorize(string, 'blue', False, False)
    @staticmethod
    def magenta(string):
        return MColors.colorize(string, 'magenta', False, False)
    @staticmethod
    def cyan(string):
        return MColors.colorize(string, 'cyan', False, False)
    @staticmethod
    def white(string):
        return MColors.colorize(string, 'white', False, False)
    @staticmethod
    def lblack(string):
        return MColors.colorize(string, 'black', False, True)
    @staticmethod
    def lred(string):
        return MColors.colorize(string, 'red', False, True)
    @staticmethod
    def lgreen(string):
        return MColors.colorize(string, 'green', False, True)
    @staticmethod
    def lyellow(string):
        return MColors.colorize(string, 'yellow', False, True)
    @staticmethod
    def lblue(string):
        return MColors.colorize(string, 'blue', False, True)
    @staticmethod
    def lmagenta(string):
        return MColors.colorize(string, 'magenta', False, True)
    @staticmethod
    def lcyan(string):
        return MColors.colorize(string, 'cyan', False, True)
    @staticmethod
    def lwhite(string):
        return MColors.colorize(string, 'white', False, True)
    @staticmethod
    def b_black(string):
        return MColors.colorize(string, 'black', True, False)
    @staticmethod
    def b_red(string):
        return MColors.colorize(string, 'red', True, False)
    @staticmethod
    def b_green(string):
        return MColors.colorize(string, 'green', True, False)
    @staticmethod
    def b_yellow(string):
        return MColors.colorize(string, 'yellow', True, False)
    @staticmethod
    def b_blue(string):
        return MColors.colorize(string, 'blue', True, False)
    @staticmethod
    def b_magenta(string):
        return MColors.colorize(string, 'magenta', True, False)
    @staticmethod
    def b_cyan(string):
        return MColors.colorize(string, 'cyan', True, False)
    @staticmethod
    def b_white(string):
        return MColors.colorize(string, 'white', True, False)
    @staticmethod
    def b_lblack(string):
        return MColors.colorize(string, 'black', True, True)
    @staticmethod
    def b_lred(string):
        return MColors.colorize(string, 'red', True, True)
    @staticmethod
    def b_lgreen(string):
        return MColors.colorize(string, 'green', True, True)
    @staticmethod
    def b_lyellow(string):
        return MColors.colorize(string, 'yellow', True, True)
    @staticmethod
    def b_lblue(string):
        return MColors.colorize(string, 'blue', True, True)
    @staticmethod
    def b_lmagenta(string):
        return MColors.colorize(string, 'magenta', True, True)
    @staticmethod
    def b_lcyan(string):
        return MColors.colorize(string, 'cyan', True, True)
    @staticmethod
    def b_lwhite(string):
        return MColors.colorize(string, 'white', True, True)

    @staticmethod
    def attributize(string, attribute):
        if not Colored:
            return string
        set_attr = acode[attribute]
        set_attr_off = set_attr + acode['endoffset']
        retstr = str(ESC+str(set_attr)+"m "+string+ESC+str(set_attr_off)+"m")
        return retstr

    @staticmethod
    def bold(string):
        return MColors.attributize(string, 'bold')
    @staticmethod
    def faint(string):
        return MColors.attributize(string, 'faint')
    @staticmethod
    def italic(string):
        return MColors.attributize(string, 'italic')
    @staticmethod
    def underline(string):
        return MColors.attributize(string, 'underline')
    @staticmethod
    def sblink(string):
        return MColors.attributize(string, 'sblink')
    @staticmethod
    def fblick(string):
        return MColors.attributize(string, 'fblick')
    @staticmethod
    def reverse(string):
        return MColors.attributize(string, 'reverse')
    @staticmethod
    def strike(string):
        return MColors.attributize(string, 'strike')
    @staticmethod
    def superscript(string):
        return MColors.attributize(string, 'superscript')
    @staticmethod
    def subscript(string):
        return MColors.attributize(string, 'subscript')

    # For all symbols
    @staticmethod
    def nm_symbol(string):
        return MColors.b_cyan(string)
    @staticmethod
    def nm_method(string):
        return MColors.b_magenta(string)

        # Maple Colors used for trace frames.
    @staticmethod
    def bt_addr(string):
        return MColors.lblue(string)
    @staticmethod
    def bt_fnname(string):
        return MColors.yellow(string)
    @staticmethod
    def bt_src(string):
        #TODO: implement fainr as dark
        #return MColors.dgreen(string)
        return MColors.green(string)
    @staticmethod
    def bt_argname(string):
        return MColors.lblue(string)

    # Maple Colors used for breakpoints
    @staticmethod
    def bp_addr(string):
        return MColors.lblue(string)
    @staticmethod
    def bp_attr(string):
        return MColors.b_magenta(string)
    @staticmethod
    def bp_symbol(string):
        return MColors.nm_symbol(MColors.underline(string))
    @staticmethod
    def bp_state_gr(string):
        return MColors.lgreen(string)
    @staticmethod
    def bp_state_rd(string):
        return MColors.lred(string)

    # Maple Colors used for mtype prints
    @staticmethod
    def tp_cname(string):
        return MColors.lblue(string)
    @staticmethod
    def tp_mname(string):
        return MColors.nm_symbol(MColors.bold(string))

    # Maple Colors used for msymbol prints
    @staticmethod
    def sp_sname(string):
        return MColors.nm_symbol(MColors.italic(string))

    # Maple Colors used for mprint
    @staticmethod
    def mp_cname(string):
        return MColors.lblue(string)
    @staticmethod
    def mp_syntax(string):
        return MColors.lyellow(string)
    @staticmethod
    def mp_str_v(string):
        return MColors.lgreen(string)

def is_interactive():
    """ check if mdb runs in interactive mode
    returns: True if mdb runs in interactive mode
    """
    stdout = os.readlink('/proc/%d/fd/1' % os.getpid())
    return True if stdout.startswith("/dev/pts/") else False

def color_symbol(c, sym):
    """ color symbol
    """
    if not Colored:
        return sym;
    s = sym.split('_7C')
    if len(s) != 3:
        return sym
    return ''.join([c, s[0],'_7C', MColors.NM_METHOD, s[1], c, '_7C', s[2], MColors.ENDC])

def enable_color_output(c = True):
    """ enable colored output if possible

    args:
        c (boolean): True to enable colored output
                     False to disable colored output

    returns:
    """
    global Colored, Highlighted
    Colored = c
    MColors.init_maple_colors(colored = c)
    exe = '/usr/bin/highlight'
    Highlighted = True if c and os.path.isfile(exe) and os.access(exe, os.X_OK) else False

def mdb_exec(cmd):
    """execute a mdb command with mdb.execute()

    args:
       cmd (string): mdb command to be executed

    returns:
    """
    #mdb.execute(cmd)
    mdb_exec_to_str(cmd)

mretrace = re.compile(r'^- [- ]*<-- .*|^\+ [+ ]*==> .*', re.M)
def mdb_exec_to_null(cmd):
    """execute a mdb command with mdb.execute()
    The output of mdb will be thrown away.
    If it is in the trace mode of Maple debugger, all trace output will be printed to stderr.

    args:
       cmd (string): mdb command to be executed

    returns:
    """
#    if sys.getprofile():
#        buf = mdb.execute(cmd, to_string=True)
#        for m in re.findall(mretrace, buf):
#            #mdb.write(m +" @_@\n", mdb.STDERR)
#            mdb.write(m +" @_@\n");
#    else:
#        mdb.execute(cmd, to_string=True)
    mdb_exec_to_str(cmd)

def mdb_exec_to_str(cmd):
    """execute a lldb command with mdb.execute()
    The output of mdb will be put into a string and return it.
    It should not be used to execute a Maple debugger command.

    args:
       cmd (string): mdb command to be executed

    returns:
       A string which includes the output of the mdb command
    """
    #return mdb.execute(cmd, to_string=True)
    # Retrieve the associated command interpreter from our debugger.
    ci = lldb.debugger.GetCommandInterpreter()
    res = lldb.SBCommandReturnObject()
    ci.HandleCommand(cmd, res)

    #print("Cmd:", cmd, "Output:", res.GetOutput())
    return(res.GetOutput())

def mdb_echo_exec(cmd):
    """echo a mdb command and execute it with mdb.execute()

    args:
       cmd (string): mdb command to be echoed and executed

    returns:
    """
    mdb_write('echo (mdb_exec) %s\n' % cmd)
    mdb_exec_to_str(cmd)

#def mdb_print(string, stream = mdb.STDOUT):
def mdb_print(string):
    """a wrapper function of mdb.write with '\n' to the end of string
    """
    #mdb.write(string + '\n', stream)
    print(string)

#def mdb_write(string, stream = mdb.STDOUT):
def mdb_write(string):
    """a wrapper function of mdb.write
    """
    #mdb.write(string, stream)
    print(string + '\n')

def shell_cmd(cmd):
    """execute a command through shell and return results decoded from its stdout
    """
    subproc = subprocess.Popen(cmd, stdout=subprocess.PIPE, shell=True)
    (output, err) = subproc.communicate()
    return output.decode("utf-8")

def proc_popen(cmd):
    """execute a command with Popen of subprocess in background
    """
    subproc = subprocess.Popen(cmd, stdout=subprocess.PIPE, shell=True, env={'PYTHONIOENCODING':'latin1'})
    return subproc

def proc_communicate(subproc):
    """get the stdout output of subprocess, decode it and return results
    """
    output = subproc.communicate()[0]
    return output

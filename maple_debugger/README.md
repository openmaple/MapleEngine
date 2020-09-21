```
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
```
## Table of contents
* [General info](#general-info)
* [Prerequisite](#Prerequisite)
* [Recommendation](#Recommendation)
* [Configuration](#Configuration)
* [Launch Up](#Launch-Up)
* [Commands and Categories](#Commands-and-Categories)
* [Summary of Commands](#Summary-of-Commands)


## General info
Maple Multi-language Debugger, short for Maple Debugger, is an important part of Ark Programming Eco-system. As an extention of GDB, Maple Debugger works with Maple Engine, for users to debug their Maple application. Specifically source code that was compiled by the Maple Compiler and then executed by the Maple Engine.


## Prerequisite
* Python version: 3.5 and later
* GDB version: 8.1 and later
* Build a user application, see https://gitee.com/openarkcompiler-incubator/maple_engine
* Installation of the highlight package. If not installed, run `sudo apt install highlight`


## Recommendation
OpenJDK source code that was used to build libcore library, please place it under **~/my_openjdk8**. Maple Debugger uses this to search the source code that builds your library.

Please refer to maple_engine/maple_build/docs/build_OpenJDK8.md, or https://gitee.com/openarkcompiler-incubator/maple_engine for details.


## Configuration
Maple Debugger has a default configuration file named as **.mgdbinit** under the same directory as the Maple Debugger. This file is used when launching the Maple Debugger and provides initial settings upon the startup of gdb. Users can modify it to change, remove or add new valid settings as needed.

Maple Debugger default configuration file **.mgdbinit** is shown as below:
```python
# python3 and gdb initial commands for Maple debugger
python
import os.path
gdb.execute(os.path.expandvars('source $MAPLE_DEBUGGER_ROOT/m_gdb.py'))
end

# handle SIGUSR2
handle SIGUSR2 pass noprint nostop

# add one maple_lib_asm_path into the directory search list
mset -add maple_lib_asm_path $MAPLE_DEBUGGER_LIBCORE/

# add another maple_lib_asm_path into the directory search list
mset -add maple_lib_asm_path ./

# add openjdk8 source code file search path in your development environment
msrcpath -add $JAVA_CORE_SRC/build/linux-x86_64-normal-server-release/
msrcpath -add $JAVA_CORE_SRC/jdk/src/

# add the local path to your application source code in your development environment
msrcpath -add ./

python
from m_util import gdb_echo_exec as exec
for v in [os.path.expanduser('~/.mgdbinit'), './.mgdbinit']:
    if os.path.isfile(v):
        exec('source %s' % v)
end
```

## Launch Up
Check out Maple Debugger, and assume it is placed at ~/gitee/maple_engine/maple_debugger.

To launch the Maple Debugger along with user's application code using example of HelloWorld
```
$ cd ~/gitee/maple_engine/maple_build/examples/HelloWorld
$ source ~/gitee/maple_engine/envsetup.sh
$ "$MAPLE_BUILD_TOOLS"/run-app.sh -gdb -classpath ./HelloWorld.so HelloWorld
```

## Commands and Categories
* **Breakpoint**: _mbreakpoint_
* **Stack**     : _mbacktrace_, _mup_, _mdown_
* **Data**      : _mlocal_, _mprint_, _mtype_, _msymbol_
* **File**      : _mlist_, _msrcpath_
* **Control**   : _mstepi_, _mnexti_, _mfinish_
* **Misc**      : _mset_, _mhelp_


## Summary of Commands

* _mbreakpoint_: Sets and manages Maple breakpoints
* _mbacktrace_ : Displays Maple backtrace in multiple modes
* _mup_        : Selects and prints Maple stack frame that called the current one
* _mdown_      : Selects and prints Maple stack frame called by the current one
* _mlist_      : Lists source code in multiple modes
* _msrcpath_   : Adds and manages the Maple source code search path
* _mlocal_     : Displays selected Maple frame arguments and local variables
* _mprint_     : Prints Maple runtime object data
* _mtype_      : Prints a matching class and its inheritance hierarchy by a given search expression
* _msymbol_    : Prints a matching symbol list or its detailed infomatioin
* _mstepi_     : Steps specified number of Maple instructions
* _mstep_      : Steps program until it reaches a different source line of Maple Application
* _mnexti_     : Steps one Maple instruction, but proceed through subroutine calls
* _mnext_      : Steps program, proceeding through subroutine calls of Maple application
* _mfinish_    : Executes until selected Maple stack frame returns
* _mset_       : Sets and displays Maple debugger settings
* _mhelp_      : Prints command help and usage

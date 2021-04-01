```
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
Maple Multi-language Debugger, or Maple Debugger for short, is an important part of Ark Programming Eco-system. As an extention of GDB, the Maple Debugger works with the Maple Engine, to enable users to debug their Maple applications. To be specific, applications are the source code that was compiled by the Maple Compiler and subsequently executed by the Maple Engine.

Maple Debugger supports languages, both static (e.g. Java) and dynamic language (e.g. JavaScript), using same set of debugger code. On the execution of the Maple Engine runtime, the Maple Debugger dynamically determines which language is currently running, and therefore makes adapts the Maple Debugger commands to run for that language.

## Prerequisites
* Python version: 3.5 and later
* GDB version: 8.1 and later
* A user application for debugging, see https://gitee.com/openarkcompiler-incubator/maple_engine
* Installation of the highlight package. If not installed, run `sudo apt install highlight`


## Recommendation
Please place the OpenJDK source code that was used to build the libcore library under **~/my_openjdk8**. The Maple Debugger uses this to find and display the source code that built your library.

Please refer to maple_engine/maple_build/docs/build_OpenJDK8.md, or https://gitee.com/openarkcompiler-incubator/maple_engine for details.

Note that for JavaScript support, there are no particular requirements for Maple Debugger.

## Configuration
Maple Debugger has a default configuration file named **.mdbinit** under the same directory as the Maple Debugger. This file is used when launching the Maple Debugger and provides initial settings upon the startup of gdb. Users can modify it to change, remove or add new valid settings as needed.

For JavaScript support, there is no particular requirement for Maple Debugger

The default Maple Debugger configuration file **.mdbinit** is shown as below:
```python
# python3 and gdb initial commands for Maple debugger
python
import os.path
gdb.execute(os.path.expandvars('source $MAPLE_DEBUGGER_ROOT/m_gdb.py'))
end

# handle SIGUSR2
handle SIGUSR2 pass noprint nostop

# add one maple_lib_asm_path directory into the search path list
mset -add maple_lib_asm_path $MAPLE_DEBUGGER_LIBCORE/

# add another maple_lib_asm_path directory into the search path list
mset -add maple_lib_asm_path ./

# add the openjdk8 source code to the file search path in your development environment
msrcpath -add $JAVA_CORE_SRC/build/linux-x86_64-normal-server-release/
msrcpath -add $JAVA_CORE_SRC/jdk/src/

# add the local path to your application's source code in your development environment
msrcpath -add ./

python
from m_util import gdb_echo_exec as exec
for v in [os.path.expanduser('~/.mdbinit'), './.mdbinit']:
    if os.path.isfile(v):
        exec('source %s' % v)
end
```

## Launching
Check out Maple Debugger, and assume it is placed at ~/gitee/maple_engine/maple_debugger.

## Launching a Java language based program
To launch the Maple Debugger along with your user application code using the HelloWorld example, do the following:
```
$ cd ~/gitee/maple_engine/maple_build/examples/HelloWorld
$ source ~/gitee/maple_engine/envsetup.sh
$ "$MAPLE_BUILD_TOOLS"/run-app.sh -gdb -classpath ./HelloWorld.so HelloWorld
```

## Launching a JavaScript language based program
To launch the Maple Debugger along with your user JavaScript application code using the HelloWorld example, do the following:
```
$ cd ~/gitee/maple_engine/maple_build/examples/JavaScript/add/
$ source ~/gitee/maple_engine/envsetup.sh
$ "$MAPLE_BUILD_TOOLS"/run-js-app.sh add.js 
$ "$MAPLE_BUILD_TOOLS"/run-js-app.sh -gdb add.so
```


## Commands and Categories
* **Breakpoint**: _mbreakpoint_
* **Stack**     : _mbacktrace_, _mup_, _mdown_
* **Data**      : _mlocal_, _mprint_, _mtype_, _msymbol_
* **File**      : _mlist_, _msrcpath_
* **Control**   : _mstepi_, _mnexti_, _mfinish_
* **Misc**      : _mset_, _mhelp_
**Note, command _mlocal_, _mtype_, _msymbol_ don't have JavaScript support yet in Maple Debugger version 2.0**

## Summary of Commands

* _mbreakpoint_: Sets and manages Maple breakpoints
* _mbacktrace_ : Displays Maple backtrace in multiple modes
* _mup_        : Selects and prints the Maple stack frame that called the current one
* _mdown_      : Selects and prints the Maple stack frame called by the current one
* _mlist_      : Lists source code in multiple modes
* _msrcpath_   : Adds and manages the Maple source code search path
* _mlocal_     : Displays selected Maple frame arguments and local variables and stack dynamic data
* _mprint_     : Prints Maple runtime object data
* _mtype_      : Prints a matching class and its inheritance hierarchy by a given search expression
* _msymbol_    : Prints a matching symbol list or its detailed infomation
* _mstepi_     : Steps a specified number of Maple instructions
* _mstep_      : Steps the program until it reaches a different source line of the Maple Application
* _mnexti_     : Steps one Maple instruction, but proceed through subroutine calls
* _mnext_      : Steps the program, proceeding through subroutine calls of the Maple application
* _mfinish_    : Executes until the selected Maple stack frame returns
* _mset_       : Sets and displays the Maple debugger settings
* _mhelp_      : Prints help for commands and their usage




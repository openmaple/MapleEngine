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
Maple Multi-language Debugger, short for Maple Debugger, is an important part of Ark Programming Eco-system. As an extention of GDB, Maple Debugger works with Maple Engine, for users to debug their Maple application, code that was compiled by Maple Compiler and then executed by Maple Engine.


## Prerequisite
* Python version: 3.5 and later
* GDB version: 8.1 and later
* Build user's application, see https://gitee.com/openarkcompiler-incubator/maple_engine


## Recommendation
OpenJDK source code that was used to build libcore library, please place it under **~/my_openjdk8**. Maple Debugger uses this to search the source code that builds your library.

Please refer to maple_engine/maple_build/docs/build_OpenJDK8.md, or https://gitee.com/openarkcompiler-incubator/maple_engine for detail.


## Configuration
Maple Debugger has a default configuration file named as .mgdbinit under the same directory of Maple Debugger. This file is to launch the Maple Debugger and provide initial settings upon the launch up of gdb. User can modify it to add new valid settings as needed.

Maple Debugger default configuration file .mgdbinit is shown as below:
```
python
import os.path
gdb.execute(os.path.expandvars('source $MAPLE_DEBUGGER_ROOT/m_gdb.py'))
end

# add one maple_lib_asm_path into the search list
mset -add maple_lib_asm_path $MAPLE_DEBUGGER_ROOT/out/x86_64/

# add another one maple_lib_asm_path into the search list
mset -add maple_lib_asm_path ./

# add openjdk8 source code file search path in your development environment
msrcpath -add $JAVA_CORE_SRC/build/linux-x86_64-normal-server-release/
msrcpath -add $JAVA_CORE_SRC/jdk/src/

# add local path where your application source code is in your development environment
msrcpath -add ./
```

## Launch Up
Check out Maple Debugger, and assume it is placed at ~/gitee/maple_engine/maple_debugger.

To launch the Maple Debugger along with user's application code using example of HelloWorld
```
$ cd ~/gitee/maple_engine/maple_build/examples/HelloWorld
$ source ~/gitee/maple_engine/envsetup.sh
$ "$MAPLE_BUILD_TOOLS"/run-app.sh -gdb -classpath ./HelloWorld.so HelloWorld
```

Maple Debugger can work with demangling tool provided by Maple Engine, to display Maple symbols (including function names, class object names, etc) in a demangled format with different colors. This is a feature for developers to see same output with demangled and mangled format in one command. To enable this feature, launch Maple Debugger using following commands
```
$ cd ~/gitee/maple_engine/maple_build/examples/HelloWorld
$ source ~/gitee/maple_engine/envsetup.sh
$ "$MAPLE_BUILD_TOOLS"/run-app.sh -gdb -classpath ./HelloWorld.so HelloWorld | "$MAPLE_BUILD_TOOLS"/demangle.sh
```


## Commands and Categories
* **Breakpoint**: _mbreakpoint_
* **Stack**     : _mbacktrace_, _mup_, _mdown_
* **Data**      : _mlocal_, _mprint_, _mtype_
* **File**      : _mlist_, _msrcpath_
* **Control**   : _mstepi_, _mnexti_
* **Misc**      : _mset_, _mhelp_


## Summary of Commands

* _mbreakpoint_: Set and manage Maple breakpoints
* _mbacktrace_ : Display Maple backtrace in multiple modes
* _mup_        : Select and print Maple stack frame that called this one
* _mdown_      : Select and print Maple stack frame called by this ones
* _mlist_      : List source code in multiple modes
* _msrcpath_   : Add and manage the Maple source code search path
* _mlocal_     : Display selected Maple frame arguments and local variables
* _mprint_     : Print Maple runtime object data
* _mtype_      : Print a matching class and its inheritance hierarchy by a given search expression
* _mstepi_     : Step specified number of Maple instructions
* _mnexti_     : Step one Maple instruction, but proceed through subroutine calls
* _mset_       : Sets and displays Maple debugger settings

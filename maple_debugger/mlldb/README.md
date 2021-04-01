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
* [Launch Up](#Launch-Up)
* [Commands](#Commands)
* [Commands and Categories](#Commands-and-Categories)
* [Command Samples](#Command-Samples)


## General info
Maple Multi-language Debugger, short for Maple Debugger, is an important part of Ark Programming Eco-system. This is an **exploratory and experimental** extentsion of LLDB. Maple Debugger works with Maple Engine, for users to debug their Maple application. Specifically source code that was compiled by the Maple Compiler and then executed by the Maple Engine.


## Prerequisite
* Ubuntu 18.04.5 LTS
* Python version: 3.5 and later
* Installation of lldb version 11 or later
* The MAPLE Engine build environment setup and build with it's prerequisites. See https://gitee.com/openarkcompiler-incubator/maple_engine
* Installation of the highlight package. If not installed, run `sudo apt install highlight`


* In case user wants to upgrade old version of lldb to lldb version 11, following steps can be referenced.
```
# Cleanup for older versions
sudo apt-get autoremove lldb
sudo apt update
pushd /tmp
wget -O - https://apt.llvm.org/llvm-snapshot.gpg.key|sudo apt-key add -

# Install new from site.  This may not be necessary on updated Ubuntu distros
wget https://apt.llvm.org/llvm.sh
chmod +x ./llvm.sh
sudo ./llvm.sh 11
pushd /usr/bin
sudo ln -s lldb-11 lldb
echo The next line should report that lldb 11.0.x is being used:
lldb -v
```

## Recommendation
OpenJDK source code that was used to build libcore library, please place it under **~/my_openjdk8**. Maple Debugger uses this to search the source code that builds your library.

Please refer to maple_engine/maple_build/docs/build_OpenJDK8.md, or https://gitee.com/openarkcompiler-incubator/maple_engine for details.


## Launch Up
Check out Maple Engine from gitee, and we assume it is placed at ~/gitee/maple_engine/

To launch the Maple Debugger LLDB along with user's application code using example of HelloWorld

```
$ cd ~/gitee/maple_engine/maple_build/examples/HelloWorld
$ source ~/gitee/maple_engine/envsetup.sh
$ "$MAPLE_BUILD_TOOLS"/run-app.sh -lldb -classpath ./HelloWorld.so HelloWorld
```

General procedures for use:

1. Checkout the gitee maple_engine repository.
2. Review the readme and document files in that repository.
3. Setup the environment in the maple_engine directory using: "source ./envsetup.sh"
4. Build the maple engine following the repo directions
5. Build your source code. Alternatively you can build/use maple_build/examples projects for testing
6. In the tools directory of this repo a version of the run_app.sh has been enhanced to launch lldb
7. For example use: "$MAPLE_BUILD_TOOLS"/run-app.sh -lldb -classpath ./HelloWorld.so HelloWorld
   To run your application code on Maple Debugger LLDB


## Commands:
  **Currently, this is a proof of concept and is incomplete and untested!**

* mbreak --set symbol   # allows the setting of a Maple breakpoint
* mbt or mbacktrace     # allow you to see the maple stack leading to the current frame (-a, -m and -f options are availble)
* mlist                 # allow you to see the various source code of the current frame (-a and -m options are availble)
* mset                  # allows option setting like enable debug mode
* mhelp                 # for more Maple Debugger Commands details (command implemented and not implemented yet)


## Commands and Categories
* **Breakpoint**: _mbreakpoint_
* **Stack**     : _mbacktrace_, _mup_, _mdown_
* **Data**      : _mlocal_, _mprint_, _mtype_, _msymbol_
* **File**      : _mlist_, _msrcpath_
* **Control**   : _mstepi_, _mnexti_, _mfinish_
* **Misc**      : _mset_, _mhelp_


## Command Samples

* Launch up LLDB with HelloWorld.so
  * source $MAPLE_ENGINE_ROOT/envsetup.sh
  * launch up lldb with HelloWold.so
```
~/maple_engine_standalone/maple_engine/maple_build/examples/HelloWorld$ "$MAPLE_BUILD_TOOLS"/run-app.sh -lldb -classpath ./HelloWorld.so HelloWorld
(lldb) target create "/home/gitee/maple_engine_standalone/maple_engine/./maple_runtime/bin/x86_64/mplsh"
Current executable set to '/home/gitee/maple_engine_standalone/maple_engine/maple_runtime/bin/x86_64/mplsh' (x86_64).
(lldb) settings set -- target.run-args  "-Xbootclasspath:libcore.so" "-classpath" "./HelloWorld.so" "HelloWorld"
(lldb) command script import /home/gitee/maple_engine_standalone/maple_engine/./maple_debugger/mlldb/maple_debugger/LLDB/m_lldb.py
The "mdump-line-tables" and "mdump-files" commands have been installed.
Maple System (Version 0.5.0) for lldb Initialized
(lldb)
```

  * Use mbreak -s command to set a Maple breakpoint
```
(lldb) mbreak -s LHelloWorld_3B_7Cmain_7C_28ALjava_2Flang_2FString_3B_29V
set_breakpoint: mbp_exist= False mbp_id= None
(lldb)
```

  * Once Maple breakpoint is set, run the program, note the load error reported is a known issue, does not bother Maple Debugger
```
(lldb) r
Process 5724 launched: '/home/gitee/test-lldb/maple_engine/maple_runtime/bin/x86_64/mplsh' (x86_64)
error: ld-2.27.so 0x7fffffff0005c836: adding range [0x1480a-0x1487a) which has a base that is less than the function's low PC 0x14f80. Please file a bug and attach the file at the start of this error message
error: ld-2.27.so 0x7fffffff0005c836: adding range [0x14890-0x14896) which has a base that is less than the function's low PC 0x14f80. Please file a bug and attach the file at the start of this error message
error: ld-2.27.so 0x7fffffff0005c897: adding range [0x1480a-0x1487a) which has a base that is less than the function's low PC 0x14f80. Please file a bug and attach the file at the start of this error message
error: ld-2.27.so 0x7fffffff0005c897: adding range [0x14890-0x14896) which has a base that is less than the function's low PC 0x14f80. Please file a bug and attach the file at the start of this error message
1 location added to breakpoint 1
Process 5724 stopped
* thread #1, name = 'main', stop reason = breakpoint 1.1
    frame #0: 0x00007ffff5ae4e2a libmplre.so`maple::maple_invoke_method(mir_header=0x00007fffe07ae1b8, caller=0x00007fffffffd838) at invoke_method.cpp:152:20
(lldb)
```

  * Once program stops at Maple breakpoint, use Maple BackTrace command to get the trace
```
(lldb) mbt
Maple Traceback (most recent call first):
# 0 HelloWorld.so :0x11b8:0000:  LHelloWorld_3B_7Cmain_7C_28ALjava_2Flang_2FString_3B_29V(a64<> %2="\xe0\xc6\xd0\xf2\xff\U0000007f") at ./HelloWorld.java :19
(lldb)
```

  * Check the source code of where we are now
```
(lldb) mlist -a
Listing
asm file: /home/gitee/maple_engine_standalone/maple_engine/maple_build/examples/HelloWorld/HelloWorld.s line: 187
=>       187 LHelloWorld_3B_7Cmain_7C_28ALjava_2Flang_2FString_3B_29V_mirbin_info:
         188    .long  LHelloWorld_3B_7Cmain_7C_28ALjava_2Flang_2FString_3B_29V_mirbin_code - .
         189    .word  1, 5, 3, 4 // func storage info
         190    // PrimType of formal arguments
         191    .byte  0xe, 0x0 // %2
         192    // PrimType of automatic variables
         193    .byte  0x1, 0x0 // %%retval
         194    .byte  0x23, 0x0        // %%thrownval
         195    .byte  0xe, 0x2 // Reg0_R1004
         196    .byte  0xe, 0   // %1
         197    .byte  0xe, 0   // %3
(lldb)
```


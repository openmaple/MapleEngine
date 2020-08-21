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
## Before Start
Once the Maple Debugger is launched, following messages will be displayed

```
Before start to use Maple debugger, please make sure to setup two sets of search paths:

1, The search paths of user's program and library source code.
   This is required to display source code of your application and library
   Use msrcpath command to show/add/del source code search paths
2, The search paths of the generated assembly files of user's program and library.
   This is required by many of Maple debugger commands
   Use mset command to show/add/del libaray asm file search paths
```
On #1, a search path list should be set, for Maple Debugger to search the source code of libraries and user's application source code. mlist command uses this search list to look up the source code files.
On #2, a search path list should be set, for Maple Debugger to search assembly files at the time of co-responding .so file is compiled. mbt, mup, mdown and other commands use this search list to look up a lot of information from the assembly files.

## Maple Command Summary
* **_mbreakpoint_**: Set and manage Maple breakpoints
* **_mbacktrace_** : Display Maple backtrace in multiple modes
* **_mup_**        : Select and print Maple stack frame that called this one
* **_mdown_**      : Select and print Maple stack frame called by this one
* **_mlist_**      : List source code in multiple modes
* **_msrcpath_**   : Add and manage the Maple source code search paths
* **_mlocal_**     : Display selected Maple frame arguments, local variables and stack dynamic data
* **_mprint_**     : Print Maple runtime object data
* **_mtype_**      : Print a matching class and its inheritance hierarchy by a given search expression
* **_mstepi_**     : Step specified number of Maple instructions
* **_mnexti_**     : Step one Maple instruction, but proceed through subroutine calls
* **_mset_**       : Sets and displays Maple debugger settings
* **_mhelp_**      : Help command, Usage etc

## Maple Breakpoint command
### mbreak command
**_mb_** is the alias of the mbreak command
#### 1. Syntax
(gdb) mbreak
mbreak <symbol>: set a new Maple breakpoint at symbol
mbreak -set <symbol>: same to 'mbreak <symbol>
mbreak -enable <symbol|index>: enable an existing Maple breakpoint at symbol
mbreak -disable <symbol|index>: disable an existing Maple breakpoint at symbol
mbreak -clear <symbol|index>: delete an existing Maple breakpoint at symbol
mbreak -clearall : delete all existing Maple breakpoints
mbreak -listall : list all existing Maple breakpoints
mbreak -ignore <symbol | index> <count> : set ignore count for specified Maple breakpoints
mbreak : usage and syntax 

#### 2. Set Maple breakpoint
use command 'mbreak <mangled Maple symbol>' or 'mbreak -set <mangled Maple symbol>'
example1:
```
(gdb) mb Ljava_2Flang_2FThreadGroup_3B_7CcheckAccess_7C_28_29V
set breakpoint  Ljava_2Flang_2FThreadGroup_3B_7CcheckAccess_7C_28_29V
```
example2:
```
(gdb) mb LTypeTest_3B_7CshowLongList_7C_28AJ_29V
set breakpoint  LTypeTest_3B_7CshowLongList_7C_28AJ_29V
Function "maple::maple_invoke_method" not defined.
Breakpoint 1 (maple::maple_invoke_method) pending.
```
***Note***: this shows a pending Maple breakpoints because the program has not been loaded in yet

#### 3. Display breakpoints and info
use 'mbreak -list' or 'mbreak -listall'
example1:
```
(gdb) mb -list
list all breakpoint
#1 Ljava_2Flang_2FThreadGroup_3B_7CcheckAccess_7C_28_29V enabled hit_count 0 ignore_count 0 at pending address in maple::maple_invoke_method()
#2 LTypeTest_3B_7CshowLongList_7C_28AJ_29V enabled hit_count 0 ignore_count 0 at pending address in maple::maple_invoke_method()
```
***Note***: this output shows Maple breakpoints when those breakpoints are at a pending state.

example2:
```
(gdb) mb -list
list all breakpoint
#1 Ljava_2Flang_2FThreadGroup_3B_7CcheckAccess_7C_28_29V enabled hit_count 1 ignore_count 0 at 0x7ffff5b51c66 in maple::maple_invoke_method(maple::method_header_t const*, maple::MFunction const*) at /home/test/gitee/maple_engine/maple_engine/src/invoke_method.cpp:158
#2 LTypeTest_3B_7CshowShortList_7C_28AS_29V enabled hit_count 0 ignore_count 0 at 0x7ffff5b51c66 in maple::maple_invoke_method(maple::method_header_t const*, maple::MFunction const*) at /home/test/gitee/maple_engine/maple_engine/src/invoke_method.cpp:158
```
***Note***: this output shows the Maple breakpoints with real address and additional information when the libraries are loaded.

#### 4. Disable a Maple breakpoint
use 'mb -disable <symbol | index>' command
example1:
```
(gdb) mb -disable 1
disable breakpoint  1
```
***Note***: use Maple breakpoint index 1 which is Ljava_2Flang_2FThreadGroup_3B_7CcheckAccess_7C_28_29V from the Maple breakpoint listed above
example2:
```
(gdb) mb -disable Ljava_2Flang_2FThreadGroup_3B_7CcheckAccess_7C_28_29V
disable breakpoint  Ljava_2Flang_2FThreadGroup_3B_7CcheckAccess_7C_28_29V
```
#### 5. Enable a Maple breakpoint
use 'mb -enable <symbol | index>' command
example:
```
(gdb) mb -enable 1
enable breakpoint  1
```
***Note***: use Maple breakpoint index 1 which is Ljava_2Flang_2FThreadGroup_3B_7CcheckAccess_7C_28_29V from the Maple breakpoint listed above
example2:
```
(gdb) mb -enable Ljava_2Flang_2FThreadGroup_3B_7CcheckAccess_7C_28_29V
enable breakpoint  Ljava_2Flang_2FThreadGroup_3B_7CcheckAccess_7C_28_29V
```
#### 6. Delete a Maple breakpoint
use 'mb -clear <symbol | index>' command
example:
```
(gdb) mb -clear 1
clear breakpoint  1
```
***Note***: use Maple breakpoint index 1 which is Ljava_2Flang_2FThreadGroup_3B_7CcheckAccess_7C_28_29V from the Maple breakpoint listed above
example2:
```
(gdb) mb -clear Ljava_2Flang_2FThreadGroup_3B_7CcheckAccess_7C_28_29V
clear breakpoint  Ljava_2Flang_2FThreadGroup_3B_7CcheckAccess_7C_28_29V
```
#### 7. Delete all Maple breakpoints
use 'mb -clearall' command
example:
```
(gdb) mb -clearall
clear all breakpoint
```
#### 8. Set symbol's ignore couont
use 'mb -ignore <ysmbol | index> <count>' command
example:
```
(gdb) mb -ignore Ljava_2Flang_2FThreadGroup_3B_7CcheckAccess_7C_28_29V 2
ignore breakpoint  1 2
```
## Maple Stack Commands
### mbacktrace command
**_mbt_** as alias of mbacktrace command. **_Syntax_**: (gdb)mbt [-full | -asm]
#### 1. View Maple frame backtrace
use command 'mbt'
example:
```
(gdb)mbt
Maple Traceback (most recent call first):
#0 libcore.so:0x26d1264:0000: Ljava_2Flang_2FThreadGroup_3B_7CcheckAccess_7C_28_29V(a64<java.lang.ThreadGroup> %2=0x15058 "\310\001\020\365\377\177") at /home/test/openjdk/openjdk8/jdk/src/share/classes/java/lang/ThreadGroup.java:313
#2 libcore.so:0x26d10d4:0018: Ljava_2Flang_2FThreadGroup_3B_7CcheckParentAccess_7C_28Ljava_2Flang_2FThreadGroup_3B_29Ljava_2Flang_2FVoid_3B(a64<java.lang.ThreadGroup> %1=0x15058 "\310\001\020\365\377\177") at /home/test/openjdk/openjdk8/jdk/src/share/classes/java/lang/ThreadGroup.java:135
#4 libcore.so:0x26d1180:0010: Ljava_2Flang_2FThreadGroup_3B_7C_3Cinit_3E_7C_28Ljava_2Flang_2FThreadGroup_3BLjava_2Flang_2FString_3B_29V(a64<java.lang.ThreadGroup> %3=0x150a0 "\310\001\020\365\377\177", a64<java.lang.ThreadGroup> %1=0x15058 "\310\001\020\365\377\177", a64<java.lang.String> %4=0x120b8 "\270\376\017\365\377\177") at /home/test/openjdk/openjdk8/jdk/src/share/classes/java/lang/ThreadGroup.java:117
```
***Note***: #0 is the stack frame level, **_libcore.so:0x26d1264:0000_** is the function at libcore.so library, function address at 0x26d1264:0000, function name is **_Ljava_2Flang_2FThreadGroup_3B_7CcheckAccess_7C_28_29V_**; in between () is the function arguments, where **_%2_** is the argument name which is **_a64_** pointer pointing to class **_java.lang.ThreadGroup_**, at the address value of 0x15058. This also shows function is at source code file **_/home/test/openjdk/openjdk8/jdk/src/share/classes/java/lang/ThreadGroup.java, line 313_**
#### 2. View Maple backtrace in asm format
use 'mbt -asm' command
example:
```
(gdb)mbt -asm
Maple Traceback (most recent call first):
#0 libcore.so:0x26d1264:0000: Ljava_2Flang_2FThreadGroup_3B_7CcheckAccess_7C_28_29V(a64<java.lang.ThreadGroup> %2=0x15058 "\310\001\020\365\377\177") at //home/test/gitee/maple_engine/maple_build/out/x86_64/libcore.VtableImpl.s:5882106
#2 libcore.so:0x26d10d4:0018: Ljava_2Flang_2FThreadGroup_3B_7CcheckParentAccess_7C_28Ljava_2Flang_2FThreadGroup_3B_29Ljava_2Flang_2FVoid_3B(a64<java.lang.ThreadGroup> %1=0x15058 "\310\001\020\365\377\177") at //home/test/gitee/maple_engine/maple_build/out/x86_64/libcore.VtableImpl.s:5881983
#4 libcore.so:0x26d1180:0010: Ljava_2Flang_2FThreadGroup_3B_7C_3Cinit_3E_7C_28Ljava_2Flang_2FThreadGroup_3BLjava_2Flang_2FString_3B_29V(a64<java.lang.ThreadGroup> %3=0x150a0 "\310\001\020\365\377\177", a64<java.lang.ThreadGroup> %1=0x15058 "\310\001\020\365\377\177", a64<java.lang.String> %4=0x120b8 "\270\376\017\365\377\177") at //home/test/gitee/maple_engine/maple_build/out/x86_64/libcore.VtableImpl.s:5882045
```
**Note**: This is exact same Maple backtrace, but the source file is assembly file. For example, **_//home/test/gitee/maple_engine/maple_build/out/x86_64/libcore.VtableImpl.s_** is the assembly file,  line number is **_5882045_** in the assmebly file.
#### 3. View a full backtrace mixed with Maple frames and gdb native frames
use 'mbt -full' command
example:
```
(gdb)mbt -full
Maple Traceback (most recent call first):
#0 libcore.so:0x26d1264:0000: Ljava_2Flang_2FThreadGroup_3B_7CcheckAccess_7C_28_29V(a64<java.lang.ThreadGroup> %2=0x15058 "\310\001\020\365\377\177") at /home/test/openjdk/openjdk8/jdk/src/share/classes/java/lang/ThreadGroup.java:313
#1  0x00007ffff5b7135a in maple::MFunction::indirect_call (this=0x7fffffff75d8, ret_ptyp=maple::PTY_void, arg_num=2) at /home/test/gitee/maple_engine/maple_engine/src/mfunction.cpp:150
#2 libcore.so:0x26d10d4:0018: Ljava_2Flang_2FThreadGroup_3B_7CcheckParentAccess_7C_28Ljava_2Flang_2FThreadGroup_3B_29Ljava_2Flang_2FVoid_3B(a64<java.lang.ThreadGroup> %1=0x15058 "\310\001\020\365\377\177") at /home/test/openjdk/openjdk8/jdk/src/share/classes/java/lang/ThreadGroup.java:135
#3  0x00007ffff5b7135a in maple::MFunction::indirect_call (this=0x7fffffffd128, ret_ptyp=maple::PTY_a64, arg_num=2) at /home/test/gitee/maple_engine/maple_engine/src/mfunction.cpp:150
#4 libcore.so:0x26d1180:0010: Ljava_2Flang_2FThreadGroup_3B_7C_3Cinit_3E_7C_28Ljava_2Flang_2FThreadGroup_3BLjava_2Flang_2FString_3B_29V(a64<java.lang.ThreadGroup> %3=0x150a0 "\310\001\020\365\377\177", a64<java.lang.ThreadGroup> %1=0x15058 "\310\001\020\365\377\177", a64<java.lang.String> %4=0x120b8 "\270\376\017\365\377\177") at /home/test/openjdk/openjdk8/jdk/src/share/classes/java/lang/ThreadGroup.java:117
#5  0x00007ffff5b741ef in __engine_shim (first_arg=140737148666236) at /home/test/gitee/maple_engine/maple_engine/src/shimfunction.cpp:100
#6  0x00007ffff54fee40 in ffi_call_unix64 () from /usr/lib/x86_64-linux-gnu/libffi.so
#7  0x00007ffff54fe8ab in ffi_call () from /usr/lib/x86_64-linux-gnu/libffi.so
#8  0x00007ffff6528643 in ?? () from /home/test/gitee/maple_engine/maple_runtime/lib/x86_64/libmplcomp-rt.so
#9  0x00007ffff65345aa in ?? () from /home/test/gitee/maple_engine/maple_runtime/lib/x86_64/libmplcomp-rt.so
#10 0x00007ffff652fdab in MRT_Reflect_InvokeMethod_A_void () from /home/test/gitee/maple_engine/maple_runtime/lib/x86_64/libmplcomp-rt.so
#11 0x00007ffff65d27a2 in MRT_ReflectNewObjectA () from /home/test/gitee/maple_engine/maple_runtime/lib/x86_64/libmplcomp-rt.so
#12 0x00007ffff6b7fd04 in maple::JNI::NewObjectV(_JNIEnv*, _jclass*, _jmethodID*, __va_list_tag*) () from /home/test/gitee/maple_engine/maple_runtime/lib/x86_64/libmpl-rt.so
#13 0x00007ffff6b1ce6a in _JNIEnv::NewObject(_jclass*, _jmethodID*, ...) () from /home/test/gitee/maple_engine/maple_runtime/lib/x86_64/libmpl-rt.so
#14 0x00007ffff6b183ce in maple::Runtime::InitThreadGroups(maple::Thread*) () from /home/test/gitee/maple_engine/maple_runtime/lib/x86_64/libmpl-rt.so
#15 0x00007ffff6b17712 in maple::Runtime::Init(maple::RuntimeArgumentMap&&) () from /home/test/gitee/maple_engine/maple_runtime/lib/x86_64/libmpl-rt.so
#16 0x00007ffff6b16745 in maple::Runtime::Create(std::vector<std::pair<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, void const*>, std::allocator<std::pair<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, void const*> > > const&, bool) () from /home/test/gitee/maple_engine/maple_runtime/lib/x86_64/libmpl-rt.so
#17 0x00007ffff6b0f987 in JNI_CreateJavaVM () from /home/test/gitee/maple_engine/maple_runtime/lib/x86_64/libmpl-rt.so
#18 0x00005555555558f0 in main ()
```
***Note***: #0,#2, #4 are Maple frames, others are gdb native frames
### mup command
#### 1. mup
If mbt shows current Maple frame is at stack level 0, then mup command will make next older Maple frame at its new current frame.  
example:
```
(gdb) mbt
Maple Traceback (most recent call first):
#0 libcore.so:0x26d1264:0000: Ljava_2Flang_2FThreadGroup_3B_7CcheckAccess_7C_28_29V(a64<java.lang.ThreadGroup> %2=0x15058 "\310\001\020\365\377\177") at /home/test/openjdk/openjdk8/jdk/src/share/classes/java/lang/ThreadGroup.java:313
#2 libcore.so:0x26d10d4:0018: Ljava_2Flang_2FThreadGroup_3B_7CcheckParentAccess_7C_28Ljava_2Flang_2FThreadGroup_3B_29Ljava_2Flang_2FVoid_3B(a64<java.lang.ThreadGroup> %1=0x15058 "\310\001\020\365\377\177") at /home/test/openjdk/openjdk8/jdk/src/share/classes/java/lang/ThreadGroup.java:135
#4 libcore.so:0x26d1180:0010: Ljava_2Flang_2FThreadGroup_3B_7C_3Cinit_3E_7C_28Ljava_2Flang_2FThreadGroup_3BLjava_2Flang_2FString_3B_29V(a64<java.lang.ThreadGroup> %3=0x150a0 "\310\001\020\365\377\177", a64<java.lang.ThreadGroup> %1=0x15058 "\310\001\020\365\377\177", a64<java.lang.String> %4=0x120b8 "\270\376\017\365\377\177") at /home/test/openjdk/openjdk8/jdk/src/share/classes/java/lang/ThreadGroup.java:117
````
***Note***: current frame stack level is at level 0
```
(gdb) mup
#2 libcore.so:0x26d10d4:0018: Ljava_2Flang_2FThreadGroup_3B_7CcheckParentAccess_7C_28Ljava_2Flang_2FThreadGroup_3B_29Ljava_2Flang_2FVoid_3B(a64<java.lang.ThreadGroup> %1=0x15058 "\310\001\020\365\377\177") at /home/test/openjdk/openjdk8/jdk/src/share/classes/java/lang/ThreadGroup.java:135
```
***Note***: now current Maple frame level became 2
#### 2. mup [n]
If mbt shows current Maple frame is at stack level 0, then mup command will make next n older Maple frame new current frame
example:
```
(gdb) mbt
Maple Traceback (most recent call first):
#0 libcore.so:0x26d1264:0000: Ljava_2Flang_2FThreadGroup_3B_7CcheckAccess_7C_28_29V(a64<java.lang.ThreadGroup> %2=0x15058 "\310\001\020\365\377\177") at /home/test/openjdk/openjdk8/jdk/src/share/classes/java/lang/ThreadGroup.java:313
#2 libcore.so:0x26d10d4:0018: Ljava_2Flang_2FThreadGroup_3B_7CcheckParentAccess_7C_28Ljava_2Flang_2FThreadGroup_3B_29Ljava_2Flang_2FVoid_3B(a64<java.lang.ThreadGroup> %1=0x15058 "\310\001\020\365\377\177") at /home/test/openjdk/openjdk8/jdk/src/share/classes/java/lang/ThreadGroup.java:135
#4 libcore.so:0x26d1180:0010: Ljava_2Flang_2FThreadGroup_3B_7C_3Cinit_3E_7C_28Ljava_2Flang_2FThreadGroup_3BLjava_2Flang_2FString_3B_29V(a64<java.lang.ThreadGroup> %3=0x150a0 "\310\001\020\365\377\177", a64<java.lang.ThreadGroup> %1=0x15058 "\310\001\020\365\377\177", a64<java.lang.String> %4=0x120b8 "\270\376\017\365\377\177") at /home/test/openjdk/openjdk8/jdk/src/share/classes/java/lang/ThreadGroup.java:117
```
***Note***: current frame stack level is at level 0
```
(gdb) mup 2
#4 libcore.so:0x26d1180:0010: Ljava_2Flang_2FThreadGroup_3B_7C_3Cinit_3E_7C_28Ljava_2Flang_2FThreadGroup_3BLjava_2Flang_2FString_3B_29V(a64<java.lang.ThreadGroup> %3=0x150a0 "\310\001\020\365\377\177", a64<java.lang.ThreadGroup> %1=0x15058 "\310\001\020\365\377\177", a64<java.lang.String> %4=0x120b8 "\270\376\017\365\377\177") at /home/test/openjdk/openjdk8/jdk/src/share/classes/java/lang/ThreadGroup.java:117
```
***Note***: now current Maple frame became level 4
### mdown command
#### 1. mdown
If mbt shows current Maple frame is at stack level 4, then mup command will make next newer Maple frame new current frame.
example:
```
(gdb) mbt
Maple Traceback (most recent call first):
#4 libcore.so:0x26d1180:0010: Ljava_2Flang_2FThreadGroup_3B_7C_3Cinit_3E_7C_28Ljava_2Flang_2FThreadGroup_3BLjava_2Flang_2FString_3B_29V(a64<java.lang.ThreadGroup> %3=0x150a0 "\310\001\020\365\377\177", a64<java.lang.ThreadGroup> %1=0x15058 "\310\001\020\365\377\177", a64<java.lang.String> %4=0x120b8 "\270\376\017\365\377\177") at /home/test/openjdk/openjdk8/jdk/src/share/classes/java/lang/ThreadGroup.java:117
```
***Note***: the current Maple frame is at stack level 4
```
(gdb) mdown
#2 libcore.so:0x26d10d4:0018: Ljava_2Flang_2FThreadGroup_3B_7CcheckParentAccess_7C_28Ljava_2Flang_2FThreadGroup_3B_29Ljava_2Flang_2FVoid_3B(a64<java.lang.ThreadGroup> %1=0x15058 "\310\001\020\365\377\177") at /home/test/openjdk/openjdk8/jdk/src/share/classes/java/lang/ThreadGroup.java:135
```
***Note***: the current Maple frame is at stack level 2 after mdown command
#### 2. mdown [n]
This command moves to next newer n Maple frames and stay at that level in the stack.
example:
```
(gdb) mbt
Maple Traceback (most recent call first):
#4 libcore.so:0x26d1180:0010: Ljava_2Flang_2FThreadGroup_3B_7C_3Cinit_3E_7C_28Ljava_2Flang_2FThreadGroup_3BLjava_2Flang_2FString_3B_29V(a64<java.lang.ThreadGroup> %3=0x150a0 "\310\001\020\365\377\177", a64<java.lang.ThreadGroup> %1=0x15058 "\310\001\020\365\377\177", a64<java.lang.String> %4=0x120b8 "\270\376\017\365\377\177") at /home/test/openjdk/openjdk8/jdk/src/share/classes/java/lang/ThreadGroup.java:117
```
***Note***: the current Maple frame is at stack level 4
```
(gdb) mdown 2
#0 libcore.so:0x26d1264:0000: Ljava_2Flang_2FThreadGroup_3B_7CcheckAccess_7C_28_29V(a64<java.lang.ThreadGroup> %2=0x15058 "\310\001\020\365\377\177") at /home/test/openjdk/openjdk8/jdk/src/share/classes/java/lang/ThreadGroup.java:313
```
***Note***: after mdown 2, the current Maple frame is back to stack level 0

## Maple Control commands
### mstepi command
**_msi_** as the alias of mstepi command
#### 1. Step into next Maple instruction
example:
```
(gdb) msi
Breakpoint 2 at 0x7ffff5b51c14: file /home/test/gitee/maple_engine/maple_engine/src/invoke_method.cpp, line 59.
Debug [20051] addr2line -f -e /home/test/gitee/maple_engine/maple_runtime/lib/x86_64/libcore.so 0x26d1264
Debug [20051] Symbol: 0x00007fffebc0c264, lib_addr: 0x00007fffe953b000, Running Java method: eval_depth=3
Debug [20051] 1 Args: 1: %2, a64, 0x15058

Thread 1 "mplsh" hit Breakpoint 2, __inc_opcode_cnt () at /home/test/gitee/maple_engine/maple_engine/src/invoke_method.cpp:59
59          return ++__opcode_cnt;
0x00007ffff5b608f9 in maple::maple_invoke_method (mir_header=0x7fffebc0c264 <Ljava_2Flang_2FThreadGroup_3B_7CcheckAccess_7C_28_29V_mirbin_info>, caller=0x7fffffff75d8) at /home/test/gitee/maple_engine/maple_engine/src/invoke_method.cpp:924
924         DEBUGOPCODE(addroffpc, Expr);
5882106  :      .byte OP_addroffpc, 0xe, 0x0, 0x0                 // 0000
5882107  :      .long _PTR__cinf_Ljava_2Flang_2FSystem_3B-.
5882108  :      .byte OP_intrinsiccall, 0x0, 0x4, 0x1             // 0008: MPL_CLINIT_CHECK
```

#### 2. Step into next n Maple instructions
use 'msi [n]' command.
example:
```
(gdb) msi 2
Debug [20051] 0x26d1264:0000: 0xcafef00ddeadbeef, ---, sp=0 : op=0x9c, ptyp=0x0e, op#= 0,       OP_addroffpc, Expr, 754
Debug [20051] 0x26d1264:0008: 0x00007ffff4c1a3e0, a64, sp=1 : op=0x29, ptyp=0x00, param=0x0104, OP_intrinsiccall, Stmt, 755
Debug [20051] IntrinsicId=4, __mpl_clinit_check
Debug [20051] addr2line -f -e /home/test/gitee/maple_engine/maple_runtime/lib/x86_64/libcore.so 0xb6df3e0
Debug [20051] Symbol: 0x00007ffff4c1a3e0: CLINIT classinfo
Debug [20051] addr2line -f -e /home/test/gitee/maple_engine/maple_runtime/lib/x86_64/libcore.so 0x142a2b4
Debug [20051] Symbol: 0x00007fffea9652b4: [Running CLINIT:
Debug [20051] addr2line -f -e /home/test/gitee/maple_engine/maple_runtime/lib/x86_64/libcore.so 0x142a2b8
Debug [20051] Symbol: 0x00007fffea9652b8, lib_addr: 0x00007fffe953b000, Running Java method: eval_depth=3
Debug [20051] 0 Args:

Thread 1 "mplsh" hit Breakpoint 2, __inc_opcode_cnt () at /home/test/gitee/maple_engine/maple_engine/src/invoke_method.cpp:59
59          return ++__opcode_cnt;
0x00007ffff5b53d3e in maple::maple_invoke_method (mir_header=0x7fffea9652b8 <Ljava_2Flang_2FSystem_3B_7C_3Cclinit_3E_7C_28_29V_mirbin_info>, caller=0x7fffffff1a88) at /home/test/gitee/maple_engine/maple_engine/src/invoke_method.cpp:335
335         DEBUGOPCODE(addroffunc, Expr);
30600  :        .byte OP_addroffunc, 0xe, 0x0, 0x0                // 0000
30601  :        .quad Ljava_2Flang_2FSystem_3B_7CregisterNatives_7C_28_29V
30602  :        .byte OP_icall, 0x1, 0x0, 0x1                     // 000c
```

#### 3. Step to a specified Maple instruction by Maple instruction count number
use 'msi -abs n' command
example:
```
(gdb) msi -abs 344658
...
Debug [20051] 0x429c:0270: 0x00007fffe0e2b678, a64, sp=1 : op=0x58, ptyp=0x0e, param=0x0000, OP_ireadoff, Expr, 344655
Debug [20051] 0x429c:0274: 0x00007fffe0c26a38, a64, sp=1 : op=0x1a, ptyp=0x0e, param=0xffec, OP_regassign (%13), Stmt, 344656
Debug [20051] 0x429c:0278: 0xcafef00ddeadbeef, ---, sp=0 : op=0x5a, ptyp=0x0e, param=0xffec, OP_regread (%13), Expr, ***344657***

Thread 1 "main" hit Breakpoint 2, __inc_opcode_cnt () at /home/test/gitee/maple_engine/maple_engine/src/invoke_method.cpp:59
59          return ++__opcode_cnt;
0x00007ffff5b5389a in maple::maple_invoke_method (mir_header=0x7fffe0c2929c <LTypeTest_3B_7Cmain_7C_28ALjava_2Flang_2FString_3B_29V_mirbin_info>, caller=0x7fffffffda08) at /home/test/gitee/maple_engine/maple_engine/src/invoke_method.cpp:313
313         DEBUGSOPCODE(regread, Expr, idx);
3479  :         .byte OP_regread, 0xe, 0xf3, 0xff                 // 027c: %6
3480  :         .byte OP_icall, 0x1, 0x0, 0x2                     // 0280
3481  :         // LINE TypeTest.java : 12, INSTIDX : 124||007c:  iconst_5
```
***Note***: it stops at Maple instruction count number 344658

### mni command
#### 1. Move to next Maple instruction
example:
```
(gdb) msi
Debug [29963] 0x429c:027c: 0x00007fffe0c26a38, a64, sp=1 : op=0x5a, ptyp=0x0e, param=0xfff3, OP_regread (%6), Expr, 344658

Thread 1 "main" hit Breakpoint 3, __inc_opcode_cnt () at /home/test/gitee/maple_engine/maple_engine/src/invoke_method.cpp:59
59          return ++__opcode_cnt;
0x00007ffff5b62a12 in maple::maple_invoke_method (mir_header=0x7fffe0c2929c <LTypeTest_3B_7Cmain_7C_28ALjava_2Flang_2FString_3B_29V_mirbin_info>, caller=0x7fffffffda08) at /home/test/gitee/maple_engine/maple_engine/src/invoke_method.cpp:1107
1107        DEBUGOPCODE(icall, Stmt);
3480  :         .byte OP_icall, 0x1, 0x0, 0x2                     // 0280
3481  :         // LINE TypeTest.java : 12, INSTIDX : 124||007c:  iconst_5
3482  :         // LINE TypeTest.java : 12, INSTIDX : 125||007d:  newarray
```
***Note***: currently it stops at Maple count number 344657, and about to execute instruction 344658 which is a OP_icall instruction
```
(gdb)mni
...
Debug [29963] 0x1a3c:0318: 0x0000000000035cb8, a64, sp=3 : op=0x29, ptyp=0x00, param=0x0329, OP_intrinsiccall, Stmt, 363411
Debug [29963] IntrinsicId=41, __mpl_cleanup_localrefvars
Debug [29963] 0x1a3c:031c: 0xcafef00ddeadbeef, ---, sp=0 : op=0x1e, ptyp=0x00, op#= 0,       OP_return, Stmt, 363412
0x00007ffff5b7135a in maple::MFunction::indirect_call (this=0x7fffffffd5e8, ret_ptyp=maple::PTY_void, arg_num=2) at /home/test/gitee/maple_engine/maple_engine/src/mfunction.cpp:150
150                 RETURNVAL = maple_invoke_method(header, this);
Value returned is $42 = {x = {u1 = 0 '\000', i8 = 0 '\000', i16 = 0, u16 = 0, i32 = 0, i64 = 0, f32 = 0, f64 = 0, a64 = 0x0, u8 = 0 '\000', u32 = 0, u64 = 0}, ptyp = maple::PTY_void}

Thread 1 "main" hit Breakpoint 3, __inc_opcode_cnt () at /home/test/gitee/maple_engine/maple_engine/src/invoke_method.cpp:59
59          return ++__opcode_cnt;
0x00007ffff5b53f82 in maple::maple_invoke_method (mir_header=0x7fffe0c2929c <LTypeTest_3B_7Cmain_7C_28ALjava_2Flang_2FString_3B_29V_mirbin_info>, caller=0x7fffffffda08) at /home/test/gitee/maple_engine/maple_engine/src/invoke_method.cpp:352
352         DEBUGCOPCODE(constval, Expr);
3484  :         .byte OP_constval, 0x9, 0x1, 0x0                  // 0284
3485  :         .byte OP_constval, 0x4, 0x5, 0x0                  // 0288
3486  :         .byte OP_addrof32, 0xe, 0x0, 0x0                  // 028c
```
***Note***: Maple instruction 363412 is a OP_return from OP_icall at 344658.

## Maple File Commands
### msrcpath command
**_msp_** is the alias of msrcpath command
Maple Debugger needs a search path list to search the source code files. msrcpath command is used to set and view this search path list
#### 1. msrcpath
example:
```
(gdb)msrcpath
Maple source path list: --
/home/test/openjdk/openjdk8/jdk/src/
/home/test/gitee/maple_engine/maple_build/examples/TypeTest
```
#### 2. msrcpath -add <path>
Add a path to the top of the search path list.
example:
```
(gdb)msrcpath -add /home/test/openjdk/openjdk8/jdk/src/
```
#### 3. msrcpath -del <path>
Delete one path from the search path list
```
(gdb)msrcpath -del /home/test/openjdk/openjdk8/jdk/src/
```

### mlist command
***Make sure source code search path is set before use this command***
This command list the Maple application source code block
#### 1. mlist
example:
```
(gdb) mlist
file:  /home/test/gitee/maple_engine/maple_build/examples/TypeTest/TypeTest.java  line:  54
50             System.out.println();
51         }
52
53         public static void showLongList(long[] list)
54         {   for(int i = 0; i < list.length; i++)
55                 System.out.print( list[i] + " " );
56             System.out.println();
57         }
58
```
#### 2. mlist -asm
list the current instruction in co-responding assembly file 
example:
```
(gdb) mlist -asm
file:  /home/test/gitee/maple_engine/maple_build/examples/TypeTest/TypeTest.s  line:  985
981             .ascii "%29\0\0\0\0\0\0\0\0\0\0\0\0\0"
982             .ascii "%30\0\0\0\0\0\0\0\0\0\0\0\0\0"
983             .p2align 1
984     LTypeTest_3B_7CshowLongList_7C_28AJ_29V_mirbin_code:
985             .byte OP_addroffpc, 0xe, 0x0, 0x0                 // 0000
986             .long _PTR__cinf_LTypeTest_3B-.
987             .byte OP_intrinsiccall, 0x0, 0x4, 0x1             // 0008: MPL_CLINIT_CHECK
988             // LINE TypeTest.java : 54, INSTIDX : 0||0000:  iconst_0
```

## Maple Data Commands
### mlocal command
This command displays the stack frame local varibles' value and dynamic stack data.
#### 1. mlocal
This is used to display all local variables' value of a function. Use mlocal command at a Maple breakpoint and after a msi command to step into a function
example:
```
(gdb) mlocal
local #1 :name=%%thrownval type=v2i64 value=none
local #2 :name=Reg0_R155549 type=a64 value=0x0
local #3 :name=Reg1_R20114 type=a64 value=0x0
local #4 :name=Reg0_R34665 type=a64 value=0x0
local #5 :name=%1 type=i32 value=0
local #6 :name=%2 type=i32 value=0
local #7 :name=%3 type=i32 value=0
local #8 :name=%4 type=i32 value=0
local #9 :name=%5 type=i32 value=0
local #10 :name=%6 type=a64 value=0x0
local #11 :name=%7 type=a64 value=0x0
local #12 :name=%8 type=a64 value=0x0
local #13 :name=%9 type=u16 value=0
local #14 :name=%10 type=i32 value=0
local #15 :name=%11 type=i32 value=0
local #16 :name=%12 type=i32 value=0
local #17 :name=%13 type=a64 value=0x0
local #18 :name=%14 type=u1 value=0 '\000'
local #19 :name=%15 type=i32 value=0
local #20 :name=%16 type=u1 value=0 '\000'
local #21 :name=%17 type=u1 value=0 '\000'
local #22 :name=%18 type=i32 value=0
local #23 :name=%19 type=u1 value=0 '\000'
local #24 :name=%20 type=i32 value=0
local #25 :name=%21 type=i32 value=0
local #26 :name=%22 type=a64 value=0x0
local #27 :name=%24 type=a64 value=0x0
local #28 :name=%25 type=a64 value=0x0
local #29 :name=%27 type=a64 value=0x0
```
#### 2. mlocal -stack or mlocal -s
This is used to display all stack frame's dynamic data. 
example:
```
(gdb) mlocal -stack
sp= 1 :type= a64  value= 0x7ffff49915e8 <_C_STR_aa6c9e888c754e206fadbf41fdc90033> "0u\214\366\377\177"
sp= 2 :type= a64  value= 0x0
```
### mprint command 
This command is used to display the object data and object type includind the inheritence hierarchy
#### 1. syntax: mprint <hex address>
example:
```
(gdb) mprint 0x15058
object type: class Ljava_2Flang_2FThreadGroup_3B
level 1 class Ljava_2Flang_2FObject_3B:
  #1,off= 0,len= 8,"reserved__1     ",value=hex:0x7ffff68cbd30,long:140737329806640,double:6.9533479744890109e-310
  #2,off= 8,len= 4,"reserved__2     ",value=hex:0x00000000,int:0,float:0
level 2 class Ljava_2Flang_2FThreadGroup_3B:
  #3,off=12,len= 4,"maxPriority     ",value=hex:0x0000000a,int:10,float:1.40129846e-44
  #4,off=16,len= 8,"parent          ",value=hex:0x0,long:0,double:0
  #5,off=24,len= 8,"name            ",value=hex:0x12098,long:73880,double:3.6501569914751295e-319
  #6,off=32,len= 1,"destroyed       ",value=hex:0x00,byte:0 '\000'
  #7,off=33,len= 1,"daemon          ",value=hex:0x00,byte:0 '\000'
  #8,off=34,len= 1,"vmAllowSuspension",value=hex:0x00,byte:0 '\000'
  #9,off=36,len= 4,"nUnstartedThreads",value=hex:0xffffffff,int:-1,float:-nan(0x7fffff)
  #10,off=40,len= 4,"nthreads        ",value=hex:0x00000003,int:3,float:4.20389539e-45
  #11,off=44,len= 4,"ngroups         ",value=hex:0x00000001,int:1,float:1.40129846e-45
  #12,off=48,len= 8,"threads         ",value=hex:0x16090,long:90256,double:4.4592388931047548e-319
  #13,off=56,len= 8,"groups          ",value=hex:0x16058,long:90200,double:4.4564721254880438e-319
```
### mtype command
**_syntax_**: mtype <regex-of-mangled-class-name-search-pattern>

#### 1. If only one matches
example:
```
(gdb) mtype Nodes_24ToArrayTask_24OfRef
#1 class name Ljava_2Futil_2Fstream_2FNodes_24ToArrayTask_24OfRef_3B:
  level 1 Ljava_2Flang_2FObject_3B:
    #1,off= 0,len= 8,"reserved__1     "
    #2,off= 8,len= 4,"reserved__2     "
  level 2 Ljava_2Futil_2Fconcurrent_2FForkJoinTask_3B:
    #3,off=12,len= 4,"status          "
  level 3 Ljava_2Futil_2Fconcurrent_2FCountedCompleter_3B:
    #4,off=16,len= 8,"completer       "
    #5,off=24,len= 4,"pending         "
  level 4 Ljava_2Futil_2Fstream_2FNodes_24ToArrayTask_3B:
    #6,off=28,len= 4,"offset          "
    #7,off=32,len= 8,"node            "
  level 5 Ljava_2Futil_2Fstream_2FNodes_24ToArrayTask_24OfRef_3B:
    #8,off=40,len= 8,"array   
```
#### 2. If multiple classes match
example:
```
(gdb) mtype Nodes_24ToArrayTask
#1 class name Ljava_2Futil_2Fstream_2FNodes_24ToArrayTask_24OfPrimitive_3B:
#2 class name Ljava_2Futil_2Fstream_2FNodes_24ToArrayTask_24OfLong_3B:
#3 class name Ljava_2Futil_2Fstream_2FNodes_24ToArrayTask_24OfInt_3B:
#4 class name Ljava_2Futil_2Fstream_2FNodes_24ToArrayTask_3B:
#5 class name Ljava_2Futil_2Fstream_2FNodes_24ToArrayTask_24OfDouble_3B:
#6 class name Ljava_2Futil_2Fstream_2FNodes_24ToArrayTask_24OfRef_3B:
```

## Maple Miscellaneous Commands
### mset command
This command is used to set Maple debugger's environment variables and flags
#### 1. mset -show
display the mset settings
example:
```
(gdb)mset -show
{'maple_lib_asm_path': ['~/gitee/maple_engine/maple_build/out/x86_64/', './'], 'verbose': 'off'}
```
#### 2. mset <key> <value>
example:
```
(gdb)mset verbose on
```
#### 3. mset -add maple_lib_asm_path <path>
add ~/gitee/maple_engine/maple_build/out/x86_64/ as one of the maple_lib_asm_path for asm file search path
```
(gdb)mset -add maple_lib_asm_path ~/gitee/maple_engine/maple_build/out/x86_64/
```
#### 4. mset -del maple_lib_asm_path <path>
delete ~/gitee/maple_engine/maple_build/out/x86_64/ from maple_lib_asm_path asm file search path
```
(gdb)mset -del maple_lib_asm_path ~/gitee/maple_engine/maple_build/out/x86_64/
```

### mhelp command
#### 1. List all the commands and Maple Debugger version
example:
```
(gdb) mhelp
Maple Version 1.0

mbreak:     Set and manage Maple breakpoints
mbacktrace: Display Maple backtrace in multiple modes
mup:        Select and print Maple stack frame that called this one
mdown:      Select and print Maple stack frame called by this ones
mlist:      List source code in multiple modes
msrcpath:   Add and manage the Maple source code search path
mlocal:     Display selected Maple frame arguments and local variables
mprint:     Print Maple runtime object data
mtype:      Print a matching class and its inheritance hierarchy by a given search expression
mstepi:     Step specified number of Maple instructions
mnexti:     Step one Maple instruction, but proceed through subroutine calls
mset:       Sets and displays Maple debugger settings
```
#### 2. Help of individual Maple command
mhelp <command name>
***Note***: command name must be a Maple command full name other than its alias name
example:
```
(gdb) mhelp mbreak
Maple Version 1.0
mbreak <symbol>: set a new Maple breakpoint at symbol
mbreak -set <symbol>: same to "mbreak <symbol>"
mbreak -enable <symbol|index>: enable an existing Maple breakpoint at symbol
mbreak -disable <symbol|index>: disable an existing Maple breakpoint at symbol
mbreak -clear <symbol|index>: delete an existing Maple breakpoint at symbol
mbreak -clearall : delete all existing Maple breakpoint
mbreak -listall : list all existing Maple breakpoints
mbreak -ignore <symbol | index> <count> : set ignore count for specified Maple breakpoints
```

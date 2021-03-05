```
# Copyright (C) [2021] Futurewei Technologies, Inc. All rights reserved.
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
   Use mset command to show/add/del library asm file search paths
```
On #1, a search path list should be set, for Maple Debugger to search the source code of libraries and user's application source code. mlist command uses this search list to look up the source code files.
On #2, a search path list should be set, for Maple Debugger to search assembly files at the time of co-responding .so file is compiled. mbt, mup, mdown and other commands use this search list to look up a lot of information from the assembly files.

## Maple Command Summary
* **_mbreakpoint_**: Sets and manages Maple breakpoints
* **_mbacktrace_** : Displays Maple backtrace in multiple modes
* **_mup_**        : Selects and prints Maple stack frame that called this one
* **_mdown_**      : Selects and prints Maple stack frame called by this one
* **_mlist_**      : Lists source code in multiple modes
* **_msrcpath_**   : Adds and manages the Maple source code search paths
* **_mlocal_**     : Displays selected Maple frame arguments, local variables and stack dynamic data
* **_mprint_**     : Prints Maple runtime object data
* **_mtype_**      : Prints a matching class and its inheritance hierarchy by a given search expression
* **_msymbol_**    : Prints a matching symbol list or its detailed infomatioin
* **_mstepi_**     : Steps specified number of Maple instructions
* **_mnexti_**     : Steps one Maple instruction, but proceed through subroutine calls
* **_mfinish_**    : Executes until selected Maple stack frame returns
* **_mset_**       : Sets and displays Maple debugger settings
* **_mhelp_**      : Provides help, usage and etc

## Maple Breakpoint command
### mbreak command
**_mb_** is the alias of the mbreak command
#### 1. Syntax
(gdb) mbreak
mbreak <symbol>: Sets a new Maple breakpoint at symbol
mbreak -set <symbol>: An alias for 'mbreak <symbol>'
mbreak -enable <symbol|index>: Enables an existing Maple breakpoint at symbol
mbreak -disable <symbol|index>: Disables an existing Maple breakpoint at symbol
mbreak -clear <symbol|index>: Deletes an existing Maple breakpoint at symbol
mbreak -clearall : Deletes all existing Maple breakpoints
mbreak -listall : Lists all existing Maple breakpoints
mbreak -ignore <symbol | index> <count> : Sets ignore count for specified Maple breakpoints
mbreak : Displays usage and syntax

#### 2. Set Maple breakpoint
Execute command 'mbreak <mangled Maple symbol>' or 'mbreak -set <mangled Maple symbol>'
Example 1:
```
(gdb) mb Ljava_2Flang_2FThreadGroup_3B_7CcheckAccess_7C_28_29V
set breakpoint  Ljava_2Flang_2FThreadGroup_3B_7CcheckAccess_7C_28_29V
```
Example 2:
```
(gdb) mb LTypeTest_3B_7CshowLongList_7C_28AJ_29V
set breakpoint  LTypeTest_3B_7CshowLongList_7C_28AJ_29V
Function "maple::maple_invoke_method" not defined.
Breakpoint 1 (maple::maple_invoke_method) pending.
```
***Note***: this shows a pending Maple breakpoints because the program has not been loaded in yet

#### 3. Display breakpoints and info
Execute 'mbreak -list' or 'mbreak -listall'
Example 1:
```
(gdb) mb -list
list all Maple breakpoints
#1 Ljava_2Flang_2FThreadGroup_3B_7CcheckAccess_7C_28_29V enabled hit_count 0 ignore_count 0 at pending address in maple::maple_invoke_method()
#2 LTypeTest_3B_7CshowLongList_7C_28AJ_29V enabled hit_count 0 ignore_count 0 at pending address in maple::maple_invoke_method()
```
***Note***: this output shows Maple breakpoints when those breakpoints are at a pending state.

Example 2:
```
(gdb) mb -list
list all Maple breakpoints
#1 Ljava_2Flang_2FThreadGroup_3B_7CcheckAccess_7C_28_29V enabled hit_count 1 ignore_count 0 at 0x7ffff5b51c66 in maple::maple_invoke_method(maple::method_header_t const*, maple::MFunction const*) at /home/test/gitee/maple_engine/maple_engine/src/invoke_method.cpp:158
#2 LTypeTest_3B_7CshowShortList_7C_28AS_29V enabled hit_count 0 ignore_count 0 at 0x7ffff5b51c66 in maple::maple_invoke_method(maple::method_header_t const*, maple::MFunction const*) at /home/test/gitee/maple_engine/maple_engine/src/invoke_method.cpp:158
```
***Note***: this output shows the Maple breakpoints with real address and additional information when the libraries are loaded.

#### 4. Disable a Maple breakpoint
Execute 'mb -disable <symbol | index>' command
Example 1:
```
(gdb) mb -disable 1
disable breakpoint  1
```
***Note***: use Maple breakpoint index 1 which is Ljava_2Flang_2FThreadGroup_3B_7CcheckAccess_7C_28_29V from the Maple breakpoint listed above
Example 2:
```
(gdb) mb -disable Ljava_2Flang_2FThreadGroup_3B_7CcheckAccess_7C_28_29V
disable breakpoint  Ljava_2Flang_2FThreadGroup_3B_7CcheckAccess_7C_28_29V
```
#### 5. Enable a Maple breakpoint
Execute 'mb -enable <symbol | index>' command
Example:
```
(gdb) mb -enable 1
enable breakpoint  1
```
***Note***: use Maple breakpoint index 1 which is Ljava_2Flang_2FThreadGroup_3B_7CcheckAccess_7C_28_29V from the Maple breakpoint listed above
Example 2:
```
(gdb) mb -enable Ljava_2Flang_2FThreadGroup_3B_7CcheckAccess_7C_28_29V
enable breakpoint  Ljava_2Flang_2FThreadGroup_3B_7CcheckAccess_7C_28_29V
```
#### 6. Delete a Maple breakpoint
Execute 'mb -clear <symbol | index>' command
Example:
```
(gdb) mb -clear 1
clear breakpoint  1
```
***Note***: use Maple breakpoint index 1 which is Ljava_2Flang_2FThreadGroup_3B_7CcheckAccess_7C_28_29V from the Maple breakpoint listed above
Example 2:
```
(gdb) mb -clear Ljava_2Flang_2FThreadGroup_3B_7CcheckAccess_7C_28_29V
clear breakpoint  Ljava_2Flang_2FThreadGroup_3B_7CcheckAccess_7C_28_29V
```
#### 7. Delete all Maple breakpoints
Execute 'mb -clearall' command
Example:
```
(gdb) mb -clearall
clear all breakpoint
```
#### 8. Set symbol's ignore couont
Execute 'mb -ignore <ysmbol | index> <count>' command
Example:
```
(gdb) mb -ignore Ljava_2Flang_2FThreadGroup_3B_7CcheckAccess_7C_28_29V 2
ignore breakpoint  1 2
```
## Maple Stack Commands
### mbacktrace command
**_mbt_** as alias of mbacktrace command. **_Syntax_**: mbt [-full | -asm]
#### 1. View Maple frame backtrace
Execute command 'mbt'
Example:
```
(gdb) mbt
Maple Traceback (most recent call first):
#0 libcore.so:0x26d1264:0000: Ljava_2Flang_2FThreadGroup_3B_7CcheckAccess_7C_28_29V(a64<java.lang.ThreadGroup> %2=0x15058 "\310\001\020\365\377\177") at /home/test/my_openjdk8/jdk/src/share/classes/java/lang/ThreadGroup.java:313
#2 libcore.so:0x26d10d4:0018: Ljava_2Flang_2FThreadGroup_3B_7CcheckParentAccess_7C_28Ljava_2Flang_2FThreadGroup_3B_29Ljava_2Flang_2FVoid_3B(a64<java.lang.ThreadGroup> %1=0x15058 "\310\001\020\365\377\177") at /home/test/my_openjdk8/jdk/src/share/classes/java/lang/ThreadGroup.java:135
#4 libcore.so:0x26d1180:0010: Ljava_2Flang_2FThreadGroup_3B_7C_3Cinit_3E_7C_28Ljava_2Flang_2FThreadGroup_3BLjava_2Flang_2FString_3B_29V(a64<java.lang.ThreadGroup> %3=0x150a0 "\310\001\020\365\377\177", a64<java.lang.ThreadGroup> %1=0x15058 "\310\001\020\365\377\177", a64<java.lang.String> %4=0x120b8 "\270\376\017\365\377\177") at /home/test/my_openjdk8/jdk/src/share/classes/java/lang/ThreadGroup.java:117
```
***Note***: #0 is the stack frame level, **_libcore.so:0x26d1264:0000_** is the function at libcore.so library, function address at 0x26d1264:0000, function name is **_Ljava_2Flang_2FThreadGroup_3B_7CcheckAccess_7C_28_29V_**; in between () is the function arguments, where **_%2_** is the argument name which is **_a64_** pointer pointing to class **_java.lang.ThreadGroup_**, at the address value of 0x15058. This also shows function is at source code file **_/home/test/my_openjdk8/jdk/src/share/classes/java/lang/ThreadGroup.java, line 313_**
#### 2. View Maple backtrace in asm format
Execute 'mbt -asm' command
Example:
```
(gdb) mbt -asm
Maple Traceback (most recent call first):
#0 libcore.so:0x26d1264:0000: Ljava_2Flang_2FThreadGroup_3B_7CcheckAccess_7C_28_29V(a64<java.lang.ThreadGroup> %2=0x15058 "\310\001\020\365\377\177") at //home/test/gitee/maple_engine/maple_build/out/x86_64/libcore.VtableImpl.s:5882106
#2 libcore.so:0x26d10d4:0018: Ljava_2Flang_2FThreadGroup_3B_7CcheckParentAccess_7C_28Ljava_2Flang_2FThreadGroup_3B_29Ljava_2Flang_2FVoid_3B(a64<java.lang.ThreadGroup> %1=0x15058 "\310\001\020\365\377\177") at //home/test/gitee/maple_engine/maple_build/out/x86_64/libcore.VtableImpl.s:5881983
#4 libcore.so:0x26d1180:0010: Ljava_2Flang_2FThreadGroup_3B_7C_3Cinit_3E_7C_28Ljava_2Flang_2FThreadGroup_3BLjava_2Flang_2FString_3B_29V(a64<java.lang.ThreadGroup> %3=0x150a0 "\310\001\020\365\377\177", a64<java.lang.ThreadGroup> %1=0x15058 "\310\001\020\365\377\177", a64<java.lang.String> %4=0x120b8 "\270\376\017\365\377\177") at //home/test/gitee/maple_engine/maple_build/out/x86_64/libcore.VtableImpl.s:5882045
```
**Note**: This is exact same Maple backtrace, but the source file is assembly file. For example, **_//home/test/gitee/maple_engine/maple_build/out/x86_64/libcore.VtableImpl.s_** is the assembly file,  line number is **_5882045_** in the assmebly file.
#### 3. View Maple backtrace in Maple IR format
Execute 'mbt -mir' command
Example:
```
(gdb) mbt -mir
Maple Traceback (most recent call first):
#0 libcore.so:0x2720108:0320: Ljava_2Futil_2FHashtable_3B_7Cput_7C_28Ljava_2Flang_2FObject_3BLjava_2Flang_2FObject_3B_29Ljava_2Flang_2FObject_3B(a64<java.util.Properties> %13=0x1f058 "\260\202\022\365\377\177", a64<java.lang.String> %17=0x12638 "", a64<java.lang.String> %14=0x12658 "") at /home/test/gitee/maple_engine/maple_build/out/x86_64/libcore.mpl.mir.mpl:2700423
#15 libcore.so:0x7989a1c:00ec: Ljava_2Flang_2FSystem_3B_7CinitProperties_7C_28Ljava_2Futil_2FProperties_3B_29Ljava_2Futil_2FProperties_3B(a64<java.util.Properties> %1=0x1f058 "\260\202\022\365\377\177") at /home/test/gitee/maple_engine/maple_build/out/x86_64/libcore.mpl.mir.mpl:13113755
#17 libcore.so:0x888ce38:0098: Ljava_2Flang_2FSystem_3B_7CinitializeSystemClass_7C_28_29V() at /home/test/gitee/maple_engine/maple_build/out/x86_64/libcore.mpl.mir.mpl:14922166
```
**Note**: This is an example to show mbt command in Maple MIR format. **_//home/test/gitee/maple_engine/maple_build/out/x86_64/libcore.mpl.mir.mpl_** is the MIR instruction file,  line number is **_14922166_** in the mir.mpl file.
#### 4. View a full backtrace mixed with Maple frames and gdb native frames
Execute 'mbt -full' command
Example:
```
(gdb) mbt -full
Maple Traceback (most recent call first):
#0 libcore.so:0x26d1264:0000: Ljava_2Flang_2FThreadGroup_3B_7CcheckAccess_7C_28_29V(a64<java.lang.ThreadGroup> %2=0x15058 "\310\001\020\365\377\177") at /home/test/my_openjdk8/jdk/src/share/classes/java/lang/ThreadGroup.java:313
#1  0x00007ffff5b7135a in maple::MFunction::indirect_call (this=0x7fffffff75d8, ret_ptyp=maple::PTY_void, arg_num=2) at /home/test/gitee/maple_engine/maple_engine/src/mfunction.cpp:150
#2 libcore.so:0x26d10d4:0018: Ljava_2Flang_2FThreadGroup_3B_7CcheckParentAccess_7C_28Ljava_2Flang_2FThreadGroup_3B_29Ljava_2Flang_2FVoid_3B(a64<java.lang.ThreadGroup> %1=0x15058 "\310\001\020\365\377\177") at /home/test/my_openjdk8/jdk/src/share/classes/java/lang/ThreadGroup.java:135
#3  0x00007ffff5b7135a in maple::MFunction::indirect_call (this=0x7fffffffd128, ret_ptyp=maple::PTY_a64, arg_num=2) at /home/test/gitee/maple_engine/maple_engine/src/mfunction.cpp:150
#4 libcore.so:0x26d1180:0010: Ljava_2Flang_2FThreadGroup_3B_7C_3Cinit_3E_7C_28Ljava_2Flang_2FThreadGroup_3BLjava_2Flang_2FString_3B_29V(a64<java.lang.ThreadGroup> %3=0x150a0 "\310\001\020\365\377\177", a64<java.lang.ThreadGroup> %1=0x15058 "\310\001\020\365\377\177", a64<java.lang.String> %4=0x120b8 "\270\376\017\365\377\177") at /home/test/my_openjdk8/jdk/src/share/classes/java/lang/ThreadGroup.java:117
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
Example:
```
(gdb) mbt
Maple Traceback (most recent call first):
#0 libcore.so:0x26d1264:0000: Ljava_2Flang_2FThreadGroup_3B_7CcheckAccess_7C_28_29V(a64<java.lang.ThreadGroup> %2=0x15058 "\310\001\020\365\377\177") at /home/test/my_openjdk8/jdk/src/share/classes/java/lang/ThreadGroup.java:313
#2 libcore.so:0x26d10d4:0018: Ljava_2Flang_2FThreadGroup_3B_7CcheckParentAccess_7C_28Ljava_2Flang_2FThreadGroup_3B_29Ljava_2Flang_2FVoid_3B(a64<java.lang.ThreadGroup> %1=0x15058 "\310\001\020\365\377\177") at /home/test/my_openjdk8/jdk/src/share/classes/java/lang/ThreadGroup.java:135
#4 libcore.so:0x26d1180:0010: Ljava_2Flang_2FThreadGroup_3B_7C_3Cinit_3E_7C_28Ljava_2Flang_2FThreadGroup_3BLjava_2Flang_2FString_3B_29V(a64<java.lang.ThreadGroup> %3=0x150a0 "\310\001\020\365\377\177", a64<java.lang.ThreadGroup> %1=0x15058 "\310\001\020\365\377\177", a64<java.lang.String> %4=0x120b8 "\270\376\017\365\377\177") at /home/test/my_openjdk8/jdk/src/share/classes/java/lang/ThreadGroup.java:117
````
***Note***: current frame stack level is at level 0
```
(gdb) mup
#2 libcore.so:0x26d10d4:0018: Ljava_2Flang_2FThreadGroup_3B_7CcheckParentAccess_7C_28Ljava_2Flang_2FThreadGroup_3B_29Ljava_2Flang_2FVoid_3B(a64<java.lang.ThreadGroup> %1=0x15058 "\310\001\020\365\377\177") at /home/test/my_openjdk8/jdk/src/share/classes/java/lang/ThreadGroup.java:135
```
***Note***: now current Maple frame level became 2
#### 2. mup [n]
If mbt shows current Maple frame is at stack level 0, then mup command will make next n older Maple frame new current frame
Example:
```
(gdb) mbt
Maple Traceback (most recent call first):
#0 libcore.so:0x26d1264:0000: Ljava_2Flang_2FThreadGroup_3B_7CcheckAccess_7C_28_29V(a64<java.lang.ThreadGroup> %2=0x15058 "\310\001\020\365\377\177") at /home/test/my_openjdk8/jdk/src/share/classes/java/lang/ThreadGroup.java:313
#2 libcore.so:0x26d10d4:0018: Ljava_2Flang_2FThreadGroup_3B_7CcheckParentAccess_7C_28Ljava_2Flang_2FThreadGroup_3B_29Ljava_2Flang_2FVoid_3B(a64<java.lang.ThreadGroup> %1=0x15058 "\310\001\020\365\377\177") at /home/test/my_openjdk8/jdk/src/share/classes/java/lang/ThreadGroup.java:135
#4 libcore.so:0x26d1180:0010: Ljava_2Flang_2FThreadGroup_3B_7C_3Cinit_3E_7C_28Ljava_2Flang_2FThreadGroup_3BLjava_2Flang_2FString_3B_29V(a64<java.lang.ThreadGroup> %3=0x150a0 "\310\001\020\365\377\177", a64<java.lang.ThreadGroup> %1=0x15058 "\310\001\020\365\377\177", a64<java.lang.String> %4=0x120b8 "\270\376\017\365\377\177") at /home/test/my_openjdk8/jdk/src/share/classes/java/lang/ThreadGroup.java:117
```
***Note***: current frame stack level is at level 0
```
(gdb) mup 2
#4 libcore.so:0x26d1180:0010: Ljava_2Flang_2FThreadGroup_3B_7C_3Cinit_3E_7C_28Ljava_2Flang_2FThreadGroup_3BLjava_2Flang_2FString_3B_29V(a64<java.lang.ThreadGroup> %3=0x150a0 "\310\001\020\365\377\177", a64<java.lang.ThreadGroup> %1=0x15058 "\310\001\020\365\377\177", a64<java.lang.String> %4=0x120b8 "\270\376\017\365\377\177") at /home/test/my_openjdk8/jdk/src/share/classes/java/lang/ThreadGroup.java:117
```
***Note***: now current Maple frame became level 4
### mdown command
#### 1. mdown
If mbt shows current Maple frame is at stack level 4, then mup command will make next newer Maple frame new current frame.
Example:
```
(gdb) mbt
Maple Traceback (most recent call first):
#4 libcore.so:0x26d1180:0010: Ljava_2Flang_2FThreadGroup_3B_7C_3Cinit_3E_7C_28Ljava_2Flang_2FThreadGroup_3BLjava_2Flang_2FString_3B_29V(a64<java.lang.ThreadGroup> %3=0x150a0 "\310\001\020\365\377\177", a64<java.lang.ThreadGroup> %1=0x15058 "\310\001\020\365\377\177", a64<java.lang.String> %4=0x120b8 "\270\376\017\365\377\177") at /home/test/my_openjdk8/jdk/src/share/classes/java/lang/ThreadGroup.java:117
```
***Note***: the current Maple frame is at stack level 4
```
(gdb) mdown
#2 libcore.so:0x26d10d4:0018: Ljava_2Flang_2FThreadGroup_3B_7CcheckParentAccess_7C_28Ljava_2Flang_2FThreadGroup_3B_29Ljava_2Flang_2FVoid_3B(a64<java.lang.ThreadGroup> %1=0x15058 "\310\001\020\365\377\177") at /home/test/my_openjdk8/jdk/src/share/classes/java/lang/ThreadGroup.java:135
```
***Note***: the current Maple frame is at stack level 2 after mdown command
#### 2. mdown [n]
This command moves to next newer n Maple frames and stay at that level in the stack.
Example:
```
(gdb) mbt
Maple Traceback (most recent call first):
#4 libcore.so:0x26d1180:0010: Ljava_2Flang_2FThreadGroup_3B_7C_3Cinit_3E_7C_28Ljava_2Flang_2FThreadGroup_3BLjava_2Flang_2FString_3B_29V(a64<java.lang.ThreadGroup> %3=0x150a0 "\310\001\020\365\377\177", a64<java.lang.ThreadGroup> %1=0x15058 "\310\001\020\365\377\177", a64<java.lang.String> %4=0x120b8 "\270\376\017\365\377\177") at /home/test/my_openjdk8/jdk/src/share/classes/java/lang/ThreadGroup.java:117
```
***Note***: the current Maple frame is at stack level 4
```
(gdb) mdown 2
#0 libcore.so:0x26d1264:0000: Ljava_2Flang_2FThreadGroup_3B_7CcheckAccess_7C_28_29V(a64<java.lang.ThreadGroup> %2=0x15058 "\310\001\020\365\377\177") at /home/test/my_openjdk8/jdk/src/share/classes/java/lang/ThreadGroup.java:313
```
***Note***: after mdown 2, the current Maple frame is back to stack level 0

## Maple Control commands
### mstepi command
**_msi_** as the alias of mstepi command
#### 1. Step into next Maple instruction
Example:
```
(gdb) msi
asm file: /home/test/maple_engine_standalone/maple_engine/maple_build/out/x86_64/libcore.VtableImpl.s
=> 5854919 :    .byte OP_intrinsiccall, 0x1, 0x7f, 0x1            // 0004: MCCSyncEnterFast2
   5854920 :    // LINE Hashtable.java : 459, INSTIDX : 0||0000:  aload_2
   5854921 :    // LINE Hashtable.java : 459, INSTIDX : 1||0001:  ifnonnull
```

#### 2. Step into next n Maple instructions
Execute 'msi [n]' command.
Example:
```
(gdb) msi 20
asm file: /home/test/maple_engine_standalone/maple_engine/maple_build/out/x86_64/libcore.VtableImpl.s
=> 5854977 :    .byte OP_constval, 0xe, 0x8, 0x0                  // 00b4
   5854978 :    .byte OP_add, 0xe, 0x0, 0x2                       // 00b8
   5854979 :    .byte OP_ireadoff, 0x9, 0x0, 0x0                  // 00bc
```

#### 3. Step to a specified Maple instruction by Maple instruction count number
Execute 'msi -abs n' command
Example:
```
(gdb) msi -abs 10000
asm file: /home/test/maple_engine_standalone/maple_engine/maple_build/out/x86_64/libcore.VtableImpl.s
=> 1062 :       .byte OP_ge, 0x4, 0x4, 0x2                        // 017c
   1063 :       .byte OP_brfalse32, 0x0, 0x0, 0x1                 // 0180
   1064 :       .long mirbin_label_177681_4-.
```

***Note***: it stops at Maple instruction count number 344658

#### 4. msi -c. Display current opcode count
Example:
```
(gdb) msi -c
current opcode count is 40001
```

### mni command
#### 1. Move to next Maple instruction
Example:
```
(gdb) mni
asm file: /home/test/maple_engine_standalone/maple_engine/maple_build/out/x86_64/libcore.VtableImpl.s
1002 :  .byte OP_regread, 0x4, 0xfd, 0xff                 // 00c8: %2
1003 :  .byte OP_regassign, 0x4, 0xfa, 0xff               // 00cc: %5
1004 :  // LINE String.java : 1471, INSTIDX : 36||0024:  imul
```
***Note***: Maple instruction 363412 is a OP_return from OP_icall at 344658.

### mstep command
Steps program until it reaches a different source line of Maple Application
Example:
```
(gdb) mlist
src file: /home/test/my_openjdk8/jdk/src/share/classes/java/util/Hashtable.java line: 465
     461         }
     462
     463         // Makes sure the key is not already in the hashtable.
     464         Entry<?,?> tab[] = table;
=>   465         int hash = key.hashCode();
     466         int index = (hash & 0x7FFFFFFF) % tab.length;
     467         @SuppressWarnings("unchecked")
     468         Entry<K,V> entry = (Entry<K,V>)tab[index];
     469         for(; entry != null ; entry = entry.next) {
     470             if ((entry.hash == hash) && entry.key.equals(key)) {

(gdb) ms
Info: executed 2 opcodes
src file: /home/test/my_openjdk8/jdk/src/share/classes/java/util/Hashtable.java line: 466
     462
     463         // Makes sure the key is not already in the hashtable.
     464         Entry<?,?> tab[] = table;
     465         int hash = key.hashCode();
=>   466         int index = (hash & 0x7FFFFFFF) % tab.length;
     467         @SuppressWarnings("unchecked")
     468         Entry<K,V> entry = (Entry<K,V>)tab[index];
     469         for(; entry != null ; entry = entry.next) {
     470             if ((entry.hash == hash) && entry.key.equals(key)) {
     471                 V old = entry.value;
```

### mnext command
Steps program, proceeding through subroutine calls of Maple application
Example:
```
(gdb) mlist
src file: /home/test/my_openjdk8/jdk/src/share/classes/java/util/Hashtable.java line: 468
     464         Entry<?,?> tab[] = table;
     465         int hash = key.hashCode();
     466         int index = (hash & 0x7FFFFFFF) % tab.length;
     467         @SuppressWarnings("unchecked")
=>   468         Entry<K,V> entry = (Entry<K,V>)tab[index];
     469         for(; entry != null ; entry = entry.next) {
     470             if ((entry.hash == hash) && entry.key.equals(key)) {
     471                 V old = entry.value;
     472                 entry.value = value;
     473                 return old;
(gdb) mn
Info: executed 2 opcodes
src file: /home/test/my_openjdk8/jdk/src/share/classes/java/util/Hashtable.java line: 468
     464         Entry<?,?> tab[] = table;
     465         int hash = key.hashCode();
     466         int index = (hash & 0x7FFFFFFF) % tab.length;
     467         @SuppressWarnings("unchecked")
=>   468         Entry<K,V> entry = (Entry<K,V>)tab[index];
     469         for(; entry != null ; entry = entry.next) {
     470             if ((entry.hash == hash) && entry.key.equals(key)) {
     471                 V old = entry.value;
     472                 entry.value = value;
     473                 return old;
```

### mfinish command
#### 1. Execute until selected Maple stack frame returns
Example:
```
(gdb) mfinish
asm file: /home/test/maple_engine_standalone/maple_engine/maple_build/out/x86_64/libcore.VtableImpl.s
=> 5854918 :    .byte OP_regread, 0xe, 0x1, 0x0                   // 0000: %13
   5854919 :    .byte OP_intrinsiccall, 0x1, 0x7f, 0x1            // 0004: MCCSyncEnterFast2
   5854920 :    // LINE Hashtable.java : 459, INSTIDX : 0||0000:  aload_2
src file: /home/test/my_openjdk8/jdk/src/share/classes/java/util/Hashtable.java line: 459
     455      * @see     #get(Object)
     456      */
     457     public synchronized V put(K key, V value) {
     458         // Make sure the value is not null
=>   459         if (value == null) {
     460             throw new NullPointerException();
     461         }
     462
     463         // Makes sure the key is not already in the hashtable.
     464         Entry<?,?> tab[] = table;
```


## Maple File Commands
### msrcpath command
**_msp_** is the alias of msrcpath command
Maple Debugger needs a search path list to search the source code files. msrcpath command is used to set and view this search path list
#### 1. msrcpath
Example:
```
(gdb) msrcpath
Maple source path list: --
/home/test/my_openjdk8/jdk/src/
/home/test/gitee/maple_engine/maple_build/examples/TypeTest
```
#### 2. msrcpath -add <path>
Add a path to the top of the search path list.
Example:
```
(gdb) msrcpath -add /home/test/new_openjdk8/jdk/src/
```
#### 3. msrcpath -del <path>
Delete one path from the search path list
```
(gdb) msrcpath -del /home/test/new_openjdk8/jdk/src/
```

### mlist command
***Make sure source code search path is set before use this command***
This command list the Maple application source code block
#### 1. mlist
Example:
```
(gdb) mlist
src file: /home/test/my_openjdk8/jdk/src/share/classes/sun/util/PreHashedMap.java line: 115
     111         this.size = size;
     112         this.shift = shift;
     113         this.mask = mask;
     114         this.ht = new Object[rows];
=>   115         init(ht);
     116     }
     117
     118     /**
     119      * Initializes this map.
     120      *

```
#### 2. mlist -asm
list the current instruction in co-responding assembly file
Example:
```
(gdb) mlist -asm
asm file: /home/test/gitee/maple_engine/maple_build/out/x86_64/libcore.VtableImpl.s line: 355834
      355825    // icallassigned : unknown
      355826    .byte OP_regread, 0xe, 0x1, 0x0                   // 0088: %5
      355827    .byte OP_ireadoff, 0xe, 0x0, 0x0                  // 008c
      355828    .byte OP_ireadoff, 0xe, 0x18, 0x0                 // 0090
      355829    .byte OP_constval, 0xe, 0x10, 0x1                 // 0094
      355830    .byte OP_add, 0xe, 0x0, 0x2                       // 0098
      355831    .byte OP_ireadoff, 0x9, 0x0, 0x0                  // 009c
      355832    .byte OP_regread, 0xe, 0x1, 0x0                   // 00a0: %5
      355833    .byte OP_regread, 0xe, 0xfd, 0xff                 // 00a4: %6
=>    355834    .byte OP_icall, 0x1, 0x0, 0x3                     // 00a8
      355835    // LINE PreHashedMap.java : 116, INSTIDX : 41||0029:  return
      355836    .byte OP_dread, 0xe, 0xfe, 0xff                   // 00ac: Reg1_R20417
      355837    .byte OP_intrinsiccall, 0x0, 0x29, 0x1            // 00b0: MPL_CLEANUP_LOCALREFVARS
      355838    .byte OP_return, 0x0, 0x0, 0x0                    // 00b4

```
#### 3. mlist num:
Lists current source code file at line of [line-num]
Example:
```
(gdb) mlist 110
src file: /home/test/my_openjdk8/jdk/src/share/classes/sun/util/PreHashedMap.java line: 110
     106      * @param mask
     107      *        The value with which hash codes are masked after being shifted
     108      */
     109     protected PreHashedMap(int rows, int size, int shift, int mask) {
=>   110         this.rows = rows;
     111         this.size = size;
     112         this.shift = shift;
     113         this.mask = mask;
     114         this.ht = new Object[rows];
     115         init(ht);

```
#### 4. mlist filename:line_num
Lists specified source code file at line of [line-num]
Example:
```
(gdb) mlist System.java:1168
src file: /home/test/my_openjdk8/jdk/src/share/classes/java/lang/System.java line: 1168
    1164         // initialization. So make sure the "props" is available at the
    1165         // very beginning of the initialization and all system properties to
    1166         // be put into it directly.
    1167         props = new Properties();
=>  1168         initProperties(props);  // initialized by the VM
    1169
    1170         // There are certain system configurations that may be controlled by
    1171         // VM options such as the maximum amount of direct memory and
    1172         // Integer cache size used to support the object identity semantics
    1173         // of autoboxing.  Typically, the library will obtain these values
```
#### 5. mlist +|-[num]
Lists current source code file offsetting from previous listed line, offset can be + or -
Example:
```
(gdb) mlist PreHashedMap.java:110
src file: /home/test/my_openjdk8/jdk/src/share/classes/sun/util/PreHashedMap.java line: 110
     106      * @param mask
     107      *        The value with which hash codes are masked after being shifted
     108      */
     109     protected PreHashedMap(int rows, int size, int shift, int mask) {
=>   110         this.rows = rows;
     111         this.size = size;
     112         this.shift = shift;
     113         this.mask = mask;
     114         this.ht = new Object[rows];
     115         init(ht);
(gdb) mlist +8
src file: /home/test/my_openjdk8/jdk/src/share/classes/sun/util/PreHashedMap.java line: 118
     114         this.ht = new Object[rows];
     115         init(ht);
     116     }
     117
=>   118     /**
     119      * Initializes this map.
     120      *
     121      * <p> This method must construct the map's hash chains and store them into
     122      * the appropriate elements of the given hash-table row array.
     123      *
```
#### 6. mlist -asm:+|-[num]
Lists current assembly instructions offsetting from previous listed line. offset can be + or -
```
(gdb) mlist -asm
asm file: /home/test/gitee/maple_engine/maple_build/out/x86_64/libcore.VtableImpl.s line: 355834
      355825    // icallassigned : unknown
      355826    .byte OP_regread, 0xe, 0x1, 0x0                   // 0088: %5
      355827    .byte OP_ireadoff, 0xe, 0x0, 0x0                  // 008c
      355828    .byte OP_ireadoff, 0xe, 0x18, 0x0                 // 0090
      355829    .byte OP_constval, 0xe, 0x10, 0x1                 // 0094
      355830    .byte OP_add, 0xe, 0x0, 0x2                       // 0098
      355831    .byte OP_ireadoff, 0x9, 0x0, 0x0                  // 009c
      355832    .byte OP_regread, 0xe, 0x1, 0x0                   // 00a0: %5
      355833    .byte OP_regread, 0xe, 0xfd, 0xff                 // 00a4: %6
=>    355834    .byte OP_icall, 0x1, 0x0, 0x3                     // 00a8
      355835    // LINE PreHashedMap.java : 116, INSTIDX : 41||0029:  return
      355836    .byte OP_dread, 0xe, 0xfe, 0xff                   // 00ac: Reg1_R20417
      355837    .byte OP_intrinsiccall, 0x0, 0x29, 0x1            // 00b0: MPL_CLEANUP_LOCALREFVARS
      355838    .byte OP_return, 0x0, 0x0, 0x0                    // 00b4
(gdb) mlist -asm:+10
asm file: /home/test/gitee/maple_engine/maple_build/out/x86_64/libcore.VtableImpl.s line: 355838
      355829    .byte OP_constval, 0xe, 0x10, 0x1                 // 0094
      355830    .byte OP_add, 0xe, 0x0, 0x2                       // 0098
      355831    .byte OP_ireadoff, 0x9, 0x0, 0x0                  // 009c
      355832    .byte OP_regread, 0xe, 0x1, 0x0                   // 00a0: %5
      355833    .byte OP_regread, 0xe, 0xfd, 0xff                 // 00a4: %6
      355834    .byte OP_icall, 0x1, 0x0, 0x3                     // 00a8
      355835    // LINE PreHashedMap.java : 116, INSTIDX : 41||0029:  return
      355836    .byte OP_dread, 0xe, 0xfe, 0xff                   // 00ac: Reg1_R20417
      355837    .byte OP_intrinsiccall, 0x0, 0x29, 0x1            // 00b0: MPL_CLEANUP_LOCALREFVARS
=>    355838    .byte OP_return, 0x0, 0x0, 0x0                    // 00b4
(gdb) mlist -asm:-15
asm file: /home/test/gitee/maple_engine/maple_build/out/x86_64/libcore.VtableImpl.s line: 355823
      355814    // LINE PreHashedMap.java : 114, INSTIDX : 30||001e:  putfield
      355815    .byte OP_regread, 0xe, 0x1, 0x0                   // 0070: %5
      355816    .byte OP_regread, 0xe, 0x1, 0x0                   // 0074: %5
      355817    .byte OP_constval, 0xe, 0x30, 0x0                 // 0078
      355818    .byte OP_add, 0xe, 0x0, 0x2                       // 007c
      355819    .byte OP_regread, 0xe, 0xfd, 0xff                 // 0080: %6
      355820    .byte OP_intrinsiccall, 0x1, 0x26, 0x3            // 0084: MCCWrite
      355821    // LINE PreHashedMap.java : 115, INSTIDX : 33||0021:  aload_0
      355822    // LINE PreHashedMap.java : 115, INSTIDX : 34||0022:  aload_0
=>    355823    // LINE PreHashedMap.java : 115, INSTIDX : 35||0023:  getfield
      355824    // LINE PreHashedMap.java : 115, INSTIDX : 38||0026:  invokevirtual
      355825    // icallassigned : unknown
      355826    .byte OP_regread, 0xe, 0x1, 0x0                   // 0088: %5
      355827    .byte OP_ireadoff, 0xe, 0x0, 0x0                  // 008c
      355828    .byte OP_ireadoff, 0xe, 0x18, 0x0                 // 0090
      355829    .byte OP_constval, 0xe, 0x10, 0x1                 // 0094
      355830    .byte OP_add, 0xe, 0x0, 0x2                       // 0098
      355831    .byte OP_ireadoff, 0x9, 0x0, 0x0                  // 009c
      355832    .byte OP_regread, 0xe, 0x1, 0x0                   // 00a0: %5
      355833    .byte OP_regread, 0xe, 0xfd, 0xff                 // 00a4: %6
```
#### 7. mlist .
Lists code located by the filename and line number of current Maple frame
Example:
```
(gdb) mbt
Maple Traceback (most recent call first):
#2 libcore.so:0x1589f2c:00a8: Lsun_2Futil_2FPreHashedMap_3B_7C_3Cinit_3E_7C_28IIII_29V(a64<sun.nio.cs.StandardCharsets$Aliases> %5=0x17598 "@\263\036\365\377\177", i32<> %1=1024, i32<> %2=211, i32<> %3=0, i32<> %4=1023) at /home/test/my_openjdk8/jdk/src/share/classes/sun/util/PreHashedMap.java:115
#4 libcore.so:0x19cefe0:0020: Lsun_2Fnio_2Fcs_2FStandardCharsets_24Aliases_3B_7C_3Cinit_3E_7C_28_29V(a64<sun.nio.cs.StandardCharsets$Aliases> %1=0x17598 "@\263\036\365\377\177") at /home/test/my_openjdk8/jdk/src/share/classes/java/nio/charset/StandardCharsets.java:377
#6 libcore.so:0x2691e34:0010: Lsun_2Fnio_2Fcs_2FStandardCharsets_24Aliases_3B_7C_3Cinit_3E_7C_28Lsun_2Fnio_2Fcs_2FStandardCharsets_241_3B_29V(a64<sun.nio.cs.StandardCharsets$Aliases> %2=0x17598 "@\263\036\365\377\177", a64<> %1=0x0) at /home/test/my_openjdk8/jdk/src/share/classes/java/nio/charset/StandardCharsets.java:367
#8 libcore.so:0x70ca46c:0050: Lsun_2Fnio_2Fcs_2FStandardCharsets_3B_7C_3Cinit_3E_7C_28_29V(a64<sun.nio.cs.StandardCharsets> %5=0x16870 "\340\336 \365\377\177") at /home/test/my_openjdk8/jdk/src/share/classes/java/nio/charset/StandardCharsets.java:711
#10 libcore.so:0x285fa9c:0048: Ljava_2Fnio_2Fcharset_2FCharset_3B_7C_3Cclinit_3E_7C_28_29V() at /home/test/my_openjdk8/jdk/src/share/classes/java/nio/charset/Charset.java:320
#28 libcore.so:0x798997c:00ec: Ljava_2Flang_2FSystem_3B_7CinitProperties_7C_28Ljava_2Futil_2FProperties_3B_29Ljava_2Futil_2FProperties_3B(a64<java.util.Properties> %1=0x1f058 "\220\202\022\365\377\177") at unknown:0
#30 libcore.so:0x888cd98:0098: Ljava_2Flang_2FSystem_3B_7CinitializeSystemClass_7C_28_29V() at /home/test/my_openjdk8/jdk/src/share/classes/java/lang/System.java:1168

(gdb) mlist .
src file: /home/test/my_openjdk8/jdk/src/share/classes/sun/util/PreHashedMap.java line: 115
     111         this.size = size;
     112         this.shift = shift;
     113         this.mask = mask;
     114         this.ht = new Object[rows];
=>   115         init(ht);
     116     }
     117
     118     /**
     119      * Initializes this map.
     120      *

```
#### 8. mlist -asm:.
Lists code located by the filename and line number of current Maple frame
```
(gdb) mbt -asm
Maple Traceback (most recent call first):
#2 libcore.so:0x1589f2c:00a8: Lsun_2Futil_2FPreHashedMap_3B_7C_3Cinit_3E_7C_28IIII_29V(a64<sun.nio.cs.StandardCharsets$Aliases> %5=0x17598 "@\263\036\365\377\177", i32<> %1=1024, i32<> %2=211, i32<> %3=0, i32<> %4=1023) at /home/test/gitee/maple_engine/maple_build/out/x86_64/libcore.VtableImpl.s:355834
#4 libcore.so:0x19cefe0:0020: Lsun_2Fnio_2Fcs_2FStandardCharsets_24Aliases_3B_7C_3Cinit_3E_7C_28_29V(a64<sun.nio.cs.StandardCharsets$Aliases> %1=0x17598 "@\263\036\365\377\177") at /home/test/gitee/maple_engine/maple_build/out/x86_64/libcore.VtableImpl.s:1660865
#6 libcore.so:0x2691e34:0010: Lsun_2Fnio_2Fcs_2FStandardCharsets_24Aliases_3B_7C_3Cinit_3E_7C_28Lsun_2Fnio_2Fcs_2FStandardCharsets_241_3B_29V(a64<sun.nio.cs.StandardCharsets$Aliases> %2=0x17598 "@\263\036\365\377\177", a64<> %1=0x0) at /home/test/gitee/maple_engine/maple_build/out/x86_64/libcore.VtableImpl.s:5694501
#8 libcore.so:0x70ca46c:0050: Lsun_2Fnio_2Fcs_2FStandardCharsets_3B_7C_3Cinit_3E_7C_28_29V(a64<sun.nio.cs.StandardCharsets> %5=0x16870 "\340\336 \365\377\177") at /home/test/gitee/maple_engine/maple_build/out/x86_64/libcore.VtableImpl.s:26283373
#10 libcore.so:0x285fa9c:0048: Ljava_2Fnio_2Fcharset_2FCharset_3B_7C_3Cclinit_3E_7C_28_29V() at /home/test/gitee/maple_engine/maple_build/out/x86_64/libcore.VtableImpl.s:6204417
#28 libcore.so:0x798997c:00ec: Ljava_2Flang_2FSystem_3B_7CinitProperties_7C_28Ljava_2Futil_2FProperties_3B_29Ljava_2Futil_2FProperties_3B(a64<java.util.Properties> %1=0x1f058 "\220\202\022\365\377\177") at /home/test/gitee/maple_engine/maple_build/out/x86_64/libcore.VtableImpl.s:28704324
#30 libcore.so:0x888cd98:0098: Ljava_2Flang_2FSystem_3B_7CinitializeSystemClass_7C_28_29V() at /home/test/gitee/maple_engine/maple_build/out/x86_64/libcore.VtableImpl.s:32916877

(gdb) mlist -asm:.
asm file: /home/test/gitee/maple_engine/maple_build/out/x86_64/libcore.VtableImpl.s line: 355834
      355825    // icallassigned : unknown
      355826    .byte OP_regread, 0xe, 0x1, 0x0                   // 0088: %5
      355827    .byte OP_ireadoff, 0xe, 0x0, 0x0                  // 008c
      355828    .byte OP_ireadoff, 0xe, 0x18, 0x0                 // 0090
      355829    .byte OP_constval, 0xe, 0x10, 0x1                 // 0094
      355830    .byte OP_add, 0xe, 0x0, 0x2                       // 0098
      355831    .byte OP_ireadoff, 0x9, 0x0, 0x0                  // 009c
      355832    .byte OP_regread, 0xe, 0x1, 0x0                   // 00a0: %5
      355833    .byte OP_regread, 0xe, 0xfd, 0xff                 // 00a4: %6
=>    355834    .byte OP_icall, 0x1, 0x0, 0x3                     // 00a8
      355835    // LINE PreHashedMap.java : 116, INSTIDX : 41||0029:  return
      355836    .byte OP_dread, 0xe, 0xfe, 0xff                   // 00ac: Reg1_R20417
      355837    .byte OP_intrinsiccall, 0x0, 0x29, 0x1            // 00b0: MPL_CLEANUP_LOCALREFVARS
      355838    .byte OP_return, 0x0, 0x0, 0x0                    // 00b4
```
#### 9. mlist -mir
Lists Maple IR located by the filename and line number of current Maple frame. User can cross check with
the result of mlist -asm command
```
(gdb) mlist -mir
asm file: /home/test/gitee/maple_engine/maple_build/out/x86_64/libcore.mpl.mir.mpl line: 2700423
     2700414 LOC 5845 473
     2700415   regassign a64 %18 (regread a64 %20)
     2700416   dassign %Reg0_R20359 0 (regread a64 %18)
     2700417   #LINE Hashtable.java : 473, INSTIDX : 81||0051:  areturn
     2700418   call &MCC_SyncExitFast (regread a64 %13)
     2700419   intrinsiccall MPL_CLEANUP_LOCALREFVARS_SKIP (dread a64 %Reg0_R159247, dread a64 %Reg9_R20359, dread a64 %Reg0_R159245, dread a64 %Reg10_R20359, dread a64 %Reg0_R20359)
     2700420   return (regread a64 %18)
     2700421 @label267857   #LINE Hashtable.java : 469, INSTIDX : 82||0052:  aload 6
     2700422 LOC 5845 469
=>   2700423   #LINE Hashtable.java : 469, INSTIDX : 84||0054:  getfield
     2700424   #intrinsiccallassigned : unknown
     2700425   call &MCC_LoadRefField_NaiveRCFast (
     2700426     regread a64 %19,
     2700427     add a64 (regread a64 %19, constval a64 32))
     2700428   regassign a64 %6 (regread a64 %%retval0)
     2700429   call &MCC_DecRef_NaiveRCFast (dread a64 %Reg0_R159245)
     2700430   regassign a64 %21 (regread a64 %6)
     2700431   dassign %Reg0_R159245 0 (regread a64 %21)
     2700432   #LINE Hashtable.java : 469, INSTIDX : 87||0057:  astore 6
     2700433   call &MCC_IncDecRef_NaiveRCFast (regread a64 %21, regread a64 %19)

```
#### 10. mlist -mir:+|-[num]
Lists current Maple IR offsetting from previous listed line. offset can be + or -
```
(gdb) mlist -mir
asm file: /home/test/gitee/maple_engine/maple_build/out/x86_64/libcore.mpl.mir.mpl line: 2700423
     2700414 LOC 5845 473
     2700415   regassign a64 %18 (regread a64 %20)
     2700416   dassign %Reg0_R20359 0 (regread a64 %18)
     2700417   #LINE Hashtable.java : 473, INSTIDX : 81||0051:  areturn
     2700418   call &MCC_SyncExitFast (regread a64 %13)
     2700419   intrinsiccall MPL_CLEANUP_LOCALREFVARS_SKIP (dread a64 %Reg0_R159247, dread a64 %Reg9_R20359, dread a64 %Reg0_R159245, dread a64 %Reg10_R20359, dread a64 %Reg0_R20359)
     2700420   return (regread a64 %18)
     2700421 @label267857   #LINE Hashtable.java : 469, INSTIDX : 82||0052:  aload 6
     2700422 LOC 5845 469
=>   2700423   #LINE Hashtable.java : 469, INSTIDX : 84||0054:  getfield
     2700424   #intrinsiccallassigned : unknown
     2700425   call &MCC_LoadRefField_NaiveRCFast (
     2700426     regread a64 %19,
     2700427     add a64 (regread a64 %19, constval a64 32))
     2700428   regassign a64 %6 (regread a64 %%retval0)
     2700429   call &MCC_DecRef_NaiveRCFast (dread a64 %Reg0_R159245)
     2700430   regassign a64 %21 (regread a64 %6)
     2700431   dassign %Reg0_R159245 0 (regread a64 %21)
     2700432   #LINE Hashtable.java : 469, INSTIDX : 87||0057:  astore 6
     2700433   call &MCC_IncDecRef_NaiveRCFast (regread a64 %21, regread a64 %19)
(gdb) mlist -mir:+10
asm file: /home/test/gitee/maple_engine/maple_build/out/x86_64/libcore.mpl.mir.mpl line: 2700433
     2700424   #intrinsiccallassigned : unknown
     2700425   call &MCC_LoadRefField_NaiveRCFast (
     2700426     regread a64 %19,
     2700427     add a64 (regread a64 %19, constval a64 32))
     2700428   regassign a64 %6 (regread a64 %%retval0)
     2700429   call &MCC_DecRef_NaiveRCFast (dread a64 %Reg0_R159245)
     2700430   regassign a64 %21 (regread a64 %6)
     2700431   dassign %Reg0_R159245 0 (regread a64 %21)
     2700432   #LINE Hashtable.java : 469, INSTIDX : 87||0057:  astore 6
=>   2700433   call &MCC_IncDecRef_NaiveRCFast (regread a64 %21, regread a64 %19)
     2700434   regassign a64 %19 (regread a64 %21)
     2700435   dassign %Reg9_R20359 0 (regread a64 %19)
     2700436 @@m11   #LINE Hashtable.java : 469, INSTIDX : 39||0027:  aload 6
     2700437   #LINE Hashtable.java : 469, INSTIDX : 41||0029:  ifnull
     2700438   brfalse @@m6 (eq i32 a64 (regread a64 %21, constval a64 0))
     2700439 @label267858   try { @label267859 }
     2700440   #LINE Hashtable.java : 477, INSTIDX : 92||005c:  aload_0
     2700441 LOC 5845 477
     2700442   #LINE Hashtable.java : 477, INSTIDX : 93||005d:  iload 4
     2700443   #LINE Hashtable.java : 477, INSTIDX : 95||005f:  aload_1
(gdb) mlist -mir:-8
asm file: /home/test/gitee/maple_engine/maple_build/out/x86_64/libcore.mpl.mir.mpl line: 2700425
     2700416   dassign %Reg0_R20359 0 (regread a64 %18)
     2700417   #LINE Hashtable.java : 473, INSTIDX : 81||0051:  areturn
     2700418   call &MCC_SyncExitFast (regread a64 %13)
     2700419   intrinsiccall MPL_CLEANUP_LOCALREFVARS_SKIP (dread a64 %Reg0_R159247, dread a64 %Reg9_R20359, dread a64 %Reg0_R159245, dread a64 %Reg10_R20359, dread a64 %Reg0_R20359)
     2700420   return (regread a64 %18)
     2700421 @label267857   #LINE Hashtable.java : 469, INSTIDX : 82||0052:  aload 6
     2700422 LOC 5845 469
     2700423   #LINE Hashtable.java : 469, INSTIDX : 84||0054:  getfield
     2700424   #intrinsiccallassigned : unknown
=>   2700425   call &MCC_LoadRefField_NaiveRCFast (
     2700426     regread a64 %19,
     2700427     add a64 (regread a64 %19, constval a64 32))
     2700428   regassign a64 %6 (regread a64 %%retval0)
     2700429   call &MCC_DecRef_NaiveRCFast (dread a64 %Reg0_R159245)
     2700430   regassign a64 %21 (regread a64 %6)
     2700431   dassign %Reg0_R159245 0 (regread a64 %21)
     2700432   #LINE Hashtable.java : 469, INSTIDX : 87||0057:  astore 6
     2700433   call &MCC_IncDecRef_NaiveRCFast (regread a64 %21, regread a64 %19)
     2700434   regassign a64 %19 (regread a64 %21)
     2700435   dassign %Reg9_R20359 0 (regread a64 %19)
```
#### 11. mlist -mir:.
Lists Maple IR located by the filename and line number of current Maple frame
```
(gdb) mlist -mir:.
asm file: /home/test/gitee/maple_engine/maple_build/out/x86_64/libcore.mpl.mir.mpl line: 2700423
     2700414 LOC 5845 473
     2700415   regassign a64 %18 (regread a64 %20)
     2700416   dassign %Reg0_R20359 0 (regread a64 %18)
     2700417   #LINE Hashtable.java : 473, INSTIDX : 81||0051:  areturn
     2700418   call &MCC_SyncExitFast (regread a64 %13)
     2700419   intrinsiccall MPL_CLEANUP_LOCALREFVARS_SKIP (dread a64 %Reg0_R159247, dread a64 %Reg9_R20359, dread a64 %Reg0_R159245, dread a64 %Reg10_R20359, dread a64 %Reg0_R20359)
     2700420   return (regread a64 %18)
     2700421 @label267857   #LINE Hashtable.java : 469, INSTIDX : 82||0052:  aload 6
     2700422 LOC 5845 469
=>   2700423   #LINE Hashtable.java : 469, INSTIDX : 84||0054:  getfield
     2700424   #intrinsiccallassigned : unknown
     2700425   call &MCC_LoadRefField_NaiveRCFast (
     2700426     regread a64 %19,
     2700427     add a64 (regread a64 %19, constval a64 32))
     2700428   regassign a64 %6 (regread a64 %%retval0)
     2700429   call &MCC_DecRef_NaiveRCFast (dread a64 %Reg0_R159245)
     2700430   regassign a64 %21 (regread a64 %6)
     2700431   dassign %Reg0_R159245 0 (regread a64 %21)
     2700432   #LINE Hashtable.java : 469, INSTIDX : 87||0057:  astore 6
     2700433   call &MCC_IncDecRef_NaiveRCFast (regread a64 %21, regread a64 %19)
```

## Maple Data Commands
### mlocal command
This command displays the stack frame local varibles' value and dynamic stack data.
#### 1. mlocal
This is used to display all local variables' value of a function. Use mlocal command at a Maple breakpoint and after a msi command to step into a function
Example:
```
(gdb) mlocal
local #1: name=%%thrownval type=v2i64 value=none
local #2: name=Reg0_R155549 type=a64 value=0x0
local #3: name=Reg1_R20114 type=a64 value=0x0
local #4: name=Reg0_R34665 type=a64 value=0x0
local #5: name=%1 type=i32 value=0
local #6: name=%2 type=i32 value=0
local #7: name=%3 type=i32 value=0
local #8: name=%4 type=i32 value=0
local #9: name=%5 type=i32 value=0
local #10: name=%6 type=a64 value=0x0
local #11: name=%7 type=a64 value=0x0
local #12: name=%8 type=a64 value=0x0
local #13: name=%9 type=u16 value=0
local #14: name=%10 type=i32 value=0
local #15: name=%11 type=i32 value=0
local #16: name=%12 type=i32 value=0
local #17: name=%13 type=a64 value=0x0
local #18: name=%14 type=u1 value=0 '\000'
local #19: name=%15 type=i32 value=0
local #20: name=%16 type=u1 value=0 '\000'
local #21: name=%17 type=u1 value=0 '\000'
local #22: name=%18 type=i32 value=0
local #23: name=%19 type=u1 value=0 '\000'
local #24: name=%20 type=i32 value=0
local #25: name=%21 type=i32 value=0
local #26: name=%22 type=a64 value=0x0
local #27: name=%24 type=a64 value=0x0
local #28: name=%25 type=a64 value=0x0
local #29: name=%27 type=a64 value=0x0
```
#### 2. mlocal -stack or mlocal -s
This is used to display all stack frame's dynamic data.
Example:
```
(gdb) mlocal -stack
sp=1: type=a64 value=0x7ffff49915e8 <_C_STR_aa6c9e888c754e206fadbf41fdc90033> "0u\214\366\377\177"
sp=2: type=a64 value=0x0
```
### mprint command
This command is used to display the object data and object type includind the inheritence hierarchy
#### 1. syntax: mprint <hex address>
Example:
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
Example:
```
(gdb) mtype Nodes_24ToArrayTask_24OfRef
#1 class name: Ljava_2Futil_2Fstream_2FNodes_24ToArrayTask_24OfRef_3B:
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
    #8,off=40,len= 8,"array           "

  Method list:
    #1 Ljava_2Futil_2Fstream_2FNodes_24ToArrayTask_24OfRef_3B_7C_3Cinit_3E_7C_28Ljava_2Futil_2Fstream_2FNode_3BALjava_2Flang_2FObject_3BILjava_2Futil_2Fstream_2FNodes_241_3B_29V
    #2 Ljava_2Futil_2Fstream_2FNodes_24ToArrayTask_24OfRef_3B_7C_3Cinit_3E_7C_28Ljava_2Futil_2Fstream_2FNode_3BALjava_2Flang_2FObject_3BI_29V
    #3 Ljava_2Futil_2Fstream_2FNodes_24ToArrayTask_24OfRef_3B_7C_3Cinit_3E_7C_28Ljava_2Futil_2Fstream_2FNodes_24ToArrayTask_24OfRef_3BLjava_2Futil_2Fstream_2FNode_3BI_29V
    #4 Ljava_2Futil_2Fstream_2FNodes_24ToArrayTask_24OfRef_3B_7CcopyNodeToArray_7C_28_29V
    #5 Ljava_2Futil_2Fstream_2FNodes_24ToArrayTask_24OfRef_3B_7CmakeChild_7C_28II_29Ljava_2Futil_2Fstream_2FNodes_24ToArrayTask_24OfRef_3B
    #6 Ljava_2Futil_2Fstream_2FNodes_24ToArrayTask_24OfRef_3B_7CmakeChild_7C_28II_29Ljava_2Futil_2Fstream_2FNodes_24ToArrayTask_3B
```
#### 2. If multiple classes match
Example:
```
(gdb) mtype Nodes_24ToArrayTask
#1 class name: Ljava_2Futil_2Fstream_2FNodes_24ToArrayTask_24OfPrimitive_3B:
#2 class name: Ljava_2Futil_2Fstream_2FNodes_24ToArrayTask_24OfLong_3B:
#3 class name: Ljava_2Futil_2Fstream_2FNodes_24ToArrayTask_24OfInt_3B:
#4 class name: Ljava_2Futil_2Fstream_2FNodes_24ToArrayTask_3B:
#5 class name: Ljava_2Futil_2Fstream_2FNodes_24ToArrayTask_24OfDouble_3B:
#6 class name: Ljava_2Futil_2Fstream_2FNodes_24ToArrayTask_24OfRef_3B:
```

### msymbol command
**_syntax_**: msymbol <regex-of-symbol-name-search-pattern>

#### 1. If only one matches
Example:
```
(gdb) msymbol Lsun_2Fsecurity_2Fprovider_2FPolicyParser_3B_7Cmain_7C_28ALjava_2Flang_2FString_3B_29V
#1 symbol name: Lsun_2Fsecurity_2Fprovider_2FPolicyParser_3B_7Cmain_7C_28ALjava_2Flang_2FString_3B_29V
assembly file : /home/test/gitee/maple_engine/maple_build/out/x86_64/libcore.VtableImpl.s
source        : /home/test/my_openjdk8/jdk/src/share/classes/sun/security/provider/PolicyParser.java
demangled name: Lsun.security.provider.PolicyParser;main(ALjava.lang.String;)V
```
#### 2. If multiple symbols match
Example:
```
(gdb) msymbol executor
#1 symbol name: Lsun_2Fnio_2Fch_2FAsynchronousChannelGroupImpl_3B_7Cexecutor_7C_28_29Ljava_2Futil_2Fconcurrent_2FExecutorService_3B
#2 symbol name: Lsun_2Fnio_2Fch_2FAsynchronousFileChannelImpl_3B_7Cexecutor_7C_28_29Ljava_2Futil_2Fconcurrent_2FExecutorService_3B
#3 symbol name: Lsun_2Fnio_2Fch_2FThreadPool_3B_7Cexecutor_7C_28_29Ljava_2Futil_2Fconcurrent_2FExecutorService_3B
```

## Maple Miscellaneous Commands
### mset command
This command is used to set Maple debugger's environment variables and flags
#### 1. mset -show
display the mset settings
Example:
```
(gdb) mset -show
{'maple_lib_asm_path': ['~/gitee/maple_engine/maple_build/out/x86_64/', './'], 'verbose': 'off'}
{'maple_lib_asm_path': ['~/gitee/maple_engine/maple_build/out/x86_64/', './'], 'verbose': 'off', 'event': 'disconnect'}
```
#### 2. mset <key> <value>
Example:
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
#### 5. mset linecount <n>
set the number of lines mlist will display for source file and assembly file
```
(gdb) mset linecount 20
(gdb) mlist
src file: /home/test/my_openjdk8/jdk/src/share/classes/java/util/AbstractMap.java line: 74
      65  * @since 1.2
      66  */
      67
      68 public abstract class AbstractMap<K,V> implements Map<K,V> {
      69     /**
      70      * Sole constructor.  (For invocation by subclass constructors, typically
      71      * implicit.)
      72      */
      73     protected AbstractMap() {
=>    74     }
      75
      76     // Query Operations
      77
      78     /**
      79      * {@inheritDoc}
      80      *
      81      * @implSpec
      82      * This implementation returns <tt>entrySet().size()</tt>.
      83      */
      84     public int size() {
```

### mhelp command
#### 1. List all the commands and Maple Debugger version
Example:
```
(gdb) mhelp
Maple Version 1.2

mbreak:     Sets and manages Maple breakpoints
mbacktrace: Displays Maple backtrace in multiple modes
mup:        Selects and prints the Maple stack frame that called this one
mdown:      Selects and prints the Maple stack frame called by this one
mlist:      Lists source code in multiple modes
msrcpath:   Adds and manages the Maple source code search paths
mlocal:     Displays selected Maple frame arguments, local variables and stack dynamic data
mprint:     Prints Maple runtime object data
mtype:      Prints a matching class and its inheritance hierarchy by a given search expression
msymbol:    Prints a matching symbol list or its detailed infomatioin
mstepi:     Steps a specified number of Maple instructions
mnexti:     Steps one Maple instruction, but proceeds through subroutine calls
mstep:      Steps program until it reaches a different source line of Maple Application
mnext:      Steps program, proceeding through subroutine calls of Maple application
mset:       Sets and displays Maple debugger settings
mfinish:    Execute until the selected Maple stack frame returns
mhelp:      Lists Maple help information
```

#### 2. Help of individual Maple command
mhelp <command name>
***Note***: command name must be a Maple command full name other than its alias name
Example:
```
(gdb) mhelp mbreak
Maple Version 1.2
mbreak <symbol>: Sets a new Maple breakpoint at symbol
mbreak -set <symbol>: Alias for "mbreak <symbol>"
mbreak -enable <symbol|index>: Enables an existing Maple breakpoint at symbol
mbreak -disable <symbol|index>: Disables an existing Maple breakpoint at symbol
mbreak -clear <symbol|index>: Deletes an existing Maple breakpoint at symbol
mbreak -clearall : Deletes all existing Maple breakpoints
mbreak -listall : Lists all existing Maple breakpoints
mbreak -ignore <symbol | index> <count>: Sets ignore count for specified Maple breakpoints
```

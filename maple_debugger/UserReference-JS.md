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
## Purpose of This Document
This document provides a user reference for the JavaScript features of the Maple Debugger. However, please understand that the Maple Debugger supports both static language (e.g. OpenJDK/Java) and dynamic language (e.g. JavaScript) using the same code. The Maple Debugger determines the language of a a running program such as Java or JavaScript. The Maple Debugger then dynamically selects the corresponding code/data to debug correctly for the end user.

```
This document describes the commands that can run against a program written in JavaScript.

```
## Prior to launch
Once the Maple Debugger is launched, the following message will be displayed:

```
Before starting to use the Maple debugger, please make sure to setup two sets of search paths:

1, The search paths for the user program and library source code.
   This is required to display the source code of your application and libraries
   Use the "msrcpath" command to show/add/delete source code search paths
2, The search paths of the generated assembly files of the user program and library.
   This is required by many of the Maple debugger commands
   Use the "mset" command to show/add/delete the library asm file search paths
```
For item #1, a search path list should be set for the Maple Debugger to find the source code of libraries and user applications. The "mlist" command uses this search list to look up the source code files.
For item #2, a search path list should be set for the Maple Debugger to find assembly files when the corresponding .so file is compiled. The "mbt", "mup", "mdown" and other commands will use this search list to look up information from the assembly files.

## Maple Command Summary
* **_mbreakpoint_**: Sets and manages Maple breakpoints
* **_mbacktrace_** : Displays Maple backtrace in multiple modes
* **_mup_**        : Selects and prints the Maple stack frame that called the current one
* **_mdown_**      : Selects and prints the Maple stack frame called by the current one
* **_mlist_**      : Lists source code in multiple modes
* **_msrcpath_**   : Adds and manages the Maple source code search paths
* **_mlocal_**       : Displays selected Maple frame arguments and local variables and stack dynamic data
* **_mprint_**       : Prints Maple runtime object data
* **_mtype_**        : Prints a matching class and its inheritance hierarchy by a given search expression
* **_msymbol_**      : Prints a matching symbol list or its detailed infomation
* **_mstepi_**     : Steps specified number of Maple instructions
* **_mnexti_**     : Step one Maple instruction, but proceed through subroutine calls
* **_mfinish_**    : Executes until the selected Maple stack frame returns
* **_mset_**       : Sets and displays the Maple debugger settings
* **_mhelp_**      : Provides help for commands and their usage

## Maple Breakpoint command
### mbreak command
**_mb_** is an alias of the "mbreak" command
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
(gdb) mb __jsmain
set breakpoint __jsmain
Function "maple::maple_invoke_method" not defined.
Breakpoint 1 (maple::maple_invoke_method) pending.
Function "maple::InvokeInterpretMethod" not defined.
Breakpoint 2 (maple::InvokeInterpretMethod) pending.
```
Example 2:
```
(gdb) mb Add__a63c049
set breakpoint Add__a63c049
```
***Note***: this shows "pending" Maple breakpoints because the program has not been loaded yet

#### 3. Display breakpoints and info
Execute 'mbreak -list' or 'mbreak -listall'
Example 1:
```
(gdb) mb -list
list all Maple breakpoints
#1 __jsmain enabled hit_count 1 ignore_count 0 at pending address in maple::maple_invoke_method()
#2 Add__a63c049 enabled hit_count 0 ignore_count 0 at pending address in maple::maple_invoke_method()
```
***Note***: this output shows the Maple breakpoints when those breakpoints are in a "pending" state.

#### 4. Disable a Maple breakpoint
Execute 'mb -disable <symbol | index>' command
Example 1:
```
(gdb) mb -disable 1
disable breakpoint  1
```
***Note***: use the Maple breakpoint index 1 which is __jsmain from the Maple breakpoint listed above
Example 2:
```
(gdb) mb -disable __jsmain
disable breakpoint __jsmain
```
#### 5. Enable a Maple breakpoint
Execute 'mb -enable <symbol | index>' command
Example:
```
(gdb) mb -enable 1
enable breakpoint  1
```
***Note***: use the Maple breakpoint index 1 which is __jsmain from the Maple breakpoint listed above
Example 2:
```
(gdb) mb -enable __jsmain
enable breakpoint  __jsmain
```
#### 6. Delete a Maple breakpoint
Execute 'mb -clear <symbol | index>' command
Example:
```
(gdb) mb -clear 1
clear breakpoint  1
```
***Note***: use the Maple breakpoint index 1 which is __jsmain from the Maple breakpoint listed above
Example 2:
```
(gdb) mb -clear __jsmain
clear breakpoint  __jsmain
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
(gdb) mb -ignore 1 2
ignore breakpoint  1 2
```
## Maple Stack Commands
### mbacktrace command
**_mbt_** is an alias of "mbacktrace" command. **_Syntax_**: mbt [-full | -asm]
#### 1. View the Maple frame's backtrace
Execute command 'mbt'
Example:
```
(gdb) mbt
Maple Traceback (most recent call first):
#0 add.so:0xa64:0000: Add__59aaa481(dynany<> %par1=0x7ffff6066fc8, dynany<> %par2=0x7ffff6066fd0) at ./add.js:18
#3 add.so:0xad4:008c: __jsmain() at ./add.js:23
```
***Note***: #0 is the stack frame level, **_Add__59aaa481_** is the function symbol in **_add.so_**, the function address is 0xa64:0000. This function has 2 passed in parameters. The first one is marked as %par1, and its memory address is **_0x7ffff6066fc8_**, its compile time type is **_dynany_**, its runtime value can be retrieved by **_mprint_** command. This also shows the function is in the source code file **_./add.js, line 18_**

#### 2. View Maple backtrace in asm format
Execute 'mbt -asm' command
Example:
```
(gdb) mbt -asm
Maple Traceback (most recent call first):
#0 add.so:0xa64:0000: Add__59aaa481(dynany<> %par1=0x7ffff6066fc8, dynany<> %par2=0x7ffff6066fd0) at /home/test/gitee/maple_engine/maple_build/examples/JavaScript/add/add.s:203
#3 add.so:0xad4:008c: __jsmain() at /home/che/maple_engine_standalone/maple_engine/maple_build/examples/JavaScript/add/add.s:290
```
**Note**: This is exact same Maple backtrace, but the displayed source file is the assembly file. For example, **_/home/test/gitee/maple_engine/maple_build/examples/JavaScript/add/add.s_** is the assembly file,  line number is **_203_** in the assmebly file.

#### 3. View Maple backtrace in Maple IR format
Execute 'mbt -mir' command
Example:
```
(gdb) mbt -mir
**_mbt -mir_** is not supported for JavaScript
```

#### 4. View a full backtrace mixed with Maple frames and gdb native frames
Execute 'mbt -full' command
Example:
```
(gdb) mbt -full
Maple Traceback (most recent call first):
#0 add.so:0xa64:0000: Add__59aaa481(dynany<> %par1=0x7ffff6066fc8, dynany<> %par2=0x7ffff6066fd0) at ./add.js:18
#1  0x00007ffff6977a19 in maple::maple_invoke_dynamic_method (header=0x7ffff6c9da64 <Add__59aaa481_mirbin_info>, obj=0x7ffff58671d0) at /home/test/gitee/maple_engine/maple_engine/src/invoke_dyn_method.cpp:2744
#2  0x00007ffff6981d13 in maple::InterSource::FuncCall (this=0x55555576d5f0, callee=0x7ffff6c9da60 <Add__59aaa481>, isIntrinsiccall=false, env=0x0, args=0x7fffffffa840, numArgs=4, start=2, nargs=-1, strictP=false) at /home/test/gitee/maple_engine/maple_engine/src/shimdynfunction.cpp:2159
#3 add.so:0xad4:008c: __jsmain() at ./add.js:23
#4  0x00007ffff6977b49 in maple::maple_invoke_dynamic_method_main (mPC=0x7ffff6c9daec <__jsmain_mirbin_code> "]\n", cheader=0x7ffff6c9dad4 <__jsmain_mirbin_info>) at /home/test/gitee/maple_engine/maple_engine/src/invoke_dyn_method.cpp:2750
#5  0x00007ffff6982d4d in EngineShimDynamic (firstArg=140737333811920, appPath=0x55555576c140 "/home/test/gitee/maple_engine/maple_build/examples/JavaScript/add/add.so") at /home/test/gitee/maple_engine/maple_engine/src/shimdynfunction.cpp:2377
#6  0x0000555555555eab in maple::mplsh (argc=<optimized out>, argv=0x7fffffffdec0) at ../../../mrt/platform-rt/mplsh/mplsh.cc:268
#7  main (argc=<optimized out>, argv=<optimized out>) at ../../../mrt/platform-rt/mplsh/mplsh.cc:392

```
***Note***: #0 is the Maple frames, the others are gdb native frames

### mup command
#### 1. mup
If "mbt" shows current Maple frame is at stack level 0, then "mup" command will make next older Maple frame the current frame.
Example:
```
(gdb) mbt
Maple Traceback (most recent call first):
#0 add.so:0xa64:0000: Add__59aaa481(dynany<> %par1=0x7ffff6066fc8, dynany<> %par2=0x7ffff6066fd0) at ./add.js:18
#3 add.so:0xad4:008c: __jsmain() at ./add.js:23
````
***Note***: current frame stack level is at level 0
```
(gdb) mup
#3 add.so:0xad4:008c: __jsmain() at ./add.js:23
```
***Note***: now the current Maple frame level becomes 3
#### 2. mup [n]
If "mbt" shows that the current Maple frame is set at stack level 0, then the "mup" command makes the next n older Maple frame the current frame
Example:
```
(gdb) mbt
Maple Traceback (most recent call first):
#0 add.so:0xa64:0000: Add__59aaa481(dynany<> %par1=0x7ffff6066fc8, dynany<> %par2=0x7ffff6066fd0) at ./add.js:18
#3 add.so:0xad4:008c: __jsmain() at ./add.js:23
```
***Note***: the current frame stack level is set to level 0
```
(gdb) mup 1
#3 add.so:0xad4:008c: __jsmain() at ./add.js:23
```
***Note***: now the current Maple frame level becomes 3

### mdown command
#### 1. mdown
If "mbt" shows that the current Maple frame is set to stack level 3, then the "mdown" command makes the next newer Maple frame the current frame.
Example:
```
(gdb) mbt
Maple Traceback (most recent call first):
#3 add.so:0xad4:008c: __jsmain() at ./add.js:23
```
***Note***: the current Maple frame is at stack level 3
```
(gdb) mdown
#0 add.so:0xa64:0000: Add__59aaa481(dynany<> %par1=0x7ffff6066fc8, dynany<> %par2=0x7ffff6066fd0) at ./add.js:18
```
***Note***: the current Maple frame is set to stack level 0 after the "mdown" command
#### 2. mdown [n]
This command moves to  the next newer n Maple frames and stays at that level in the stack.
Example:
```
(gdb) mbt
Maple Traceback (most recent call first):
#3 add.so:0xad4:008c: __jsmain() at ./add.js:23
```
***Note***: the current Maple frame is set to stack level 3
```
(gdb) mdown 1
#0 add.so:0xa64:0000: Add__59aaa481(dynany<> %par1=0x7ffff6066fc8, dynany<> %par2=0x7ffff6066fd0) at ./add.js:18
```
***Note***: after "mdown 1", the current Maple frame is set back to stack level 0

## Maple Control commands
### mstepi command
**_msi_** is an alias of the "mstepi" command
#### 1. Step into the next Maple instruction
Example:
```
(gdb) msi
asm file: /home/test/gitee/maple_engine/maple_build/examples/JavaScript/add/add.s
=> 205 :        .byte OP_iassignfpoff, 0x16, 0xf0, 0xff           // 000c
   206 :        // LINE add.js : 3: JSOP_POP
   207 :        // LINE add.js : 4:   sum = par1 + par2;
```

#### 2. Step into next n Maple instructions
Execute 'msi [n]' command.
Example:
```
(gdb) msi 10
asm file: /home/test/gitee/maple_engine/maple_build/examples/JavaScript/add/add.s
=> 225 :        .byte OP_return, 0x0, 0x0, 0x0                    // 0034
   226 :        // LINE add.js : 5: JSOP_RETRVAL
   227 :        .byte OP_constval, 0x4, 0x0, 0x0                  // 0038
```

#### 3. Step to a specified Maple instruction by the Maple instruction count number
Execute 'msi -abs n' command
Example:
```
(gdb) msi -abs 40
asm file: /home/test/gitee/maple_engine/maple_build/examples/JavaScript/add/add.s
=> 300 :        .byte OP_regread, 0xe, 0xfd, 0xff                 // 009c: %-3
   301 :        .byte OP_constval, 0x4, 0x10, 0x0                 // 00a0
   302 :        .byte OP_add, 0xe, 0x0, 0x2                       // 00a4
```

***Note***: it stops at Maple instruction count number 344658

#### 4. "msi -c" will display current opcode count
Example:
```
(gdb) msi -c
current opcode count is 32
```

### mni command
#### 1. Move to the next Maple instruction
Example:
```
(gdb) mni
asm file: /home/test/gitee/maple_engine/maple_build/examples/JavaScript/add/add.s
301 :   .byte OP_constval, 0x4, 0x10, 0x0                 // 00a0
302 :   .byte OP_add, 0xe, 0x0, 0x2                       // 00a4
303 :   .byte OP_intrinsicop, 0x16, 0xd5, 0x1             // 00a8: JSOP_GET_THIS_PROP_BY_NAME
```

### mstep command
Steps program until it reaches a different source line of Maple Application
Example:
```
(gdb) mlist
src file: ./add.js line: 10
       6 }
       7
       8 var v1 = Add(1, 2);
       9
=>    10 if (v1 ==3 ){
      11   print(" add: pass\n");
      12 } else {
      13   $ERROR("test failed v1 expect 3 but get",  v1, "\n");
      14 }
(gdb) ms
Info: executed 6 opcodes
src file: ./add.js line: 11
       7
       8 var v1 = Add(1, 2);
       9
      10 if (v1 ==3 ){
=>    11   print(" add: pass\n");
      12 } else {
      13   $ERROR("test failed v1 expect 3 but get",  v1, "\n");
      14 }
```

### mnext command
Steps program, proceeding through subroutine calls of Maple application
Example:
```
(gdb) mlist
src file: ./add.js line: 11
       7
       8 var v1 = Add(1, 2);
       9
      10 if (v1 ==3 ){
=>    11   print(" add: pass\n");
      12 } else {
      13   $ERROR("test failed v1 expect 3 but get",  v1, "\n");
      14 }
(gdb) mn
 add: pass

Info: executed 7 opcodes
src file: ./add.js line: 11
       7
       8 var v1 = Add(1, 2);
       9
      10 if (v1 ==3 ){
=>    11   print(" add: pass\n");
      12 } else {
      13   $ERROR("test failed v1 expect 3 but get",  v1, "\n");
      14 }
```

### mfinish command
#### 1. Execute until selected Maple stack frame returns
Example:
```
(gdb) mfinish
Breakpoint 4 at 0x7ffff6a43feb: file /home/test/gitee/maple_engine/maple_engine/src/invoke_dyn_method.cpp, line 178.
asm file: /home/test/gitee/maple_engine/maple_build/examples/JavaScript/add/add.s
=> 286 :        .byte OP_regread, 0x16, 0xfa, 0xff                // 0080: %-6
   287 :        .byte OP_iassignfpoff, 0x16, 0xe8, 0xff           // 0084
   288 :        // LINE add.js : 8: JSOP_SETNAME
src file: ./add.js line: 8
       4   sum = par1 + par2;
       5   return(sum)
       6 }
       7
=>     8 var v1 = Add(1, 2);
       9
      10 if (v1 ==3 ){
      11   print(" add: pass\n");
      12 } else {
      13   $ERROR("test failed v1 expect 3 but get",  v1, "\n");
```


## Maple File Commands
### msrcpath command
**_msp_** is an alias of the "msrcpath" command
Maple Debugger needs a search path list to search the source code files. "msrcpath" command is used to set and view this search path list
#### 1. msrcpath
Example:
```
(gdb) msrcpath
Maple source path list: --
./
/home/test/my_openjdk8/jdk/src/
/home/test/my_openjdk8/build/linux-x86_64-normal-server-release/
/home/test/gitee/maple_engine/maple_build/examples/JavaScript/add
```
#### 2. msrcpath -add <path>
Add a path to the top of the search path list.
Example:
```
(gdb) msrcpath -add /home/test/gitee/maple_engine/maple_build/examples/JavaScript/js0065
```
#### 3. msrcpath -del <path>
Delete one path from the search path list
```
(gdb) msrcpath -del /home/test/gitee/maple_engine/maple_build/examples/JavaScript/js0065
```

### mlist command
***Make sure source code search path is set before use this command***
This command lists the Maple application source code block
#### 1. mlist
Example:
```
(gdb) mlist
src file: ./add.js line: 3
       1 function Add(par1, par2)
       2 {
=>     3   var sum;
       4   sum = par1 + par2;
       5   return(sum)
       6 }
       7
       8 var v1 = Add(1, 2);
```
#### 2. mlist -asm
list the current instruction in corresponding assembly file
Example:
```
(gdb) mlist -asm
asm file: /home/test/gitee/maple_engine/maple_build/examples/JavaScript/add/add.s line: 203
         194    .word 4, 4              // formalWords bit vector byte count, localWords bit vector byte count
         195    .byte 0x15, 0x0, 0x0, 0x0       // formalWordsTypeTagged
         196    .byte 0x0, 0x0, 0x0, 0x0        // formalWordsRefCounted
         197    .byte 0x28, 0x0, 0x0, 0x0       // localWordsTypeTagged
         198    .byte 0x0, 0x0, 0x0, 0x0        // localWordsRefCounted
         199    .p2align 1
         200 Add__a63c049_mirbin_code:
         201    // LINE add.js : 3:   var sum;
         202    // LINE add.js : 3: JSOP_GETLOCAL
=>       203    .byte OP_constval64, 0x17, 0x0, 0x0               // 0000
         204    .byte 0x0, 0x0, 0x0, 0x0, 0x8, 0x0, 0x0, 0x0
         205    .byte OP_iassignfpoff, 0x16, 0xf0, 0xff           // 000c
         206    // LINE add.js : 3: JSOP_POP
         207    // LINE add.js : 4:   sum = par1 + par2;
         208    // LINE add.js : 4: JSOP_GETARG
         209    // LINE add.js : 4: JSOP_GETARG
         210    // LINE add.js : 4: JSOP_ADD
         211    .byte OP_ireadfpoff, 0x16, 0x8, 0x0               // 0010
         212    .byte OP_ireadfpoff, 0x16, 0x10, 0x0              // 0014
         213    .byte OP_intrinsiccall, 0x0, 0x32, 0x2            // 0018: JSOP_ADD
```
#### 3. mlist num:
Lists current source code file at line of [line-num]
Example:
```
(gdb) mlist 10
src file: ./add.js line: 10
       6 }
       7
       8 var v1 = Add(1, 2);
       9
=>    10 if (v1 ==3 ){
      11   print(" add: pass\n");
      12 } else {
      13   $ERROR("test failed v1 expect 3 but get",  v1, "\n");
      14 }
```
#### 4. mlist filename:line_num
Lists specified source code file at line number [line-num]
Example:
```
(gdb) mlist add.js:12
src file: ./add.js line: 12
       8 var v1 = Add(1, 2);
       9
      10 if (v1 ==3 ){
      11   print(" add: pass\n");
=>    12 } else {
      13   $ERROR("test failed v1 expect 3 but get",  v1, "\n");
      14 }
```
#### 5. mlist +|-[num]
Lists current source code file offsetting from the previous listed line, offset can be + or -
Example:
```
(gdb) mlist add.js:12
src file: ./add.js line: 12
       8 var v1 = Add(1, 2);
       9
      10 if (v1 ==3 ){
      11   print(" add: pass\n");
=>    12 } else {
      13   $ERROR("test failed v1 expect 3 but get",  v1, "\n");
      14 }
(gdb) mlist +2
src file: ./add.js line: 14
      10 if (v1 ==3 ){
      11   print(" add: pass\n");
      12 } else {
      13   $ERROR("test failed v1 expect 3 but get",  v1, "\n");
=>    14 }
```

#### 6. mlist -asm:+|-[num]
Lists current assembly instructions offsetting from the previous listed line. offset can be + or -
```
(gdb) mlist -asm
asm file: /home/test/gitee/maple_engine/maple_build/examples/JavaScript/add/add.s line: 223
         214    .byte OP_regread, 0x16, 0xfa, 0xff                // 001c: %-6
         215    .byte OP_iassignfpoff, 0x16, 0xe8, 0xff           // 0020
         216    // LINE add.js : 4: JSOP_SETLOCAL
         217    .byte OP_ireadfpoff, 0x16, 0xe8, 0xff             // 0024
         218    .byte OP_iassignfpoff, 0x16, 0xf0, 0xff           // 0028
         219    // LINE add.js : 4: JSOP_POP
         220    // LINE add.js : 5:   return(sum)
         221    // LINE add.js : 5: JSOP_GETLOCAL
         222    // LINE add.js : 5: JSOP_RETURN
=>       223    .byte OP_ireadfpoff, 0x16, 0xf0, 0xff             // 002c
         224    .byte OP_regassign, 0x16, 0xfa, 0xff              // 0030: %-6
         225    .byte OP_return, 0x0, 0x0, 0x0                    // 0034
         226    // LINE add.js : 5: JSOP_RETRVAL
         227    .byte OP_constval, 0x4, 0x0, 0x0                  // 0038
         228    .byte OP_regassign, 0x4, 0xfa, 0xff               // 003c: %-6
         229    .byte OP_return, 0x0, 0x0, 0x0                    // 0040
(gdb) mlist -asm:+5
asm file: /home/test/gitee/maple_engine/maple_build/examples/JavaScript/add/add.s line: 228
         219    // LINE add.js : 4: JSOP_POP
         220    // LINE add.js : 5:   return(sum)
         221    // LINE add.js : 5: JSOP_GETLOCAL
         222    // LINE add.js : 5: JSOP_RETURN
         223    .byte OP_ireadfpoff, 0x16, 0xf0, 0xff             // 002c
         224    .byte OP_regassign, 0x16, 0xfa, 0xff              // 0030: %-6
         225    .byte OP_return, 0x0, 0x0, 0x0                    // 0034
         226    // LINE add.js : 5: JSOP_RETRVAL
         227    .byte OP_constval, 0x4, 0x0, 0x0                  // 0038
=>       228    .byte OP_regassign, 0x4, 0xfa, 0xff               // 003c: %-6
         229    .byte OP_return, 0x0, 0x0, 0x0                    // 0040
(gdb) mlist -asm:-15
asm file: /home/test/gitee/maple_engine/maple_build/examples/JavaScript/add/add.s line: 213
         204    .byte 0x0, 0x0, 0x0, 0x0, 0x8, 0x0, 0x0, 0x0
         205    .byte OP_iassignfpoff, 0x16, 0xf0, 0xff           // 000c
         206    // LINE add.js : 3: JSOP_POP
         207    // LINE add.js : 4:   sum = par1 + par2;
         208    // LINE add.js : 4: JSOP_GETARG
         209    // LINE add.js : 4: JSOP_GETARG
         210    // LINE add.js : 4: JSOP_ADD
         211    .byte OP_ireadfpoff, 0x16, 0x8, 0x0               // 0010
         212    .byte OP_ireadfpoff, 0x16, 0x10, 0x0              // 0014
=>       213    .byte OP_intrinsiccall, 0x0, 0x32, 0x2            // 0018: JSOP_ADD
         214    .byte OP_regread, 0x16, 0xfa, 0xff                // 001c: %-6
         215    .byte OP_iassignfpoff, 0x16, 0xe8, 0xff           // 0020
         216    // LINE add.js : 4: JSOP_SETLOCAL
         217    .byte OP_ireadfpoff, 0x16, 0xe8, 0xff             // 0024
         218    .byte OP_iassignfpoff, 0x16, 0xf0, 0xff           // 0028
         219    // LINE add.js : 4: JSOP_POP
         220    // LINE add.js : 5:   return(sum)
         221    // LINE add.js : 5: JSOP_GETLOCAL
         222    // LINE add.js : 5: JSOP_RETURN
         223    .byte OP_ireadfpoff, 0x16, 0xf0, 0xff             // 002c

```
#### 7. mlist .
Lists code located by the filename and line number of the current Maple frame
Example:
```
(gdb) mbt
Maple Traceback (most recent call first):
#0 add.so:0xa64:0000: Add__59aaa481(dynany<> %par1=0x7ffff6066fc8, dynany<> %par2=0x7ffff6066fd0) at ./add.js:18
#3 add.so:0xad4:008c: __jsmain() at ./add.js:23
(gdb) mlist .
src file: ./add.js line: 18
      14 //
      15
      16 function Add(par1, par2)
      17 {
=>    18   var sum;
      19   sum = par1 + par2;
      20   return(sum)
      21 }
      22
      23 var v1 = Add(1, 2);
```
#### 8. mlist -asm:.
Lists the asm code located by the filename and line number of the current Maple frame
```
(gdb) mbt -asm
Maple Traceback (most recent call first):
#0 add.so:0xa64:0000: Add__59aaa481(dynany<> %par1=0x7ffff6066fc8, dynany<> %par2=0x7ffff6066fd0) at /home/test/gitee/maple_engine/maple_build/examples/JavaScript/add/add.s:203
#3 add.so:0xad4:008c: __jsmain() at /home/test/gitee/maple_engine/maple_build/examples/JavaScript/add/add.s:290
(gdb) mlist -asm
asm file: /home/test/gitee/maple_engine/maple_build/examples/JavaScript/add/add.s line: 223
         214    .byte OP_regread, 0x16, 0xfa, 0xff                // 001c: %-6
         215    .byte OP_iassignfpoff, 0x16, 0xe8, 0xff           // 0020
         216    // LINE add.js : 19: JSOP_SETLOCAL        0
         217    .byte OP_ireadfpoff, 0x16, 0xe8, 0xff             // 0024
         218    .byte OP_iassignfpoff, 0x16, 0xf0, 0xff           // 0028
         219    // LINE add.js : 19: JSOP_POP
         220    // LINE add.js : 20:   return(sum)
         221    // LINE add.js : 20: JSOP_GETLOCAL        0
         222    // LINE add.js : 20: JSOP_RETURN
=>       223    .byte OP_ireadfpoff, 0x16, 0xf0, 0xff             // 002c
         224    .byte OP_regassign, 0x16, 0xfa, 0xff              // 0030: %-6
         225    .byte OP_return, 0x0, 0x0, 0x0                    // 0034
         226    // LINE add.js : 20: JSOP_RETRVAL
         227    .byte OP_constval64, 0x17, 0x0, 0x0               // 0038
         228    .byte 0x0, 0x0, 0x0, 0x0, 0x8, 0x0, 0x0, 0x0
         229    .byte OP_regassign, 0x17, 0xfa, 0xff              // 0044: %-6
         230    .byte OP_return, 0x0, 0x0, 0x0                    // 0048
```
#### 9. mlist -mir
"mlist -mir" is not supported for JavaScript

#### 10. mlist -mir:+|-[num]
"mlist -mir:+|1[num]" is not support for JavaScript

#### 11. mlist -mir:.
"mlist -mir:." is not support for JavaScript

## Maple Data Commands
### mlocal command
This command displays the stack frame local variables' value and dynamic stack data.
#### 1. mlocal
This is used to display all local variables' value for a function. Use mlocal command at a Maple breakpoint and after a msi command to step into a function
Example:
```
(gdb) mbt
Maple Traceback (most recent call first):
#0 add.so:0xa64:0000: Add__59aaa481(dynany<> %par1=0x7ffff6066fc8, dynany<> %par2=0x7ffff6066fd0) at ./add.js:18
#3 add.so:0xad4:008c: __jsmain() at ./add.js:23
(gdb) msi 4
Breakpoint 4 at 0x7ffff695180b: file /home/test/gitee/maple_engine/maple_engine/src/invoke_dyn_method.cpp, line 230.
asm file: /home/test/gitee/maple_engine/maple_build/examples/JavaScript/add/add.s
=> 212 :        .byte OP_ireadfpoff, 0x16, 0x10, 0x0              // 0014
   213 :        .byte OP_intrinsiccall, 0x0, 0x32, 0x2            // 0018: JSOP_ADD
   214 :        .byte OP_regread, 0x16, 0xfa, 0xff                // 001c: %-6
(gdb) mlocal
local #0: name=%var1 type=dynany addr=0x7ffff6066fb0 value=0x00000000
local #1: name=%var2 type=dynany addr=0x7ffff6066fa8 value=0x00000000
```
#### 2. mlocal -stack or mlocal -s
This is used to display all stack frame's dynamic data.
Example:
```
(gdb) mbt
Maple Traceback (most recent call first):
#0 add.so:0xa64:0000: Add__59aaa481(dynany<> %par1=0x7ffff6066fc8, dynany<> %par2=0x7ffff6066fd0) at ./add.js:18
#3 add.so:0xad4:008c: __jsmain() at ./add.js:23
(gdb) msi 4
Breakpoint 4 at 0x7ffff695180b: file /home/test/gitee/maple_engine/maple_engine/src/invoke_dyn_method.cpp, line 230.
asm file: /home/test/gitee/maple_engine/maple_build/examples/JavaScript/add/add.s
=> 212 :        .byte OP_ireadfpoff, 0x16, 0x10, 0x0              // 0014
   213 :        .byte OP_intrinsiccall, 0x0, 0x32, 0x2            // 0018: JSOP_ADD
   214 :        .byte OP_regread, 0x16, 0xfa, 0xff                // 001c: %-6
(gdb) mlocal -s
sp=1: type=JSTYPE_NUMBER value=1
```

### mprint command
This command is used to display the JavaScript property value, or a property value at a specified memory address at runtime

#### 1. syntax: mprint <hex address>
Display a property runtime at a specified memory address
Example:
```
(gdb) mbt
Maple Traceback (most recent call first):
#0 add.so:0xa64:0000: Add__59aaa481(dynany<> %par1=0x7ffff6066fc8, dynany<> %par2=0x7ffff6066fd0) at ./add.js:18
#3 add.so:0xad4:008c: __jsmain() at ./add.js:23
(gdb) mprint 0x7ffff6066fc8
0x7ffff6066fc8: type=JSTYPE_NUMBER, value=1
```

#### 2. Syntax: mprint <JavaScript property name>
This syntax displays a JavaScript program's global property value
Example:
```
(gdb) mprint v1
property name    : v1
  tag            : JSTYPE_NUMBER
  value          : 3
  node addr      : 0x7ffff586719c
  next addr      : 0x0
  parent         : Global Object
```

#### 3. Syntax: mprint <node hex address of an internal JS Object>
This syntax displays a JavaScript program's property contents via a specified JS object property node address in memory

Example:
```
(gdb) mprint 0x7ffff585a2d4
0x7ffff585a2d4: arguments
  tag         : JSTYPE_OBJECT
  value       : prop_list = 0x7ffff585a204, prop_index_map = 0x55555576edb0, prop_string_map = 0x55555576ee70
                extensible = 1 '\001', object_class = __jsobj_class::JSARGUMENTS, object_type = 2 '\002', is_builtin = 0 '\000'
                proto_is_builtin = 1 '\001', builtin_id = __jsbuiltin_object_id::JSBUILTIN_GLOBALOBJECT
                prototype = {obj = 0x2, id = __jsbuiltin_object_id::JSBUILTIN_OBJECTPROTOTYPE}
                shared = {fast_props = 0x0, array_props = 0x0, fun = 0x0, intl = 0x0, prim_string = 0x0, prim_number = 0, prim_bool = 0, arr = 0x0, primDouble = 0}
  node addr   : 140737312563924
  next addr   : 0x0
  parent      : Global Object
```

### mtype command
This command is used to display a JavaScript property contents via a regular expression search pattern

**_syntax_**: mtype <regex-of-a-property-name-search-pattern>

#### 1. If only one matches
Example:
```
(gdb) mtype v1
#1 property name=v1, node addr=0x7ffff586719c, tag=JSTYPE_NUMBER, value=3
```
#### 2. If multiple properties match
Example:
```
(gdb) mtype [a-zA-Z]
#1 property name=Add, node addr=0x7ffff5867048, tag=JSTYPE_UNDEFINED, value=undefined
#2 property name=v1, node addr=0x7ffff586719c, tag=JSTYPE_NUMBER, value=3
```

#### 3. To get all properties
```
(gdb) mtype $
#1 property name=Add, node addr=0x7ffff5867048, tag=JSTYPE_UNDEFINED, value=undefined
#2 property name=v1, node addr=0x7ffff586719c, tag=JSTYPE_NUMBER, value=3
#3 property name=arguments, node addr=0x7ffff585a2d4, tag=JSTYPE_OBJECT, value={prop_list: 0x7ffff585a204, prop_index_map: 0x55555576edb0, prop_string_map: 0x55555576ee70, prototype: obj = 0x2, id = __jsbuiltin_object_id::JSBUILTIN_OBJECTPROTOTYPE, extensible: 1 '\001', object_class: __jsobj_class::JSARGUMENTS, object_type: 2 '\002', is_builtin: 0 '\000', proto_is_builtin: 1 '\001', builtin_id: __jsbuiltin_object_id::JSBUILTIN_GLOBALOBJECT, shared: fast_props = 0x0, array_props = 0x0, fun = 0x0, intl = 0x0, prim_string = 0x0, prim_number = 0, prim_bool = 0, arr = 0x0, primDouble = 0}
```

### msymbol command
This command is used to display JavaScript function symbols via a regular expression search pattern

**_syntax_**: msymbol <regex-of-symbol-name-search-pattern>

#### 1. If only one matches
Example:
```
(gdb) msymbol Add
#1 symbol name: Add__59aaa481
assembly file : /home/test/gitee/maple_engine/maple_build/examples/JavaScript/add/add.s
source        : ./add.js
```
#### 2. If multiple symbols match
Example:
```
(gdb) msymbol [a-zA-Z]
#1 symbol name: Add__59aaa481
#2 symbol name: __jsmain
```
#### 3. To get all symbols
```
(gdb) msymbol $
#1 symbol name: Add__59aaa481
#2 symbol name: __jsmain
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
set the number of lines mlist will display at a time for source and assembly files
```
(gdb) mset linecount 5
(gdb) mlist
src file: ./add.js line: 13
      11   print(" add: pass\n");
      12 } else {
=>    13   $ERROR("test failed v1 expect 3 but get",  v1, "\n");
      14 }
```

### mhelp command
#### 1. List all the commands and the Maple Debugger version
Example:
```
(gdb) mhelp
Maple Version 2.0

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

#### 2. Help for individual Maple commands
mhelp <command name>
***Note***: the command name must be a Maple command's full name, and not it's alias name
Example:
```
(gdb) mhelp mbreak
Maple Version 2.0
mbreak <symbol>: Sets a new Maple breakpoint at symbol
mbreak -set <symbol>: Alias for "mbreak <symbol>"
mbreak -enable <symbol|index>: Enables an existing Maple breakpoint at symbol
mbreak -disable <symbol|index>: Disables an existing Maple breakpoint at symbol
mbreak -clear <symbol|index>: Deletes an existing Maple breakpoint at symbol
mbreak -clearall : Deletes all existing Maple breakpoints
mbreak -listall : Lists all existing Maple breakpoints
mbreak -ignore <symbol | index> <count>: Sets ignore count for specified Maple breakpoints
```

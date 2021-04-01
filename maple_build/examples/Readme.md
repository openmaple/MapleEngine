```
# Copyright (C) [2020-2021] Futurewei Technologies, Inc. All rights reserved.
#
# OpenArkCompiler is licensed underthe Mulan Permissive Software License v2
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
```

## Run a Java app

cd HelloWorld

### Compile a Java program to assembly code: .java -> .s

```
  "$MAPLE_BUILD_TOOLS"/java2asm.sh HelloWorld.java
```

### Compile the assembly code to a shared library: .s -> .so
```
  "$MAPLE_BUILD_TOOLS"/asm2so.sh HelloWorld.s
```

### Run the program with Maple engine
```
  "$MAPLE_BUILD_TOOLS"/run-app.sh -classpath ./HelloWorld.so HelloWorld
```

### Debug the program with Maple debugger
```
  "$MAPLE_BUILD_TOOLS"/run-app.sh -gdb -classpath ./HelloWorld.so HelloWorld
```

## Run a JavaScript app

First of all, run "$MAPLE_BUILD_TOOLS"/build-maple-js.sh to build Maple JS compiler and engine.

```
cd JavaScript/add
```
### Command to run a JavaScript app
```
"$MAPLE_BUILD_TOOLS"/run-js-app.sh add.js
```

### Debug the program with Maple debugger
```
"$MAPLE_BUILD_TOOLS"/run-js-app.sh -gdb add.js
```

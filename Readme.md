[//]: # (Copyright (C) [2020] Futurewei Technologies, Inc. All rights reverved.)
[//]: # ( )
[//]: # (Licensed under the Mulan Permissive Software License v2)
[//]: # (You can use this software according to the terms and conditions of the MulanPSL - 2.0.)
[//]: # (You may obtain a copy of MulanPSL - 2.0 at:)
[//]: # ( )
[//]: # (  https://opensource.org/licenses/MulanPSL-2.0)
[//]: # ( )
[//]: # (THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER)
[//]: # (EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR)
[//]: # (FIT FOR A PARTICULAR PURPOSE.)
[//]: # (See the MulanPSL - 2.0 for more details.)
[//]: # ( )

## Build envionment setup
  Needs a Linux server with at least 24GB RAM and Ubuntu 16.04/18.04 installed.

  Execute the following command to install the packages used by the Maple build.

    sudo apt install -y build-essential clang cmake libffi-dev libelf-dev libunwind-dev \
            libssl-dev openjdk-8-jdk-headless unzip python-minimal python3

## Setup Maple environment
  cd maple_engine
  source ./envsetup.sh

## Build Maple compiler and engine
  ./maple_build/tools/build-maple.sh

## Build Java core library
   Following the doc at ./maple_build/doc/build_OpenJDK8.md to build the customized OpenJDK8.
   Make sure the needed .jar files are copied into ./maple_build/jar/ directory.

   ./maple_build/tools/build-libcore.sh

## Build and run app
  cd ./maple_build/examples/HelloWorld
  $MAPLE_BUILD_TOOLS/java2asm.sh HelloWorld.java
  $MAPLE_BUILD_TOOLS/asm2so.sh HelloWorld.s

  $MAPLE_BUILD_TOOLS/run-app.sh -classpath ./HelloWorld.so HelloWorld

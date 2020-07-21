```
#
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
#
```
# Maple 编译器和引擎 

![输入图片说明](https://images.gitee.com/uploads/images/2020/0721/050904_9b03f7b1_5583371.png "Maple-engine.PNG")

## Linux服务器环境设置
  需要一台Linux 服务器，建议24GB 或更多内存，安装Ubuntu 16.04或18.04系统.

  登录服务器后，执行如下命令安装所需的软件包：

```
    sudo apt install -y build-essential clang cmake libffi-dev libelf-dev libunwind-dev \
            libssl-dev openjdk-8-jdk-headless unzip python-minimal python3
```

## Maple 构建环境设置

  下载代码并设置Maple 构建环境：
```
  git clone https://gitee.com/openarkcompiler-incubator/maple_engine.git
  cd maple_engine
  source ./envsetup.sh
```
  若要执行后续命令，需先执行`source ./envsetup.sh`.

## 构建Maple 编译器和Maple引擎
```
  ./maple_build/tools/build-maple.sh
```

## 构建Java 核心库
   按照文档./maple_build/doc/build_OpenJDK8.md 中介绍的方法构建定制的OpenJDK-8.
   将文档中提到的必需的.jar 文件拷贝到./maple_build/jar/ 目录中，再执行如下命令：

```
   ./maple_build/tools/build-libcore.sh
```

## 编译和执行应用程序

以HelloWorld为例，编译和执行一个应用程序：
```
  cd ./maple_build/examples/HelloWorld
  $MAPLE_BUILD_TOOLS/java2asm.sh HelloWorld.java
  $MAPLE_BUILD_TOOLS/asm2so.sh HelloWorld.s

  $MAPLE_BUILD_TOOLS/run-app.sh -classpath ./HelloWorld.so HelloWorld
```

---

## Build envionment setup
  Needs a Linux server with at least 24GB RAM and Ubuntu 16.04/18.04 installed.

  Execute the following command to install the packages used by the Maple build.

    sudo apt install -y build-essential clang cmake libffi-dev libelf-dev libunwind-dev \
            libssl-dev openjdk-8-jdk-headless unzip python-minimal python3

## Setup Maple environment
```
  cd maple_engine
  source ./envsetup.sh
```

## Build Maple compiler and engine
```
  ./maple_build/tools/build-maple.sh
```
## Build Java core library
   Following the doc at ./maple_build/doc/build_OpenJDK8.md to build the customized OpenJDK8.
   Make sure the needed .jar files are copied into ./maple_build/jar/ directory.

```
   ./maple_build/tools/build-libcore.sh
```
## Build and run app
```
  cd ./maple_build/examples/HelloWorld
  $MAPLE_BUILD_TOOLS/java2asm.sh HelloWorld.java
  $MAPLE_BUILD_TOOLS/asm2so.sh HelloWorld.s

  $MAPLE_BUILD_TOOLS/run-app.sh -classpath ./HelloWorld.so HelloWorld
```

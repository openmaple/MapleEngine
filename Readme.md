##### [English Version Readme](#table-of-contents)
##### 目录
- [Maple Java 编译器和引擎](#maple-编译器和引擎)
  * [Linux服务器环境设置](#linux服务器环境设置)
  * [Maple 构建环境设置](#maple-构建环境设置)
  * [构建Maple 编译器和Maple引擎](#构建maple-编译器和maple引擎)
  * [构建Java 核心库](#构建java-核心库)
  * [编译和执行应用程序](#编译和执行应用程序)
  * [调试应用程序](#调试应用程序)      
  <br/>
- [Maple JavaScript 编译器和引擎](#maple-javascript编译器和引擎) 
  * [Linux服务器环境设置](#linux服务器环境设置) 
  * [MapleJS 构建环境设置](#maplejs构建环境设置)
  * [构建MapleJS 编译器和Maple引擎](#构建maplejs-编译器和maple引擎)
  * [编译和执行应用程序](#编译和执行js应用程序)


```
#
# Copyright (C) [2020-2021] Futurewei Technologies, Inc. All rights reserved.
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
#
```

# Maple 编译器和引擎

![输入图片说明](https://images.gitee.com/uploads/images/2020/0721/050904_9b03f7b1_5583371.png "Maple-engine.PNG")

## Linux服务器环境设置
  需要一台Linux x86_64服务器，至少16GB或以上内存，安装Ubuntu 16.04或18.04系统.
  若您的服务器只有16GB内存，需要创建8GB或以上的swap空间。

  编译和链接 OpenJDK 核心库时，会占用很多内存。对于少于16GB内存的服务器，可以考虑
在其它内存较多的服务器上先构建好libcore.so, 将maple_engine/maple_build/out/这个
目录打包传到您的本地机器，展开后放到本地机器 同名目录下，再拷贝其中的文件 libcore.so
到maple_engine/maple_runtime/lib/x86_64/libcore.so，就可以本地编译和运行应用程序了。

  登录服务器后，执行如下命令安装所需的软件包：

```
    sudo apt install -y build-essential clang cmake libffi-dev libelf-dev libunwind-dev \
            libssl-dev openjdk-8-jdk-headless unzip python-minimal python3 gdb bc highlight
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
  构建Java 核心库完成后，会将编译应用代码所需要的文件拷贝到maple_runtime/lib/x86_64/目录下。

## 编译和执行应用程序

以HelloWorld为例，编译和执行一个应用程序：
```
  cd ./maple_build/examples/HelloWorld
  "$MAPLE_BUILD_TOOLS"/java2asm.sh HelloWorld.java
  "$MAPLE_BUILD_TOOLS"/asm2so.sh HelloWorld.s

  "$MAPLE_BUILD_TOOLS"/run-app.sh -classpath ./HelloWorld.so HelloWorld
```

## 调试应用程序
```
  "$MAPLE_BUILD_TOOLS"/run-app.sh -gdb -classpath ./HelloWorld.so HelloWorld
```

---

## Build environment setup
  This requires an x86_64 server with more than 16GB of RAM and Ubuntu 16.04/18.04 installed.
  If your server has 16GB RAM, please setup 8GB or more swap space.

  Execute the following command to install the packages used by the Maple build.

```
    sudo apt install -y build-essential clang cmake libffi-dev libelf-dev libunwind-dev \
            libssl-dev openjdk-8-jdk-headless unzip python-minimal python3 gdb bc highlight
```

## Setup the Maple environment
```
  cd maple_engine
  source ./envsetup.sh
```

## Build the Maple compiler and engine
```
  ./maple_build/tools/build-maple.sh
```
## Build the Java core library
   Following the doc at ./maple_build/doc/build_OpenJDK8.md to build the customized OpenJDK8.
   Make sure the needed .jar files are copied into ./maple_build/jar/ directory.

```
   ./maple_build/tools/build-libcore.sh
```
## Build and run an app
```
  cd ./maple_build/examples/HelloWorld
  "$MAPLE_BUILD_TOOLS"/java2asm.sh HelloWorld.java
  "$MAPLE_BUILD_TOOLS"/asm2so.sh HelloWorld.s

  "$MAPLE_BUILD_TOOLS"/run-app.sh -classpath ./HelloWorld.so HelloWorld
```

## Debug an app with Maple debugger
```
  "$MAPLE_BUILD_TOOLS"/run-app.sh -gdb -classpath ./HelloWorld.so HelloWorld
```
                            

# Maple JavaScript编译器和引擎 

## Linux服务器环境设置
  需要一台Linux x86_64服务器，至少6GB或以上内存，安装Ubuntu 16.04或18.04系统.
  
  登录服务器后，执行如下命令安装所需的软件包：
```
sudo apt install -y build-essential clang cmake libffi-dev libunwind-dev openjdk-8-jdk-headless \
       libssl-dev:i386 libelf-dev:i386 g++-5 gcc-5 g++-multilib gcc-multilib libc6-dev-i386 \
       unzip python-minimal python3 gdb bc highlight libicu-dev
```

## MapleJS构建环境设置

  下载代码并设置MapleJS 构建环境：

```
  git clone https://gitee.com/openarkcompiler-incubator/maple_engine.git
```

  若要执行后续命令，需先执行`source  maple_engine/envsetup.sh`.

## 构建MapleJS 编译器和Maple引擎

  以下脚本下载与构建JavaScript引擎，构建与安装maple编译器，构建MapleJS引擎:

```
maple_engine/maple_build/tools/build-maple-js.sh
```

## 编译和执行JS应用程序

以add.js为例，编译和执行一个应用程序：

```
cd maple_engine/maple_build/examples/JavaScript/add
$MAPLE_ROOT/maple_build/tools/run-js-app.sh add
```

你将看到：

```
add: pass
```

## Linux server setup
This requires an x86_64 server with more than 6GB of RAM and Ubuntu 16.04/18.04 installed.
  
Execute the following command to install the packages used by the MapleJS build.

```
sudo apt install -y build-essential clang cmake libffi-dev libunwind-dev openjdk-8-jdk-headless \
       libssl-dev:i386 libelf-dev:i386 g++-5 gcc-5 g++-multilib gcc-multilib libc6-dev-i386 \
       unzip python-minimal python3 gdb bc highlight libicu-dev
```    

## Setup the MapleJS environment

Download MapleJS code:

```
  git clone https://gitee.com/openarkcompiler-incubator/maple_engine.git
```
Set up build enviroment: `source  maple_engine/envsetup.sh`.

## Build the MapleJS compiler and engine

Run following script to:
  Download and build jscre
  Build Maple compiler release (mplbe, mplcg, js2mpl) for Maple Engine
  Build Maple Engine for JS

```
  ./maple_build/tools/build-maple-js.sh
```

## Build and run a JS App
   Build and run examples in JavaScript:
```
cd maple_engine/maple_build/examples/JavaScript/add
$MAPLE_ROOT/maple_build/tools/run-js-app.sh add
```
##### Table of Contents
- [Maple Compiler and Maple Engine](#maple-编译器和引擎)
  * [Setup the Maple environment](#setup-the-maple-environment)
  * [Build the Maple compiler and engine](#build-the-maple-compiler-and-engine)
  * [Build the Java core library](#build-the-java-core-library)
  * [Build and run an app](#build-and-run-an-app)
  * [Debug an app with Maple debugger](#debug-an-app-with-maple-debugger)
  <br/>
- [Maple JavaScript Compiler and Maple Engine](#linux-server-setup)
  * [Linux server setup](#linux-server-setup)
  * [Setup the MapleJS environment](#setup-the-maplejs-environment)
  * [Build the MapleJS compiler and engine](#build-the-maplejs-compiler-and-engine)
  * [Build and run a JS App](#build-and-run-a-js-app)

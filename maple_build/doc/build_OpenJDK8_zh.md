```
# Copyright (C) [2020] Futurewei Technologies, Inc. All rights reverved.
#
# Licensed under the Mulan Permissive Software License v2
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

# Download OpenJDK-8, customize and build required OpenJDK-8 components
# 下载 OpenJDK-8 , 定制和构建需要的 OpenJDK-8 组件

The following OpenJDK-8 components are needed for building libcore.so and running user's Java program.

需要以下的 OpenJDK-8 组件来构建 libcore.so 以及运行用户的 Java 程序.

Java Core libraries:

Java 核心库:
```
    rt.jar (customized)
    charsets.jar
    jce.jar
    jsse.jar
```
Please note that one of the components rt.jar is a customized version for Maple Engine. In order
to generate the customized version of rt.jar file, modification to Object.java file of OpenJDK-8
and building OpenJDK-8 from source are required.

请注意其中一个组件 rt.jar 是方舟引擎定制版. 要生成方舟引擎定制版的 rt.jar 文件, 首先需要修改 OpenJDK-8 
的 Object.java ，然后从源代码构建  OpenJDK-8 

The followings are instructions of how to build OpenJDK-8 components on OpenJDK-8 build machine,
how to modify Object.java file or customize rt.jar, and where to copy required components built
to the designated libcore.so build directories.

以下是构建 OpenJDK-8 组件的指南，包括如何修改 Object.java 或者 定制 rt.jar ，以及如何将已经构建好的组
件复制到指定的 libcore.so 构建目录
## 1. Download OpenJDK- 8 Source
## 1. 下载 OpenJDK- 8源代码

Create OpenJDK-8 build environment and download OpenJDK-8 source 

In order to build OpenJDK-8 from source, OpenJDK 7 JRE or JDK is required. To install OpenJDK 7 
on Ubuntu 16.04 Linux, use the following command:

搭建 OpenJDK-8 构建环境以及下载 OpenJDK-8 的源代码
要从源代码构建OpenJDK-8，首先需要OpenJDK 7 JRE 或者 JDK。要在Ubuntu 16.04上 安装OpenJDK 7，需要使用以下命令：
```
$ sudo add-apt-repository ppa:openjdk-r/ppa
More info: https://launchpad.net/~openjdk-r/+archive/ubuntu/ppa
Press [ENTER] to continue or ctrl-c to cancel adding it
                Press ENTER.
gpg: keyring '/tmp/tmpccfu5qmx/secring.gpg' created
gpg: keyring '/tmp/tmpccfu5qmx/pubring.gpg' created
gpg: requesting key 86F44E2A from hkp server keyserver.ubuntu.com
gpg: /tmp/tmpccfu5qmx/trustdb.gpg: trustdb created
gpg: key 86F44E2A: public key "Launchpad OpenJDK builds (all archs)" imported
gpg: Total number processed: 1
gpg: imported: 1 (RSA: 1)
OK
```
```
$ sudo apt-get update
$ sudo apt-get install openjdk-7-jdk
```
Note: Install all dependent software development packages required if they have not been already
installed. You may install these packages with following commend:
```
$ sudo apt install mercurial build-essential cpio zip libx11-dev libxext-dev libxrender-dev \
              libxtst-dev libxt-dev libcups2-dev libfreetype6-dev libasound2-dev
```
注意：安装依赖（例如hg）

Download OpenJDK-8 source:

下载OpenJDK-8源代码：
```
$ hg clone http://hg.openjdk.java.net/jdk8/jdk8 ~/my_opejdk8
$ cd ~/my_opejdk8
$ bash ./get_source.sh
```
## 2. Customize Object Class

## 2. 定制 Object 类
Add two extra fields in Object class by modifying Object.java file:
修改Object.java文件，在Object类添加两个额外字段:

Add reserved_1 and reserved_2 fields right after the line `public class Object {` in
 ~/my_opejdk8/jdk/src/share/classes/java/lang/Object.java file as the first two fields of Object class:
编辑 ~/my_opejdk8/jdk/src/share/classes/java/lang/Object.java 文件，在`public class Object {`行之后插入字段声明：
```
public class Object {
    long reserved_1; int reserved_2; // Add two extra fields here
    private static native void registerNatives();
```

## 3. Build OpenJDK- 8
## 3. 构建 OpenJDK-8 

Build OpenJDK-8 using the following commands:

使用以下命令构建OpenJDK-8 
```
$ cd ~/my_opejdk8
$ bash ./configure
$ export DISABLE_HOTSPOT_OS_VERSION_CHECK=ok; make all
```
Note 1: you may need to follow the hints of the error message of configure command to install any missing
dependent packages required for building OpenJDK-8.
注意：您可能需要遵循上述输出的提示以安装 构建 OpenJDK-8 所需的缺失的依赖包。
Note 2: you may get error during configure with the following messages even libfreetype6-dev has been installed:
```
configure: error: Could not find freetype! You might be able to fix this by running 'sudo apt-get install libfreetype6-dev'.
configure exiting with result code 1
```
If that happens, please use the following command instead of "bash ./configure":
```
$ bash ./configure --with-freetype-include=/usr/include/freetype2 --with-freetype-lib=/usr/lib/x86_64-linux-gnu
```
Note 3: If you see the following error messages during make:
```
Creating sa.make ...
/usr/bin/make: invalid option -- '8'
/usr/bin/make: invalid option -- '/'
/usr/bin/make: invalid option -- 'a'
/usr/bin/make: invalid option -- '/'
/usr/bin/make: invalid option -- 'c'
```
Then you need to modify ~/my_opejdk8/hotspot/make/linux/makefiles/adjust-mflags.sh file with the following patch:
```
diff -r 87ee5ee27509 make/linux/makefiles/adjust-mflags.sh
--- a/make/linux/makefiles/adjust-mflags.sh Tue Mar 04 11:51:03 2014 -0800
+++ b/make/linux/makefiles/adjust-mflags.sh Wed Sep 30 16:51:55 2015 -0700
@@ -64,7 +64,6 @@
    echo "$MFLAGS" \
    | sed '
        s/^-/ -/
-       s/ -\([^    ][^     ]*\)j/ -\1 -j/
        s/ -j[0-9][0-9]*/ -j/
        s/ -j\([^   ]\)/ -j -\1/
        s/ -j/ -j'${HOTSPOT_BUILD_JOBS:-${default_build_jobs}}'/
```
Note 4: You may having the following problem when doing make:
```
make[1]: *** [~/my_opejdk8/build/linux-x86_64-normal-server-release/nashorn/classes/_the.nasgen.run] Error 1
BuildNashorn.gmk:75: recipe for target '~/my_opejdk8/build/linux-x86_64-normal-server-release/nashorn/classes/_the.nasgen.run' failed
```
Then you need modify make/BuildNashorn.gmk file by applying the following patch:
```
diff -r 096dc407d310 make/BuildNashorn.gmk
--- a/make/BuildNashorn.gmk     Tue Mar 04 11:52:23 2014 -0800
+++ b/make/BuildNashorn.gmk     Mon Jul 20 22:33:16 2020 -0700
@@ -77,7 +77,7 @@
        $(RM) -rf $(@D)/jdk $(@D)/netscape
        $(CP) -R -p $(NASHORN_OUTPUTDIR)/nashorn_classes/* $(@D)/
        $(FIXPATH) $(JAVA) \
-           -cp "$(NASHORN_OUTPUTDIR)/nasgen_classes$(PATH_SEP)$(NASHORN_OUTPUTDIR)/nashorn_classes" \
+           -Xbootclasspath/p:"$(NASHORN_OUTPUTDIR)/nasgen_classes$(PATH_SEP)$(NASHORN_OUTPUTDIR)/nashorn_classes" \
            jdk.nashorn.internal.tools.nasgen.Main $(@D) jdk.nashorn.internal.objects $(@D)
        $(TOUCH) $@
```

## 4. Copy required OpenJDK components to Maple build directory

## 4. 构建libcore.so并运行HelloWorld

Copy following built .jar files from OpenJDK-8 build to directory maple_engine/maple_build/jar/:
```
   ./linux-x86_64-normal-server-release/images/lib/rt.jar
   ./linux-x86_64-normal-server-release/images/lib/jce.jar
   ./linux-x86_64-normal-server-release/images/lib/jsse.jar
   ./linux-x86_64-normal-server-release/images/lib/charsets.jar
```

## 5. Build libcore.so and run HelloWorld

Build Maple compiler&engine and libcore.so:

构建方舟编译器、方舟引擎以及 libcore.so
```
$ cd maple_engine
$ source ./envsetup.sh
$ ./maple_build/tools/build-maple.sh
$ ./maple_build/tools/build-libcore.sh
```

Build and run HelloWorld.java:

构建并运行HelloWorld.java:
```
$ cd maple_build/examples/HelloWorld
$ $MAPLE_BUILD_TOOLS/java2asm.sh HelloWorld.java
$ $MAPLE_BUILD_TOOLS/asm2so.sh HelloWorld.s
$ $MAPLE_BUILD_TOOLS/run-app.sh -classpath ./HelloWorld.so HelloWorld

```


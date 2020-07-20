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

# Download OpenJDK-8, customize and build required OpenJDK-8 components

The following OpenJDK-8 components are needed for building libcore.so and running user's Java program.

Java Core libraries:
```
    rt.jar (customized)
    charsets.jar
    jce.jar
    jsse.jar
```
Shared libraries(Can use these three shared libraries from the openjdk-8-jdk-headless package also):
```
    libjava.so
    libjvm.so
    libverify.so
```
Please note that one of the components rt.jar is a customized version for Maple Engine. In order
to generate the customized version of rt.jar file, modification to Object.java file of OpenJDK-8
and building OpenJDK-8 from source are required.

The followings are instructions of how to build OpenJDK-8 components on OpenJDK-8 build machine,
how to modify Object.java file or customize rt.jar, and where to copy required components built
to the destinated libcore.so build directorie.

## 1. Download OpenJDK- 8 Source

Create OpenJDK-8 build environment and download OpenJDK-8 source 

In order to build OpenJDK-8 from source, OpenJDK 7 JRE or JDK is required. To install OpenJDK 7 
on Ubuntu 16.04 Linux, use the following command:

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
installed (such as hg).

Download OpenJDK-8 source:
```
$ hg clone http://hg.openjdk.java.net/jdk8/jdk8 ~/my_opejdk8
$ cd ~/my_opejdk8
$ bash ./get_source.sh
```
## 2. Customize Object Class

Add two extra fields in Object class by modifying Object.java file:

Add the following fields in Object class of Object.java file as the first two fields of the class
by editing the  ~/my_opejdk8/jdk/src/share/classes/java/lang/Object.java file by inserting the 
following 2 lines right after the line `public class Object {`:
```
public class Object {
    long reserved_1; int reserved_2; // Add two extra fields here
    private static native void registerNatives();
```

## 3. Build OpenJDK- 8

Build OpenJDK-8 using the following commands:
```
$ cd ~/my_opejdk8
$ bash ./configure
```

Note: you may need to following the hints of the outputs of above commend to install missing
dependent packages required for building OpenJDK-8.

```
$ export DISABLE_HOTSPOT_OS_VERSION_CHECK=ok; make all
```

## 4. Copy required OpenJDK components to Maple build directory

Copy following built files from OpenJDK-8 build (under ~/my_opejdk8/build directory) to 
directory maple_engine/maple_build/jar/:
```
   rt.jar
   jce.jar
   jsse.jar
   charsets.jar
```

Copy following built files from OpenJDK-8 build to directory maple/engine/maple_runtime/lib/x86_64/:
```
   libjava.so
   libjvm.so
   libverify.so
```

## 4. Build libcore.so and run HelloWorld

Build Maple compiler&engine and libcore.so:
```
$ cd maple_engine
$ source ./envsetup.sh
$ ./maple_build/tools/build-maple.sh
$ ./maple_build/tools/build-libcore.sh
```

Build and run HelloWorld.java:
```
$ cd maple_build/examples/HelloWorld
$ $MAPLE_BUILD_TOOLS/java2asm.sh HelloWorld.java
$ $MAPLE_BUILD_TOOLS/asm2so.sh HelloWorld.s
$ $MAPLE_BUILD_TOOLS/run-app.sh -classpath ./HelloWorld.so HelloWorld

```

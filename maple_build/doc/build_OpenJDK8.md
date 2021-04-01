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

# Download OpenJDK-8, customize and build required OpenJDK-8 components

The following OpenJDK-8 components are needed for building libcore.so and running user's Java program.

Java Core libraries:
```
    rt.jar (customized)
    charsets.jar
    jce.jar
    jsse.jar
```
Please note that one of the components rt.jar is a customized version for Maple Engine. In order
to generate the customized version of rt.jar file, modification to Object.java file of OpenJDK-8
and building OpenJDK-8 from source are required.

The followings are instructions of how to build OpenJDK-8 components on OpenJDK-8 build machine,
how to modify Object.java file or customize rt.jar, and where to copy required components built
to the designated libcore.so build directories.

## 1. Download OpenJDK- 8 Source

Create OpenJDK-8 build environment and download OpenJDK-8 source 

In order to build OpenJDK-8 from source, OpenJDK-7-JRE or JDK is required, so using a separated
machine other than the one which will run Maple Engine for building OpenJDK-8 is recommended.
To install OpenJDK-7 on Ubuntu 16.04 Linux, use the following command:

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
$ sudo apt update
$ sudo apt install openjdk-7-jdk
```
Note: Install all dependent software development packages required if they have not been already
installed. You may install these packages with following commend:
```
$ sudo apt install mercurial build-essential cpio zip libx11-dev libxext-dev libxrender-dev \
              libxtst-dev libxt-dev libcups2-dev libfreetype6-dev libasound2-dev libfontconfig1-dev
```
Determine the OpenJDK-8-JRE revision installed on the machine which will install and run Maple Engine. From the outputs of the following command, `8u272-b10` is the revision to be used to download the OpenJDK-8 source:
```
$ apt list openjdk-8-jre
Listing... Done
openjdk-8-jre/xenial-updates,xenial-security,now 8u272-b10-0ubuntu2~16.04 amd64 [installed]
```

Download OpenJDK-8 source which matches the OpenJDK-8-JRE revision, 8u272-b10 for example, using `jdk8u272-b10` tag:
```
$ hg clone http://hg.openjdk.java.net/jdk8u/jdk8u -r jdk8u272-b10 ~/my_openjdk8
$ cd ~/my_openjdk8
$ bash ./get_source.sh
```
## 2. Customize Object Class

Add two extra fields in Object class by modifying Object.java file:

Add reserved_1 and reserved_2 fields right after the line `public class Object {` in
 ~/my_openjdk8/jdk/src/share/classes/java/lang/Object.java file as the first two fields of Object class:
```
public class Object {
    long reserved_1; int reserved_2; // Add two extra fields here
    private static native void registerNatives();
```

## 3. Build OpenJDK- 8

You may skip this step if you prefer to update a copy of rt.jar from the installed package openjdk-8-jre.

Build OpenJDK-8 using the following commands:
```
$ cd ~/my_openjdk8
$ bash ./configure
$ export DISABLE_HOTSPOT_OS_VERSION_CHECK=ok; make all
```
Note 1: you may need to follow the hints of the error message of configure command to install any missing
dependent packages required for building OpenJDK-8.
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
Then you need to modify ~/my_openjdk8/hotspot/make/linux/makefiles/adjust-mflags.sh file with the following patch:
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
make[1]: *** [~/my_openjdk8/build/linux-x86_64-normal-server-release/nashorn/classes/_the.nasgen.run] Error 1
BuildNashorn.gmk:75: recipe for target '~/my_openjdk8/build/linux-x86_64-normal-server-release/nashorn/classes/_the.nasgen.run' failed
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

Copy following built .jar files from OpenJDK-8 build to directory maple_engine/maple_build/jar/:
```
   ./linux-x86_64-normal-server-release/images/lib/rt.jar
   ./linux-x86_64-normal-server-release/images/lib/jce.jar
   ./linux-x86_64-normal-server-release/images/lib/jsse.jar
   ./linux-x86_64-normal-server-release/images/lib/charsets.jar
```

Alternatively you may copy these .jar files from the installed package openjdk-8-jre and update
rt.jar with the customized Object.class.

```
  source maple_engine/envsetup.sh
  cd maple_engine/maple_build/jar
  for j in rt jce jsse charsets; do
    cp "${JAVA_HOME}"/jre/lib/$j.jar .
  done
  mkdir -p java/lang/
  cp ~/my_openjdk8/jdk/src/share/classes/java/lang/Object.java java/lang/
  javac -target 1.8 -g java/lang/Object.java
  jar uf rt.jar java/lang/Object.class
```

## 5. Build libcore.so and run HelloWorld

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
$ "$MAPLE_BUILD_TOOLS/java2asm.sh" HelloWorld.java
$ "$MAPLE_BUILD_TOOLS/asm2so.sh" HelloWorld.s
$ "$MAPLE_BUILD_TOOLS/run-app.sh" -classpath ./HelloWorld.so HelloWorld

```


#!/bin/bash
#
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
#
PROCNUM=$(grep -c ^processor /proc/cpuinfo)

for v in ROOT COMPILER_ROOT ENGINE_ROOT RUNTIME_ROOT BUILD_ROOT TARGET_ARCH; do
    eval x=\$MAPLE_$v
    [ -n "$x" ] || { echo MAPLE_"$v" not set. Please source envsetup.sh.; exit 1; }
done

MAPLE_COMPILER_PARENT=$(dirname "$MAPLE_COMPILER_ROOT")
mkdir -p "$MAPLE_COMPILER_PARENT"
cd "$MAPLE_COMPILER_PARENT" || { echo Failed to enter dir "$MAPLE_COMPILER_PARENT"/..; exit 1; }
if [ ! -d "mapleall" ]; then
    git clone -b master https://gitee.com/openarkcompiler-incubator/mapleall.git || exit 1
else
    cd mapleall || exit 1
    git pull || exit 1
fi
[ -d "$MAPLE_ROOT"/../mapleall ] || ln -sf "$MAPLE_COMPILER_ROOT" "$MAPLE_ROOT"/../mapleall

cd "$MAPLE_COMPILER_ROOT" || exit 1
cd tools || exit 1
./setup_tools.sh
[ -f dwarf/include/dwarf2.h -a -f ninja/ninja -a -f gn/gn -a open64_prebuilt/README.md ] || ./setup_tools.sh
[ -f dwarf/include/dwarf2.h -a -f ninja/ninja -a -f gn/gn -a open64_prebuilt/README.md ] || { echo Failed to setup tools.; exit 2; }


# Download and build Mozilla
cd "$MAPLE_COMPILER_ROOT" || exit 2
if [ ! -d "mozjs" ]; then
    mkdir -p mozjs
    cd mozjs
    wget https://src.fedoraproject.org/repo/pkgs/mozjs31/mozjs-31.2.0.rc0.tar.bz2/d1ad39d0451b7231056a07bf1c57acee/mozjs-31.2.0.rc0.tar.bz2 || exit 2
    tar xvfj mozjs-31.2.0.rc0.tar.bz2
    patch -p0 < "$MAPLE_ROOT"/third-party/mozjs-31.2.0-maple.patch || exit 2
    patch -p0 < "$MAPLE_ROOT"/third-party/issue491.patch || exit 2
    cp "$MAPLE_ROOT"/third-party/configure.mozjs mozjs-31.2.0/js/src/configure
    cp "$MAPLE_ROOT"/third-party/Makefile.mozjs ./Makefile
    make config || exit 2
    make -j$PROCNUM || exit 2
#Ubuntu 18.04  Changes do not get rebuilt, configure file bug shows up
#else
#    cd mozjs
#    cp "$MAPLE_ROOT"/third-party/configure.mozjs mozjs-31.2.0/js/src/configure
#    cp "$MAPLE_ROOT"/third-party/Makefile.mozjs ./Makefile
#    make config || exit 2
#    make -j$PROCNUM || exit 2
fi
echo Built mozilla

cd "$MAPLE_COMPILER_ROOT" || exit 3
if [ ! -d "js2mpl" ]; then
    git clone ssh://10.212.145.47:/home/git/maple/js2mpl.git || exit 3
    cd js2mpl || exit 3
    sed -i "s/zeiss\///" BUILD.gn || exit 3
else
    cd js2mpl
    git pull
fi

# Download and build jscre
cd "$MAPLE_COMPILER_ROOT" || exit 2
if [ ! -d "jscre" ]; then
    git clone https://github.com/plenluno/jscre.git || exit 3
    cp "$MAPLE_ROOT"/third-party/CMakeLists.txt.jscre ./jscre/CMakeLists.txt
    mkdir -p jscre/build
    cd jscre/build
    cmake ..
    make -j$PROCNUM || exit 2
else
    cd jscre
    git pull
fi
echo Built jscre

# Build Maple compiler release (mplbe, mplcg, js2mpl) for Maple Engine
cd "$MAPLE_COMPILER_ROOT" || exit 4
unset IS_JS2MPL_EXISTS
source envsetup.sh ark release
make || exit 4
make mplbe_js || exit 4
export IS_JS2MPL_EXISTS=1
make -j$PROCNUM js2mpl || exit 4
unset IS_JS2MPL_EXISTS
echo Done building js2mpl
make install || exit 4
echo Installed Maple compiler into "$MAPLE_COMPILER_ROOT/bin/ark-clang-release/".

# Build Maple Engine for JS
cd "$MAPLE_ENGINE_ROOT" || exit 5
git pull || exit 5
./buildit.sh mplre-dyn || exit 5

if [ -f "$MAPLE_ENGINE_ROOT"/build/src/libmplre-dyn.so ]; then
    cp "$MAPLE_ENGINE_ROOT"/build/src/libmplre-dyn.so "$MAPLE_RUNTIME_ROOT/lib/$MAPLE_TARGET_ARCH"
else
    echo Maple Engine shared library libmplre-dyn.so has not been generated.
fi

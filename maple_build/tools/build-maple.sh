#!/bin/bash
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

for v in ROOT COMPILER_ROOT ENGINE_ROOT RUNTIME_ROOT BUILD_ROOT TARGET_ARCH; do
    eval x=\$MAPLE_$v
    [ -n "$x" ] || { echo MAPLE_"$v" not set. Please source envsetup.sh.; exit 1; }
done

MAPLE_COMPILER_PARENT=$(dirname "$MAPLE_COMPILER_ROOT")
mkdir -p "$MAPLE_COMPILER_PARENT"
cd "$MAPLE_COMPILER_PARENT" || { echo Failed to enter dir "$MAPLE_COMPILER_PARENT"/..; exit 1; }
if [ ! -d "mapleall" ]; then
    git clone https://gitee.com/openarkcompiler-incubator/mapleall.git || exit 1
else
    cd mapleall || exit 1
    git pull || exit 1
fi
[ -d "$MAPLE_ROOT"/../mapleall ] || ln -sf "$MAPLE_COMPILER_ROOT" "$MAPLE_ROOT"/../mapleall

cd "$MAPLE_COMPILER_ROOT" || exit 2
cd tools || exit 2
./setup_tools.sh

cd "$MAPLE_COMPILER_ROOT" || exit 2
source envsetup.sh ark release
make || exit 2
make install || exit 2
echo Installed Maple compiler into "$MAPLE_COMPILER_ROOT/bin/ark-clang-release/".

cd "$MAPLE_ENGINE_ROOT" || exit 1
git pull
./buildit.sh

if [ -f "$MAPLE_ENGINE_ROOT"/build/src/libmplre.so ]; then
    cp "$MAPLE_ENGINE_ROOT"/build/src/libmplre.so "$MAPLE_RUNTIME_ROOT/lib/$MAPLE_TARGET_ARCH"
else
    echo Maple Engine shared library libmplre.so has not been generated.
fi

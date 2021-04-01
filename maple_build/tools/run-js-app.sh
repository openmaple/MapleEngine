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

for v in COMPILER_ROOT RUNTIME_ROOT; do
    eval x=\$MAPLE_$v
    [ -n "$x" ] || { echo MAPLE_$v not set. Please source envsetup.sh.; exit 1; }
done

BIN=$MAPLE_COMPILER_ROOT/bin/ark-clang-release
JS2MPL=$BIN/js2mpl
MPLBE=$BIN/mplbe_js
MPLCG=$BIN/mplcg
MPLSH=$MAPLE_RUNTIME_ROOT/bin/x86_64/mplsh

export MAPLE_ENGINE_DEBUG=none
DBCMD=
if [ "x$1" = "x-gdb" ]; then
    DBCMD='gdb -x '"$MAPLE_DEBUGGER_ROOT/.mdbinit"' --args '
    shift
fi

script="$(basename -- $0)"
if [ $# -lt 1 ]; then
    echo "Usage: $script <javascript source file>"
    exit 1
fi

file="$(basename -- $1)"
file="${file%.*}"
$JS2MPL $file.js
$MPLBE $file.mpl
$MPLCG -O2 --quiet --no-pie --verbose-asm --fpic $file.mmpl
/usr/bin/x86_64-linux-gnu-g++-5 -g3 -pie -O2 -x assembler-with-cpp -c $file.s -o $file.o
/usr/bin/x86_64-linux-gnu-g++-5 -g3 -pie -O2 -fPIC -shared -o $file.so $file.o -rdynamic
export LD_LIBRARY_PATH=$MAPLE_RUNTIME_ROOT/lib/x86_64
$DBCMD $MPLSH -cp $file.so

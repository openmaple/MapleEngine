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
    [ -n "$x" ] || { echo MAPLE_$v not set. Please source envsetup.sh.; exit 1; }
done
[ -n "${JAVA_CORE_LIB}" ] || { echo JAVA_CORE_LIB not set. Please source envsetup.sh.; exit 1; }

RUNTIME_LIB=${MAPLE_RUNTIME_ROOT}/lib/${MAPLE_TARGET_ARCH}
RUNTIME_BIN=${MAPLE_RUNTIME_ROOT}/bin/${MAPLE_TARGET_ARCH}

MPLSH=${RUNTIME_BIN}/mplsh
[ -x ${MPLSH} ] || { echo ${MPLSH} not found, please download Maple runtime.; exit 2; }

export JAVA_NATIVE_LIB=libjava.so
export LD_LIBRARY_PATH=.:${RUNTIME_LIB}:${JAVA_HOME}/jre/lib/amd64:${JAVA_HOME}/jre/lib/amd64/server:${LD_LIBRARY_PATH}
export MAPLE_ENGINE_DEBUG=none

GDB=
if [ "x$1" = "x-gdb" ]; then
    GDB="gdb --args "
    shift
fi

if [ $# -lt 1 ]; then
    echo "Usage: $0 -classpath <App-shared-lib> <Classname>"
    exit 1
fi

libcore="-Xbootclasspath:${JAVA_CORE_LIB}.so"
grep -q -- "-Xbootclasspath:${JAVA_CORE_LIB}.so" <<< "$*" && libcore=

$GDB ${MPLSH} $libcore "$@"

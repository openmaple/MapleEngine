#!/bin/echo This script should be sourced in a shell. Execute command: source
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

if [ "${BASH_SOURCE[0]}" -ef "$0" ]; then
    echo "This script should be sourced in a bash shell, not executed directly"
    exit 1
fi

export MAPLE_ROOT=$(dirname ${BASH_SOURCE[0]})
if [[ ${MAPLE_ROOT::1} != / ]]; then
    export MAPLE_ROOT="$(pwd)/$MAPLE_ROOT"
fi
export MAPLE_COMPILER_ROOT="$MAPLE_ROOT/../mapleall"
export MAPLE_ENGINE_ROOT="$MAPLE_ROOT/maple_engine"
export MAPLE_RUNTIME_ROOT="$MAPLE_ROOT/maple_runtime"
export MAPLE_BUILD_ROOT="$MAPLE_ROOT/maple_build"
export MAPLE_BUILD_TOOLS="$MAPLE_BUILD_ROOT/tools"
export MAPLE_DEBUGGER_ROOT="$MAPLE_ROOT/maple_debugger"
export MAPLE_DEBUGGER_TOOLS="$MAPLE_DEBUGGER_ROOT/tools"
export MAPLE_DEBUGGER_LLDB_ROOT="$MAPLE_DEBUGGER_ROOT/mlldb"
export MAPLE_DEBUGGER_LLDB_SRC="$MAPLE_DEBUGGER_LLDB_ROOT/maple_debugger"
export MAPLE_TARGET_ARCH="x86_64"
export MAPLE_DEBUGGER_LIBCORE="$MAPLE_BUILD_ROOT/out/$MAPLE_TARGET_ARCH"
export JAVA_CORE_LIB="libcore"
export JAVA_CORE_SRC="$HOME/my_openjdk8"
export JAVA_HOME=/usr/lib/jvm/java-8-openjdk-amd64
if [[ $MAPLE_COMPILER_ROOT =~ [[:space:]]+ ]]; then
    export MAPLE_COMPILER_ROOT="$HOME/tmp/mapleall"
    echo "MAPLE_COMPILER_ROOT is set to $MAPLE_COMPILER_ROOT"
elif [ ! -d "$JAVA_HOME" ]; then
    echo "Error: directory $JAVA_HOME not found"
    echo "Please install the openjdk-8-jdk-headless package"
    export MAPLE_ROOT=
fi

#!/bin/bash
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


# link one or more .s to .so
# using: asm2so.sh xxx.s yyy.s ...
# output: xxx.so

for v in ROOT COMPILER_ROOT ENGINE_ROOT RUNTIME_ROOT BUILD_ROOT TARGET_ARCH; do
    eval x=\$MAPLE_$v
    [ -n "$x" ] || { echo MAPLE_$v not set. Please source envsetup.sh.; exit 1; }
done

if [ "$#" -eq 0 ]; then
  echo "usage: asm2so.sh xxx.s yyy.s zzz.s ..."
  exit 1
fi

# library path for -lcore -lcommon-bridge
RUNTIME_LIB="${MAPLE_RUNTIME_ROOT}"/lib/"${MAPLE_TARGET_ARCH}"
OUT="${MAPLE_BUILD_ROOT}"/out/"${MAPLE_TARGET_ARCH}"

if [ ! -f "${RUNTIME_LIB}"/mrt_module_init.o ]; then
    [ -f "${OUT}"/mrt_module_init.o ] || { echo Need file "${RUNTIME_LIB}/mrt_module_init.o"; exit 2; } 
    cp "${OUT}"/mrt_module_init.o "${RUNTIME_LIB}"/ || { echo Failed to copy "${OUT}"/mrt_module_init.o; exit 2; }
fi

# link script
LINKER="${MAPLE_BUILD_ROOT}"/linker/"${MAPLE_TARGET_ARCH}"

SO_ROOT_NAME=${1%.*}

# .o
O_LIST=""
for f in $* ; do
  if [ ! -f "$f" ]; then
    echo "$f does not exist"
    continue
  fi

  o_name=${f%.*}.o
  g++ -g3 -pie -O2 -x assembler-with-cpp -c "$f" -o "$o_name" || exit 2
  O_LIST="$O_LIST $o_name"
done

# .so
g++ -g3 -pie -O2 -fPIC -shared -o $SO_ROOT_NAME.so $O_LIST "${RUNTIME_LIB}"/mrt_module_init.o -rdynamic -L"${RUNTIME_LIB}" -lcore -lcommon-bridge -T "${LINKER}"/mapleld.so.lds || exit 2


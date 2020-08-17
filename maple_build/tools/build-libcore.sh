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
[ -n "${JAVA_CORE_LIB}" ] || { echo JAVA_CORE_LIB not set. Please source envsetup.sh.; exit 1; }
cd "$MAPLE_ROOT/" || { echo Failed to enter dir "$MAPLE_ROOT"; exit 1; }

CC=gcc
CXX=g++

PREBUILT_BIN=${MAPLE_BUILD_ROOT}/prebuilt/bin
JAR=${MAPLE_BUILD_ROOT}/jar
LINKER=${MAPLE_BUILD_ROOT}/linker/${MAPLE_TARGET_ARCH}
TOOLS=${MAPLE_BUILD_ROOT}/tools
AUX=${TOOLS}/auxiliary
RUNTIME_LIB=${MAPLE_RUNTIME_ROOT}/lib/${MAPLE_TARGET_ARCH}

JBC2MPL=${PREBUILT_BIN}/jbc2mpl
IRB_OPENARK=${PREBUILT_BIN}/irbuild
IRB_MAPLE=${MAPLE_COMPILER_ROOT}/bin/ark-clang-release/irbuild
MAPLE_COMPILER=${MAPLE_COMPILER_ROOT}/bin/ark-clang-release/maple
MAPLE_COLLECT_ROOTS=${AUX}/mpl_collect_groot_lists

[ -x ${MAPLE_COMPILER} ] || { echo "${MAPLE_COMPILER}" not found, please build Maple compiler.; exit 2; }

ORIG_JAVA_CORE_LIB=orig-${JAVA_CORE_LIB}
OUT=${MAPLE_BUILD_ROOT}/out/${MAPLE_TARGET_ARCH}
mkdir -p "${OUT}"/ || exit 2
rm -f "${OUT}"/*"${JAVA_CORE_LIB}".*

cd "${OUT}"/ || exit 2
CC_FLAGS="-std=c++11 -fPIC -g -pie -O2 -fvisibility=default"
echo Building mrt_module_init.o...
${CC} ${CC_FLAGS} -c "${MAPLE_BUILD_ROOT}"/src/mrt_module_init.cpp -o mrt_module_init.o
[ $? -eq 0 ] || { echo Failed to compile mrt_module_init.cpp.; exit 2; }
 
echo "Building ${JAVA_CORE_LIB}.so (It may take a few minutes depending on your server and its workload)"...
${CC} ${CC_FLAGS} -c "${MAPLE_BUILD_ROOT}"/src/mrt_primitive_class.cpp \
    -I"${JAVA_HOME}"/include -I"${JAVA_HOME}"/include/linux -o mrt_primitive_class.o
[ $? -eq 0 ] || { echo Failed to compile mrt_primitive_class.cpp.; exit 2; }

# Generate ${JAVA_CORE_LIB}.mpl and ${JAVA_CORE_LIB}.mplt
cd "${JAR}"/ || exit 2
JARLIST=$(find . -name "*.jar" -type f | sort -r | xargs | tr ' ' ,)
[ -n "${JARLIST}" ] || { echo No any .jar files under "${JAR}"; exit 2; }
"${JBC2MPL}" -injar "${JARLIST}" -out "${JAVA_CORE_LIB}" || { echo Failed with "${JBC2MPL}".; exit 2; }

[ -f "${JAVA_CORE_LIB}".mpl -a -f "${JAVA_CORE_LIB}".mplt ] || { echo Failed to generate .mpl file; exit 2; }
mv -f "${JAVA_CORE_LIB}".mpl "${JAVA_CORE_LIB}".mplt "${OUT}"/ || exit 2

cd "${OUT}"/ || exit 2
cp "${JAVA_CORE_LIB}".mplt "${ORIG_JAVA_CORE_LIB}".mplt
"${IRB_OPENARK}" i "${JAVA_CORE_LIB}".mplt
mv "${JAVA_CORE_LIB}".irb.mpl "${JAVA_CORE_LIB}".tmpl

# Convert ASCII mplt to binary using irbuild from Maple build.
"${IRB_MAPLE}" -b "${JAVA_CORE_LIB}".tmpl
mv "${JAVA_CORE_LIB}".irb.mplt "${JAVA_CORE_LIB}".mplt

"${AUX}"/patch-mpl.sh "${JAVA_CORE_LIB}".mpl

ME_FLAG="-O2 --quiet --no-nativeopt --noignoreipa"
MPL2MPL_FLAG="-O1 --quiet --regnativefunc --no-nativeopt --maplelinker --emitVtableImpl --maplelinker-nolocal"
MPLCG_FLAG="-O2 --quiet --no-pie --verbose-asm --gen-groot-list --gen-c-macro-def --maplelinker --gen-mir-mpl --fpic"
"${MAPLE_COMPILER}" -exe=me,mpl2mpl,mplcg -option="${ME_FLAG}:${MPL2MPL_FLAG}:${MPLCG_FLAG}" "${JAVA_CORE_LIB}".mpl || exit 3

"${MAPLE_COLLECT_ROOTS}" -o unified.groots.s "${JAVA_CORE_LIB}".VtableImpl.groots.txt
${CC} -c unified.groots.s -o unified.groots.o || exit 4
${CXX} -g -O2 \
-fvisibility=hidden \
-fPIC \
-shared \
-o "${JAVA_CORE_LIB}".so \
mrt_module_init.o \
mrt_primitive_class.o \
unified.groots.o \
-x assembler-with-cpp \
-rdynamic -Wl,--no-as-needed \
-lmplcomp-rt -lpthread -ldl -lunwind \
-L"${RUNTIME_LIB}" \
"${JAVA_CORE_LIB}".VtableImpl.s \
-T "${LINKER}"/mapleld.so.lds || exit 4
cp "${JAVA_CORE_LIB}".so mrt_module_init.o *.mplt "${RUNTIME_LIB}"/ || exit 4
echo Copied "${JAVA_CORE_LIB}".so and mrt_module_init.o *.mplt to "${RUNTIME_LIB}"/.
echo Done.

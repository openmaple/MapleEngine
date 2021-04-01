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


# compile one or more .java -> .mpl -> .s using javac, jbc2mpl and maple
# using: java2o.sh A/xxx.java B/yyy.java C/zzz.java
# output: A/xxx.s B/yyy.s C/zzz.s

# MAPLE_COMPILER_ROOT has to be set by source envsetup_ark.sh

for v in ROOT COMPILER_ROOT ENGINE_ROOT RUNTIME_ROOT BUILD_ROOT TARGET_ARCH; do
    eval x=\$MAPLE_$v
    [ -n "$x" ] || { echo MAPLE_$v not set. Please source envsetup.sh.; exit 1; }
done
[ -n "${JAVA_CORE_LIB}" ] || { echo JAVA_CORE_LIB not set. Please source envsetup.sh.; exit 1; }

if [ "$#" -eq 0 ]; then
  echo "usage: java2asm2.sh A/xxx.java B/yyy.java C/zzz.java ..."
  exit 1
fi

CURR_DIR=$(pwd)

CC=gcc
CXX=g++

PREBUILT_BIN="${MAPLE_BUILD_ROOT}"/prebuilt/bin
JAR="${MAPLE_BUILD_ROOT}"/jar

JBC2MPL="${PREBUILT_BIN}"/jbc2mpl
IRB_OPENARK="${PREBUILT_BIN}"/irbuild
IRB_MAPLE="${MAPLE_COMPILER_ROOT}"/bin/ark-clang-release/irbuild
MAPLE_COMPILER="${MAPLE_COMPILER_ROOT}"/bin/ark-clang-release/maple
AUX="${MAPLE_BUILD_ROOT}"/tools/auxiliary
RUNTIME_LIB="${MAPLE_RUNTIME_ROOT}"/lib/"${MAPLE_TARGET_ARCH}"
OUT="${MAPLE_BUILD_ROOT}"/out/"${MAPLE_TARGET_ARCH}"

ORIG_COREALL_MPLT="${RUNTIME_LIB}"/orig-"${JAVA_CORE_LIB}".mplt
COREALL_MPLT="${RUNTIME_LIB}"/"${JAVA_CORE_LIB}".mplt

if [ ! -f "${ORIG_COREALL_MPLT}" ]; then
    [ -f "${OUT}"/orig-"${JAVA_CORE_LIB}".mplt ] || { echo Need file "${ORIG_COREALL_MPLT}"; exit 2; }
    cp "${OUT}"/orig-"${JAVA_CORE_LIB}".mplt "${ORIG_COREALL_MPLT}" || \
        { echo Failed to copy "${OUT}/orig-${JAVA_CORE_LIB}".mplt; exit 2; }
fi
if [ ! -f "${COREALL_MPLT}" ]; then
    [ -f "${OUT}"/"${JAVA_CORE_LIB}".mplt ] || { echo Need file "${COREALL_MPLT}"; exit 2; }
    cp "${OUT}"/"${JAVA_CORE_LIB}".mplt "${COREALL_MPLT}" || \
        { echo Failed to copy "${OUT}/${JAVA_CORE_LIB}".mplt; exit 2; }
fi

JARLIST=$(find "${JAR}" -name "*.jar" -type f | tr '\n' :)

# compile each file .java -> .s
for f in $* ; do
  if [ ! -f "$f" ]; then
    echo "$f does not exist"
    continue
  fi

  cd "$CURR_DIR"

  OUTPUT_DIR="$( cd "$( dirname "$f" )" >/dev/null 2>&1 && pwd )"
  DIR_NAME=$(dirname "$f")

  FILE_ROOT_NAME=$(basename "$f")
  FILE_ROOT_NAME=${FILE_ROOT_NAME%.*}
  MPL_NAME=${f%.*}.mpl
  
  echo "Compiling $f -> $DIR_NAME/$FILE_ROOT_NAME.mpl"
  
  TMPDIR=`mktemp -d`  # Ensure every build has a distinct temporary directory
  
  # assumes javac is in the execution path.
  javac -g -d "$TMPDIR" -bootclasspath "$JARLIST" "$f" || exit 1
  
  cd "$TMPDIR" 
  CLASSES=`ls *.class | tr '\n' ',' | sed "s/^/$FILE_ROOT_NAME.class,/" | sed -e "s/,$FILE_ROOT_NAME.class//" -e "s/[,]*$//"`
  "$PREBUILT_BIN"/jbc2mpl -dumpPragma -inclass "$CLASSES" -mplt "$ORIG_COREALL_MPLT" -asciimplt || exit 2
  
  if [ -f "$FILE_ROOT_NAME".mpl ]; then
    sed -e '/^var/d' -e '/^func/d' -e '/^ *pragma .* var / s/".*"//' "$FILE_ROOT_NAME".mplt > "$FILE_ROOT_NAME".tmpl
    "$IRB_MAPLE" -srclang=java -b "$FILE_ROOT_NAME".tmpl
    mv -f "$FILE_ROOT_NAME".irb.mplt "$OUTPUT_DIR/$FILE_ROOT_NAME".mplt
    sed "s/^import.*libcore.mplt\"/import \"${COREALL_MPLT//\//\\/}\"/" "$FILE_ROOT_NAME".mpl > "$OUTPUT_DIR/$FILE_ROOT_NAME".mpl
  fi
  cd -
  
  ME_FLAG="-O2 --quiet --no-nativeopt --noignoreipa"
  MPL2MPL_FLAG="-O1 --quiet --regnativefunc --no-nativeopt --maplelinker --emitVtableImpl"
  MPLCG_FLAG="-O2 --quiet --no-pie --verbose-asm --gen-groot-list --gen-c-macro-def --maplelinker --fpic --gen-mir-mpl"
  
  echo "Compiling $DIR_NAME/$FILE_ROOT_NAME.mpl -> $DIR_NAME/$FILE_ROOT_NAME.s"
  cd "$OUTPUT_DIR"
  # .s
  "${AUX}"/patch-mpl.sh "${FILE_ROOT_NAME}".mpl
  "${MAPLE_COMPILER}" -exe=me,mpl2mpl,mplcg -option="${ME_FLAG}:${MPL2MPL_FLAG}:${MPLCG_FLAG}" "$FILE_ROOT_NAME".mpl > maple.log 2>&1 || exit 3
  "${AUX}"/patch-asm.sh "$FILE_ROOT_NAME".VtableImpl.s
  
  mv "$FILE_ROOT_NAME".VtableImpl.s "$FILE_ROOT_NAME".s

  rm -rf "$TMPDIR"
  cd -
  echo " "
done


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

MPL="$1"
sedcmds=$(sed -n "/\/\/ mpl patches/,\$p" "$0" | grep '^extern "C"' \
    | sed -e 's/.* \([^ ]*\) *(.*/-e "s\/^func \&\1 \\([^w]\\)\/func \\\&\1 weak \\1\/"/')
eval sed $sedcmds "$MPL" > "$MPL"~
sed -e 's|addrof ptr $_C_STR_[0-9a-f]*|callassigned \&MCC_GetOrInsertLiteral (&)|' \
    -e 's|dread ptr $_PTR_C_STR_[0-9a-f]*|callassigned \&MCC_GetOrInsertLiteral (&)|' \
    -e 's|callassigned &MCC_GetOrInsertLiteral (callassigned &MCC_GetOrInsertLiteral (\([^)]*\))|callassigned \&MCC_GetOrInsertLiteral (\1|' \
    -e 's|\(dassign [^(]*\) (\(callassigned &MCC_GetOrInsertLiteral ([^)]*)\))|\2 {\1}|' \
    -e 's|= callassigned &MCC_GetOrInsertLiteral (\([^)]*\))|= \1|' \
    "$MPL"~ > "$MPL"
rm -f "$MPL"~
exit 0

// mpl patches
extern "C" jboolean Ljava_2Flang_2FClassLoader_3B_7CregisterAsParallelCapable_7C_28_29Z(jclass);
extern "C" void Ljava_2Flang_2FClassLoader_3B_7CloadLibrary_7C_28Ljava_2Flang_2FClass_3BLjava_2Flang_2FString_3BZ_29V(jclass, jstring, jboolean);
extern "C" jstring Ljava_2Flang_2FString_3B_7Cintern_7C_28_29Ljava_2Flang_2FString_3B(jstring);
extern "C" void Ljava_2Flang_2FShutdown_3B_7CbeforeHalt_7C_28_29V(jclass klass);

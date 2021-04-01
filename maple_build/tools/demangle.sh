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
line=
while IFS= read -rn1 k; do
   [ -z "$k" ] || { echo -n "$k"; line="$line$k"; continue; }
   echo
   if grep -v "^(gdb)" <<< "$line" | grep -q -e _7C -e _2F; then
       sed -e 's/__/_/g'  -e 's|_2F|.|g' -e 's|_3B|;|g' -e 's/_7C/|/g' \
           -e 's/_29/)/g' -e 's/_28/(/g' -e 's|_3C|<|g' -e 's|_3E|>|g' -e 's|_24|\$|g' \
           -e 's/L[^ ;]*;/\x1b[92m&\x1b[0m/g' \
           -e 's/|\([A-Za-z0-9_$<>]*\)|/\x1b[95m\1\x1b[0m/g' \
           -e 's/at \([^:]*\):\([0-9][0-9]*\)/at \x1b[33m\1\x1b[0m:\x1b[96m\2\x1b[0m/g' \
           -e 's/^#[0-9][0-9]*/\x1b[34m&\x1b[0m/' \
           -e 's/level [0-9][0-9]*/\x1b[7;49;39m&\x1b[0m/' \
           -e 's/^.*\x1b\[/\x1b[44m^\x1b[0m&/' <<< "$line"
   fi
   line=
done

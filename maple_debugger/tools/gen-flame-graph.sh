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

for v in ROOT COMPILER_ROOT ENGINE_ROOT RUNTIME_ROOT BUILD_ROOT TARGET_ARCH DEBUGGER_TOOLS; do
    eval x=\$MAPLE_$v
    [ -n "$x" ] || { echo MAPLE_$v not set. Please source envsetup.sh.; exit 1; }
done

if [ ! -d "$MAPLE_ROOT"/../FlameGraph ]; then
   cd "$MAPLE_ROOT"/..
   git clone https://github.com/brendangregg/FlameGraph.git || exit 1
   cd - > /dev/null
fi

if [ $# -lt 1 -o ! -f "$1" ]; then
    echo Please specify a file which contains Maple debugger trace.
    exit 1
fi
filename="$1"
grep -q -- "- <-- .*[0-9]us, ret:" "$filename"
if [ $? -ne 0 ]; then
    echo "File $filename does not contain valid Maple debugger trace"
    exit 2
fi

"$MAPLE_DEBUGGER_TOOLS"/trace2stack.py "$filename" > "$filename".stack || exit 2
"$MAPLE_ROOT"/../FlameGraph/stackcollapse.pl < "$filename".stack > "$filename".folded || exit 3
sub=$(basename "$filename" .log)
"$MAPLE_ROOT"/../FlameGraph/flamegraph.pl --height 20 --width 1600 --countname us --title "Maple Debugger Profiler" \
    --subtitle "$sub" --nametype "LOC,FUNC:" "$filename".folded > "$filename".svg || exit 4
"$MAPLE_ROOT"/../FlameGraph/flamegraph.pl --height 20 --width 1600 --countname us --title "Maple Debugger Profiler" \
    --subtitle "$sub stack-reversed" --reverse --nametype "LOC,FUNC:" "$filename".folded > "$filename".rev.svg || exit 4
echo Generated "$filename".svg
echo You may view it in your browser.

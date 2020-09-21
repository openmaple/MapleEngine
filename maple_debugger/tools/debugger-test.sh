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

for v in ROOT COMPILER_ROOT ENGINE_ROOT RUNTIME_ROOT BUILD_ROOT TARGET_ARCH DEBUGGER_ROOT; do
    eval x=\$MAPLE_$v
    [ -n "$x" ] || { echo MAPLE_$v not set. Please source envsetup.sh.; exit $LINENO; }
done
[ -n "${JAVA_CORE_LIB}" ] || { echo JAVA_CORE_LIB not set. Please source envsetup.sh.; exit $LINENO; }

ps -ef | expand | grep -v grep | grep -v " $$ "| grep "/bash.*debugger-test.sh" && { echo Another test is running.; exit $LINENO; }

TESTROOT="$MAPLE_DEBUGGER_ROOT"/testcases
TESTOUT="$MAPLE_BUILD_ROOT"/out/debugger/test
SPECIFIED="."
[ $# -gt 0 ] && { SPECIFIED="/$1"; shift; }
[ -d "$TESTOUT" ] && rm -rf "$TESTOUT"
mkdir -p "$TESTOUT"
find -L "$TESTROOT" -maxdepth 2 -name "[A-Z]*[0-9][0-9][0-9][0-9]-*" -type d |
grep -e "$SPECIFIED/*[^/]*\$" | sort |
while read d; do
    testcase=$(basename "$d")
    echo -n Running test case "$testcase"...
    cp -aL "$d" "$TESTOUT"/"$testcase" || exit $LINENO
    cd "$TESTOUT"/"$testcase" || exit $LINENO
    [ -f test.cfg ] || { echo FAILED; echo "Error: File $testcase/test.cfg not found" > FAILED; continue; }
    [ -f .mgdbinit ] || { echo FAILED; echo "Error: File $testcase/.mgdbinit not found" > FAILED; continue; }

    TESTCASE= UNEXPECTED= EXPECTED= TIMEOUT=
    source ./test.cfg > /dev/null || { echo FAILED; echo "Error: Failed to source $testcase/cfg" > FAILED; continue; }
    TIMEOUT=${TIMEOUT:-60}  # Set default timeout limit to 60 seconds if it is not set
    [ -n "$TESTCASE" ] || { echo FAILED; echo "Error: TESTCASE not set in $testcase/cfg" > FAILED; continue; }
    app=$(basename "$TESTCASE")
    mkdir -p "$app"
    cp -a .mgdbinit "$MAPLE_BUILD_ROOT/$TESTCASE"/*.java "$app"/ || exit $LINENO
    grep -v "^echo " .mgdbinit | sed -e '/^python/,/^end/s/^/@/' -e 's/^[^@].*/echo (gdb) & \\n\n&/' -e 's/^@//' > "$app"/.mgdbinit
    cd "$app" || exit $LINENO
    "$MAPLE_DEBUGGER_TOOLS"/debugger-trace.sh >& log.txt &
    pid=$!
    maxtry=$TIMEOUT
    while [ $((maxtry--)) -gt 0 ]; do
        ps -p $pid > /dev/null || break
        sleep 1
    done
    if ps -p $pid > /dev/null; then
        kill -9 -$pid >& /dev/null
        echo TIMEOUT; echo $TIMEOUT > TIMEOUT
        continue
    fi

    grep -e "Error while executing Python" -e "Error occurred in Python:" -e "SyntaxError: invalid syntax" log.txt > err.txt
    if [ $? -eq 0 ]; then
        echo FAILED; echo "Error: Failed due to error in Python code" > FAILED
        continue
    fi
    [ -s err.txt ] || rm -f err.txt

    if [ -n "$UNEXPECTED" ]; then
        grep -- "$UNEXPECTED" log.txt > unexpected-output.txt
        if [ $? -eq 0 ]; then
            echo FAILED; echo "Error: Failed due to matching unexpected output in log.txt" > FAILED
            continue
        fi
        [ -s unexpected-output.txt ] || rm -f unexpected-output.txt
    fi

    if [ -n "$EXPECTED" ]; then
        grep -- "$EXPECTED" log.txt > expected-output.txt
        if [ $? -ne 0 ]; then
            echo FAILED; echo "Error: Failed due to no expected output in log.txt" > FAILED
            continue
        fi
        [ -s expected-output.txt ] || rm -f expected-output.txt
    fi
    echo PASSED; echo PASSED > PASSED
done

cd "$TESTOUT" || exit $LINENO
passed=$(find . -name PASSED | wc -l)
failed=$(find . -name FAILED | wc -l)
timeout=$(find . -name TIMEOUT | wc -l)
echo Summary of testing results:
echo "  Passed:  "$passed
echo "  Failed:  "$failed
echo "  Timeout: "$timeout
echo Dir: "$TESTOUT"
if [ "$passed" -gt 0 -a "$failed" -eq 0 -a "$timeout" -eq 0 ]; then
    echo Passed
    exit 0
else
    echo Failed
    exit $LINENO
fi

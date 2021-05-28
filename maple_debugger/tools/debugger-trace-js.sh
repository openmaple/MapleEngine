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

if [ $# -ge 1 -a -f "$1" ]; then
    arg2=${2:-99999999}
    [[ "$arg2" == +([0-9]) ]] || { grep -n -e "^(gdb)" -e "^(gdb_exec)" -e "$arg2" "$1"; exit 0; }
    lines=$(sed -n "1,${arg2}p" "$1" | nl -hn -ba -w6 -s' ' | tac)
    sed '1,/^[0-9 ]*+ + =/d' <<< "$lines" | grep -e "^[0-9 ]*(gdb) " -e "^[0-9 ]*(gdb_exec) " | tac
    prefix=$(grep -m1 "^[0-9 ]*+ [+ ]*=.[=>] " <<< "$lines" | sed -e 's/=.*/=/' -e 's/^[0-9 ]*//')
    while [ ${#prefix} -ge 2 ]; do
        grep -m1 "^[0-9 ]*$prefix" <<< "$lines" || break
        prefix=$(sed 's/^..//' <<< "$prefix")
    done | tac
    exc="^[0-9 ]*Oops, an uncaught exception occured"
    grep -q "$exc" <<< "$lines" || exit
    echo -e "\nAn uncaught exception occured. Its backtrace:"
    sed -n "/$exc/,/^[0-9 ]*+ [+ ]*=/p" <<< "$lines" | tac | grep "^[0-9 ]*- .*, ret: None"
    exit 1
fi
currdir=$(pwd)
workingdir=$(sed 's|/maple_engine/.*|/maple_engine|' <<< "$currdir")
[ "$currdir" = "$workingdir" ] && { echo Not a Maple engine working directory; exit 1; }
source "$workingdir"/envsetup.sh
app=$(basename "$(pwd)")
if [ ! -f "$app".so ]; then
    [ -f "$app".js ] || { echo Not a working directory; exit 2; }
    echo Building "$app".so
    "$MAPLE_BUILD_TOOLS"/run-js-app.sh "$app".js || exit 2
fi
if [ "x$1" = "x-f" ]; then
    sed -n '/^mset trace on/,$p' "$0" | sed -e "s/\${app}/$app/" -e '/^python/,/^end/s/^/@/' \
        -e 's/^[^@].*/echo (gdb) & \\n\n&/' -e 's/^@//' > .mdbinit
fi
tracelog="$app-$(date +%y%m%d-%H%M).log"
"$MAPLE_BUILD_TOOLS"/run-js-app.sh -gdb ./"$app".so "$app" 32 2> >(tee -a "$tracelog" >&2)
"$MAPLE_DEBUGGER_TOOLS"/gen-flame-graph.sh "$tracelog" > /dev/null || exit 3
mkdir -p tmp
mv -f "$tracelog".{stack,folded,rev.svg} "$tracelog" tmp/ || exit 1
if [ -f "$tracelog".svg ]; then
    sed -e 's/\[/<|/g' -e 's/\]/|>/g' \
        -e 's/^\(+[+ ]*=\)=> /\1[= /' -e 's/^\(-[- ]*\)<-- /\1-]- /' tmp/"$tracelog" > "$tracelog"
    #google-chrome-stable "$tracelog".svg 2> /dev/null &
    echo Generated the flame graph "$tracelog".svg
else
    echo Failed to generate the flame graph
fi
exit

### .mdbinit
mset trace on
b main
run
msi 5000
python
from m_util import gdb_echo_exec as exec
def cmds(cnt):
    mlst=["","+10","-20","20"]
    masm=["",":+10",":-20",":+10"]
    exec('mstepi 200')
    if cnt % 4 == 0: exec('mbt')
    exec('mnexti')
    exec('mlist ' + mlst[cnt % 4])
    exec('mup')
    exec('mlist -asm' + masm[cnt % 4])
    exec('mdown')
for i in range(20):
    cmds(i)
    exec('mfinish')
exec('mb L${app}_3B_7Cmain_7C_28ALjava_2Flang_2FString_3B_29V')
exec('mb -listall')
exec('continue')
for i in range(20):
    cmds(i)
end
mset trace off
mbt
quit

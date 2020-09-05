#!/bin/bash
currdir=$(pwd)
workingdir=$(sed 's|/maple_engine/.*|/maple_engine|' <<< "$currdir")
[ "$currdir" = "$workingdir" ] && { echo Not a Maple engine working directory; exit 1; }
source "$workingdir"/envsetup.sh
app=$(basename "$(pwd)")
if [ ! -f "$app".so ]; then
    [ -f "$app".java ] || { echo Not a working directory; exit 2; }
    echo Building "$app".so
    "$MAPLE_BUILD_TOOLS"/java2asm.sh "$app".java || exit 2
    "$MAPLE_BUILD_TOOLS"/asm2so.sh "$app".s || exit 2
fi
if [ "x$1" = "x-f" ]; then
    shift
    cat > .mgdbinit << EOF
mset trace on
mb L${app}_3B_7Cmain_7C_28ALjava_2Flang_2FString_3B_29V
run
python
for i in range(500):
    gdb_echo_and_exec('mstepi')
for i in range(20):
    gdb_echo_and_exec('mstepi')
    gdb_echo_and_exec('mstepi')
    gdb_echo_and_exec('mstepi')
    gdb_echo_and_exec('mbt')
    gdb_echo_and_exec('mup')
    gdb_echo_and_exec('down')
for i in range(20):
    gdb_echo_and_exec('mnexti')
end
mset trace off
mbt
quit
EOF
    sed -i -e '/^python/,/^end/s/^/@/' -e 's/^[^@].*/echo (gdb) & \\n\n&/' -e 's/^@//' .mgdbinit
fi
tracelog="$app-$(date +%y%m%d-%H%M).log"
"$MAPLE_BUILD_TOOLS"/run-app.sh -gdb -classpath ./"$app".so "$app" 32 2> >(tee -a "$tracelog" >&2)
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

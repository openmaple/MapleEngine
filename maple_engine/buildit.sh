#
# Copyright (C) [2020-2021] Futurewei Technologies, Inc. All rights reserved.
#
# OpenArkCompiler is licensed underthe Mulan Permissive Software License v2
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

#Use Clang
export CC=/usr/bin/clang
export CXX=/usr/bin/clang++

# Move old build dir if it exists
rm -rf build.old2
[ -d build.old ] && mv build.old build.old2
[ -d build ] && mv build build.old
mkdir build
cd build || exit 1

# create build system
if [ $# -lt 1 ]; then
  cmake .. -DMACHINE64=True
elif [ $1 = "mplre" ]; then
  cmake ..
fi

# build
make VERBOSE=1 "$@"


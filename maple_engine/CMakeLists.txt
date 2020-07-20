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

# CMakeLists files in this project can
# refer to the root source directory of the project as ${MAPLE_ENG_SOURCE_DIR} and
# to the root binary directory of the project as ${MAPLE_ENG_BINARY_DIR}.
cmake_minimum_required(VERSION 3.0)

# The main project
project (MAPLE_ENG)

enable_language(C)
enable_language(CXX)

include(UseJava)

SET (CMAKE_C_FLAGS_INIT                "-Wall")
SET (CMAKE_C_FLAGS_DEBUG_INIT          "-g")
SET (CMAKE_C_FLAGS_MINSIZEREL_INIT     "-Os -DNDEBUG")
SET (CMAKE_C_FLAGS_RELEASE_INIT        "-O3 -DNDEBUG")
SET (CMAKE_C_FLAGS_RELWITHDEBINFO_INIT "-O2 -g")

SET (CMAKE_CXX_FLAGS_INIT                "-Wall")
SET (CMAKE_CXX_FLAGS_DEBUG_INIT          "-g")
SET (CMAKE_CXX_FLAGS_MINSIZEREL_INIT     "-Os -DNDEBUG")
SET (CMAKE_CXX_FLAGS_RELEASE_INIT        "-O3 -DNDEBUG")
SET (CMAKE_CXX_FLAGS_RELWITHDEBINFO_INIT "-O2 -g")


# Recurse into the subdirectories. This does not actually
# cause another cmake executable to run. The same process will walk through
# the project's entire directory structure.
add_subdirectory (src)
#add_subdirectory (tests)



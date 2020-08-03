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

# Create a library called "maple_libs" which includes the source file "xxxx.cxx".
# The extension is already found. Any number of sources could be listed here.
#add_library (maple_libs maple_libs.cpp)

# Make sure the compiler can find include files for our maple_libs library
# when other libraries or executables link to maple_libs
#target_include_directories (.../include PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# Link the executable to the maple_libs library. Since the maple_libs library has
# public include directories we will use those link directories when building
# maple_engine

####target_link_libraries (maple_engine LINK_PUBLIC src)

# Set include file based on if the headers are local or in the compile directory
# Note: md5.h requires package libssl-dev to be installed

add_definitions(-std=c++11 -DTARGX86_64=1 -DUSE_CLANG -DDYNAMICLANG -g -O2 -fno-omit-frame-pointer -funsigned-char )

if(DEFINED ENV{USE_LOCAL_REP_HEADERS})
	SET(BASE_INC_DIR  ${CMAKE_CURRENT_SOURCE_DIR}/../include)
else()
	SET(BASE_INC_DIR  ${CMAKE_CURRENT_SOURCE_DIR}/../../../mapleall/mapleall)
endif()

include_directories (AFTER PUBLIC 
	/usr/include/
	${CMAKE_CURRENT_SOURCE_DIR}/../include
	${CMAKE_CURRENT_SOURCE_DIR}/../../third-party/md5/include/
	${BASE_INC_DIR}/
	${BASE_INC_DIR}/mempool/include
	${BASE_INC_DIR}/maple_util/include
	${BASE_INC_DIR}/maple_ir/include
	${BASE_INC_DIR}/maple_be/include
	${BASE_INC_DIR}/maple_be/include/be
	${BASE_INC_DIR}/maple_be/include/cg/ark
	)

add_library (mplre SHARED invoke_method.cpp mdebug.cpp mfunction.cpp mloadstore.cpp shimfunction.cpp )


find_library( PBmpl_LIB mpl-rt "${CMAKE_CURRENT_SOURCE_DIR}/../lib/*" )
find_library( PBcorea_LIB core-all "${CMAKE_CURRENT_SOURCE_DIR}/../lib/*" )
find_library( PBcomb_LIB common-bridge "${CMAKE_CURRENT_SOURCE_DIR}/../lib/*" )
find_library( PBjaddr2_LIB java_addr2line "${CMAKE_CURRENT_SOURCE_DIR}/../lib/*" )
find_library( PBmplre_LIB mplre "${CMAKE_CURRENT_SOURCE_DIR}/../lib/*" )
find_library( PBunwind_LIB unwind "${CMAKE_CURRENT_SOURCE_DIR}/../lib/*" )

#target_link_libraries( mplsh ${PBmpl_LIB} )
#target_link_libraries( mplsh ${PBcorea_LIB} )
#target_link_libraries( mplsh ${PBcomb_LIB} )
#target_link_libraries( mplsh ${PBjaddr2_LIB} )
#target_link_libraries( mplsh ${PBmplre_LIB} )
#target_link_libraries( mplsh ${PBunwind_LIB} )

string(APPEND CMAKE_EXE_LINKER_FLAGS "-v ")
string(APPEND CMAKE_EXE_LINKER_FLAGS "-ldl -lpthread -lstdc++ -lm -lgcc_s -lc -lelf -lz")

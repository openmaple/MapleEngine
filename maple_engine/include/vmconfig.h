/*
 * Copyright (C) [2021] Futurewei Technologies, Inc. All rights reserved.
 *
 * OpenArkCompiler is licensed under the Mulan Permissive Software License v2.
 * You can use this software according to the terms and conditions of the MulanPSL - 2.0.
 * You may obtain a copy of MulanPSL - 2.0 at:
 *
 *   https://opensource.org/licenses/MulanPSL-2.0
 *
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR
 * FIT FOR A PARTICULAR PURPOSE.
 * See the MulanPSL - 2.0 for more details.
 */

#ifndef MAPLEVM_INCLUDE_VM_VMCONFIG_H_
#define MAPLEVM_INCLUDE_VM_VMCONFIG_H_

#ifdef TEST_BENCHMARK
#define APP_MEMORY_SIZE (8 * 1024 * 1024)       // 8M application memory
// make sure VM_MEMORY_SIZE == VM_MEMORY_BIG_SIZE + VM_MEMORY_SMALL_SIZE
#define VM_MEMORY_BIG_SIZE (4 * 1024 * 1024)    // 4M
#define VM_MEMORY_SMALL_SIZE (2 * 1024 * 1024)  // 2M
#define VM_MEMORY_SIZE (8 * 1024 * 1024)        // 8M VM internal data
#define TOTAL_MEMORY_SIZE (APP_MEMORY_SIZE + VM_MEMORY_SIZE)
#define STACKOFFSET APP_MEMORY_SIZE

// make sure HEAP_SIZE == HEAP_BIG_SIZE + HEAP_SMALL_SIZE
#define HEAP_BIG_SIZE (8 * 1024 * 1024)     // 8M
#define HEAP_SMALL_SIZE (4 * 1024 * 1024)   // 4M
#define HEAP_SIZE (12 * 1024 * 1024)        // 12M
#define MAXCALLARGNUM 255
#else
#define APP_MEMORY_SIZE (17 * 1024)      // 16K application memory
// make sure VM_MEMORY_SIZE == VM_MEMORY_BIG_SIZE + VM_MEMORY_SMALL_SIZE
#define VM_MEMORY_BIG_SIZE (24 * 1024)   // 24k
#define VM_MEMORY_SMALL_SIZE (8 * 1024)  // 8k
#define VM_MEMORY_SIZE (32 * 1024)       // 32K VM internal data
#define TOTAL_MEMORY_SIZE (APP_MEMORY_SIZE + VM_MEMORY_SIZE)
#define STACKOFFSET APP_MEMORY_SIZE

// make sure HEAP_SIZE == HEAP_BIG_SIZE + HEAP_SMALL_SIZE
#define HEAP_BIG_SIZE (7 * 1024)
#define HEAP_SMALL_SIZE (6 * 1024)
#define HEAP_SIZE (13 * 1024)

#define MAXCALLARGNUM 10
#endif

// The stack of (pending) operands for next few (virtual) instructions
// (expression or statements). This is for MJSVM-CMPL (v2)
#define OPERANDS_STACK_SIZE 128

#define MVALSIZE 8
#define PTRSIZE 4

// find the aliged addr
#define ALIGNMENTNEGOFFSET(off, align) \
  {                                    \
    off = -off;                        \
    if (off % align)                   \
      off += align - (off % align);    \
    off = -off;                        \
  }

#define MAX_REGISTER_NUM 128

#endif  // MAPLEVM_INCLUDE_VM_VMCONFIG_H_

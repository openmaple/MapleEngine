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

// license / copyright comes here.
// An plain C implementation of MMapTable for mapleJS VM memory manager.

#ifndef MAPLEVM_INCLUDE_VM_MMAP
#define MAPLEVM_INCLUDE_VM_MMAP

#include <climits>
#include <cstdint>  // use to determine pointer size

#ifndef RC_NO_MMAP
struct AddrMapNode {
  void *ptr1;  // address in user's address space, which can be heap, stack (for locals) or global variable area
  void *ptr2;  // pointing to string/object
  AddrMapNode *next;
};

#define MMAP_NODE_SET_FREE(node_ptr) (node_ptr->ptr1 = NULL)
#define MMAP_NODE_IS_FREE(node_ptr) (node_ptr->ptr1 == NULL)
#define MMAP_NODE_NOT_FREE(node_ptr) (node_ptr->ptr1 != NULL)

// The MMAP_SIZE should be made to be configurable
#define HEAP_MMAP_SIZE 64    // using 6 bits for HEAP_MMAP index
#define GLOBAL_MMAP_SIZE 16  // using 4 bits for GLOBAL_MMAP index
#define FUNC_MMAP_SIZE 32    // using 5 bits for FUNC_MMAP index

// What's the smallest size of JSobject/JSstring. here assumes 8Bytes
#define HEAP_MMAP_INDEX_MASK (0x000001F8)    // 6bits from bit 3
#define GLOBAL_MMAP_INDEX_MASK (0x00000078)  // 4bits from bit 3
#define FUNC_MMAP_INDEX_MASK (0x000000F8)    // 5bits from bit 3
#define MMAP_INDEX_SHIFT (3)                 // 3 bits for JSobject/string unit

// use AddrMap to distinguish linux system call mmap()
// There is one and only one AddrMap for the global block, one and only one
// AddrMap for the heap and one AddrMap for each function's locals (on the
// function stack). Each function's AddrMap is allocated when the function is
// invoked, and deallocated when the function exits.
class AddrMap {
 public:
  AddrMapNode *hashtab;  // dynamically allocated hash table
  AddrMap *next;         // for linking together in the free AddrMap pool
  uint32_t hashtab_idx_mask;
  uint32_t hashtab_size;
#if MM_DEBUG
  static uint32_t total_count;
  static uint32_t max_count;
  static uint32_t find_count;
  static uint32_t nodes_count;
#endif
 public:
  AddrMapNode *FindInAddrMap(void *);
  void AddAddrMapNode(void *, void *);
  void RemoveAddrMapNode(AddrMapNode *);
  int GetIndex(void *ptr) {
#if (UINTPTR_MAX > UINT_MAX)  // 64bit platform
    return ((int)((((long)(ptr)) & hashtab_idx_mask) >> MMAP_INDEX_SHIFT));
#else
    return ((int)((((int)(ptr)) & hashtab_idx_mask) >> MMAP_INDEX_SHIFT));
#endif
  }

#if MIR_DEBUG
  void print_mmap_info();
#endif  // MIR_DEBUG
};
#endif
#endif  // MAPLEVM_INCLUDE_VM_MMAP

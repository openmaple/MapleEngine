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

// license/copyright here
//
// This file implements the VM address map table in order to support
// reference counting garbage collection.
//
// The address map is for keeping track of pointer locations in the user's
// address space pointing to dynamically allocated objects that need garbage
// collection.  During program execution, when a pointer location is made to
// point to a dynamically allocated object, that object's reference count is
// increased by 1.  The GC also needs to know that the pointer location is now
// pointing to a dynamically allocated object, and it does this by creating
// an AddrMapNode to record this.  When a pointer location's previous value is
// destroyed due to assignment, if it previously pointed to an object, that
// object's reference count needs to be decremented.  GC relies on an existing
// AddrMapNode to be 100% certain that the pointer location is pointing to
// an object.  If it cannot locate an AddrMapNode with the pointer location's
// address (field ptr1), it is not necessary to decrement any reference count.

#include "vmmmap.h"
#include "mir_config.h"
#include <string.h>  // for memset.
#include "vmmemory.h"

#ifndef RC_NO_MMAP
#if MM_DEBUG
uint32_t AddrMap::total_count = 0;
uint32_t AddrMap::max_count = 0;
uint32_t AddrMap::find_count = 0;
uint32_t AddrMap::nodes_count = 0;
#endif

// see if an AddrMapNode for ptr has been created
AddrMapNode *AddrMap::FindInAddrMap(void *ptr) {
  MIR_ASSERT(ptr);
  AddrMapNode *node = &(hashtab[GetIndex(ptr)]);
#if MM_DEBUG
  find_count++;
  uint32_t count = 0;
  while (node && (node->ptr1 != ptr)) {
    node = node->next;
    count++;
    total_count++;
  }
  if (count > max_count) {
    max_count = count;
  }
#else
  while (node && (node->ptr1 != ptr)) {
    node = node->next;
  }
#endif  // MM_DEBUG
  if (node && node->ptr1 != ptr) {
    return NULL;
  }
  return node;
}

// ptr1 is now pointing to the object ptr2; create an AddrMapNode (if not there
// already) to record this
void AddrMap::AddAddrMapNode(void *ptr1, void *ptr2) {
  MIR_ASSERT(ptr1 && ptr2);

  AddrMapNode *node = &(hashtab[GetIndex(ptr1)]);
  AddrMapNode *pre_node = node;

  while (node && MMAP_NODE_NOT_FREE(node) && (node->ptr1 != ptr1)) {
    pre_node = node;
    node = node->next;
  }

  if (node) {
    // an existing free node founded.
    if (MMAP_NODE_IS_FREE(node)) {
      node->ptr1 = ptr1;
      node->ptr2 = ptr2;
#if MM_DEBUG
      nodes_count++;
#endif
    } else {
      // mmap node already exists.
      MIR_ASSERT(node->ptr2 == ptr2);
    }
  } else {
    // not found. need a new node
    node = memory_manager->NewAddrMapNode();  // new_mmap_node();
    node->ptr1 = ptr1;
    node->ptr2 = ptr2;
    node->next = NULL;
    pre_node->next = node;
#if MM_DEBUG
    nodes_count++;
#endif
  }
}

// invalidate the given AddrMapNode, which also recycles it for later use
void AddrMap::RemoveAddrMapNode(AddrMapNode *map_node) {
  MIR_ASSERT(map_node);
  // we mark it as dirty. This is to make MM easier.

#if MIR_DEBUG
  AddrMapNode *node = FindInAddrMap(map_node->ptr1);  // find_in_mmap(map, map_node->ptr1);
  MIR_ASSERT(map_node == node);
#endif  // MIR_DEBUG
#if MM_DEBUG
  nodes_count--;
#endif  // MM_DEBUG
  MMAP_NODE_SET_FREE(map_node);
}

#if MIR_DEBUG
void AddrMap::print_mmap_info() {
  // TODO: implement this
}

#endif  // MIR_DEBUG
#endif

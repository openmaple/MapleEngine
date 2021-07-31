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

#include <string.h>
#include "vmmemory.h"
#include "jsobject.h"
#include "jsobjectinline.h"
#include "jsvalueinline.h"
#include "jsfunction.h"
#include "cmpl.h"
#include "jsarray.h"
#include "jsiter.h"
#include "securec.h"
#include "jsdataview.h"
#include <cmath>
// This module performs memory management for both the app's heap space and the
// VM's own dynamic memory space.  For memory blocks allocated in the app's
// heap space, we rely on reference counting in order to know when a memory
// block can be recycled for use (garbage collected).  For memory blocks
// allocated in VM's own dynamic memory space, the VM is in charge of their
// recycling as it knows when something is no longer needed.
//
// Because of the need to support reference counting in the app's heap space,
// all memory blocks allocated there are enlarged by a 32 bit header
// (MemHeader) that precedes the memory space returned to the user program.
//
// In both memory spaces, MemoryChunk is used to keep track of an allocated
// block.  When a block is being used, there does not need to be a MemoryChunk
// to record it.  Its MemoryChunk is created only when the block is to be
// recycled.  To recycle a block, its MemoryChunk is kept inside MemoryHash,
// which groups them based on size. Thus, there are 2 MemoryHash instances, one
// for the app's heap space (heap_memory_bank_) and one for the VM's own
// dynamic memory space (vm_memory_bank_).
//
// MemoryChunk nodes are VM's own dynamic data structures, so their allocation
// are in the VM's own dynamic memory space, and their re-uses are managed by
// the VM.  The list of MemoryChunk nodes available for re-use is maintained
// in free_memory_chunk_.
using namespace maple;

MemoryManager *memory_manager = NULL;
#ifdef MARK_CYCLE_ROOTS
CycleRoot *cycle_roots = NULL;
CycleRoot *garbage_roots = NULL;
#else
__jsobject *obj_list = NULL;
#endif
bool is_sweep = false;

void *VMMallocGC(uint32 size, MemHeadTag tag, bool init_p) {
  uint32 alignedsize = memory_manager->Bytes4Align(size);
  uint32 head_size = MALLOCHEADSIZE;
#ifndef MARK_CYCLE_ROOTS
  if (tag == MemHeadJSObj) {
    head_size = MALLOCEXHEADSIZE;
  }
#endif
  void *memory = memory_manager->Malloc(alignedsize + head_size, init_p);
  if (memory == NULL) {
    MIR_FATAL("run out of memory");
  }

  MemHeader *memheaderp = (MemHeader *)memory;
#ifndef MARK_CYCLE_ROOTS
  if (tag == MemHeadJSObj) {
    memheaderp = (MemHeader *)((uint32 *)memory + 2);
  }
#endif
  memheaderp->memheadtag = tag;
  memheaderp->refcount = 0;
  if (memory_manager->IsDebugGC()) {
    printf("memory %p was allocated with header %d size %d\n", ((void *)((uint8 *)memory + head_size)), tag, alignedsize);
  }
  return (void *)((uint8 *)memory + head_size);
}

void *VMReallocGC(void *origptr, uint32 origsize, uint32 newsize) {
  uint32 alignedorigsize = memory_manager->Bytes4Align(origsize);
  uint32 alignednewsize = memory_manager->Bytes4Align(newsize);
  void *memory = memory_manager->Realloc(origptr, alignedorigsize, alignednewsize);
  return (void *)((uint8 *)memory + MALLOCHEADSIZE);
}

#if MIR_FEATURE_FULL && MIR_DEBUG
void MemoryHash::Debug() {
  for (uint32 i = 0; i < MEMHASHTABLESIZE; i++) {
    MemoryChunk *mchunk = table_[i];
    for (MemoryChunk *node = mchunk; node; node = node->next) {
      printf("memory offset:%u, memory size:%u\n", node->offset_, node->size_);
    }
  }
}

#endif

#if DEBUGGC
bool MemoryHash::CheckOffset(uint32 offset) {
  for (uint32 i = 0; i < MEMHASHTABLESIZE; i++) {
    for (MemoryChunk *node = table_[i]; node; node = node->next) {
      if ((offset >= node->offset_) && (offset < (node->size_ + node->offset_))) {
        return false;
      }
    }
  }
  return true;
}

#endif

void MemoryHash::PutFreeChunk(MemoryChunk *mchunk) {
  uint32 index = mchunk->size_ >> 2;
  bool issmall = (index >= 1 && index < MEMHASHTABLESIZE);
  uint32 x = issmall ? (--index) : (MEMHASHTABLESIZE - 1);
  // link the mchunk to the table
  if (issmall) {
    mchunk->next = table_[x];
    table_[x] = mchunk;
  } else {  // find the poisition and put free chunk to it
    MemoryChunk *node = table_[x];
    MemoryChunk *prenode = NULL;
    while (node && (node->offset_ < mchunk->offset_)) {
      prenode = node;
      node = node->next;
    }
    if (prenode) {
      mchunk->next = node;
      prenode->next = mchunk;
    } else {
      if (node) {
        mchunk->next = node;
        table_[x] = mchunk;
      } else {
        mchunk->next = NULL;
        table_[x] = mchunk;
      }
    }
  }
}

void MemoryHash::MergeBigFreeChunk() {
  // size must be >= MEMHASHTABLESIZE << 2
  MemoryChunk *node = table_[MEMHASHTABLESIZE - 1];
  if (!node) {
    return;
  }
  MemoryChunk *nextnode = node->next;
  while (nextnode) {  // merge the node
    // MemoryChunk *nextnode = node->next;
    if (node->offset_ + node->size_ == nextnode->offset_) {
      // merge node and nextnode
      node->size_ += nextnode->size_;
      node->next = nextnode->next;
      memory_manager->DeleteMemoryChunk(nextnode);
    } else if (node->offset_ + node->size_ < nextnode->offset_) {
      node = nextnode;
    } else {
      MIR_FATAL("something wrong with big free chunk node");
    }
    nextnode = node->next;
  }
}

MemoryChunk *MemoryHash::GetFreeChunk(uint32 size) {
  // the size is supposed to be 4 bytes aligned
  uint32 index = size >> 2;
  if (index >= 1 && index < MEMHASHTABLESIZE) {
    index--;
    MemoryChunk *node = table_[index];
    if (node) {
      table_[index] = node->next;
    }
    return node;
  } else {
    MemoryChunk *node = table_[MEMHASHTABLESIZE - 1];
    MemoryChunk *prenode = NULL;
    while (node && node->size_ < size) {
      prenode = node;
      node = node->next;
    }
    if (!node) {
      return NULL;
    }
    if (node->size_ == size) {
      if (prenode) {
        prenode->next = node->next;
      } else {
        table_[MEMHASHTABLESIZE - 1] = node->next;
      }
    } else {
      // node->size > size;
      MemoryChunk *newnode = memory_manager->NewMemoryChunk(node->offset_ + size, node->size_ - size, node->next);
      if (prenode) {
        prenode->next = newnode;
      } else {
        table_[MEMHASHTABLESIZE - 1] = newnode;
      }
      node->size_ = size;
    }
    return node;
  }
}

#if MACHINE64
void MemoryManager::SetF64Builtin() {
  // from 0 to 6, there are Math.E, LN10, LN2, LOG10E, LOG2E, PI, SQRT1_2, SQRT2
  f64ToU32Vec.push_back(0);
  f64ToU32Vec.push_back(MathE);
  f64ToU32Vec.push_back(MathLn10);
  f64ToU32Vec.push_back(MathLn2);
  f64ToU32Vec.push_back(MathLog10e);
  f64ToU32Vec.push_back(MathLog2e);
  f64ToU32Vec.push_back(MathPi);
  f64ToU32Vec.push_back(MathSqrt1_2);
  f64ToU32Vec.push_back(MathSqrt2);
  f64ToU32Vec.push_back(NumberMaxValue);
  f64ToU32Vec.push_back(NumberMinValue);
}

__jsvalue MemoryManager::GetF64Builtin(uint32_t index) {
  assert(index <= MATH_LAST_INDEX && "error");
  if (f64ToU32Vec.size() == 0) {
    SetF64Builtin();
  }
  __jsvalue jsval;
  jsval.tag = JSTYPE_DOUBLE;
  jsval.s.u32 = index;
  return jsval;
}

double MemoryManager::GetF64FromU32 (uint32 index) {
  assert(index < f64ToU32Vec.size() && "false");
  return f64ToU32Vec[index];
}

#if 0
uint32_t MemoryManager::SetF64ToU32 (double f64) {
  // std::map<double, uint32>::iterator it = f64ToU32Map.find(f64);
  uint32 size = f64ToU32Vec.size();
  if (size == 0) {
    SetF64Builtin();
    size = f64ToU32Vec.size();
    assert(size == MATH_LAST_INDEX + 1 && "error");
    if (f64 == 0) {
      return DOUBLE_ZERO_INDEX;
    }
    // f64ToU32Vec.push_back(f64);
    // return f64ToU32Vec.size() - 1;
  }
  assert(size > MATH_LAST_INDEX && "error");
  for (uint32 i = 0; i < size; i++) {
    if (fabs(f64ToU32Vec[i] - f64) < NumberMinValue) {
      return i;
    }
  }
  f64ToU32Vec.push_back(f64);
  return size;
  /*
  if (it == f64ToU32Map.end()) {
    uint32 index = f64ToU32Vec.size();
    f64ToU32Map.insert(std::pair<double, uint32>(f64, index));
    f64ToU32Vec.push_back(f64);
    return index;
  } else {
    return it->second;
  }
  */
}
#endif
#endif

// malloc memory in VM internal memory region, re-cycled via MemoryChunk nodes
void *MemoryManager::MallocInternal(uint32 malloc_size) {
  uint32 alignedsize = Bytes4Align(malloc_size);
  MemoryChunk *mchunk = vm_memory_bank_->GetFreeChunk(alignedsize);
  void *return_ptr = NULL;
  if (!mchunk) {
    if (vm_memory_bank_->IsBigSize(alignedsize)) {
      if (alignedsize + vm_free_big_offset_ > vm_memory_size_) {
        // merge the big memory chunk
        vm_memory_bank_->MergeBigFreeChunk();
        mchunk = vm_memory_bank_->GetFreeChunk(malloc_size);
        if (!mchunk) {
          MIR_FATAL("run out of VM heap memory.\n");
        }
        return_ptr = (void *)((uint8 *)vm_memory_ + mchunk->offset_);
        DeleteMemoryChunk(mchunk);
      } else {
        uint32 free_offset = vm_free_big_offset_;
        vm_free_big_offset_ += alignedsize;
        return (void *)((uint8 *)vm_memory_ + free_offset);
      }
    } else {
      if (alignedsize + vm_free_small_offset_ > vm_memory_small_size_) {
        MIR_FATAL("TODO run out of VM internal small memory.\n");
      }
      return_ptr = (void *)((char *)vm_memory_ + vm_free_small_offset_);
      vm_free_small_offset_ += alignedsize;
    }
  } else {
    return_ptr = (void *)((uint8 *)vm_memory_ + mchunk->offset_);
    DeleteMemoryChunk(mchunk);
  }
  return return_ptr;
}

void MemoryManager::FreeInternal(void *addr, uint32 size) {
  uint32 alignedsize = Bytes4Align(size);
  uint32 offset = (uint32)((uint8 *)addr - (uint8 *)vm_memory_);
  MemoryChunk *mchunk = NewMemoryChunk(offset, alignedsize, NULL);
  vm_memory_bank_->PutFreeChunk(mchunk);
}

void MemoryManager::ReleaseVariables(uint8_t *base, uint8_t *typetagged, uint8_t *refcounted, uint32_t size,
                                     bool reversed) {
  for (uint32_t i = 0; i < size; i++) {
    for (uint32_t j = 0; j < 8; j++) {
      uint32_t offset = (i * 8 + j) * 4;
      uint8_t *addr = reversed ? (base - offset - 4) : (base + offset);
      if (typetagged[i] & (1 << j)) {
        MIR_ASSERT(!(refcounted[i] & (1 << j)));
        // uint64_t val = load_64bits(addr);
        uint64_t val = *((uint64 *)addr);
        assert(false&&"NYI");
        // GCCheckAndDecRf(val, IsNeedRc(GetFlagFromMemory(addr)));
      } else if (refcounted[i] & (1 << j)) {
        uint64_t val = *(uint64_t *)(addr);
        GCDecRf((void *)val);
      } else {
        continue;
      }
    }
  }
}

#ifdef MM_DEBUG
// A VM self-check for memory-leak.
// When exit the app, stack-variables have been released. Only global-variables
// and builtin-variables may refer to a heap-object or a heap-string.
// If we release global-variables and builtin-variables here, there must be no heap
// memory used.
void MemoryManager::AppMemLeakCheck() {
#ifdef MEMORY_LEAK_CHECK

#if 0
  GCCheckAndDecRf(ginterpreter->retval0_.u64);
  ReleaseVariables(ginterpreter->gp_, ginterpreter->gp_typetagged_, ginterpreter->gp_refcounted_,
                   BlkSize2BitvectorSize(ginterpreter->gp_size_), false /* offset is positive */);
#endif

  printf("before release builtin, app_mem_usage= %u alloc= %u release= %u max= %u\n", app_mem_usage, mem_allocated, mem_released, max_app_mem_usage);

  __jsobj_release_builtin();

  printf("after release builtin, app_mem_usage= %u alloc= %u release= %u max= %u\n", app_mem_usage, mem_allocated, mem_released, max_app_mem_usage);

#ifdef MARK_CYCLE_ROOTS
  RecallCycle();
  RecallRoots(cycle_roots);
#else
  MarkAndSweep();
#endif
  printf("after reclaim, app_mem_usage= %u alloc= %u release= %u max= %u\n", app_mem_usage, mem_allocated, mem_released, max_app_mem_usage);

  if (app_mem_usage != 0) {
    printf("[Memory Manager] %d Bytes heap memory leaked!\n", app_mem_usage);
    MIR_FATAL("memory leak.\n");
  }
#endif
}

void MemoryManager::AppMemUsageSummary() {
  printf("\n");
  printf("[Memory Manager] max app heap memory usage is %d Bytes\n", app_mem_usage);
  printf("[Memory Manager] vm internal memory usage is %d Bytes\n\n",
         vm_free_small_offset_ + (vm_free_big_offset_ - vm_memory_small_size_));
}

void MemoryManager::AppMemAccessSummary() {
#ifndef RC_NO_MMAP
  printf("[Memory Manager] the max mmap node visiting length for VM APP access is %d \n", AddrMap::max_count);
  printf("[Memory Manager] the number of traversed mmap node for VM APP access is %d \n", AddrMap::total_count);
  printf("[Memory Manager] the visiting times of mmap for VM APP access is %d \n", AddrMap::find_count);
  printf("\n");
  printf("[Memory Manager] the number of used node in mmap is %d\n\n", AddrMap::nodes_count);
#endif
}

void MemoryManager::AppMemShapeSummary() {
#ifndef RC_NO_MMAP
  uint32_t i = 0, j = 0;
  AddrMap *mmap_table[3] = { NULL };
  mmap_table[0] = ginterpreter->GetHeapRefList();
  mmap_table[1] = ginterpreter->GetGlobalRefList();
  // when interpretation finishes, vmfunc_ is no longer available
  // mmap_table[2] = ginterpreter->vmfunc_->ref_list_;
  uint32_t max_length[3] = { 0, 0, 0 };
  uint32_t node_count[3];
  uint32_t efficiency[3];
  // for(i = 0; i < 3; i++) {
  for (i = 0; i < 2; i++) {
    AddrMap *map = mmap_table[i];
    if (map) {
      uint32_t free_num = 0;
      uint32_t node_num = 0;
      for (j = 0; j < map->hashtab_size; j++) {
        AddrMapNode *node = &(map->hashtab[j]);
        uint32_t list_len = 0;
        while (node) {
          node_num++;
          list_len++;
          if (MMAP_NODE_IS_FREE(node)) {
            free_num++;
          }
          node = node->next;
        }
        if (max_length[i] < list_len) {
          max_length[i] = list_len;
        }
      }
      node_count[i] = node_num;
      if (node_num != 0) {
        efficiency[i] = (uint32_t)(((double)(node_num - free_num) / (double)node_num) * 100);
      } else {
        efficiency[i] = 100;
      }
    }
  }
  if (mmap_table[0]) {
    printf("[Memory Manager] the number of node in heap_ref_list is %d\n", node_count[0]);
    printf("[Memory Manager] the use efficiency of heap_ref_list is %d percent\n", efficiency[0]);
    printf("[Memory Manager] the max length of the list in heap_ref_list hash slot is %d\n", max_length[0]);
    printf("\n");
  }
  if (mmap_table[1]) {
    printf("[Memory Manager] the number of node in global_ref_list is %d\n", node_count[1]);
    printf("[Memory Manager] the use efficiency of global_ref_list is %d percent\n", efficiency[1]);
    printf("[Memory Manager] the max length of the list in global_ref_list hash slot is %d\n", max_length[1]);
    printf("\n");
  }
  if (mmap_table[2]) {
    printf("[Memory Manager] the number of node in func_ref_list is %d\n", node_count[2]);
    printf("[Memory Manager] the use efficiency of func_ref_list is %d percent\n", efficiency[2]);
    printf("[Memory Manager] the max length of the list in func_ref_list hash slot is %d\n", max_length[2]);
    printf("\n");
  }
#endif
}

#endif  // MM_DEBUG

MemoryChunk *MemoryManager::NewMemoryChunk(uint32 offset, uint32 size, MemoryChunk *next) {
  MemoryChunk *new_chunk;
  if (free_memory_chunk_) {
    new_chunk = free_memory_chunk_;
    free_memory_chunk_ = new_chunk->next;
  } else {
    // new_chunk = (MemoryChunk *) MallocInternal(sizeof(MemoryChunk));
    if (size + vm_free_small_offset_ > vm_memory_small_size_) {
      MIR_FATAL("TODO run out of VM internal memory.\n");
    }
    new_chunk = (MemoryChunk *)((uint8 *)vm_memory_ + vm_free_small_offset_);
    vm_free_small_offset_ += Bytes4Align(sizeof(MemoryChunk));
  }
  new_chunk->offset_ = offset;
  new_chunk->size_ = size;
  new_chunk->next = next;
  return new_chunk;
}

void MemoryManager::Init(void *app_memory, uint32 app_memory_size, void *vm_memory, uint32 vm_memory_size) {
  memory_ = app_memory;
  memoryFlagMap = (uint8_t *)calloc((app_memory_size + vm_memory_size) / sizeof(void *), sizeof(uint8_t));
  total_small_size_ = app_memory_size / 2;
#if DEBUGGC
  assert((total_small_size_ % 4) == 0 && (VM_MEMORY_SIZE % 4) == 0 && "make sure HEAP SIZE is aligned to 4B");
#endif
  total_size_ = app_memory_size;
  heap_end = (void *)((uint8 *)memory_ + app_memory_size);
  vm_memory_ = vm_memory;
  vm_memory_size_ = vm_memory_size;
  vm_memory_small_size_ = VM_MEMORY_SMALL_SIZE;
  vm_free_small_offset_ = 0;
  vm_free_big_offset_ = vm_memory_small_size_;
  heap_free_small_offset_ = 0;

  heap_free_big_offset_ = total_small_size_;
  free_memory_chunk_ = NULL;
#ifndef RC_NO_MMAP
  free_mmaps_ = NULL;
  free_mmap_nodes_ = NULL;
#endif
  // avail_link_ = NewMemoryChunk(0, app_memory_size, NULL);
  heap_memory_bank_ = (MemoryHash *)((uint8 *)vm_memory_ + vm_free_small_offset_);
  uint32 alignedsize = Bytes4Align(sizeof(MemoryHash));
  vm_free_small_offset_ += alignedsize;
  errno_t mem_ret1 = memset_s(heap_memory_bank_, alignedsize, 0, alignedsize);
  if (mem_ret1 != EOK) {
    MIR_FATAL("call memset_s firstly failed in MemoryManager::Init");
  }
  vm_memory_bank_ = (MemoryHash *)((uint8 *)vm_memory_ + vm_free_small_offset_);
  vm_free_small_offset_ += alignedsize;
  errno_t mem_ret2 = memset_s(vm_memory_bank_, alignedsize, 0, alignedsize);
  if (mem_ret2 != EOK) {
    MIR_FATAL("call memset_s secondly failed in MemoryManager::Init");
  }

#if MM_DEBUG
  app_mem_usage = 0;
  max_app_mem_usage = 0;
  mem_allocated = 0;
  mem_released = 0;
#endif  // MM_DEBUG
#ifdef MACHINE64
  addrMap.push_back(NULL);
  addrOffset = addrMap.size();
#endif
}

void *MemoryManager::Malloc(uint32 size, bool init_p) {
  MemoryChunk *mchunk;
#if MM_DEBUG
  app_mem_usage += size;
  mem_allocated += size;
  if(app_mem_usage > max_app_mem_usage)
    max_app_mem_usage = app_mem_usage;
#endif  // MM_DEBUG
#if DEBUGGC
  assert((IsAlignedBy4(size)) && "memory doesn't align by 4 bytes");
#endif
  mchunk = heap_memory_bank_->GetFreeChunk(size);
  void *retmem = NULL;
  if (!mchunk) {
    uint32 heap_free_offset = 0;
    if (heap_memory_bank_->IsBigSize(size)) {  // for big size we merge the free chunk to see if we can get a big memory
      if (size + heap_free_big_offset_ > total_size_) {
        // try to merge some free chunk;
        heap_memory_bank_->MergeBigFreeChunk();
        mchunk = heap_memory_bank_->GetFreeChunk(size);
        if (!mchunk) {
          MIR_FATAL("run out of VM heap memory.\n");
        }
      } else {
        heap_free_offset = heap_free_big_offset_;
        heap_free_big_offset_ += size;
        return (void *)((uint8 *)memory_ + heap_free_offset);
      }
    } else {
      if (size + heap_free_small_offset_ > total_small_size_) {
        // try to use big size heap space if it has not been used
        if (heap_free_big_offset_ == total_small_size_ &&
            heap_free_big_offset_ < total_size_ - (1024 * 1024)) {
            total_small_size_ += 1024 * 1024;
            heap_free_big_offset_ = total_small_size_;
        } else {
          MIR_FATAL("TODO run out of VM heap memory.\n");
        }
      }
      heap_free_offset = heap_free_small_offset_;
      heap_free_small_offset_ += size;
      return (void *)((uint8 *)memory_ + heap_free_offset);
    }
  }
  retmem = (void *)((uint8 *)memory_ + mchunk->offset_);
  if (init_p) {
    errno_t ret = memset_s(retmem, size, 0, size);
    if (ret != EOK) {
      MIR_FATAL("call memset_s failed in MemoryManager::Malloc");
    }
  }
  DeleteMemoryChunk(mchunk);

#if DEBUGGC
  assert(IsAlignedBy4((uint32)retmem));
#endif
  return retmem;
}

void *MemoryManager::Realloc(void *origptr, uint32 origsize, uint32 newsize) {
#if DEBUGGC
  assert((IsAlignedBy4(origsize) && IsAlignedBy4(newsize)) && "memory doesn't align by 4 bytes");
#endif
  void *newptr = Malloc(newsize + MALLOCHEADSIZE);
  if (!newptr) {
    MIR_FATAL("out of memory");
  }
  if (newsize > origsize) {
    errno_t cpy_ret = memcpy_s(newptr, newsize + MALLOCHEADSIZE, (void *)((uint8 *)origptr - MALLOCHEADSIZE),
                               origsize + MALLOCHEADSIZE);
    if (cpy_ret != EOK) {
      MIR_FATAL("call memcpy_s failed in MemoryManager::Realloc");
    }
    errno_t set_ret =
      memset_s((void *)((uint8 *)newptr + origsize + MALLOCHEADSIZE), newsize + MALLOCHEADSIZE, 0, newsize - origsize);
    if (set_ret != EOK) {
      MIR_FATAL("call memcpy_s failed in MemoryManager::Realloc");
    }
  } else {
    errno_t cpy_ret =
      memcpy_s(newptr, newsize + MALLOCHEADSIZE, (void *)((uint8 *)origptr - MALLOCHEADSIZE), newsize + MALLOCHEADSIZE);
    if (cpy_ret != EOK) {
      MIR_FATAL("call memcpy_s failed in MemoryManager::Realloc");
    }
  }
  RecallMem(origptr, origsize);
  return newptr;
}

// actuall we need to recall mem - MALLOCHEADSIZE with size+MALLOCHEADSIZE
void MemoryManager::RecallMem(void *mem, uint32 size) {
  uint32 head_size = MALLOCHEADSIZE;

#ifndef MARK_CYCLE_ROOTS
  if (GetMemHeader(mem).memheadtag == MemHeadJSObj) {
    head_size = MALLOCEXHEADSIZE;
  }
#endif

  if (IsDebugGC()) {
    switch (GetMemHeader(mem).memheadtag) {
      case MemHeadAny:
        printf("unknown mem %p size %d is going to be released\n", mem, size);
        break;
      case MemHeadJSFunc:
        printf("jsfunc mem %p size %d is going to be released\n", mem, size);
        break;
      case MemHeadJSObj:
        printf("jsobj mem %p size %d is going to be released\n", mem, size);
        break;
      case MemHeadJSProp:
        printf("jsprop mem %p size %d is going to be released\n", mem, size);
        break;
      case MemHeadJSString:
        printf("jsstring mem %p size %d is going to be released\n", mem, size);
        break;
      case MemHeadJSList:
        printf("jsstring mem %p size %d is going to be released\n", mem, size);
        break;
      case MemHeadJSIter:
        printf("jsiter mem %p size %d is going to be released\n", mem, size);
        break;
      default:
        MIR_FATAL("unknown VM memory header value");
    }
  }
  uint32 alignedsize = Bytes4Align(size);
  uint32 offset = (uint32)((uint8 *)mem - (uint8 *)memory_ - head_size);
#if DEBUGGC
  assert(IsAlignedBy4(offset) && "not aligned by 4 bytes");
#endif
  MIR_ASSERT(offset < total_size_);

  MemoryChunk *mchunk = NewMemoryChunk(offset, alignedsize + head_size, NULL);
  // InsertMemoryChunk(mchunk);
  heap_memory_bank_->PutFreeChunk(mchunk);

#if MM_DEBUG
  app_mem_usage -= (alignedsize + head_size);
  mem_released += (alignedsize + head_size);
#endif  // MM_DEBUG
}

void MemoryManager::RecallString(__jsstring *str) {
  if (TurnoffGC())
    return;
#if MIR_DEX
#else  // !MIR_DEX
  // determine whether the string content is allocated in local heap.
  // Do not need recall the JSSTRING_BUILTIN, which is in constant pool;
  if (IsHeap((void *)str)) {
    if (IsDebugGC()) {
      printf("recollect a string in heap\n");
    }
    MemHeader &header = memory_manager->GetMemHeader((void *)str);
    if (header.refcount == 0) {
      RecallMem((void *)str, __jsstr_get_bytesize(str));
    }
  }
  else {
    if (IsDebugGC()) {
      printf("recollect a global string\n");
    }
  }
#endif  // MIR_DEX
        // RecallMem((void *)str, sizeof(__jsstring));
}

void MemoryManager::RecallArray_props(__jsvalue *array_props) {
  if (TurnoffGC())
    return;
  uint32_t length = __jsval_to_uint32(&array_props[0]);
  length = length > ARRAY_MAXINDEXNUM_INTERNAL ? ARRAY_MAXINDEXNUM_INTERNAL : length;
  for (uint32_t i = 0; i < length + 1; i++) {
    if (__is_js_object(&array_props[i]) || __is_string(&array_props[i])) {
// #ifndef RC_NO_MMAP
#if  0
      AddrMap *mmap = ginterpreter->GetHeapRefList();
      AddrMapNode *mmap_node = mmap->FindInAddrMap(&array_props[i].s.payload.ptr);
      MIR_ASSERT(mmap_node);
      mmap->RemoveAddrMapNode(mmap_node);
#endif
#if MACHINE64
      GCDecRf((array_props[i].s.ptr));
#else
      GCDecRf(array_props[i].s.payload.ptr);
#endif
    }
  }
  RecallMem((void *)array_props, (length + 1) * sizeof(__jsvalue));
}

void MemoryManager::RecallList(__json_list *list) {
  if (TurnoffGC())
    return;
  uint32_t count = list->count;
  __json_node *node = list->first;
  for (uint32_t i = 0; i < count; i++) {
    GCCheckAndDecRf(node->value.s.asbits, IsNeedRc((node->value.tag)));
    VMFreeNOGC(node, sizeof(__json_node));
    node = node->next;
  }
  RecallMem((void *)list, sizeof(__json_list));
}

// decrease the memory, recall it if necessary
void MemoryManager::GCDecRf(void *addr) {
  if (TurnoffGC())
    return;
  if (!IsHeap(addr)) {
    return;
  }
  MemHeader &header = GetMemHeader(addr);
  MIR_ASSERT(header.refcount > 0);  // must > 0
  if(header.refcount < UINT14_MAX)
    header.refcount--;
  // DEBUG
  // printf("address: 0x%x   rf - to: %d\n", addr, header.refcount);
  if (header.refcount == 0) {
    switch (header.memheadtag) {
      case MemHeadJSObj: {
        __jsobject *obj = (__jsobject *)addr;
        ManageObject(obj, RECALL);
        return;
      }
      case MemHeadJSString: {
        __jsstring *str = (__jsstring *)addr;
        RecallString(str);
        return;
      }
      case MemHeadEnv: {
        ManageEnvironment(addr, RECALL);
        return;
      }
      case MemHeadJSIter: {
        MemHeader &header = memory_manager->GetMemHeader(addr);
        RecallMem(addr, sizeof(__jsiterator));
        return;
      }
      default:
        MIR_FATAL("unknown GC object type");
    }
  }
}

// #ifndef RC_NO_MMAP
#if 0
AddrMap *MemoryManager::GetMMapValList(void *plshval) {
  VMMembank membank = ginterpreter->GetMemBank(plshval);
  switch (membank) {
    case VMMEMHP:  // on heap
      return ginterpreter->GetHeapRefList();
    case VMMEMGP:
      return ginterpreter->GetGlobalRefList();
    case VMMEMST:
      assert(false && "NYI");
      return ginterpreter->vmfunc_->ref_list_;
    default:
      MIR_FATAL("unknown memory bank type");
      return NULL;
  }
}

// this function is called when we write rshval into plshval
// TODO: We should make sure the type of right value is dynamic-type before call IsMvalObject&IsMvalString.
void MemoryManager::UpdateGCReference(void *plshval, Mval rshval) {
  // MIR_ASSERT(IsMvalObject(rshval) || IsMvalString(rshval));
  AddrMap *ref_list = GetMMapValList(plshval);
  AddrMapNode *val_it = ref_list->FindInAddrMap(plshval);
  if (val_it) {
    void *rptr = val_it->ptr2;
    if (IsMvalObject(rshval) || IsMvalString(rshval) || IsMvalEnv(rshval)) {
      (*val_it).ptr2 = rshval.asptr;
      // rshval Inc
      GCIncRf(rshval.asptr);
    } else {
      // remove the memory map node
      ref_list->RemoveAddrMapNode(val_it);
      // remove_mmap_node(ref_list, val_it);
    }
    GCDecRf(rptr);  // decrease the reference counting after the increase
                    // in case store the same object into the same address
                    // then it won't recall the object if referece down to zero
  } else {
    if (IsMvalObject(rshval) || IsMvalString(rshval) || IsMvalEnv(rshval)) {
      ref_list->AddAddrMapNode(plshval, rshval.asptr);
      GCIncRf(rshval.asptr);
    }
  }
}

#endif

#if MIR_FEATURE_FULL && MIR_DEBUG
void MemoryManager::DebugMemory() {
  printf("heap big offset: %d\n", heap_free_big_offset_);
  printf("heap small offset: %d\n", heap_free_small_offset_);
  printf("heap memory table:\n");
  heap_memory_bank_->Debug();
  printf("vm memory offset: small offset = %d, big offset = %d\n", vm_free_small_offset_, vm_free_big_offset_);
  printf("vm memory table:\n");
  vm_memory_bank_->Debug();
  printf("chunk memory usage:\n");
  for (MemoryChunk *chunk = free_memory_chunk_; chunk; chunk = chunk->next) {
    printf("memory offset:%u, memory size:%u\n", chunk->offset_, chunk->size_);
  }
}

#endif  // MIR_FEATURE_FULL

#if DEBUGGC
// check if ptr is a legal address
bool MemoryManager::CheckAddress(void *ptr) {
  if (IsHeap(ptr)) {
    uint32 offset = (uint8 *)ptr - (uint8 *)memory_;
    return heap_memory_bank_->CheckOffset(offset);
  }
  return false;
}

#endif

//#ifndef RC_NO_MMAP
#if 0
AddrMap *MemoryManager::NewAddrMap(uint32_t size, uint32_t mask) {
  AddrMap *mmap;
  if (free_mmaps_) {
    mmap = free_mmaps_;
    free_mmaps_ = mmap->next;
  } else {
    mmap = (AddrMap *)VMMallocNOGC(sizeof(AddrMap));
    MIR_ASSERT(mmap);
  }
  mmap->next = NULL;
  mmap->hashtab = (AddrMapNode *)VMMallocNOGC(size * sizeof(AddrMapNode));
  memset_s((void *)(mmap->hashtab), size * sizeof(AddrMapNode), 0, size * sizeof(AddrMapNode));
  mmap->hashtab_idx_mask = mask;
  mmap->hashtab_size = size;
  return mmap;
}

AddrMapNode *MemoryManager::NewAddrMapNode() {
  AddrMapNode *node;
  if (free_mmap_nodes_) {
    node = free_mmap_nodes_;
    free_mmap_nodes_ = node->next;
    node->next = NULL;
    MMAP_NODE_SET_FREE(node);
  } else {
    node = (AddrMapNode *)VMMallocNOGC(sizeof(AddrMapNode));
  }
  return node;
}

void MemoryManager::FreeAddrMap(AddrMap *map) {
  MIR_ASSERT(map);
  map->next = free_mmaps_;
  free_mmaps_ = map;

  // add all chained mmap nodes to free nodes lists
  uint32_t i;
  for (i = 0; i < map->hashtab_size; i++) {
    AddrMapNode *node = map->hashtab[i].next;
    while (node) {
      // TODO: we can find the last node and add the whole chain
      AddrMapNode *next_node = node->next;
      node->next = free_mmap_nodes_;
      free_mmap_nodes_ = node;
      node = next_node;
    }
  }

  VMFreeNOGC(map->hashtab, map->hashtab_size * sizeof(AddrMapNode));
  map->hashtab = NULL;
  map->hashtab_idx_mask = 0;
  map->hashtab_size = 0;
}

void MemoryManager::UpdateAddrMap(uint32_t *origptr, uint32_t *newptr, uint32_t old_len, uint32_t new_len) {
  AddrMap *mmap = ginterpreter->GetHeapRefList();
  uint32_t len = new_len >= old_len ? old_len : new_len;
  for (uint32_t i = 0; i < len; i++) {
    AddrMapNode *mmap_node = mmap->FindInAddrMap((void *)origptr[i]);
    if (mmap_node != NULL) {
      mmap->AddAddrMapNode((void *)newptr[i], mmap_node->ptr2);
      mmap->RemoveAddrMapNode(mmap_node);
    }
  }
  for (uint32_t i = new_len; i < old_len; i++) {
    AddrMapNode *mmap_node = mmap->FindInAddrMap((void *)origptr[i]);
    if (mmap_node != NULL) {
      mmap->RemoveAddrMapNode(mmap_node);
    }
  }
}

#endif

#ifdef MARK_CYCLE_ROOTS
void MemoryManager::AddCycleRootNode(CycleRoot **roots_head, __jsobject *obj) {
  CycleRoot *root = (CycleRoot *)VMMallocGC(sizeof(CycleRoot));
  root->obj = obj;
  root->next = *roots_head;
  if (*roots_head) {
    (*roots_head)->pre = root;
  }
  *roots_head = root;
  (*roots_head)->pre = NULL;
}

void MemoryManager::DeleteCycleRootNode(CycleRoot *root) {
  if (root->pre) {
    root->pre->next = root->next;
  } else {
    cycle_roots = root->next;
  }
  if (root->next) {
    root->next->pre = root->pre;
  }
  RecallMem(root, sizeof(CycleRoot));
}

void MemoryManager::ManageChildObj(__jsobject *obj, ManageType flag) {
  if (!obj) {
    return;
  }
  if (flag == DECREASE) {
    if (!(--GetMemHeader(obj).refcount)) {
      if (GetMemHeader(obj).is_root && GetMemHeader(obj).is_decreased) {
        return;
      }
      ManageObject(obj, flag);
    }
  } else if (flag == RESTORE) {
    if (++GetMemHeader(obj).refcount == 1) {
      ManageObject(obj, flag);
    }
  } else if (flag == COLLECT) {
    if ((!GetMemHeader(obj).refcount) && (!GetMemHeader(obj).is_collected)) {
      AddCycleRootNode(&garbage_roots, obj);
      GetMemHeader(obj).is_collected = true;
      ManageObject(obj, flag);
    }
  } else if (flag == RECALL) {
    GCDecRf(obj);
  } else {
    return;
  }
}

void MemoryManager::ManageJsvalue(__jsvalue *val, ManageType flag) {
  if (flag == SWEEP || is_sweep) {
    // if the val is an object, then do nothing
    if (__is_string(val)) {
#ifdef MACHINE64
      GCDecRf((val->s.ptr));
#else
      GCDecRf(val->s.ptr);
#endif
    }
  } else if (flag == RECALL) {
    GCCheckAndDecRf(val->s.asbits, IsNeedRc(val->tag));
  } else {
    if (__is_js_object(val)) {
#ifdef MACHINE64
      ManageChildObj((val->s.obj), flag);
#else
      ManageChildObj(val->s.obj, flag);
#endif
    }
  }
}

void MemoryManager::ManageEnvironment(void *envptr, ManageType flag) {
  if (!envptr) {
    return;
  }
#ifdef MACHINE64
  assert(false && "NYI");
#else
  uint32 *u32envptr = (uint32 *)envptr;
  Mval *mvalptr = (Mval *)envptr;
  void *parentenv = (void *)u32envptr[1];
  if (parentenv) {
    if (flag != RECALL) {
      ManageEnvironment(parentenv, flag);
    } else {
      GCDecRf(parentenv);
    }
  }
  uint32 argnums = u32envptr[0];
  uint32 totalsize = sizeof(uint32) + sizeof(void *);  // basic size
  for (uint32 i = 1; i <= argnums; i++) {
    __jsvalue val = MvalToJsval(mvalptr[i]);
    ManageJsvalue(&val, flag);
    totalsize += sizeof(Mval);
  }
  if (flag == RECALL) {
    RecallMem(envptr, totalsize);
  }
#endif
}

void MemoryManager::ManageProp(__jsprop *prop, ManageType flag) {
  __jsprop_desc desc = prop->desc;
  if (__has_value(desc)) {
    __jsvalue jsvalue = __get_value(desc);
    ManageJsvalue(&jsvalue, flag);
  }
  if (__has_set(desc)) {
    ManageChildObj(__get_set(desc), flag);
  }
  if (__has_get(desc)) {
    ManageChildObj(__get_get(desc), flag);
  }
  if (flag == SWEEP || flag == RECALL) {
    RecallMem((void *)prop, sizeof(__jsprop));
  }
}

void MemoryManager::ManageObject(__jsobject *obj, ManageType flag) {
  if (!obj) {
    return;
  }
  __jsprop *jsprop = obj->prop_list;
  while (jsprop) {
    __jsprop *next_jsprop = jsprop->next;
    ManageProp(jsprop, flag);
    jsprop = next_jsprop;
  }

  if (!obj->proto_is_builtin) {
    ManageChildObj(obj->prototype.obj, flag);
  }

  switch (obj->object_class) {
    case JSSTRING:
      if (flag == SWEEP || flag == RECALL) {
        GCDecRf(obj->shared.prim_string);
      }
      break;
    case JSARRAY:
      if (obj->object_type == JSREGULAR_ARRAY) {
        __jsvalue *array = obj->shared.array_props;
        uint32 arrlen = (uint32_t)__jsval_to_number(&array[0]);
        for (uint32_t i = 0; i < arrlen; i++) {
          __jsvalue jsvalue = obj->shared.array_props[i + 1];
          ManageJsvalue(&jsvalue, flag);
        }
        if (flag == SWEEP || flag == RECALL) {
          uint32_t size = (arrlen + 1) * sizeof(__jsvalue);
          RecallMem((void *)obj->shared.array_props, size);
        }
      }
      break;
    case JSFUNCTION:
      // Some builtins may not have a function.
      if (!obj->is_builtin || obj->shared.fun) {
        // Release bound function first.
        __jsfunction *fun = obj->shared.fun;
        if (fun->attrs & JSFUNCPROP_BOUND) {
          ManageChildObj((__jsobject *)(fun->fp), flag);
          uint32_t bound_argc = ((fun->attrs >> 16) & 0xff);
          __jsvalue *bound_args = (__jsvalue *)fun->env;
          for (uint32_t i = 0; i < bound_argc; i++) {
            __jsvalue val = bound_args[i];
            ManageJsvalue(&val, flag);
          }
          if (flag == SWEEP || flag == RECALL) {
            RecallMem(fun->env, bound_argc * sizeof(__jsvalue));
          }
        } else {
          if ((flag != SWEEP) && (flag != RECALL)) {
            ManageEnvironment(fun->env, flag);
          } else {
            GCDecRf(fun->env);
          }
        }
        if (flag == SWEEP || flag == RECALL) {
          RecallMem(fun, sizeof(__jsfunction));
        }
      }
      break;
    case JSARRAYBUFFER: {
      __jsarraybyte *arrayByte = obj->shared.arrayByte;
      RecallMem((void *)arrayByte, sizeof(uint8_t) * __jsval_to_number(&arrayByte->length));
    }
    break;
    case JSDATAVIEW:
    case JSOBJECT:
    case JSBOOLEAN:
    case JSNUMBER:
    case JSGLOBAL:
    case JSON:
    case JSMATH:
    case JSDOUBLE:
    case JSREGEXP:
    case JSDATE:
    case JSINTL:
    case JSARGUMENTS:
      break;
    default:
      MIR_FATAL("manage unknown js object class");
  }
  if (flag == SWEEP || flag == RECALL) {
    if (obj->prop_index_map)
      delete(obj->prop_index_map);
    if (obj->prop_string_map)
      delete(obj->prop_string_map);
    RecallMem((void *)obj, sizeof(__jsobject));
  }
}

void MemoryManager::RecallRoots(CycleRoot *root) {
  if (TurnoffGC())
    return;
  if (!root) {
    return;
  }
  while (root) {
    CycleRoot *cur_root = root;
    root = root->next;
    RecallMem(cur_root, sizeof(CycleRoot));
  }
}

void MemoryManager::ResetCycleRoots() {
  if (!cycle_roots) {
    return;
  }
  CycleRoot *root = cycle_roots;
  while (root) {
    GetMemHeader(root->obj).is_decreased = false;
    GetMemHeader(root->obj).need_restore = false;
    GetMemHeader(root->obj).is_collected = false;
    root = root->next;
  }
}

void MemoryManager::RecallCycle() {
  if (!cycle_roots) {
    return;
  }
  CycleRoot *root = cycle_roots;
  // Decrease cycle_roots
  while (root) {
    // this must not be a cycle header, so delete the node
    if (!GetMemHeader(root->obj).refcount) {
      DeleteCycleRootNode(root);
    } else {
      GetMemHeader(root->obj).is_decreased = true;
      ManageObject(root->obj, DECREASE);
    }
    root = root->next;
  }
  root = cycle_roots;
  // After decrement, if the refcount of a cycle-root object is greater than 0,
  // then it needs to be restored.
  bool need_restore = false;
  while (root) {
    if (GetMemHeader(root->obj).refcount) {
      GetMemHeader(root->obj).need_restore = true;
      need_restore = true;
    }
    root = root->next;
  }
  root = cycle_roots;
  // Restore cycle_roots
  if (need_restore) {
    while (root) {
      if (GetMemHeader(root->obj).need_restore) {
        ManageObject(root->obj, RESTORE);
      }
      root = root->next;
    }
  }
  root = cycle_roots;
  // After decrement and restoring, if the refcount of a cycle-root object is 0, then this object
  // is a garbage cycle header. Trace this header to collect garbage objects in this cycle.
  while (root) {
    CycleRoot *next_root = root->next;  // in case the current root is deleted after collected
    if (!GetMemHeader(root->obj).refcount) {
      if (!GetMemHeader(root->obj).is_collected) {
        AddCycleRootNode(&garbage_roots, root->obj);
        GetMemHeader(root->obj).is_collected = true;
        ManageObject(root->obj, COLLECT);
      }
      DeleteCycleRootNode(root);
    }
    root = next_root;
  }
  root = garbage_roots;
  // Sweep garbage-cycle
  is_sweep = true;
  while (root) {
    ManageObject(root->obj, SWEEP);
    root = root->next;
  }
  is_sweep = false;
  ResetCycleRoots();
  RecallRoots(garbage_roots);
  garbage_roots = NULL;
}

#else
void MemoryManager::ManageChildObj(__jsobject *obj, ManageType flag) {
  if (!obj) {
    return;
  }
  if (flag == MARK) {
    ManageObject(obj, MARK);
  } else if (flag == DECRF) {
    GetExMemHeader((void *)obj).gc_refs--;
  } else if (flag == RECALL) {
    GCDecRf(obj);
  } else if (GetExMemHeader((void *)obj).marked) {
    GCDecRf(obj);
  }
}

void MemoryManager::ManageJsvalue(__jsvalue *val, ManageType flag) {
  if (flag == MARK || flag == DECRF) {
    if (__is_js_object(val)) {
#ifdef MACHINE64
      ManageChildObj((val->s.obj), flag);
#else
      ManageChildObj(val->s.obj, flag);
#endif
    }
  } else if (is_sweep && NotMarkedObject(val)) {
    // do nothing
  } else {
// #ifdef RC_NO_MMAP
#if 1
    GCCheckAndDecRf(val->asbits);
#else
    AddrMap *mmap = ginterpreter->GetHeapRefList();
    AddrMapNode *mmap_node = mmap->FindInAddrMap(&val->s.payload.ptr);
    MIR_ASSERT(mmap_node);
    mmap->RemoveAddrMapNode(mmap_node);
    // remove_mmap_node(mmap, mmap_node);
    GCCheckAndDecRf(val->payload.asbits);
#endif
  }
}

void MemoryManager::ManageEnvironment(void *envptr, ManageType flag) {
  /*
     type $bar_env_type <struct {
     @argnums u32,
     @parentenv <* void>,
     @arg1 dynany,
     @arg2 dynany,
     ....
     @argn dynany}
   */
  // get the argnums
  // not that the following code can only work on 32-bits machine
  if (!envptr) {
    return;
  }
  assert(false&&"NYI");
  /*
  uint32 *u32envptr = (uint32 *)envptr;
  Mval *mvalptr = (Mval *)envptr;
  void *parentenv = (void *)u32envptr[1];
  if (parentenv) {
    if (flag == MARK || flag == DECRF) {
      ManageEnvironment(parentenv, flag);
    } else {
      GCDecRf(parentenv);
    }
  }
  uint32 argnums = u32envptr[0];
  uint32 totalsize = sizeof(uint32) + sizeof(void *);  // basic size
  for (uint32 i = 1; i <= argnums; i++) {
    __jsvalue val = MvalToJsval(mvalptr[i]);
    ManageJsvalue(&val, flag);
    totalsize += sizeof(Mval);
  }
  if (flag == RECALL || flag == SWEEP) {
    RecallMem(envptr, totalsize);
  }
  */
}

void MemoryManager::ManageProp(__jsprop *prop, ManageType flag) {
  __jsprop_desc desc = prop->desc;
  if (__has_value(desc)) {
    __jsvalue jsvalue = __get_value(desc);
    ManageJsvalue(&jsvalue, flag);
  }
  if (__has_set(desc)) {
    ManageChildObj(__get_set(desc), flag);
  }
  if (__has_get(desc)) {
    ManageChildObj(__get_get(desc), flag);
  }

  if (flag == RECALL || flag == SWEEP) {
    GCDecRf(prop->name);
    RecallMem((void *)prop, sizeof(__jsprop));  // recall the prop
  }
}

void MemoryManager::ManageObject(__jsobject *obj, ManageType flag) {
  if (flag == MARK) {
    ExMemHeader &exheader = GetExMemHeader((void *)obj);
    if (exheader.marked) {
      return;
    } else {
      exheader.marked = true;
    }
  }

  __jsprop *jsprop = obj->prop_list;
  while (jsprop) {
    __jsprop *next_jsprop = jsprop->next;
    ManageProp(jsprop, flag);
    jsprop = next_jsprop;
  }

  if (!obj->proto_is_builtin) {
    ManageChildObj(obj->prototype.obj, flag);
  }

  switch (obj->object_class) {
    case JSSTRING:
      if (flag == RECALL || flag == SWEEP) {
        GCDecRf(obj->shared.prim_string);
      }
      break;
    case JSARRAY:
      /*TODO : Be related to VMReallocGC, AddrMapNode information will miss after VMReallocGC.  */
      if (obj->object_type == JSREGULAR_ARRAY) {
        __jsvalue *array = obj->shared.array_props;
        uint32 arrlen = (uint32_t)__jsval_to_number(&array[0]);
        for (uint32_t i = 0; i < arrlen; i++) {
          __jsvalue jsvalue = obj->shared.array_props[i + 1];
          ManageJsvalue(&jsvalue, flag);
        }
        if (flag == RECALL || flag == SWEEP) {
          uint32_t size = (arrlen + 1) * sizeof(__jsvalue);
          RecallMem((void *)obj->shared.array_props, size);
        }
      }
      break;
    case JSFUNCTION:
      // Some builtins may not have a function.
      if (!obj->is_builtin || obj->shared.fun) {
        // Release bound function first.
        __jsfunction *fun = obj->shared.fun;
        if (fun->attrs & JSFUNCPROP_BOUND) {
          ManageChildObj((__jsobject *)(fun->fp), flag);
          uint32_t bound_argc = ((fun->attrs >> 16) & 0xff);
          __jsvalue *bound_args = (__jsvalue *)fun->env;
          for (uint32_t i = 0; i < bound_argc; i++) {
            __jsvalue val = bound_args[i];
            ManageJsvalue(&val, flag);
          }
          if (flag == RECALL || flag == SWEEP) {
            RecallMem(fun->env, bound_argc * sizeof(__jsvalue));
          }
        } else {
          if (flag == MARK || flag == DECRF) {
            ManageEnvironment(fun->env, flag);
          } else {
            GCDecRf(fun->env);
          }
        }
        if (flag == RECALL || flag == SWEEP) {
          RecallMem(fun, sizeof(__jsfunction));
        }
      }
      break;
    case JSOBJECT:
    case JSBOOLEAN:
    case JSNUMBER:
    case JSGLOBAL:
    case JSON:
    case JSMATH:
      break;
    default:
      MIR_FATAL("manage unknown js object class");
  }
  if (flag == RECALL || flag == SWEEP) {
    DeleteObjListNode(obj);
    RecallMem((void *)obj, sizeof(__jsobject));
  }
}

bool MemoryManager::NotMarkedObject(__jsvalue *val) {
  bool not_marked_obj = true;
  if (__is_js_object(val)) {
#ifdef MACHINE64
    assert(false && "NYI");
#else
    not_marked_obj = !(GetExMemHeader((void *)val->payload.ptr).marked);
#endif
  }
  return not_marked_obj;
}

void MemoryManager::Mark() {
  __jsobject *obj = obj_list;
  while (obj) {
    ExMemHeader &exheader = GetExMemHeader((void *)obj);
    if (exheader.gc_refs > 0) {
      ManageObject(obj, MARK);
    }
    obj = exheader.next;
  }
}

/* steps to find garbage reference cycles:
   1.For each container object, set gc_refs equal to the object's reference count.
   2.For each container object, find which container objects it references and decrement
     the referenced container's gc_refs field.
   3.All container objects that now have a gc_refs field greater than zero are referenced
     from outside the set of container objects. We cannot free these objects so we set the
     marked flag of them to true. Any objects referenced from the objects marked also cannot
     be freed. We mark them and all the objects reachable from them too.
   4.Objects unmarked in the obj_list are only referenced by objects within cycles (ie. they
     are inaccessible and are garbage). We can now go about freeing these objects. */
void MemoryManager::DetectRCCycle() {
  __jsobject *obj = obj_list;
  while (obj) {
    ExMemHeader &exheader = GetExMemHeader((void *)obj);
    uint16_t refcount = exheader.refcount;
    // in case a created object is unmaintained with RC befor invoking of mark-and-sweep
    if (refcount == 0) {
      ManageObject(obj, MARK);
    } else {
      exheader.gc_refs = refcount;
    }
    obj = exheader.next;
  }
  obj = obj_list;
  while (obj) {
    ExMemHeader &exheader = GetExMemHeader((void *)obj);
    if (!exheader.marked) {
      ManageObject(obj, DECRF);
    }
    obj = exheader.next;
  }
}

void MemoryManager::Sweep() {
  is_sweep = true;
  __jsobject *obj = obj_list;
  while (obj) {
    ExMemHeader &exheader = GetExMemHeader((void *)obj);
    if (exheader.marked) {
      exheader.marked = false;
      obj = exheader.next;
    } else {
      __jsobject *pre = exheader.pre;
      ManageObject(obj, SWEEP);
      if (pre) {
        obj = GetExMemHeader((void *)pre).next;
      } else {
        obj = obj_list;
      }
    }
  }
  is_sweep = false;
}

void MemoryManager::AddObjListNode(__jsobject *obj) {
  GetExMemHeader((void *)obj).next = obj_list;
  if (obj_list) {
    GetExMemHeader((void *)obj_list).pre = obj;
  }
  obj_list = obj;
  GetExMemHeader((void *)obj_list).pre = NULL;
}

void MemoryManager::DeleteObjListNode(__jsobject *obj) {
  ExMemHeader &exheader = GetExMemHeader((void *)obj);
  if (exheader.pre) {
    GetExMemHeader((void *)(exheader.pre)).next = exheader.next;
  } else {
    obj_list = exheader.next;
  }
  if (exheader.next) {
    GetExMemHeader((void *)(exheader.next)).pre = exheader.pre;
  }
}

void MemoryManager::MarkAndSweep() {
  DetectRCCycle();
  Mark();
  Sweep();
}

#endif

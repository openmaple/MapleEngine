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

#ifndef MAPLEBE_INCLUDE_MAPLEVM_VMMEMORY_H_
#define MAPLEBE_INCLUDE_MAPLEVM_VMMEMORY_H_

#include "types_def.h"
#include "jsobject.h"
#include "mval.h"
#include "vmmmap.h"
#include "vmconfig.h"
#include "json.h"
#include <vector>
#include <map>
using namespace maple;

#define MEMHASHTABLESIZE 0x100

enum MemHeadTag {
  MemHeadAny,
  MemHeadJSFunc,
  MemHeadJSObj,
  MemHeadJSString,
  MemHeadJSProp,
  MemHeadJSList,
  MemHeadEnv,
  MemHeadJSIter,
};

#ifdef MARK_CYCLE_ROOTS
/* Enumeration flags to distinguish different management of object, prop and environment.
   DECREASE for detecting garbage reference cycles by decrement of the reference count.
   RESTORE for restoring the reference count of non-garbage objects.
   COLLECT for collecting the objects in garbage cycles.
   SWEEP for releasing the garbage reference cycles.
   RECALL for normal garbage collection when reference count reduce to zero. */
enum ManageType { DECREASE, RESTORE, COLLECT, SWEEP, RECALL };

// struct to maintain cycle roots
struct CycleRoot {
  struct CycleRoot *pre;
  struct CycleRoot *next;
  __jsobject *obj;
};
#else
/* Enumeration flags to distinguish different management of object, prop and environment.
   MARK for setting the marked flag of object to true.
   SWEEP for releasing the garbage reference cycles.
   RECALL for normal garbage collection when reference count reduce to zero.
   DECRF for detecting garbage reference cycles by decrement of the reference count. */
enum ManageType {
  DECRF,
  MARK,
  SWEEP,
  RECALL,
};
#endif

struct MemHeader {
  uint16 refcount : 14;
  MemHeadTag memheadtag : 3;
#ifdef MARK_CYCLE_ROOTS
  bool is_root : 1;
  bool is_decreased : 1;
  bool need_restore : 1;
  bool is_collected : 1;
  uint16 : 11;
#else
  uint16 : 15;
#endif
};

#define MALLOCHEADSIZE sizeof(MemHeader)  // 4 bytes reserved for GC information

#ifndef MARK_CYCLE_ROOTS
// extended memory header for objects when using mark-sweep method to recall garbage cycles
struct ExMemHeader {
  __jsobject *pre;
  __jsobject *next;
  uint16 refcount : 14;
  MemHeadTag memheadtag : 3;
  uint16 gc_refs : 14;
  bool marked : 1;
};

#define MALLOCEXHEADSIZE sizeof(ExMemHeader)  // 12 bytes reserved for GC information
#endif

struct MemoryChunk {
  uint32 offset_;
  uint32 size_;
  MemoryChunk *next;
};

#define UINT14_MAX 0x3fff

class MemoryHash {
 public:
  MemoryChunk *table_[MEMHASHTABLESIZE];

 public:
  MemoryHash() {}

  MemoryChunk *GetFreeChunk(uint32);  // get the free chunk and release it
  void PutFreeChunk(MemoryChunk *);
  void MergeBigFreeChunk();
  bool IsBigSize(uint32 size) {
    return ((size >> 2) >= MEMHASHTABLESIZE);
  }

#if MIR_FEATURE_FULL || MIR_DEBUG
  void Debug();
#endif
#if DEBUGGC
  bool CheckOffset(uint32);
#endif
};

class MemoryManager {
 public:
  void *memory_;       // points to the base of the app's heap
  void *gpMemory;      // points to the global memory
  uint32 total_size_;  // total memory size of the app's heap
  uint32 total_small_size_;
  uint32 heap_free_small_offset_;
  uint32 heap_free_big_offset_;   // for gc
  MemoryHash *heap_memory_bank_;  // for app need gc
// MemoryChunk *avail_link_;
#if MM_DEBUG                 // this macro control the debug informaiton of memory manager
  uint32 max_app_mem_usage;  // the max heap memory used by current app.
  void AppMemLeakCheck();
  void AppMemUsageSummary();   // output the memory usage summary of an app.
  void AppMemAccessSummary();  // output the mmap node visiting length summary of an app.
  void AppMemShapeSummary();   // output the mmap shape summary of an app.
#endif                         // MM_DEBUG

  // Internal VM memory management
  void *vm_memory_;                 // points to the base of the VM's own dynamic memory space
  uint32 vm_memory_size_;           // internal memory size
  uint32 vm_memory_small_size_;     // internal memory small size, it's a fence before vm_memory_size_
  uint32 vm_free_small_offset_;     // small memory offset of next free VM memory
  uint32 vm_free_big_offset_;       // big memory offset
  MemoryHash *vm_memory_bank_;      // for vm itself, no need gc
  MemoryChunk *free_memory_chunk_;  // for memory chunk descriptor only, reusable
#ifndef RC_NO_MMAP
  AddrMap *free_mmaps_;           // a link list of mmaps for reuse.
  AddrMapNode *free_mmap_nodes_;  // a link list of mmap node for reuse.
#endif
#if MACHINE64
  uint32 addrOffset;
  // std::map<uint32, void *> addrMap;  // map the 32 bits address to 64 bits, for 64-bits machine only
  std::vector<void *> addrMap;  // map the 32 bits address to 64 bits, for 64-bits machine only
  std::vector<double> f64ToU32Vec;  // index to double value
  // std::map<double, uint32_t> f64ToU32Map; // map the double value to index
  uint8_t *memoryFlagMap;
  uint8_t *gpMemoryFlagMap;
  const uint8_t spBaseFlag = (__jstype)JSTYPE_SPBASE;
  const uint8_t fpBaseFlag = (__jstype)JSTYPE_FPBASE;
  const uint8_t gpBaseFlag = (__jstype)JSTYPE_GPBASE;
#endif

 public:
  MemoryManager() {}  // initialization delayed to Init();

  // app_memory_ptr, app_memory_size, vm_memory_ptr, vm_memory_size
  void Init(void *, uint32, void *, uint32);
  // malloc for VM management, need to manage life-cycle by user
  void *MallocInternal(uint32);
  void FreeInternal(void *, uint32);

  void ReleaseVariables(uint8_t *base, uint8_t *typetagged, uint8_t *refcounted, uint32_t size, bool reversed);
  // malloc for application objects, reference counted
  void *Malloc(uint32 size, bool init_p = true);
  void *Realloc(void *, uint32, uint32);
  MemHeader &GetMemHeader(void *memory) {
    uint32 *u32memory = (uint32 *)(memory);
    MemHeader *header_ptr = (MemHeader *)(u32memory - 1);
    return *header_ptr;
  }

#ifndef MARK_CYCLE_ROOTS
  ExMemHeader &GetExMemHeader(void *memory) {
    uint32 *u32memory = (uint32 *)(memory);
    ExMemHeader *exheader_ptr = (ExMemHeader *)(u32memory - 3);
    return *exheader_ptr;
  }

#endif
  MemoryChunk *NewMemoryChunk(uint32 offset, uint32 size, MemoryChunk *next);
  void DeleteMemoryChunk(MemoryChunk *chunk) {
    // add to head of free memory chunk descriptor list
    chunk->next = free_memory_chunk_;
    free_memory_chunk_ = chunk;
  }

  void RecallMem(void *, uint32);
  // Fixme: These recall functions should not be defined here.
  // Move these to jsobjet.cpp.
  void RecallString(__jsstring *);
  void RecallArray_props(__jsvalue *);
  void RecallList(__json_list *);

  // function for garbage collection
  void ManageChildObj(__jsobject *obj, ManageType flag);
  void ManageJsvalue(__jsvalue *val, ManageType flag);
  void ManageEnvironment(void *envptr, ManageType flag);
  void ManageProp(__jsprop *prop, ManageType flag);
  void ManageObject(__jsobject *obj, ManageType flag);
#ifdef MARK_CYCLE_ROOTS
  void AddCycleRootNode(CycleRoot **roots_head, __jsobject *obj);
  void DeleteCycleRootNode(CycleRoot *root);
  void ResetCycleRoots();
  void RecallRoots(CycleRoot *root);
  void RecallCycle();
#else
  void AddObjListNode(__jsobject *obj);
  void DeleteObjListNode(__jsobject *obj);
  void DetectRCCycle();
  void Mark();
  void Sweep();
  void MarkAndSweep();
  bool NotMarkedObject(__jsvalue *val);
#endif

// GC related
#ifndef RC_NO_MMAP
  void UpdateGCReference(void *, Mval);
#endif
  void GCDecRf(void *);
  void GCDecRfJsvalue(__jsvalue jsval) {
    if (TurnoffGC())
      return;
    __jstype tag = (__jstype)jsval.tag;
    switch (tag) {
      case JSTYPE_OBJECT:
      case JSTYPE_STRING:
        GCDecRf((jsval.s.obj));
      default:
        assert(false && "unexpected");
    }
  }
  // Decrease reference-counting but not recall the memory.
  // Sometimes we need inc-rf and dec-rf pairs to protect a heap-object(or string),
  // But the heap-object(or string) is still useful after dec-rf.
  void GCDecRfNoRecall(void *addr) {
    if (IsHeap(addr)) {
      GetMemHeader(addr).refcount--;
    }
  }

  void GCIncRf(void *addr) {
    if (TurnoffGC())
      return;
    if (IsHeap(addr)) {
      MemHeader &header = GetMemHeader(addr);
      if(header.refcount < UINT14_MAX)
        header.refcount++;
    }
  }
  void GCIncRfJsvalue(__jsvalue jsval) {
    __jstype tag = (__jstype)jsval.tag;
    switch (tag) {
      case JSTYPE_OBJECT:
      case JSTYPE_STRING:
        GCIncRf((jsval.s.obj));
      default:
        assert(false && "unexpected");
    }
  }
#ifndef RC_NO_MMAP
  AddrMap *GetMMapValList(void *);
#endif
  bool IsHeap(void *addr) {
    return (((void *)addr >= memory_) && ((void *)addr < ((void *)((uint8 *)memory_ + HEAP_SIZE))));
  }

  uint32 Bytes4Align(uint32 size) {
    return ((size + 3) & (~0U << 2));
  }

#ifndef RC_NO_MMAP
  AddrMap *NewAddrMap(uint32_t size, uint32_t mask);
  AddrMapNode *NewAddrMapNode();
  void FreeAddrMap(AddrMap *);
  void UpdateAddrMap(uint32_t *origptr, uint32_t *newptr, uint32_t old_len, uint32_t new_len);
#endif
#if MIR_FEATURE_FULL || MIR_DEBUG
  void DebugMemory();
#endif
#if DEBUGGC
  bool CheckAddress(void *);
  bool IsAlignedBy4(uint32 val) {
    return ((val)&0x3) == 0;
  }
#endif


#if MACHINE64
  void *GetRealAddr(uint32_t a32off) {
    assert(a32off < addrMap.size() && "wrong 32 bits address");
    return addrMap[a32off];
  }

  uint32 MapRealAddr(void *realAddr) {
    // create a 32bits offset to map realAddr
    // addrMap.insert(std::pair<uint32_t, void *>(a32off, realAddr));
    assert(addrMap.size() == addrOffset && "mismatch");
    addrMap.push_back(realAddr);
    return addrOffset++;
  }

  __jsvalue GetF64Builtin(uint32_t);
  void SetF64Builtin();
  double GetF64FromU32 (uint32);
  uint64_t SetF64ToU32 (double x) {
    union{
      double t;
      uint64_t t1;
    }xx;
    xx.t = x;
    return xx.t1;
  }
  uint8_t GetFlagFromMemory(uint8_t *memory, uint8_t addrType) {
    uint32_t index;
    if (addrType == spBaseFlag || addrType == fpBaseFlag || addrType == (uint8_t)JSTYPE_ENV) {
      index = (memory - (uint8_t *)memory_)/sizeof(void *);
      return *(memoryFlagMap + index);
    } else {
      assert(addrType == gpBaseFlag);
      index = (memory - (uint8_t *)gpMemory)/sizeof(void *);
      return *(gpMemoryFlagMap + index);
    }
  }
  void SetFlagFromMemory(uint8_t *mem, uint8_t addrType, uint8_t flg) {
    if (addrType == spBaseFlag || addrType == fpBaseFlag || addrType == (uint8_t)JSTYPE_ENV) {
      uint32_t index = (mem - (uint8_t *)memory_)/sizeof(void *);
      *(memoryFlagMap + index) = flg;
    } else {
      assert(addrType == gpBaseFlag);
      uint32_t index = (mem - (uint8_t *)gpMemory)/sizeof(void *);
      *(gpMemoryFlagMap + index) = flg;
    }
  }
  void SetUpGpMemory(void *gpMem, uint8_t *gpFlagMem) {
    gpMemory = gpMem;
    gpMemoryFlagMap = gpFlagMem;
  }
 __jsvalue EmulateLoad(uint64_t *memory, uint8_t addrTy) {
   __jsvalue retMv;
   retMv.s.asbits = *((uint64_t *)memory);
   retMv.tag = (__jstype)GetFlagFromMemory((uint8_t *)memory, addrTy);
   return retMv;
 }
#endif
  bool IsDebugGC() {return false;}
  bool TurnoffGC() {return false;}
};

extern MemoryManager *memory_manager;  // global instance.
#ifdef MARK_CYCLE_ROOTS
extern CycleRoot *cycle_roots;
#else
extern bool is_sweep;
#endif

// C-style interfaces.
void *VMMallocGC(uint32, MemHeadTag tag = MemHeadAny, bool init_p = true);
void *VMReallocGC(void *, uint32, uint32);

static inline void *VMMallocNOGC(uint32 size) {
  return memory_manager->MallocInternal(size);
}

static inline void VMFreeNOGC(void *mem, uint32 size) {
  memory_manager->FreeInternal(mem, size);
}


static inline void GCDecRf(void *p) {
  if (memory_manager->TurnoffGC())
    return;
  memory_manager->GCDecRf(p);
}

static inline void GCIncRf(void *p) {
  if (memory_manager->TurnoffGC())
    return;
  memory_manager->GCIncRf(p);
}

static inline void GCDecRfNoRecall(void *p) {
  if (memory_manager->TurnoffGC())
    return;
  memory_manager->GCDecRfNoRecall(p);
}

static inline void GCCheckAndDecRf(uint64 val, bool needRc) {
  if (memory_manager->TurnoffGC())
    return;
#ifdef MACHINE64
  if (needRc) {
    void *ptr = (void *)val;
    GCDecRf(ptr);
  }
#else
  if (val >> 32 == JSTYPE_OBJECT || val >> 32 == JSTYPE_STRING || val >> 32 == JSTYPE_ENV) {
    GCDecRf((void *)(val & 0xffffffff));
  }
#endif
}

static inline void GCCheckAndIncRf(uint64 val, bool needrc) {
  if (memory_manager->TurnoffGC())
    return;
#ifdef MACHINE64
  if (needrc) {
    GCIncRf((void *)val);
  }
#else
  if (val >> 32 == JSTYPE_OBJECT || val >> 32 == JSTYPE_STRING || val >> 32 == JSTYPE_ENV) {
    GCIncRf((void *)(val & 0xffffffff));
  }
#endif
}

#ifdef MEMORY_LEAK_CHECK
static inline int32_t GCGetRf(void *addr) {
  return memory_manager->GetMemHeader(addr).refcount;
}

#endif

#ifdef RC_NO_MMAP
static inline void GCUpdateRf(void *laddr, void *raddr) {
  // Increase rf-count first.
  // If laddr == raddr && laddr.rf == 1, decrease rf-count first
  // will recall the still useful memory.
  // Don't add the branch:
  //   if (laddr == raddr) return;
  // because there're few assign-statements between
  // same values in an well optimized program.
  GCIncRf(raddr);
  GCDecRf(laddr);
}

static inline void GCCheckAndUpdateRf(uint64 lval, bool lneedrc, uint64 rval, bool rneedrc) {
  // Increase rf-count first. The reason is same as GCUpdateRf.
  GCCheckAndIncRf(rval, rneedrc);
  GCCheckAndDecRf(lval, lneedrc);
}

#else
static inline void UpdateGCReference(void *addr, Mval rval) {
  return memory_manager->UpdateGCReference(addr, rval);
}

#endif

#endif  // MAPLEBE_INCLUDE_MAPLEVM_VMMEMORY_H_

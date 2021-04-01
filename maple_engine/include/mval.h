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

#ifndef MAPLEVM_INCLUDE_VM_MVALUE_H_
#define MAPLEVM_INCLUDE_VM_MVALUE_H_
#include "jsvalueinline.h"
#include "types_def.h"
#include "mvalue.h"
using namespace maple;

enum ExecStatus {
  Exec_error,
  Exec_ok,
  Exec_br,
  Exec_return,
  Exec_terminate,
  Exec_retsub,
  Exec_handle_exc,
  Exec_brtostmt,
};

struct Mval {
  union {
    int32_t i32;
    uint32_t u32;
    int64_t i64;
    uint64_t u64;
    uint64_t asbits;
    void *asptr;
    uint32_t val[2];  // for 64bit value manipulation
  }payload;
  __jstype tag;
};

#ifndef RC_NO_MMAP
struct MMapVal {
  void *ptr1;  // stack/heap address
  void *ptr2;  // pointing to string/object
};
#endif

#ifdef DYNAMICLANG
inline __jstype GetJSTag(Mval mval) {
  return mval.tag;
}

inline void SetJSTag(Mval &mval, __jstype tag) {
  mval.tag = tag;
}

inline bool IsMvalObject(Mval mval) {
  return GetJSTag(mval) == JSTYPE_OBJECT;
}

inline bool IsMvalString(Mval mval) {
  return GetJSTag(mval) == JSTYPE_STRING;
}

inline bool IsMvalEnv(Mval mval) {
  return GetJSTag(mval) == JSTYPE_ENV;
}

inline __jsobject *GetMvalObject(Mval mval) {
  MIR_ASSERT(IsMvalObject(mval));
  return (__jsobject *)mval.payload.asptr;
}

inline __jsstring *GetMvalString(Mval mval) {
  MIR_ASSERT(IsMvalString(mval));
  return (__jsstring *)mval.payload.asptr;
}

inline __jsvalue MvalToJsval(Mval mv) {
  __jsvalue jv;
  jv.asbits = mv.payload.asbits;
  jv.s.tag = mv.tag;
  return jv;
}

inline uint32_t GetJSTag(MValue mval) {
  return mval.x.u64 >> 32;
}

inline bool IsMValueObject(MValue mval) {
  return GetJSTag(mval) == JSTYPE_OBJECT;
}

inline bool IsMValueString(MValue mval) {
  return GetJSTag(mval) == JSTYPE_STRING;
}

inline bool IsMValueEnv(MValue mval) {
  return GetJSTag(mval) == JSTYPE_ENV;
}

inline __jsvalue MValueToJsval(MValue mv) {
  __jsvalue jv;
  jv.asbits = mv.x.u64;
  return jv;
}

inline Mval JsvalToMval(__jsvalue jv) {
  Mval mv;
  mv.payload.asbits = jv.asbits;
  return mv;
}

inline MValue JsvalToMValue(__jsvalue jv) {
  MValue mv;
  mv.x.u64 = jv.asbits;
  return mv;
}

inline MValue NullPointValue() {
  return JsvalToMValue(__null_value());
}

#endif
#endif  //  MAPLEVM_INCLUDE_VM_MVALUE_H_

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

#ifndef RC_NO_MMAP
struct MMapVal {
  void *ptr1;  // stack/heap address
  void *ptr2;  // pointing to string/object
};
#endif

#ifdef DYNAMICLANG
/*
inline uint32_t GetJSTag(MValue mval) {
  return mval.ptyp;
}
*/
inline bool IsMValueObject(MValue mval) {
  return mval.ptyp == JSTYPE_OBJECT;
}

inline bool IsMValueString(MValue mval) {
  return mval.ptyp == JSTYPE_STRING;
}

inline bool IsMValueEnv(MValue mval) {
  return mval.ptyp == JSTYPE_ENV;
}
/*
inline __jsvalue MValueToJsval(MValue mv) {
  __jsvalue jv;
  jv.x.asbits = mv.x.u64;
  jv.ptyp = (__jstype)mv.ptyp;
  return jv;
}

inline MValue JsvalToMValue(__jsvalue jv) {
  MValue mv;
  mv.x.u64 = jv.x.asbits;
  mv.ptyp = jv.ptyp;
  return mv;
}
*/
inline MValue NullPointValue() {
  return __null_value();
}

#endif
#endif  //  MAPLEVM_INCLUDE_VM_MVALUE_H_

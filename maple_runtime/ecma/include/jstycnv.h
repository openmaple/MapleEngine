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

#ifndef JSTYCNV_H
#define JSTYCNV_H
#include "jsvalue.h"
#include "jsvalueinline.h"
// ecma 9.1
__jsvalue __js_ToPrimitive(__jsvalue *v, __jstype preferred_type);
__jsvalue __js_ToPrimitive2(__jsvalue *v); // difference is when v is an object, it will look if it's jsnumber or JSBOOLEAN or JSDOUBLE object
// ecma 9.2
bool __js_ToBoolean(__jsvalue *v);
// ecma 9.3~9.4
int32_t __js_ToNumberSlow(__jsvalue *v);
int64_t __js_ToNumberSlow64(__jsvalue *v);
static inline int32_t __js_ToNumber(__jsvalue *v) {
  if (__is_number(v)) {
    return __jsval_to_number(v);
  }
  return __js_ToNumberSlow(v);
}

static inline int64_t __js_ToNumber64(__jsvalue *v) {
  if (__is_number(v)) {
    return (int64_t)__jsval_to_number(v);
  }
  return __js_ToNumberSlow64(v);
}

__jsvalue __js_ToNumberSlow2(__jsvalue *v, bool &isConvertible);
static inline __jsvalue __js_ToNumber2(__jsvalue *v, bool &isConvertible) {
  if (__is_number(v)) {
    isConvertible = true;
    return *v;
  }
  return __js_ToNumberSlow2(v, isConvertible);
}

static inline int32_t __js_ToInteger(__jsvalue *v) {
  return __js_ToNumber(v);
}

__jsstring *__js_DoubleToString(double n);

// ecma 9.5
static inline int32_t __js_ToInt32(__jsvalue *v) {
  return __js_ToNumber(v);
}

// ecma 9.6, update to 7.1.7 ToUint32
static inline uint32_t __js_ToUint32(__jsvalue *v) {
  if (__is_number(v)) {
    return (uint32_t)__jsval_to_number(v);
  }
  bool convertible = false;
  __jsvalue numval = __js_ToNumber2(v, convertible);
  if (__is_double(v) || __is_double(&numval)) {
    double d = __is_double(v) ? __jsval_to_double(v) : __jsval_to_double(&numval);
    return (uint32_t)(d);
  } else if (__is_number(&numval)) {
    return (uint32_t)(__jsval_to_number(&numval));
  }
  return 0;
}

// ecma 9.7
static inline uint16_t __js_ToUint16(__jsvalue *v) {
  return (uint16_t)__js_ToNumber(v);
}

/* ecma 9.8.  */
__jsstring *__js_NumberToString(int32_t n);
__jsstring *__js_ToStringSlow(__jsvalue *v);
static inline __jsstring *__js_ToString(__jsvalue *v) {
  if (__is_string(v)) {
    return __jsval_to_string(v);
  }
  return __js_ToStringSlow(v);
}

// ecma 9.9
__jsobject *__js_ToObject(__jsvalue *v);
// ecma 9.10
static inline void CheckObjectCoercible(__jsvalue *v) {
  if (__is_null_or_undefined(v)) {
    MAPLE_JS_TYPEERROR_EXCEPTION();
  }
}

// ecma 9.11
bool __js_IsCallable(__jsvalue *v);
// ecma 9.12
bool __js_SameValue(__jsvalue *x, __jsvalue *y);

// ecma6.0 7.1.15 ToLength
uint64_t __js_toLength(__jsvalue *v);
#endif

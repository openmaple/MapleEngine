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

/// Only small inline functions for jsvalue.
#ifndef JSVALUEINLINE_H
#define JSVALUEINLINE_H
#include "jsvalue.h"
#include "jsobject.h"
#include "jsnum.h"
static inline bool __is_null(__jsvalue *data) {
  return data->tag == JSTYPE_NULL;
}

static inline bool __is_undefined(__jsvalue *data) {
  return data->tag == JSTYPE_UNDEFINED;
}

static inline bool __is_nan(__jsvalue *data) {
  return data->tag == JSTYPE_NAN;
}

static inline bool __is_infinity(__jsvalue *data) {
  return data->tag == JSTYPE_INFINITY;
}

static inline bool __is_positive_infinity(__jsvalue *data) {
  return __is_infinity(data) && (data->s.u32 == 0);
}

static inline bool __is_neg_infinity(__jsvalue *data) {
  return __is_infinity(data) && (data->s.u32 == 1);
}

static inline bool __is_null_or_undefined(__jsvalue *data) {
  return (data->tag == JSTYPE_NULL) || (data->tag == JSTYPE_UNDEFINED);
}

static inline bool __is_boolean(__jsvalue *data) {
  return data->tag == JSTYPE_BOOLEAN;
}

static inline bool __is_string(__jsvalue *data) {
  return data->tag == JSTYPE_STRING;
}

static inline bool __is_number(__jsvalue *data) {
  return data->tag == JSTYPE_NUMBER;
}

static inline bool __is_double(__jsvalue *data) {
  return data->tag == JSTYPE_DOUBLE;
}

static inline bool __is_int32(__jsvalue *data) {
  return __is_number(data);
}

static inline bool __is_primitive(uint32_t tag) {
  return (tag != JSTYPE_NONE && tag < JSTYPE_OBJECT) || (tag == JSTYPE_UNDEFINED)
         || tag == JSTYPE_DOUBLE || tag == JSTYPE_NAN || tag == JSTYPE_INFINITY;
}

static inline bool __is_primitive(__jsvalue *data) {
  return __is_primitive(data->tag);
}

static inline bool __is_js_object(__jsvalue *data) {
  return data->tag == JSTYPE_OBJECT;
}

static inline bool __is_js_object_or_primitive(__jsvalue *data) {
  return __is_js_object(data) || __is_primitive(data);
}

static inline bool __is_positive_zero(__jsvalue *data) {
  return (data->tag == JSTYPE_NUMBER && data->s.asbits == 0);
}

static inline bool __is_negative_zero(__jsvalue *data) {
  return (data->tag == JSTYPE_DOUBLE && data->s.asbits == 0);
}

#if MACHINE64
static inline bool IsNeedRc(uint8_t flag) {
  __jstype flagt = (__jstype)flag;
  return flagt == JSTYPE_OBJECT || flagt == JSTYPE_STRING || flagt == JSTYPE_ENV;
}
static inline __jsstring *__jsval_to_string(__jsvalue *data) {
  return (__jsstring *)data->s.str;
}
static inline __jsobject *__jsval_to_object(__jsvalue *data) {
  return data->s.obj;
}
static inline void __set_string(__jsvalue *data, __jsstring *str) {
  data->tag = JSTYPE_STRING;
  data->s.str = str;
}
static inline void __set_object(__jsvalue *data, __jsobject *obj) {
  data->tag = JSTYPE_OBJECT;
  data->s.obj = obj;
}
static inline bool __is_js_function(__jsvalue *data) {
  return __is_js_object(data) && (data->s.obj->object_class == JSFUNCTION);
}
static inline bool __is_js_array(__jsvalue *data) {
  return __is_js_object(data) && (data->s.obj->object_class == JSARRAY);
}
static inline double __jsval_to_double(__jsvalue *data) {
  if (__is_number(data))
    return (double) data->s.i32;
  MAPLE_JS_ASSERT(__is_double(data));
  return data->s.f64;
}
static inline __jsvalue __double_value(double f64) {
  __jsvalue jsval;
  jsval.tag = JSTYPE_DOUBLE;
  jsval.s.f64 = f64;
  return jsval;
}
#else
static inline __jsstring *__jsval_to_string(__jsvalue *data) {
  MAPLE_JS_ASSERT(__is_string(data));
  return data->s.payload.str;
}

static inline __jsobject *__jsval_to_object(__jsvalue *data) {
  MAPLE_JS_ASSERT(__is_js_object(data));
  return data->s.payload.obj;
}
static inline void __set_string(__jsvalue *data, __jsstring *str) {
  data->tag = JSTYPE_STRING;
  data->s.payload.str = str;
}
static inline void __set_object(__jsvalue *data, __jsobject *obj) {
  data->tag = JSTYPE_OBJECT;
  data->s.payload.obj = obj;
}
static inline bool __is_js_function(__jsvalue *data) {
  if (__is_js_object(data)) {
    return data->s.payload.obj->object_class == JSFUNCTION;
  }
  return false;
}
static inline bool __is_js_array(__jsvalue *data) {
  if (__is_js_object(data)) {
    return data->s.payload.obj->object_class == JSARRAY;
  }
  return false;
}
#endif

static inline bool __is_none(__jsvalue *data) {
  return data->tag == JSTYPE_NONE && data->s.u32 == 0;
}

static inline bool __jsval_to_boolean(__jsvalue *data) {
  MAPLE_JS_ASSERT(__is_boolean(data));
  return (bool)data->s.boo;
}

static inline int32_t __jsval_to_number(__jsvalue *data) {
  MAPLE_JS_ASSERT(__is_number(data));
  return data->s.i32;
}

static inline int32_t __jsval_to_int32(__jsvalue *data) {
  return __jsval_to_number(data);
}

static inline uint32_t __jsval_to_uint32(__jsvalue *data) {
  if (__is_number(data)) {
    return (uint32_t)__jsval_to_number(data);
  } else if (__is_double(data)) {
    return (uint32_t)__jsval_to_double(data);
  }
  MAPLE_JS_ASSERT(0 && "__jsval_to_uint32");
  return 0;
}


static inline __jsvalue __string_value(__jsstring *str) {
  __jsvalue data;
  data.s.asbits = 0;
  __set_string(&data, str);
  return data;
}

static inline __jsvalue __undefined_value() {
  __jsvalue data;
  data.s.asbits = 0;
  data.tag = JSTYPE_UNDEFINED;
  return data;
}

static inline __jsvalue __object_value(__jsobject *obj) {
  __jsvalue data;
  if (!obj) {
    return __undefined_value();
  }
  __set_object(&data, obj);
  return data;
}

static inline void __set_boolean(__jsvalue *data, bool b) {
  data->tag = JSTYPE_BOOLEAN;
  data->s.boo = b;
}

static inline void __set_number(__jsvalue *data, int32_t i) {
  data->tag = JSTYPE_NUMBER;
  data->s.asbits = 0;
  data->s.i32 = i;
}


static inline __jsvalue __null_value() {
  __jsvalue data;
  data.s.asbits = 0;
  data.tag = JSTYPE_NULL;
  return data;
}


static inline __jsvalue __boolean_value(bool b) {
  __jsvalue data;
  data.s.asbits = 0;
  __set_boolean(&data, b);
  return data;
}


static inline __jsvalue __number_value(int32_t i) {
  __jsvalue data;
  data.s.asbits = 0;
  __set_number(&data, i);
  return data;
}

static inline __jsvalue __number_infinity() {
  __jsvalue data;
  data.s.asbits = 0;
  data.tag = JSTYPE_INFINITY;
  return data;
}

static inline __jsvalue __number_neg_infinity() {
  __jsvalue data;
  data.s.asbits = 1;
  data.tag = JSTYPE_INFINITY;
  return data;
}

static inline __jsvalue __none_value() {
  __jsvalue data;
  data.s.asbits = 0;
  data.tag = JSTYPE_NONE;
  return data;
}

static inline __jsvalue __nan_value() {
  __jsvalue data;
  data.s.asbits = 0;
  data.tag = JSTYPE_NAN;
  return data;
}

static inline __jsvalue __positive_zero_value() {
  __jsvalue data;
  data.s.asbits = 0;
  data.tag = JSTYPE_NUMBER;
  return data;
}

static inline __jsvalue __negative_zero_value() {
  __jsvalue data;
  data.s.f64 = 0;
  data.tag = JSTYPE_DOUBLE;
  return data;
}

static inline __jstype __jsval_typeof(__jsvalue *data) {
  MAPLE_JS_ASSERT((__is_js_object_or_primitive(data)
     || __is_none(data) || __is_nan(data) || __is_infinity(data)) && "internal error.");
  return (__jstype)data->tag;
}

#endif

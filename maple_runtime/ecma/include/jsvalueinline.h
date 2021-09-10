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
  return data->ptyp == JSTYPE_NULL;
}

static inline bool __is_undefined(__jsvalue *data) {
  return data->ptyp == JSTYPE_UNDEFINED;
}

static inline bool __is_nan(__jsvalue *data) {
  return data->ptyp == JSTYPE_NAN;
}

static inline bool __is_infinity(__jsvalue *data) {
  return data->ptyp == JSTYPE_INFINITY;
}

static inline bool __is_positive_infinity(__jsvalue *data) {
  return __is_infinity(data) && (data->x.u32 == 0);
}

static inline bool __is_neg_infinity(__jsvalue *data) {
  return __is_infinity(data) && (data->x.u32 == 1);
}

static inline bool __is_null_or_undefined(__jsvalue *data) {
  return (data->ptyp == JSTYPE_NULL) || (data->ptyp == JSTYPE_UNDEFINED);
}

static inline bool __is_boolean(__jsvalue *data) {
  return data->ptyp == JSTYPE_BOOLEAN;
}

static inline bool __is_string(__jsvalue *data) {
  return data->ptyp == JSTYPE_STRING;
}

static inline bool __is_number(__jsvalue *data) {
  return data->ptyp == JSTYPE_NUMBER;
}

static inline bool __is_double(__jsvalue *data) {
  return data->ptyp == JSTYPE_DOUBLE;
}

static inline bool __is_int32(__jsvalue *data) {
  return __is_number(data);
}

static inline bool __is_primitive(uint32_t ptyp) {
  return ptyp == JSTYPE_NUMBER || ptyp == JSTYPE_STRING || ptyp == JSTYPE_BOOLEAN || ptyp == JSTYPE_UNDEFINED
         || ptyp == JSTYPE_DOUBLE || ptyp == JSTYPE_NAN || ptyp == JSTYPE_INFINITY || ptyp == JSTYPE_NULL;
}

static inline bool __is_primitive(__jsvalue *data) {
  return __is_primitive(data->ptyp);
}

static inline bool __is_js_object(__jsvalue *data) {
  return data->ptyp == JSTYPE_OBJECT;
}

static inline bool __is_js_object_or_primitive(__jsvalue *data) {
  return __is_js_object(data) || __is_primitive(data);
}

static inline bool __is_positive_zero(__jsvalue *data) {
  return (data->ptyp == JSTYPE_NUMBER && data->x.asbits == 0);
}

static inline bool __is_negative_zero(__jsvalue *data) {
  return (data->ptyp == JSTYPE_DOUBLE && data->x.asbits == 0);
}

#if MACHINE64
#define IsNeedRc(v) (((uint8_t)v & 0x4) == 4)
/*
static inline bool IsNeedRc(uint8_t flag) {
  __jstype flagt = (__jstype)flag;
  return flagt == JSTYPE_OBJECT || flagt == JSTYPE_STRING || flagt == JSTYPE_ENV;
}
*/
static inline __jsstring *__jsval_to_string(__jsvalue *data) {
  return (__jsstring *)data->x.str;
}
static inline __jsobject *__jsval_to_object(__jsvalue *data) {
  return data->x.obj;
}
static inline void __set_string(__jsvalue *data, __jsstring *str) {
  data->ptyp = JSTYPE_STRING;
  data->x.str = str;
}
static inline void __set_object(__jsvalue *data, __jsobject *obj) {
  data->ptyp = JSTYPE_OBJECT;
  data->x.obj = obj;
}
static inline bool __is_js_function(__jsvalue *data) {
  return __is_js_object(data) && (data->x.obj->object_class == JSFUNCTION);
}
static inline bool __is_js_array(__jsvalue *data) {
  return __is_js_object(data) && (data->x.obj->object_class == JSARRAY);
}
static inline double __jsval_to_double(__jsvalue *data) {
  if (__is_number(data))
    return (double) data->x.i32;
  //  MAPLE_JS_ASSERT(__is_double(data));
  return data->x.f64;
}
static inline __jsvalue __double_value(double f64) {
  __jsvalue jsval;
  jsval.ptyp = JSTYPE_DOUBLE;
  jsval.x.f64 = f64;
  return jsval;
}
static inline __jsvalue __function_value (void *addr) {
  __jsvalue jsval;
  jsval.ptyp = JSTYPE_FUNCTION;
  jsval.x.ptr = addr;
  return jsval;
}
#else
static inline __jsstring *__jsval_to_string(__jsvalue *data) {
  MAPLE_JS_ASSERT(__is_string(data));
  return data->x.payload.str;
}

static inline __jsobject *__jsval_to_object(__jsvalue *data) {
  MAPLE_JS_ASSERT(__is_js_object(data));
  return data->x.payload.obj;
}
static inline void __set_string(__jsvalue *data, __jsstring *str) {
  data->ptyp = JSTYPE_STRING;
  data->x.payload.str = str;
}
static inline void __set_object(__jsvalue *data, __jsobject *obj) {
  data->ptyp = JSTYPE_OBJECT;
  data->x.payload.obj = obj;
}
static inline bool __is_js_function(__jsvalue *data) {
  if (__is_js_object(data)) {
    return data->x.payload.obj->object_class == JSFUNCTION;
  }
  return false;
}
static inline bool __is_js_array(__jsvalue *data) {
  if (__is_js_object(data)) {
    return data->x.payload.obj->object_class == JSARRAY;
  }
  return false;
}
#endif

static inline bool __is_none(__jsvalue *data) {
  return data->ptyp == JSTYPE_NONE && data->x.u32 == 0;
}

static inline bool __jsval_to_boolean(__jsvalue *data) {
  //  MAPLE_JS_ASSERT(__is_boolean(data));
  return (bool)data->x.boo;
}

static inline int32_t __jsval_to_number(__jsvalue *data) {
  //  MAPLE_JS_ASSERT(__is_number(data));
  return data->x.i32;
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
  //  MAPLE_JS_ASSERT(0 && "__jsval_to_uint32");
  return 0;
}


static inline __jsvalue __string_value(__jsstring *str) {
  __jsvalue data;
  data.ptyp = JSTYPE_STRING;
  data.x.str = str;
  return data;
}

static inline __jsvalue __undefined_value() {
  __jsvalue data;
  data.x.asbits = 0;
  data.ptyp = JSTYPE_UNDEFINED;
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
  data->ptyp = JSTYPE_BOOLEAN;
  data->x.boo = b;
}

static inline void __set_number(__jsvalue *data, int32_t i) {
  data->ptyp = JSTYPE_NUMBER;
  data->x.asbits = 0;
  data->x.i32 = i;
}


static inline __jsvalue __null_value() {
  __jsvalue data;
  data.x.asbits = 0;
  data.ptyp = JSTYPE_NULL;
  return data;
}


static inline __jsvalue __boolean_value(bool b) {
  __jsvalue data;
  data.x.asbits = 0;
  __set_boolean(&data, b);
  return data;
}


static inline __jsvalue __number_value(int32_t i) {
  __jsvalue data;
  data.x.asbits = 0;
  __set_number(&data, i);
  return data;
}

static inline __jsvalue __number_infinity() {
  __jsvalue data;
  data.x.asbits = 0;
  data.ptyp = JSTYPE_INFINITY;
  return data;
}

static inline __jsvalue __number_neg_infinity() {
  __jsvalue data;
  data.x.asbits = 1;
  data.ptyp = JSTYPE_INFINITY;
  return data;
}

static inline __jsvalue __none_value() {
  __jsvalue data;
  data.x.asbits = 0;
  data.ptyp = JSTYPE_NONE;
  return data;
}

static inline __jsvalue __nan_value() {
  __jsvalue data;
  data.x.asbits = 0;
  data.ptyp = JSTYPE_NAN;
  return data;
}

static inline __jsvalue __positive_zero_value() {
  __jsvalue data;
  data.x.asbits = POS_ZERO;
  data.ptyp = JSTYPE_NUMBER;
  return data;
}

static inline __jsvalue __negative_zero_value() {
  __jsvalue data;
  data.x.asbits = NEG_ZERO;
  data.ptyp = JSTYPE_DOUBLE;
  return data;
}

static inline __jstype __jsval_typeof(__jsvalue *data) {
  //  MAPLE_JS_ASSERT((__is_js_object_or_primitive(data)
  //     || __is_none(data) || __is_nan(data) || __is_infinity(data)) && "internal error.");
  return (__jstype)data->ptyp;
}

#endif

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

#include <cstring>
#include <cstdio>
#include <sstream>
#include <iomanip>
#include <cmath>
#include "jsvalueinline.h"
#include "jsnum.h"
#include "jsstring.h"
#include "jsobject.h"
#include "jsobjectinline.h"
#include "jsglobal.h"
#include "vmutils.h"
#include "vmmemory.h"

// ecma 9.1
__jsvalue __js_ToPrimitive(__jsvalue *v, __jstype preferred_type) {
  // Undefined Null Boolean Number String
  if (__is_primitive(v)) {
    return *v;
  }
  // Object
  MAPLE_JS_ASSERT(__is_js_object(v));
  __jsobject *obj = __jsval_to_object(v);
  return __object_internal_DefaultValue(obj, preferred_type);
}

__jsvalue __js_ToPrimitive2(__jsvalue *v) {
  if (v->tag == JSTYPE_OBJECT) {
    __jsobject *obj = __jsval_to_object(v);
    uint8_t oClass = obj->object_class;
    if (oClass == JSNUMBER || oClass == JSBOOLEAN) {
      if (obj->object_type == JSSPECIAL_NUMBER_OBJECT) {
        __jsstring *primstring = obj->shared.prim_string;
        if (primstring) {
          if (__jsstr_equal_to_builtin(primstring, JSBUILTIN_STRING_INFINITY_UL)) {
            return __number_infinity();
          } else if (__jsstr_equal_to_builtin(primstring, JSBUILTIN_STRING_NEG_INFINITY_UL)) {
            return __number_neg_infinity();
          } else if (__jsstr_equal_to_builtin(primstring, JSBUILTIN_STRING_NAN)) {
            return __nan_value();
          }
        }
      }
      return __number_value(obj->shared.prim_number);
    } else if (oClass == JSDOUBLE) {
      return __double_value(obj->shared.primDouble);
    }
  }
  return __js_ToPrimitive(v, JSTYPE_UNDEFINED);
}
// ecma 9.2
bool __js_ToBoolean(__jsvalue *v) {
  switch (__jsval_typeof(v)) {
    case JSTYPE_NONE:
    case JSTYPE_UNDEFINED:
    case JSTYPE_NULL:
    case JSTYPE_NAN:
      return false;
    case JSTYPE_BOOLEAN:
      return __jsval_to_boolean(v);
    case JSTYPE_STRING:
      return __jsstr_get_length(__jsval_to_string(v)) != 0;
    case JSTYPE_NUMBER:
      return __jsval_to_number(v) != 0;
    case JSTYPE_DOUBLE:
      return __jsval_to_double(v) != 0.0;
    case JSTYPE_INFINITY:
      return true;
    default:
      MAPLE_JS_ASSERT(__is_js_object(v));
  }
  return true;
}

int32_t __js_ToNumberSlow(__jsvalue *v) {
  switch (__jsval_typeof(v)) {
    case JSTYPE_UNDEFINED:
    case JSTYPE_NULL:
    case JSTYPE_NONE:
      return 0;
    case JSTYPE_BOOLEAN:
      return __jsval_to_boolean(v) ? 1 : 0;
      break;
    case JSTYPE_STRING:
      return (int32_t)__js_str2num(__jsval_to_string(v));
      break;
    case JSTYPE_OBJECT: {
      __jsvalue prim_value = __js_ToPrimitive(v, JSTYPE_NUMBER);
      GCCheckAndIncRf(prim_value.s.asbits, IsNeedRc(prim_value.tag));
      int32_t num = __js_ToNumber(&prim_value);
      GCCheckAndDecRf(prim_value.s.asbits, IsNeedRc(prim_value.tag));
      return num;
    } break;
    case JSTYPE_DOUBLE: {
      // ecma 9.5 toInt32 step 3
      double d = __jsval_to_double(v);
      bool isNeg = d < 0.0 ? true : false;
      int32_t res = (int32_t) isNeg ? (-(int64_t)(floor(-d))) : (int64_t)(floor(d));
      return res;
    }
    case JSTYPE_NAN:
    case JSTYPE_INFINITY: {
      // ecma 9.5 toInt32 step 2
      return 0;
    }
    default:
      MAPLE_JS_ASSERT(false && "unreachable.");
  }
  return 0;
}

int64_t __js_ToNumberSlow64(__jsvalue *v) {
  switch (__jsval_typeof(v)) {
    case JSTYPE_DOUBLE: {
      // ecma 9.5 toInt32 step 3
      double d = __jsval_to_double(v);
      bool isNeg = d < 0.0 ? true : false;
      int64_t res = isNeg ? (-(int64_t)(floor(-d))) : (int64_t)(floor(d));
      return res;
    }
    case JSTYPE_BOOLEAN:
      return __jsval_to_boolean(v) ? 1 : 0;
    default:
      MAPLE_JS_TYPEERROR_EXCEPTION();
  }
  return 0L;
}

__jsvalue __js_ToNumberSlow2(__jsvalue *v, bool &isConvertible) {
  switch (__jsval_typeof(v)) {
    case JSTYPE_UNDEFINED:
    case JSTYPE_NONE:
    case JSTYPE_NULL: {
      isConvertible = true;
      return __number_value(0);
    }
    case JSTYPE_BOOLEAN: {
      isConvertible = true;
      return __number_value(v->s.i32);
    }
    case JSTYPE_STRING: {
      __jsvalue jsDv = __js_str2double(__jsval_to_string(v), isConvertible);
      if (isConvertible) {
        if (__is_infinity(&jsDv) || __is_negative_zero(&jsDv) || __is_positive_zero(&jsDv))
          return jsDv;
        double db = __jsval_to_double(&jsDv);
        if (__is_double_no_decimal(db) && fabs(db) < (double)0x7fffffff)
          return __number_value(db);
      }
      return jsDv;
      // TODO: setup convert to double
    }
    case JSTYPE_OBJECT: {
      __jsobject *obj = __jsval_to_object(v);
      if (obj->object_class == JSARRAY)
        return  __nan_value();

      __jsvalue prim_value = __js_ToPrimitive(v, JSTYPE_NUMBER);
      if (__is_string(&prim_value)) {
        return __js_ToNumberSlow2(&prim_value, isConvertible);
      } else if (__is_boolean(&prim_value)) {
        isConvertible = true;
        return __number_value(prim_value.s.i32);
      } else {
        isConvertible = true; // TODO: depends
        return prim_value;
      }
      /*
      GCCheckAndIncRf(prim_value.asbits);
      int32_t num = __js_ToNumber(&prim_value);
      GCCheckAndDecRf(prim_value.asbits);
      return __number_value(num);
      */
    }
    case JSTYPE_DOUBLE: {
      // ecma 9.5 ToInt32 step 3
      double d = __jsval_to_double(v);
      bool isNeg = d < 0.0 ? true : false;
      int64_t res = isNeg ? (-1)*(floor(-d)) : (floor(d));
      isConvertible = true;
      return (res >= INT32_MIN && res <= INT32_MAX) ? __number_value(res) : __double_value(res);
    }
    case JSTYPE_INFINITY: {
      isConvertible = true;
      return *v;
    }
    case JSTYPE_NAN: {
      // ecma 9.5 toInt32 step 2
      return *v;
    }
    default:
      MAPLE_JS_ASSERT(false && "unreachable.");
  }
  return __undefined_value();
}

// ecma 9.8.1.
__jsstring *__js_NumberToString(int32_t n) {
  /* ecma 9.8.1 step 1. */
  // NaN
  /* ecma 9.8.1 step 2. */
  if (n == 0) {
    return __jsstr_get_builtin(JSBUILTIN_STRING_ZERO_CHAR);
  }
  /* ecma 9.8.1 step 3~10. */
  char result[64];
  snprintf(result, 64, "%d", n);
  return __jsstr_new_from_char((const char *)result);
}

__jsstring *__js_DoubleToString(double n) {
  char src[64];
  if (fabs(n) >= 1e16 && fabs(n) < 1e21) {
    sprintf(src, "%.0f", n);
  } else if (fabs(n) <= 1e-4 && fabs(n) >= 1e-6) {
    sprintf(src, "%.10f", n);
    // take out trailing 0s
    uint32_t len = strlen(src);
    for (int i = len - 1; i > 1; i--) {
      if (src[i] != '0')
        break;
      src[i] = 0;
    }
  } else {
    sprintf(src, "%.16g", n);
    // take out leading 0 of exponent
    uint32_t len = strlen(src);
    if (len > 3) {
      bool e = false;
      for (int i = 1; i < len; i++) {
        if (src[i] == 'e') {
          e = true;
          continue;
        }
        if (e) {
          if (src[i] == '+' || src[i] == '-')
            continue;
          if (src[i] != '0')
            break; // no leading 0
          else {
            for (; i < len; i++)
              src[i] = src[i + 1];
            break;
          }
        }
      }
    }
  }
  return __jsstr_new_from_char((const char *)(&src));
}

__jsstring *__js_ToStringSlow(__jsvalue *v) {
  switch (__jsval_typeof(v)) {
    case JSTYPE_UNDEFINED:
      return __jsstr_get_builtin(JSBUILTIN_STRING_UNDEFINED);
    case JSTYPE_NULL:
      return __jsstr_get_builtin(JSBUILTIN_STRING_NULL);
    case JSTYPE_BOOLEAN:
      return __jsval_to_boolean(v) ? __jsstr_get_builtin(JSBUILTIN_STRING_TRUE)
                                   : __jsstr_get_builtin(JSBUILTIN_STRING_FALSE);
    case JSTYPE_NUMBER:
      return __js_NumberToString(__jsval_to_int32(v));
    case JSTYPE_DOUBLE:
      return __js_DoubleToString(__jsval_to_double(v));
    case JSTYPE_OBJECT: {
      __jsvalue prim_value = __js_ToPrimitive(v, JSTYPE_STRING);
      if (!__is_string(&prim_value)) {
        GCCheckAndIncRf(prim_value.s.asbits, IsNeedRc(prim_value.tag));
      }
      __jsstring *str = __js_ToString(&prim_value);
      if (!__is_string(&prim_value)) {
        GCCheckAndDecRf(prim_value.s.asbits, IsNeedRc((prim_value.tag)));
      }
      return str;
    }
    case JSTYPE_NONE:
      //return __jsstr_get_builtin(JSBUILTIN_STRING_EMPTY);
      return __jsstr_get_builtin(JSBUILTIN_STRING_UNDEFINED);
    case JSTYPE_NAN:
      return __jsstr_get_builtin(JSBUILTIN_STRING_NAN);
    case JSTYPE_INFINITY:{
      return __jsstr_get_builtin(__is_neg_infinity(v) ? JSBUILTIN_STRING_NEG_INFINITY_UL: JSBUILTIN_STRING_INFINITY_UL);
    }
    default:
      MAPLE_JS_ASSERT(false && "unreachable.");
  }
  return __jsstr_get_builtin(JSBUILTIN_STRING_NULL);
}

// ecma 9.9
__jsobject *__js_ToObject(__jsvalue *v) {
  __jsvalue res = *v;
  switch (__jsval_typeof(v)) {
    case JSTYPE_UNDEFINED:
    case JSTYPE_NULL:
    case JSTYPE_NONE:
      MAPLE_JS_TYPEERROR_EXCEPTION();
      break;
    case JSTYPE_BOOLEAN:
      res = __js_new_boo_obj(NULL, v, 1);
      break;
    case JSTYPE_STRING:
      res = __js_new_str_obj(NULL, v, 1);
      break;
    case JSTYPE_DOUBLE:
    case JSTYPE_NUMBER:
    case JSTYPE_NAN:
    case JSTYPE_INFINITY:
      res = __js_new_num_obj(NULL, v, 1);
      break;
    default:
      MAPLE_JS_ASSERT(__is_js_object(v));
      break;
  }
  return __jsval_to_object(&res);
}

// ecma 9.11
bool __js_IsCallable(__jsvalue *v) {
  // Undefined Null Boolean Number String
  if (__is_primitive(v)) {
    return false;
  }
  // Object
  MAPLE_JS_ASSERT(__is_js_object(v));
  // If the argument object has a [[Call]] internal method,
  // then return true, otherwise return false.
  // ??? Iff v is a function has a [[Call]] internal method?
  return __is_js_function(v);
}

// ecma 9.12
bool __js_SameValue(__jsvalue *x, __jsvalue *y) {
  if (x->s.asbits == y->s.asbits && x->tag == y->tag)
    return true;
  if ((__is_number(x) && __is_boolean(y)) || (__is_number(y) && __is_boolean(x))) {
    return x->s.u32 == y->s.u32;
  }
  if ((__is_number(x) && __is_number(y))) {
    return x->s.i32 == y->s.i32;
  }
  if (__jsval_typeof(x) == JSTYPE_STRING)
    return __jsval_typeof(y) == JSTYPE_STRING && __jsstr_equal(__jsval_to_string(x), __jsval_to_string(y));
  if (__jsval_typeof(x) == JSTYPE_OBJECT && sizeof(void *) == 8)
    return __jsval_typeof(y) == JSTYPE_OBJECT &&
           x->s.obj == y->s.obj;
  return false;
}

// ecma6.0 7.1.15 ToLength
// TODO:: refine ToNumber64 and use ToNumber64
uint64_t __js_toLength(__jsvalue *v) {
  uint64_t len = 0;
  if (__is_number(v)) {
    int32_t n = __jsval_to_number(v);
    if (n < 0) return 0;  // 7.1.15 step 4. If len ≤ 0, return +0.
    return (uint64_t) n;
  } else if (__is_double(v)) {
    double d = __jsval_to_double(v);
    bool isNeg = d < 0.0 ? true : false;
    int64_t i64t = isNeg ? (-(int64_t)(floor(-d))) : (int64_t)(floor(d));
    if (i64t < 0) {
      return 0;
    }
    if (i64t > MAX_LENGTH_PROPERTY_SIZE) return MAX_LENGTH_PROPERTY_SIZE;
    return i64t;
  } else if (__is_string(v)) {
    bool convertible = false;
    __jsstring *strlen = __jsval_to_string(v);
    __jsvalue convertedval = __js_str2double(strlen, convertible);
    if (!convertible) return 0; // not a valid number
    return __js_toLength(&convertedval);
  } else if (__is_positive_infinity(v)) {
    return MAX_LENGTH_PROPERTY_SIZE;  // 7.1.15 step 5 If len is infinity, return 2^53-1.
  } else if (__is_nan(v) || __is_neg_infinity(v) ||
             __is_undefined(v) || __is_null(v) || __is_none(v)) {
    return 0;       // 7.1.15 step 4. If len ≤ 0, return +0
  } else if (__is_js_object(v)) {
    __jsvalue prim_value = __js_ToPrimitive(v, JSTYPE_NUMBER);
    GCCheckAndIncRf(prim_value.s.asbits, IsNeedRc(prim_value.tag));
    len = __js_toLength(&prim_value);
    GCCheckAndDecRf(prim_value.s.asbits, IsNeedRc(prim_value.tag));
    return len;
  } else if (__is_boolean(v)) {
    return __jsval_to_boolean(v) ? 1 : 0;
  }
  MAPLE_JS_ASSERT(0 && "__js_toLength");
  return 0;
}

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

#include <cstdlib>
#include <cstring>
#include <algorithm>
#include "jsarray.h"
#include "jsstring.h"
#include "jscontext.h"
#include "jsfunction.h"
#include "jsbinary.h"
#include "jsobjectinline.h"
#include "vmmemory.h"

// Helper function for internal use.
__jsobject *__js_new_arr_internal(uint32_t length) {
  __jsobject *arr = __create_object();
  arr->object_class = JSARRAY;
  arr->extensible = true;
  __jsobj_set_prototype(arr, JSBUILTIN_ARRAYPROTOTYPE);
  arr->object_type = JSREGULAR_ARRAY;
  // not allocated huge size here
  __jsvalue *props;
  if (length > ARRAY_MAXINDEXNUM_INTERNAL)
      props = (__jsvalue *)VMMallocGC(sizeof(__jsvalue) * (ARRAY_MAXINDEXNUM_INTERNAL+1));
  else
      props = (__jsvalue *)VMMallocGC(sizeof(__jsvalue) * (length + 1));
  arr->shared.array_props = props;
  props[0] = length > INT32_MAX ? __double_value((double)length) : __number_value(length);
  return arr;
}

// ecma 15.4.2.1
__jsobject *__js_new_arr_elems(__jsvalue *items, uint32_t length) {
  __jsobject *arr = __js_new_arr_internal(length);
  __jsvalue *array_elems = arr->shared.array_props;
  for (uint32_t i = 0; i < length; i++) {
    __jsvalue itVt = memory_manager->EmulateLoad(((uint64_t *)items->s.ptr + i), items->tag);
    __set_regular_elem(array_elems, i, &itVt);
  }
  return arr;
}

// different from the function above which will load items from memory that
// is iassigned to value
__jsobject *__js_new_arr_elems_direct(__jsvalue *items, uint32_t length) {
  __jsobject *arr = __js_new_arr_internal(length);
  __jsvalue *array_elems = arr->shared.array_props;
  for (uint32_t i = 0; i < length; i++) {
    __set_regular_elem(array_elems, i, &items[i]);
  }
  return arr;
}

static uint32_t __js_arrlengthproperty_internal(__jsvalue *len) {
  bool canconvert = false;
  // len may be number, double, string, object
  uint32_t ulen = __js_ToUint32(len);
  __jsvalue lenval = __js_ToNumber2(len, canconvert);
  double dlen;
  // throw exception if len is invalid string or nan/infinity/undefined
  if (!canconvert ||
      (__is_undefined(len) || __is_undefined(&lenval) ||
       __is_infinity(len) || __is_infinity(&lenval) ||
       __is_neg_infinity(len) || __is_neg_infinity(&lenval))) {
    MAPLE_JS_RANGEERROR_EXCEPTION();
  }
  dlen =  __is_double(len) ? __jsval_to_double(len) :
                              (__is_double(&lenval) ? __jsval_to_double(&lenval) :
                                __js_ToNumber(&lenval));
  // check range of length
  if (dlen < 0 || (ulen != dlen)) {
    MAPLE_JS_RANGEERROR_EXCEPTION();
  }
  return ulen;
}

// ecma 15.4.2.2
__jsobject *__js_new_arr_length(__jsvalue *len) {
  uint32_t length;
  bool rangeerror = false;
  // check length is number/double, check the range
  if (__is_number(len) || __is_double(len)) {
    int32_t numlen = __js_ToNumber(len);
    length = __js_ToUint32(len);
    if ((numlen < 0 && __is_number(len)) || (__is_double(len) && (length != __jsval_to_double(len)))) {
      rangeerror = true;
    }
  } else {
    // not number, set length to 1
    length = 1;
  }
  // if the argument len is a Number and ToUint32(len) is not equal to len
  if (rangeerror || __is_undefined(len) || __is_infinity(len) ||
      __is_neg_infinity(len) || __is_nan(len)) {
    MAPLE_JS_RANGEERROR_EXCEPTION();
  }
  __jsobject *arr = __js_new_arr_internal(length);
  __jsvalue *elems = &arr->shared.array_props[1];
  uint32_t allocated_length = length > ARRAY_MAXINDEXNUM_INTERNAL ? ARRAY_MAXINDEXNUM_INTERNAL : length;
  for (uint32_t i = 0; i < allocated_length; i++) {
    elems[i] = __none_value();
  }
  return arr;
}

// ecma 15.4.3.2
__jsvalue __jsarr_isArray(__jsvalue *this_array, __jsvalue *arg) {
  return __boolean_value(__is_js_array(arg));
}

// ecma 15.4.4.2
__jsvalue __jsarr_pt_toString(__jsvalue *this_array) {
  // ecma 15.4.4.2 step 1.
  __jsobject *array = __js_ToObject(this_array);
  // ecma 15.4.4.2 step 2.
  // ecma 15.4.4.2 step 3~4.
  __jsvalue result;
  if (__jsobj_helper_GetAndCall(array, JSBUILTIN_STRING_JOIN, &result)) {
    return result;
  } else {
    return __jsobj_pt_toString(this_array);
  }
}

// ecma 15.4.4.3 step 8 toLocaleString for each element of array
__jsstring *__jsarr_ElemToLocaleString(__jsvalue *elem) {
  if (__is_js_object(elem)) {
     __jsobject *obj = __jsval_to_object(elem);
     __jsvalue o = __object_value(obj);
     __jsvalue tolocalestring = __jsobj_internal_Get(obj, JSBUILTIN_STRING_TO_LOCALE_STRING_UL);
     if (!__js_IsCallable(&tolocalestring)) {
       MAPLE_JS_TYPEERROR_EXCEPTION();
     }
     __jsvalue res = __jsfun_internal_call(__jsval_to_object(&tolocalestring), &o, NULL, 0);
     return __js_ToString(&res);
  }
  return __jsarr_ElemToString(elem);
}

// ecma 15.4.4.3
// Array.prototype.Locale-specific
__jsvalue __jsarr_pt_toLocaleString(__jsvalue *this_array) {
  // ecma 15.4.4.3.1
  __jsobject *o = __js_ToObject(this_array);
  // ecma 15.4.4.3.2 & 3 : Compute the length of new regular array.
  uint32_t len = 0;
  if (__is_js_array(this_array)) {
    len = __jsobj_helper_get_length(o);
  } else {
    MAPLE_JS_ASSERT(false && "__jsarr_pt_toLocaleString::not support");
  }
  __jsstring *emptystr = __jsstr_get_builtin(JSBUILTIN_STRING_EMPTY);
  // ecma 15.4.4.3.5
  if (len == 0) {
    return __string_value(emptystr);
  }
  // ecma 15.4.4.3.4
  // TODO:: sep value could get from host environment, now use comma
  __jsstring *sep = __jsstr_get_builtin(JSBUILTIN_STRING_COMMA_CHAR);
  __jsstring *r = NULL;
  // ecma 15.4.4.3.6-7
  __jsvalue elem0 = __jsobj_internal_Get(o, (uint32_t)0);
  r = __jsarr_ElemToLocaleString(&elem0);
  // ecma 15.4.4.3.9 && 10
  for (uint32_t k = 1; k < len; k++) {
    __jsvalue elem = __jsobj_internal_Get(o, k);
    __jsstring* cur = __jsarr_ElemToLocaleString(&elem);
    __jsstring* prev = r;
    r = __jsstr_concat_3(prev, sep, cur);
    memory_manager->RecallString(cur);
    memory_manager->RecallString(prev);
  }
  GCDecRf(sep);
  // ecma 15.4.4.3.11
  return __string_value(r);
}

// Helper for __jsarr_pt_concat, copy values from src to dest.
// If the src value is not JSARRAY, it is a simple value copy and return 1,
// Else src mustbe JSARRAY, get every element of src and put them to dest,
// return the length property of src.
uint32_t __jsarr_helper_copy_values(__jsvalue *dest, __jsvalue *src) {
  if (!__is_js_array(src)) {
    GCCheckAndIncRf(src->s.asbits, IsNeedRc(src->tag));
    *dest = *src;
    return 1;
  } else {
    __jsobject *o = __jsval_to_object(src);
    uint32_t len = __jsobj_helper_get_length(o);
    for (uint32_t i = 0; i < len; i++) {
      dest[i] = __jsarr_GetElem(o, i);
      GCCheckAndIncRf(dest[i].s.asbits, IsNeedRc(dest[i].tag));
    }
    return len;
  }
}

// ecma 15.4.4.4
__jsvalue __jsarr_pt_concat(__jsvalue *this_array, __jsvalue *items, uint32_t size) {
  // ecma 15.4.4.4 step 1.
  __jsobject *o = __js_ToObject(this_array);
  // Compute the length of new regular array.
  uint32_t len = 0;
  if (__is_js_array(this_array)) {
    len += __jsobj_helper_get_length(o);
  } else {
    len += 1;
  }
  for (uint32_t i = 0; i < size; i++) {
    __jsvalue e = items[i];
    if (__is_js_array(&e)) {
      len += __jsobj_helper_get_length(__jsval_to_object(&e));
    } else {
      len++;
    }
  }
  // ecma 15.4.4.4 step 2~5.
  __jsobject *a = __js_new_arr_internal(len);
  __jsvalue *dest = &(a->shared.array_props[1]);
  uint32_t next = __jsarr_helper_copy_values(dest, this_array);
  for (uint32_t i = 0; i < size; i++) {
    __jsvalue e = items[i];
    next += __jsarr_helper_copy_values(&dest[next], &e);
  }
  return __object_value(a);
}

// ecma 15.4.4.5
__jsvalue __jsarr_pt_join(__jsvalue *this_array, __jsvalue *separator) {
  // ecma 15.4.4.5 step 1.
  __jsobject *o = __js_ToObject(this_array);
  // ecma 15.4.4.5 step 4~5.
  __jsstring *sep;
  if (__is_undefined(separator)) {
    sep = __jsstr_get_builtin(JSBUILTIN_STRING_COMMA_CHAR);
  } else {
    sep = __js_ToString(separator);
  }
  GCIncRf(sep);

  // fast path for regular array
  if (o->object_type == JSREGULAR_ARRAY) {
    __jsvalue *array = o->shared.array_props;
    uint64_t len = __jsobj_helper_get_lengthsize(o);
    if (len == 0) {
      __jsstring *empty = __jsstr_get_builtin(JSBUILTIN_STRING_EMPTY);
      return __string_value(empty);
    }
    __jsvalue element0 = __jsarr_GetRegularElem(o, array, 0);
    __jsstring *r = __jsarr_ElemToString(&element0);
    for (uint32_t k = 1; k < len; k++) {
      __jsvalue element = __jsarr_GetRegularElem(o, array, k);
      __jsstring *cur = r;
      r = __jsarr_JoinOnce(&element, cur, sep);
    }
    GCDecRf(sep);
    return __string_value(r);
  }
  // slow path for generic array
  else {
    // ecma 15.4.4.5 step 2.
    uint64_t len = __jsobj_helper_get_lengthsize(o);
    // ecma 15.4.4.5 step 6.
    if (len == 0) {
      __jsstring *empty = __jsstr_get_builtin(JSBUILTIN_STRING_EMPTY);
      return __string_value(empty);
    }
    // ecma 15.4.4.5 step 7.
    __jsvalue element0 = __jsobj_internal_Get(o, (uint32_t)0);
    // ecma 15.4.4.5 step 8.
    __jsstring *r = __jsarr_ElemToString(&element0);
    // ecma 15.4.4.5 step 9~10.
    for (uint32_t k = 1; k < len; k++) {
      __jsvalue element = __jsobj_internal_Get(o, k);
      __jsstring *cur = r;
      r = __jsarr_JoinOnce(&element, cur, sep);
    }
    // ecma 15.4.4.5 step 11.
    GCDecRf(sep);
    return __string_value(r);
  }
}

// ecma 15.4.4.6
__jsvalue __jsarr_pt_pop(__jsvalue *this_array) {
  // ecma 15.4.4.6 step 1.
  __jsobject *o = __js_ToObject(this_array);
  // fast path for regular array
  if (o->object_type == JSREGULAR_ARRAY) {
    __jsvalue *array = o->shared.array_props;
    uint64_t len = __jsobj_helper_get_lengthsize(o);
    if (len == 0) {
      return __undefined_value();
    } else {
      __jsvalue element = __jsarr_GetRegularElem(o, array, len - 1);
      o->shared.array_props = __jsarr_RegularRealloc(array, len, len - 1);
      if (!__is_none(&element)) {
        return element;
      } else {
        return __undefined_value();
      }
    }
  }
  // slow path for generic array
  else {
    // ecma 15.4.4.6 step 2~3.
    uint64_t len = __jsobj_helper_get_lengthsize(o);
    // ecma 15.4.4.6 step 4.
    if (len == 0) {
      __jsobj_helper_set_length(o, 0, true);
      return __undefined_value();
    }
    // ecma 15.4.4.6 step 5.
    else {
      uint64_t new_len = len - 1;
      __jsstring *newlenstr = __js_DoubleToString(new_len);
      __jsvalue element = __jsobj_internal_Get(o, newlenstr);
      if (new_len <= UINT32_MAX) {
        __jsobj_internal_Delete(o, new_len);
      } else {
        __jsobj_internal_Delete(o, newlenstr, false, true);
      }
      __jsobj_helper_set_length(o, new_len, true);
      return element;
    }
  }
}

// ecma 15.4.4.7
__jsvalue __jsarr_pt_push(__jsvalue *this_array, __jsvalue *items, uint32_t size) {
  // ecma 15.4.4.7 step 1.
  __jsobject *o = __js_ToObject(this_array);
  // ecma 15.4.4.7 step 2~3.
  uint64_t n = __jsobj_helper_get_lengthsize(o);
  if (size == 0) {
    return __double_value(n);
  }
  // array length is out of range
  if ((o->object_type == JSREGULAR_ARRAY) && ((n+size) > UINT32_MAX)) {
  }
  // 22.1.3.17 step 7
  if ((n+size) > MAX_LENGTH_PROPERTY_SIZE) {
    MAPLE_JS_TYPEERROR_EXCEPTION();
  }
  // fast path for regular array
  if (o->object_type == JSREGULAR_ARRAY) {
    __jsvalue *array = o->shared.array_props;
    o->shared.array_props = __jsarr_RegularRealloc(array, n, n + size);
    array = o->shared.array_props;
    for (uint32_t i = 0; i < size; i++) {
      __set_regular_elem(array, i + n, &items[i]);
    }
    n += size;
  }
  // slow path for generic array
  else {
    // ecma 15.4.4.7 step 4~5.
    for (uint32_t i = 0; i < size; i++) {
      // ecma 15.4.4.7 step 5.a.
      __jsvalue e = items[i];
      // ecma 15.4.4.7 step 5.b.
      if (n <= UINT32_MAX) {
        __jsobj_internal_Put(o, n, &e, true);
      } else {
        __jsstring *str = __js_DoubleToString(n);
        __jsobj_internal_Put(o, str, &e, true);
      }
      // ecma 15.4.4.7 step 5.c.
      n++;
    }
    // ecma 15.4.4.7 step 6.
    __jsobj_helper_set_length(o, n, true);
  }
  // ecma 15.4.4.7 step 7.
  return __double_value(n);
}

// Helper for __jsarr_pt_reverse and __jsarr_pt_sort
void __jsarr_helper_ExchangeElem(__jsobject *arr, uint32_t idx1, uint32_t idx2) {
  __jsvalue elem1, elem2;
  bool exist1 = __jsobj_helper_HasPropertyAndGet(arr, idx1, &elem1);
  bool exist2 = __jsobj_helper_HasPropertyAndGet(arr, idx2, &elem2);
  if (exist2) {
    if (exist1) {
      // Inc RF for elem1 otherwise it will goes down to 0 and get released
      GCCheckAndIncRf(elem1.s.asbits, IsNeedRc(elem1.tag));
    }
    __jsobj_internal_Put(arr, idx1, &elem2, true);
  } else {
    __jsobj_internal_Delete(arr, idx1);
  }
  if (exist1) {
    if (exist2) {
      // Inc RF for elem1 otherwise it will goes down to 0 and get released
      GCCheckAndIncRf(elem2.s.asbits, IsNeedRc((elem2.tag)));
    }
    __jsobj_internal_Put(arr, idx2, &elem1, true);
  } else {
    __jsobj_internal_Delete(arr, idx2);
  }
}

// ecma 15.4.4.8
__jsvalue __jsarr_pt_reverse(__jsvalue *this_array) {
  // ecma 15.4.4.8 step 1.
  __jsobject *o = __js_ToObject(this_array);
  // ecma 15.4.4.8 step 2~3.
  uint64_t len = __jsobj_helper_get_lengthsize(o);
  // ecma 15.4.4.8 step 4.
  uint32_t middle = len / 2;
  // ecma 15.4.4.8 step 5.
  uint32_t lower = 0;
  uint32_t upper = len - 1;
  // fast path for regular array
  if (o->object_type == JSREGULAR_ARRAY) {
    __jsvalue *array = o->shared.array_props;
    while (lower != middle) {
      __jsvalue lower_value = __jsarr_GetRegularElem(o, array, lower);
      __jsvalue upper_value = __jsarr_GetRegularElem(o, array, upper);
      __set_regular_elem(array, lower, &upper_value);
      __set_regular_elem(array, upper, &lower_value);
      lower++;
      upper--;
    }
  }
  // slow path for generic array
  else {
    // ecma 15.4.4.8 step 6.
    while (lower != middle) {
      __jsarr_helper_ExchangeElem(o, lower++, upper--);
    }
  }
  // ecma 15.4.4.8 step 7.
  return __object_value(o);
}

// ecma 15.4.4.9
__jsvalue __jsarr_pt_shift(__jsvalue *this_array) {
  // ecma 15.4.4.9 step 1.
  __jsobject *o = __js_ToObject(this_array);
  // fast path for regular array
  if (o->object_type == JSREGULAR_ARRAY) {
    __jsvalue *array = o->shared.array_props;
    uint64_t len = __jsobj_helper_get_lengthsize(o);
    if (len == 0) {
      return __undefined_value();
    }
    __jsvalue first = __jsarr_GetRegularElem(o, array, 0);
    uint32_t k = 1;
    while (k < len) {
      __jsarr_internal_MoveElem(o, array, k - 1, k);
      k++;
    }
    o->shared.array_props = __jsarr_RegularRealloc(array, len, len - 1);
    if (!__is_none(&first)) {
      return first;
    } else {
      return __undefined_value();
    }
  }
  // slow path for generic array
  else {
    // ecma 15.4.4.9 step 2~3.
    uint64_t len = __jsobj_helper_get_lengthsize(o);
    // ecma 15.4.4.9 step 4.
    if (len == 0) {
      // ecma 15.4.4.9 step 4.a.
      __jsobj_helper_set_length(o, (uint32_t)0, true);
      // ecma 15.4.4.9 step 4.b.
      return __undefined_value();
    }
    // ecma 15.4.4.9 step 5.
    __jsvalue first = __jsobj_internal_Get(o, (uint32_t)0);
    // ecma 15.4.4.9 step 6.
    uint32_t k = 1;
    // ecma 15.4.4.9 step 7.
    while (k < len) {
      __jsarr_internal_MoveElem(o, k - 1, k);
      k++;
    }
    // ecma 15.4.4.9 step 8.
    uint32_t new_len = len - 1;
    __jsobj_internal_Delete(o, new_len);
    // ecma 15.4.4.9 step 9.
    __jsobj_helper_set_length(o, new_len, true);
    // ecma 15.4.4.9 step 10.
    return first;
  }
}

// Helper for __jsarr_pt_slice, __jsarr_pt_splice and __jsarr_pt_indexOf, get the
// actual boundary value which is nonnegtive and no more than the reference value.
int64_t __jsarr_helper_get_value_from_relative(int64_t relat, int64_t refer) {
  if (relat < 0) {
    return refer + relat > 0 ? (refer + relat) : 0;
  } else {
    return refer > relat ? relat : refer;
  }
}

int64_t __jsarr_helper_get_Int64OrLength(__jsvalue *v, int64_t len) {
  int64_t i64 = 0;
 if (__is_number(v) || __is_double(v)) {
    i64 = __js_ToNumber64(v);
  } else if (__is_infinity(v)) {
    if (__is_neg_infinity(v)) {
      i64 = -len;
    } else {
      i64 = len; // use len insteadof
    }
  } else if (__is_nan(v)) {
    i64 = 0;
  } else {
    MAPLE_JS_ASSERT(0 && "__jsarr_helper_get_Int64OrLength::NIY");
  }
  return i64;
}

// ecma 15.4.4.10
__jsvalue __jsarr_pt_slice(__jsvalue *this_array, __jsvalue *start, __jsvalue *end) {
  // ecma 15.4.4.10 step 1.
  __jsobject *o = __js_ToObject(this_array);
  // ecma 15.4.4.10 step 3~4, use 22.1.3.22 step 3
  uint64_t len = __jsobj_helper_get_lengthsize(o);
  // ecma 15.4.4.10 step 5.
  bool convertible = false;
  __jsvalue startval = __js_ToNumber2(start, convertible);
  int64_t relative_start = __jsarr_helper_get_Int64OrLength(&startval, len);
  // ecma 15.4.4.10 step 6.
  int64_t k = __jsarr_helper_get_value_from_relative(relative_start, len);
  // ecma 15.4.4.10 step 7.
  int64_t relative_end;
  if (__is_undefined(end)) {
    relative_end = len;
  } else {
    convertible = false;
    __jsvalue endval = __js_ToNumber2(end, convertible);
    relative_end = __jsarr_helper_get_Int64OrLength(&endval, len);
  }
  // ecma 15.4.4.10 step 8.
  int64_t final = __jsarr_helper_get_value_from_relative(relative_end, len);
  // fast path for regular array
  if (o->object_type == JSREGULAR_ARRAY) {
    __jsvalue *array = o->shared.array_props;
    uint32_t new_len = k < final ? (final - k) : 0;
    __jsobject *a = __js_new_arr_internal(new_len);
    __jsvalue *new_array = a->shared.array_props;
    uint32_t n = 0;
    while (k < final) {
      __jsvalue k_elem = __jsarr_GetRegularElem(o, array, k);
      if (!__is_none(&k_elem)) {
        __set_regular_elem(new_array, n, &k_elem);
      }
      k++;
      n++;
    }
    return __object_value(a);
  }
  // slow path for generic array
  else {
    // ecma 15.4.4.10 step 2.
    int64_t new_len = k < final ? final - k : 0;
    // check the new length is valid array length range
    __jsvalue newlenval = __double_value(new_len);
    new_len = __js_arrlengthproperty_internal(&newlenval);
    __jsobject *a = __js_new_arr_internal(0);
    // ecma 15.4.4.10 step 9.
    // ecma 15.4.4.10 step 10.
    uint32_t n = 0;
    while (k < final) {
      // ecma 15.4.4.10 step 10.a~c
      __jsvalue k_value;
      if (__jsobj_helper_HasPropertyAndGet(o, k, &k_value)) {
        __set_generic_elem(a, n, &k_value);
      }
      // ecma 15.4.4.10 step 10.d.
      k++;
      n++;
      // ecma 15.4.4.10 step 10.e.
    }
    __jsobj_helper_set_length(a, new_len, true);  // in case the last elem is hole
    // ecma 15.4.4.10 step 11.
    return __object_value(a);
  }
}

// Helper function for __jsarr_pt_sort.
static int32_t __jsarr_SortCompare(__jsobject *obj, __jsvalue *cmpfn, uint32_t idx1, uint32_t idx2) {
  __jsvalue j, k;
  bool hasj = __jsobj_helper_HasPropertyAndGet(obj, idx1, &j);
  bool hask = __jsobj_helper_HasPropertyAndGet(obj, idx2, &k);
  // ecma 15.4.4.11 step 5.
  if (!hasj && !hask) {
    return 0;
  }
  // ecma 15.4.4.11 step 6.
  if (!hasj) {
    return 1;
  }
  // ecma 15.4.4.11 step 7.
  if (!hask) {
    return -1;
  }
  // ecma 15.4.4.11 step 10.
  if (__is_undefined(&j) && __is_undefined(&k)) {
    return 0;
  }
  // ecma 15.4.4.11 step 11.
  if (__is_undefined(&j)) {
    return 1;
  }
  // ecma 15.4.4.11 step 12.
  if (__is_undefined(&k)) {
    return -1;
  }
  // ecma 15.4.4.11 step 13.
  if (!__is_undefined(cmpfn)) {
     if (!__js_IsCallable(cmpfn)) {
       MAPLE_JS_TYPEERROR_EXCEPTION();
     }
    __jsobject *func = __jsval_to_object(cmpfn);
    __jsvalue arg_list[2] = { j, k };
    __jsvalue undefined = __undefined_value();
    __jsvalue res = __jsfun_internal_call(func, &undefined, arg_list, 2);
    return __jsval_to_number(&res);
  }
  // ecma 15.4.4.11 step 14.
  __jsstring *x_string = __js_ToString(&j);
  // ecma 15.4.4.11 step 15.
  __jsstring *y_string = __js_ToString(&k);
  ;
  // ecma 15.4.4.11 step 16~18.
  int32_t res = __jsstr_compare(x_string, y_string);
  if (!__is_string(&j)) {
    memory_manager->RecallString(x_string);
  }
  if (!__is_string(&k)) {
    memory_manager->RecallString(y_string);
  }
  return res;  // compare according to what rule?
}

// ecma 15.4.4.11
__jsvalue __jsarr_pt_sort(__jsvalue *this_array, __jsvalue *comparefn) {
  __jsobject *obj = __jsval_to_object(this_array);
  uint64_t len = __jsobj_helper_get_lengthsize(obj);
  if (len <= 0) return __object_value(obj);

  // Ignored the 4 cases that the behaviour of sort is implementation defined???
  // Use the bubble sort
  for (uint32_t m = 0; m < len - 1; m++)
    for (uint32_t n = 0; n < len - 1 - m; n++) {
      if (__jsarr_SortCompare(obj, comparefn, n, n + 1) > 0) {
        __jsarr_helper_ExchangeElem(obj, n, n + 1);
      }
    }
  return __object_value(obj);
}

// ecma 15.4.4.12
__jsvalue __jsarr_pt_splice(__jsvalue *this_array, __jsvalue *items, uint32_t size) {
  MAPLE_JS_ASSERT(size >= 2);
  // ecma 15.4.4.12 step 1.
  __jsobject *o = __js_ToObject(this_array);
  // ecma 15.4.4.12 step 3~4.
  int64_t len = __jsobj_helper_get_lengthsize(o);
  // ecma 15.4.4.12 step 5.
  bool convertible = false;
  __jsvalue start = __js_ToNumber2(&items[0], convertible);
  int64_t relative_start = __jsarr_helper_get_Int64OrLength(&start, len);
  // ecma 15.4.4.12 step 6.
  int64_t actual_start = __jsarr_helper_get_value_from_relative(relative_start, len);
  // ecma 15.4.4.12 step 7.
  convertible = false;
  __jsvalue delete_count = __js_ToNumber2(&items[1], convertible);
  int64_t delete_num = __jsarr_helper_get_Int64OrLength(&delete_count, len);
  delete_num = (delete_num > 0) ? delete_num : 0;
  uint32_t actual_delete_count = (delete_num > ((int64_t)len - actual_start)) ?
                                    ((int64_t)len - actual_start) : delete_num;
  // ecma 15.4.4.12 step 10~11.
  uint32_t item_count = (int32_t)size - 2;
  // ecma 15.4.4.12 step 2.
  __jsobject *a = __js_new_arr_internal(0);
  // ecma 15.4.4.12 step 8~9.
  for (int32_t k = 0; k < actual_delete_count; k++) {
    __jsvalue from_value;
    int64_t indexprop = actual_start + k;
    if (indexprop <= UINT32_MAX) {
      if (__jsobj_helper_HasPropertyAndGet(o, indexprop, &from_value)) {
        __set_generic_elem(a, k, &from_value);
      }
    } else {
      __jsstring *strprop = __js_DoubleToString(indexprop);
      if (__jsobj_helper_HasPropertyAndGet(o, strprop, &from_value)) {
        __set_generic_elem(a, k, &from_value);
      }
    }
  }
  uint32_t new_len = actual_delete_count;
  __jsobj_helper_set_length(a, new_len, true);
  // ecma 15.4.4.12 step 12.
  if (item_count < actual_delete_count) {
    int64_t k = actual_start;
    while (k < (int32_t)len - actual_delete_count) {
      __jsarr_internal_MoveElem(o, k + item_count, k + actual_delete_count);
      k++;
    }
    k = len;
    while (k > (int32_t)len - actual_delete_count + item_count) {
      __jsobj_internal_Delete(o, k - 1);
      k--;
    }
  }
  // ecma 15.4.4.12 step 13.
  else if (item_count > actual_delete_count) {
    int32_t k = (int64_t)len - actual_delete_count;
    while (k > actual_start) {
      __jsarr_internal_MoveElem(o, k + item_count - 1, k + actual_delete_count - 1);
      k--;
    }
  }
  // ecma 15.4.4.12 step 14.
  int64_t k = actual_start;
  // ecma 15.4.4.12 step 15.
  for (uint32_t i = 0; i < size - 2; i++) {
    __jsvalue e = items[i + 2];
    __jsobj_internal_Put(o, k, &e, true);
    k++;
  }
  // ecma 15.4.4.12 step 16.
  new_len = (uint32_t)((int32_t)len - actual_delete_count + item_count);
  __jsobj_helper_set_length(o, new_len, true);  // in case the last elem is hole
  // ecma 15.4.4.12 step 17.
  return __object_value(a);
}

// ecma 15.4.4.13
__jsvalue __jsarr_pt_unshift(__jsvalue *this_array, __jsvalue *items, uint32_t size) {
  // ecma 15.4.4.13 step 1.
  __jsobject *o = __js_ToObject(this_array);
  // ecma 15.4.4.13 step 2~3.
  uint64_t len = __jsobj_helper_get_lengthsize(o);
  // ecma 15.4.4.13 step 5.
  uint32_t k = len;
  // fast path for regular array
  if (o->object_type == JSREGULAR_ARRAY) {
    __jsvalue *array = o->shared.array_props;
    o->shared.array_props = __jsarr_RegularRealloc(array, len, len + size);
    array = o->shared.array_props;
    while (k > 0) {
      __jsarr_internal_MoveElem(o, array, k + size - 1, k - 1);
      k--;
    }
    for (uint32_t j = 0; j < size; j++) {
      __set_regular_elem(array, j, &items[j]);
    }
  }
  // slow path for generic array
  else {
    // ecma 15.4.4.13 step 6.
    while (k > 0) {
      __jsarr_internal_MoveElem(o, k + size - 1, k - 1);
      k--;
    }
    // ecma 15.4.4.13 step 7~9.
    for (uint32_t j = 0; j < size; j++) {
      __jsvalue e = items[j];
      __jsobj_internal_Put(o, j, &e, true);
    }
    // ecma 15.4.4.13 step 10.
    __jsobj_helper_set_length(o, len + size, true);
  }
  // ecma 15.4.4.13 step 11.
  return __number_value(len + size);
}

// ecma 15.4.4.14
__jsvalue __jsarr_pt_indexOf(__jsvalue *this_array, __jsvalue *arg_list, uint32_t argNum) {
  // ecma 15.4.4.14 step 1.
  __jsobject *o = __js_ToObject(this_array);
  // ecma 15.4.4.14 step 2~3.
  uint64_t len = __jsobj_helper_get_lengthsize(o);
  // ecma 15.4.4.14 step 4.
  if (len == 0) {
    return __number_value(-1);
  }
  MAPLE_JS_ASSERT((argNum <= 2) && "__jsarr_pt_indexOf");
  __jsvalue *searchElement = NULL;
  __jsvalue undefinedVal = __undefined_value();
  if (argNum > 0) {
    searchElement = &arg_list[0];
  } else {
    searchElement = &undefinedVal;
  }
  // ecma 15.4.4.14 step 5.
  int64_t n;
  if (argNum == 2) {
    bool convertible = false;
    __jsvalue nval = __js_ToNumber2(&arg_list[1], convertible);
    n = __jsarr_helper_get_Int64OrLength(&nval, len);
  } else {
    n = 0;
  }
  // ecma 15.4.4.14 step 6.
  if (n >= (int64_t)len) {
    return __number_value(-1);
  }
  // ecma 15.4.4.14 step 7-8.
  int64_t k = __jsarr_helper_get_value_from_relative(n, len);
  // fast path for regular array
  if (o->object_type == JSREGULAR_ARRAY) {
    __jsvalue *array = o->shared.array_props;
    while (k < len) {
      __jsvalue element_k = __jsarr_GetRegularElem(o, array, k);
      if (!__is_none(&element_k)) {
        if (__js_StrictEquality(searchElement, &element_k)) {
          return (k <= INT32_MAX) ? __number_value(k) : __double_value(k);
        }
      }
      k++;
    }
  }
  // slow path for generic array
  else {
    if (o->object_class == __jsobj_class::JSSTRING) {
        __jsstring *srch = __js_ToString(arg_list);
        if(__jsstr_get_length(srch) != 1)
            return __number_value(-1);
        uint16_t c = __jsstr_get_char(srch, 0);
        __jsstring *src = o->shared.prim_string;
        int32_t i;
        for (i = k; i < len; ++i) {
            if (__jsstr_get_char(src, i) == c)
                break;
        }
        return i < len ? __number_value(i) : __number_value(-1);
    }
    // ecma 15.4.4.14 step 9.
    while (k < len) {
      // ecma 15.4.4.14 step 9.a.
      __jsvalue element_k;
      if (k <= UINT32_MAX) {
        if (__jsobj_helper_HasPropertyAndGet(o, k, &element_k)) {
          // ecma 15.4.4.14 step 9.b.
          bool same = __js_StrictEquality(searchElement, &element_k);
          if (same) {
            return ((k <= INT32_MAX) ? __number_value(k) : __double_value(k));
          }
        }
      } else {
        __jsstring *str = __js_DoubleToString(k);
        if (__jsobj_helper_HasPropertyAndGet(o, str, &element_k)) {
          bool same =  __js_StrictEquality(searchElement, &element_k);
          return __double_value(k);
        }
      }
      // ecma 15.4.4.14 step 9.c.
      k++;
    }
  }
  // ecma 15.4.4.14 step 10.
  return __number_value(-1);
}

// ecma 15.4.4.15
__jsvalue __jsarr_pt_lastIndexOf(__jsvalue *this_array, __jsvalue *arg_list, uint32_t argNum) {
  // ecma 15.4.4.15 step 1.
  __jsobject *o = __js_ToObject(this_array);
  // ecma 15.4.4.15 step 2~3.
  uint64_t len = __jsobj_helper_get_lengthsize(o);
  // ecma 15.4.4.15 step 4.
  if (len == 0) {
    return __number_value(-1);
  }
  // ecma 15.4.4.15 step 5.
  MAPLE_JS_ASSERT((argNum <= 2) && "__jsarr_pt_lastIndexOf");
  __jsvalue *searchElement = NULL;
  __jsvalue undefinedVal = __undefined_value();
  if (argNum > 0) {
    searchElement = &arg_list[0];
  } else {
    searchElement = &undefinedVal;
  }
  int64_t n;
  if (argNum == 2) {
    bool convertible = false;
    __jsvalue nval = __js_ToNumber2(&arg_list[1], convertible);
    n = __jsarr_helper_get_Int64OrLength(&nval, len+1); // use len+1 to represent +infinity
  } else {
    n = (int64_t)len - 1;
  }
  // ecma 15.4.4.15 step 6.
  int64_t k;
  if (n >= 0) {
    k = n < ((int64_t)len - 1) ? n : ((int64_t)len - 1);
  }
  // ecma 15.4.4.15 step 7.
  else {
    k = (int64_t)len + n;
  }
  // fast path for regular array
  if (o->object_type == JSREGULAR_ARRAY) {
    __jsvalue *array = o->shared.array_props;
    while (k >= 0) {
      __jsvalue element_k = __jsarr_GetRegularElem(o, array, k);
      if (!__is_none(&element_k)) {
        if (__js_StrictEquality(searchElement, &element_k)) {
          return __number_value(k);
        }
      }
      k--;
    }
  }
  // slow path for generic array
  else {
    if (o->object_class == __jsobj_class::JSSTRING) {
        __jsstring *srch = __js_ToString(arg_list);
        if(__jsstr_get_length(srch) != 1)
            return __number_value(-1);
        uint16_t c = __jsstr_get_char(srch, 0);
        __jsstring *src = o->shared.prim_string;
        int32_t i;
        for (i = k; i >= 0; --i) {
            if (__jsstr_get_char(src, i) == c)
                break;
        }
        return __number_value(i);
    }
    // ecma 15.4.4.15 step 8.
    while (k >= 0) {
      // ecma 15.4.4.15 step 8.a.
      __jsvalue element_k;
      if (k <= UINT32_MAX) {
        if (__jsobj_helper_HasPropertyAndGet(o, k, &element_k)) {
          // ecma 15.4.4.15 step 8.b.
          bool same = __js_StrictEquality(searchElement, &element_k);
          if (same) {
            return (k <= INT32_MAX) ? __number_value(k) : __double_value(k);
          }
        }
      } else {
        __jsstring *str = __js_DoubleToString(k);
        if (__jsobj_helper_HasPropertyAndGet(o, str, &element_k)) {
          bool same = __js_StrictEquality(searchElement, &element_k);
          if (same) {
            return __double_value(k);
          }
        }
      }
      // ecma 15.4.4.15 step 8.c.
      k--;
    }
  }
  // ecma 15.4.4.15 step 9.
  return __number_value(-1);
}

// Helper for __jsarr_pt_every, __jsarr_pt_some, __jsarr_pt_forEach, __jsarr_pt_map and __jsarr_pt_filter,
//  __jsarr_pt_find
// traverse each element and run the specified callback function, return a boolean value.
__jsvalue __jsarr_helper_iter_method(__jsvalue *this_array, __jsvalue *callbackfn, __jsvalue *this_arg, uint64_t len,
                                     uint8_t iter_type) {
  uint64_t k = 0, to = 0;
  bool res = (iter_type == JSARR_EVERY) ? false : true;
  __jsobject *o = __js_ToObject(this_array);
  __jsvalue  vo = __object_value(o);
  __jsobject *func = __jsval_to_object(callbackfn);
  __jsobject *a = nullptr;
  if (iter_type == JSARR_MAP) {
    // ecma 6.0 22.1.3.15 step 7 Let A be ArraySpeciesCreate(O, len).
    __jsvalue lenval = len <= INT32_MAX ? __number_value(len) : __double_value(len);
    uint32_t validLen = __js_arrlengthproperty_internal(&lenval);
    a = __js_new_arr_internal(validLen);
  } else if (iter_type == JSARR_FILTER) {
    a = __js_new_arr_internal(0);
  }

  while (k < len) {
    __jsvalue k_value;
    if (__jsobj_helper_HasPropertyAndGet(o, k, &k_value)) {
      __jsvalue arg_list[3] = { k_value, __number_value(k), vo };
      __jsvalue result_val = __jsfun_internal_call(func, this_arg, arg_list, 3);
      if (iter_type == JSARR_EVERY || iter_type == JSARR_SOME) {
        if (__js_ToBoolean(&result_val) == res) {
          return __boolean_value(res);
        }
      } else if (iter_type == JSARR_FIND) {
        if (__js_ToBoolean(&result_val) == true) {
          return k_value;
        }
      } else if (iter_type == JSARR_MAP) {
        __set_generic_elem(a, k, &result_val);
      } else if (iter_type == JSARR_FILTER) {
        if (__js_ToBoolean(&result_val)) {
          __set_generic_elem(a, to++, &k_value);
        }
      }
    } else if (iter_type == JSARR_FIND) {
      // array doesn't have k property
      return __object_value(a);
    }
    k++;
  }
  return ((a == nullptr) && (iter_type != JSARR_FIND)) ? __boolean_value(!res) :  __object_value(a);
}

// ecma 15.4.4.16
__jsvalue __jsarr_pt_every(__jsvalue *this_array, __jsvalue *arg_list, uint32_t argNum) {
  // ecma 15.4.4.16 step 1.
  __jsobject *o = __js_ToObject(this_array);
  __jsobj_helper_convert_to_generic(o);
  // ecma 15.4.4.16 step 2~3.
  uint64_t len = __jsobj_helper_get_lengthsize(o);
  // ecma 15.4.4.16 step 4.
  MAPLE_JS_ASSERT((argNum <= 2) && "__jsarr_pt_every");
  __jsvalue undefinedVal = __undefined_value();
  __jsvalue *callbackfn = argNum > 0 ? (&arg_list[0]) : (&undefinedVal);
  if (__is_none(callbackfn)) {
    MAPLE_JS_REFERENCEERROR_EXCEPTION();
  } else if (!__js_IsCallable(callbackfn)) {
    MAPLE_JS_TYPEERROR_EXCEPTION();
  }
  // ecma 15.4.4.16 step 5.
  __jsvalue *t;
  if (argNum == 2) {
    t = &arg_list[1];
  } else {
    t = &undefinedVal;
  }
  // ecma 15.4.4.16 step 6-8.
  return __jsarr_helper_iter_method(this_array, callbackfn, t, len, JSARR_EVERY);
}

// ecma 15.4.4.17
__jsvalue __jsarr_pt_some(__jsvalue *this_array, __jsvalue *arg_list, uint32_t argNum) {
  // ecma 15.4.4.17 step 1.
  __jsobject *o = __js_ToObject(this_array);
  __jsobj_helper_convert_to_generic(o);
  // ecma 15.4.4.17 step 2~3.
  uint64_t len = __jsobj_helper_get_lengthsize(o);
  // ecma 15.4.4.17 step 4.
  MAPLE_JS_ASSERT((argNum <= 2) && "__jsarr_pt_some");
  __jsvalue undefinedVal = __undefined_value();
  __jsvalue *callbackfn = argNum > 0 ? (&arg_list[0]) : (&undefinedVal);
  if (__is_none(callbackfn)) {
    MAPLE_JS_REFERENCEERROR_EXCEPTION();
  } else if (!__js_IsCallable(callbackfn)) {
    MAPLE_JS_TYPEERROR_EXCEPTION();
  }
  // ecma 15.4.4.17 step 5.
  __jsvalue *t;
  if (argNum == 2) {
    t = &arg_list[1];
  } else {
    t = &undefinedVal;
  }
  // ecma 15.4.4.17 step 6-8.
  return __jsarr_helper_iter_method(this_array, callbackfn, t, len, JSARR_SOME);
}

// ecma 15.4.4.18
__jsvalue __jsarr_pt_forEach(__jsvalue *this_array,  __jsvalue *arg_list, uint32_t argNum) {
  // ecma 15.4.4.18 step 1.
  __jsobject *o = __js_ToObject(this_array);
  __jsobj_helper_convert_to_generic(o);
  // ecma 15.4.4.18 step 2~3.
  uint64_t len = __jsobj_helper_get_lengthsize(o);
  // ecma 15.4.4.18 step 4.
  MAPLE_JS_ASSERT((argNum <= 2) && "__jsarr_pt_forEach");
  __jsvalue undefinedVal = __undefined_value();
  __jsvalue *callbackfn = argNum > 0 ? (&arg_list[0]) : (&undefinedVal);
  if (__is_none(callbackfn)) {
    MAPLE_JS_REFERENCEERROR_EXCEPTION();
  } else if (!__js_IsCallable(callbackfn)) {
    MAPLE_JS_TYPEERROR_EXCEPTION();
  }
  // ecma 15.4.4.18 step 5.
  __jsvalue *t;
  if (argNum == 2) {
    t = &arg_list[1];
  } else {
    t = &undefinedVal;
  }
  // ecma 15.4.4.18 step 6-7.
  MAPLE_JS_ASSERT(len <= UINT32_MAX);
  __jsarr_helper_iter_method(this_array, callbackfn, t, len, JSARR_FOREACH);
  // ecma 15.4.4.18 step 8.
  return __undefined_value();
}

// ecma 15.4.4.19
__jsvalue __jsarr_pt_map(__jsvalue *this_array,  __jsvalue *arg_list, uint32_t argNum) {
  // ecma 15.4.4.19 step 1.
  __jsobject *o = __js_ToObject(this_array);
  __jsobj_helper_convert_to_generic(o);
  // ecma 15.4.4.19 step 2~3.
  uint64_t len = __jsobj_helper_get_lengthsize(o);
  // ecma 15.4.4.19 step 4.
  MAPLE_JS_ASSERT((argNum <= 2) && "__jsarr_pt_map");
  __jsvalue undefinedVal = __undefined_value();
  __jsvalue *callbackfn = argNum > 0 ? (&arg_list[0]) : (&undefinedVal);
  if (__is_none(callbackfn)) {
    MAPLE_JS_REFERENCEERROR_EXCEPTION();
  } else if (!__js_IsCallable(callbackfn)) {
    MAPLE_JS_TYPEERROR_EXCEPTION();
  }
  // ecma 15.4.4.19 step 5.
  __jsvalue *t;
  if (argNum == 2) {
    t = &arg_list[1];
  } else {
    t = &undefinedVal;
  }
  // ecma 15.4.4.19 step 6-9.
  return __jsarr_helper_iter_method(this_array, callbackfn, t, len, JSARR_MAP);
}

// ecma 15.4.4.20
__jsvalue __jsarr_pt_filter(__jsvalue *this_array, __jsvalue *arg_list, uint32_t argNum) {
  // ecma 15.4.4.20 step 1.
  __jsobject *o = __js_ToObject(this_array);
  __jsobj_helper_convert_to_generic(o);
  // ecma 15.4.4.20 step 2~3.
  uint64_t len = __jsobj_helper_get_lengthsize(o);
  // ecma 15.4.4.20 step 4.
  MAPLE_JS_ASSERT((argNum <= 2) && "__jsarr_pt_filter");
  __jsvalue undefinedVal = __undefined_value();
  __jsvalue *callbackfn = argNum > 0 ? (&arg_list[0]) : (&undefinedVal);
  if (__is_none(callbackfn)) {
    MAPLE_JS_REFERENCEERROR_EXCEPTION();
  } else if (!__js_IsCallable(callbackfn)) {
    MAPLE_JS_TYPEERROR_EXCEPTION();
  }
  // ecma 15.4.4.20 step 5.
  __jsvalue *t;
  if (argNum == 2) {
    t = &arg_list[1];
  } else {
    t = &undefinedVal;
  }
  // ecma 15.4.4.20 step 6-10.
  MAPLE_JS_ASSERT(len <= UINT32_MAX);
  return __jsarr_helper_iter_method(this_array, callbackfn, t, len, JSARR_FILTER);
}

// ecma 15.4.4.21
__jsvalue __jsarr_pt_reduce(__jsvalue *this_array, __jsvalue *arg_list, uint32_t argNum) {
  return __jsarr_internal_reduce(this_array, arg_list, argNum, false);
}

// ecma 15.4.4.22
__jsvalue __jsarr_pt_reduceRight(__jsvalue *this_array, __jsvalue *arg_list, uint32_t argNum) {
  return __jsarr_internal_reduce(this_array, arg_list, argNum, true);
}

__jsvalue __jsarr_internal_reduce(__jsvalue *this_array, __jsvalue *arg_list, uint32_t argNum,
                                  bool right_flag) {
  // ecma 15.4.4.21 step 1.
  __jsobject *o = __js_ToObject(this_array);
  __jsvalue vo = __object_value(o);
  __jsobj_helper_convert_to_generic(o);
  // ecma 15.4.4.21 step 2~3.
  uint64_t len = __jsobj_helper_get_lengthsize(o);
  // ecma 15.4.4.21 step 4.
  MAPLE_JS_ASSERT((argNum <= 2) && "__jsarr_pt_forEach");
  __jsvalue undefinedVal = __undefined_value();
  __jsvalue *callbackfn = argNum > 0 ? (&arg_list[0]) : (&undefinedVal);
  if (__is_none(callbackfn)) {
    MAPLE_JS_REFERENCEERROR_EXCEPTION();
  } else if (!__js_IsCallable(callbackfn)) {
    MAPLE_JS_TYPEERROR_EXCEPTION();
  }
  __jsvalue *initial_value;
  // ecma 15.4.4.21 step 5.
  if (len == 0 && argNum == 1) {
    MAPLE_JS_TYPEERROR_EXCEPTION();
  }
  // ecma 15.4.4.21 step 6.
  int64_t k = right_flag ? (int64_t)(len) - 1 : 0;
  // ecma 15.4.4.21 step 7.
  __jsvalue accumulator = __undefined_value();
  if (argNum == 2) {
    accumulator = arg_list[1];
  }
  // ecma 15.4.4.21 step 8.
  else {
    bool k_present = false;
    while (!k_present && (right_flag ? (k >= 0) : (k < len))) {
      if (k <= UINT32_MAX) {
        k_present = __jsobj_helper_HasPropertyAndGet(o, k, &accumulator);
      } else {
        __jsstring *kstr = __js_DoubleToString(k);
        k_present = __jsobj_helper_HasPropertyAndGet(o, kstr, &accumulator);
      }
      k = right_flag ? (k - 1) : (k + 1);
    }
    if (!k_present) {
      MAPLE_JS_TYPEERROR_EXCEPTION();
    }
  }
  // ecma 15.4.4.21 step 9.
  __jsobject *func = __jsval_to_object(callbackfn);
  while (right_flag ? (k >= 0) : (k < len)) {
    __jsvalue k_value;
    if (k <= UINT32_MAX) {
      if (__jsobj_helper_HasPropertyAndGet(o, k, &k_value)) {
        __jsvalue arg_list[4] = { accumulator, k_value, __number_value(k), vo };
        __jsvalue undefined = __undefined_value();
        accumulator = __jsfun_internal_call(func, &undefined, arg_list, 4);
      }
    } else {
      __jsstring *kstr = __js_DoubleToString(k);
      if (__jsobj_helper_HasPropertyAndGet(o, kstr, &k_value)) {
        __jsvalue arg_list[4] = { accumulator, k_value, __number_value(k), vo };
        __jsvalue undefined = __undefined_value();
        accumulator = __jsfun_internal_call(func, &undefined, arg_list, 4);
      }
    }
    k = right_flag ? (k - 1) : (k + 1);
  }
  // ecma 15.4.4.21 step 10.
  return accumulator;
}

void __jsarr_internal_DefineLengthProperty(__jsobject *a, __jsprop_desc desc, bool throw_p) {
  // ecma 15.4.5.1 step 1.
  __jsprop_desc oldLenDesc = __jsobj_internal_GetOwnProperty(a, JSBUILTIN_STRING_LENGTH);
  // ecma 15.4.5.1 step 2.
  __jsvalue val = __get_value(oldLenDesc);
  uint32_t oldLen = __js_ToUint32(&val);
  // ecma 15.4.5.1 step 3.a.
  if (!__has_value(desc))
  // ecma 15.4.5.1 step 3.a.i.
  {
    return __jsobj_internal_DefineOwnProperty(a, JSBUILTIN_STRING_LENGTH, desc, throw_p);
  }
  // ecma 15.4.5.1 step 3.b.
  __jsprop_desc newLenDesc = desc;
  // ecma 15.4.5.1 step 3.c.
  val = __get_value(desc);
  // ecma 15.4.5.1 step 3.d.
  uint32_t newLen = __js_arrlengthproperty_internal(&val);
  // ecma 15.4.5.1 step 3.e.
  __jsvalue newLen_val = (newLen <= INT32_MAX) ? __number_value(newLen) : __double_value(newLen);
  __set_value(&newLenDesc, &newLen_val);
  // ecma 15.4.5.1 step 3.f.
  if (newLen >= oldLen) {
    // ecma 15.4.5.1 step 3.f.i.
    __jsobj_internal_DefineOwnProperty(a, JSBUILTIN_STRING_LENGTH, newLenDesc, throw_p);
    return;
  }
  // ecma 15.4.5.1 step 3.g.
  if (__has_and_unwritable(oldLenDesc)) {
    if (throw_p) {
      MAPLE_JS_TYPEERROR_EXCEPTION();
    } else {
      return;
    }
  }
  // ecma 15.4.5.1 step 3.h.
  bool newWritable;
  if (!__has_and_unwritable(newLenDesc)) {
    newWritable = true;
  }
  // ecma 15.4.5.1 step 3.i.
  else {
    // ecma 15.4.5.1 step 3.i.i.
    __set_writable(&desc, false);
    // ecma 15.4.5.1 step 3.i.ii.
    newWritable = false;
    // ecma 15.4.5.1 step 3.i.iii.
    __set_writable(&newLenDesc, true);
  }
  // ecma 15.4.5.1 step 3.j~k.
  __jsobj_internal_DefineOwnProperty(a, JSBUILTIN_STRING_LENGTH, newLenDesc, throw_p);

  // ecma 15.4.5.1 step 3.l. iterate property list instead of iterate length value
  if (newLen < oldLen) {
    std::vector<uint32_t> propidx_arr;
    __jsprop *p = a->prop_list;
    while (p && p->isIndex) {
      if (p->n.index >= newLen) {
        propidx_arr.push_back(p->n.index);
      }
      p = p->next;
    }
    // delete property from the largest index
    std::sort(propidx_arr.begin(), propidx_arr.end(), std::greater<int>());
    for (auto idx : propidx_arr) {
      if (!__jsobj_internal_Delete(a, idx)) {
        newLenDesc.named_data_property.value = __number_value(idx + 1);
        // ecma 15.4.5.1 step 3.l.iii.2.
        if (!newWritable) {
          __set_writable(&newLenDesc, false);
        }
        // ecma 15.4.5.1 step 3.l.iii.3.
        __jsobj_internal_DefineOwnProperty(a, JSBUILTIN_STRING_LENGTH, newLenDesc, false);
        // ecma 15.4.5.1 step 3.l.iii.4.
        if (throw_p) {
          MAPLE_JS_TYPEERROR_EXCEPTION();
        } else {
          return;
        }
      }
    }
  }
  // ecma 15.4.5.1 step 3.m.
  if (!newWritable) {
    __jsprop_desc temp_desc = __new_empty_desc();
    __set_writable(&temp_desc, false);
    __jsobj_internal_DefineOwnProperty(a, JSBUILTIN_STRING_LENGTH, temp_desc, false);
  }
  // ecma 15.4.5.1 step 3.n.
  return;
}

void __jsarr_internal_DefineElemProperty(__jsobject *a, uint32_t index, __jsprop_desc desc, bool throw_p) {
  __jsprop_desc old_len_desc = __jsobj_internal_GetOwnProperty(a, JSBUILTIN_STRING_LENGTH);
  // MAPLE_JS_ASSERT(__jsprop_desc_IsDataDescriptor(old_len_desc));
  __jsvalue old_len_value = __get_value(old_len_desc);
  MAPLE_JS_ASSERT(__is_number(&old_len_value) || __is_double(&old_len_value));
  uint32_t old_len = __jsval_to_uint32(&old_len_value);
  if (index >= old_len && __has_and_unwritable(old_len_desc)) {
    if (throw_p) {
      MAPLE_JS_TYPEERROR_EXCEPTION();
    } else {
      return;
    }
  }
  if (index >= old_len) {
    __jsvalue new_len_value = (index+1) <= INT32_MAX ? __number_value(index + 1) : __double_value(index+1);
    __set_value(&old_len_desc, &new_len_value);
    __jsobj_internal_DefineOwnProperty(a, JSBUILTIN_STRING_LENGTH, old_len_desc, false);
  }
  __jsobj_internal_DefineOwnPropertyByValue(a, index, desc, throw_p);
  return;
}

void __jsarr_internal_DefineOwnPropertyByValue(__jsobject *a, uint32 index, __jsprop_desc desc, bool throw_p) {
  // ecma 15.4.5.1 step 3.
  __jsobj_helper_convert_to_generic(a);
  if (index != MAX_ARRAY_INDEX) {
    __jsarr_internal_DefineElemProperty(a, index, desc, throw_p);
    return;
  }
  // ecma 15.4.5.1 step 5.
  __jsobj_internal_DefineOwnPropertyByValue(a, index, desc, throw_p);
}
// ecma 15.4.5.1
void __jsarr_internal_DefineOwnProperty(__jsobject *a, __jsvalue *p, __jsprop_desc desc, bool throw_p) {
  // ecma 15.4.5.1 step 3.
  __jsobj_helper_convert_to_generic(a);
  if (__is_string(p)) {
    if (__jsstr_equal_to_builtin(__jsval_to_string(p), JSBUILTIN_STRING_LENGTH)) {
      __jsarr_internal_DefineLengthProperty(a, desc, throw_p);
      return;
    }
  }
  // ecma 15.4.
  uint32_t index = __jsarr_getIndex(p);
  if (index != MAX_ARRAY_INDEX && (index <= ARRAY_MAXINDEXNUM_INTERNAL)) {
    __jsarr_internal_DefineElemProperty(a, index, desc, throw_p);
    return;
  }
  // ecma 15.4.5.1 step 5.
  __jsobj_internal_DefineOwnProperty(a, p, desc, throw_p);
}

uint32_t __jsarr_getIndex(__jsvalue *p) {
  if (__is_number(p)) {
    int32_t idx = __jsval_to_number(p);
    if (idx >= 0) {
      return (uint32_t)idx;
    }
  } else if (__is_double(p)) {
    double d = __jsval_to_double(p);
    if (d >= MAX_ARRAY_INDEX || d < 0.0) {
      return MAX_ARRAY_INDEX;
    } else {
      return (uint32_t)d;
    }
  } else {
    __jsstring *name = __js_ToString(p);
    bool convertible = false;
    uint32_t idx = __jsstr_is_numidx(name, convertible);
    if (!convertible) {
      if (!__is_string(p)) {
        memory_manager->RecallString(name);
      }
      return MAX_ARRAY_INDEX;
    }
    __jsvalue int_val = __number_value(idx);
    __jsstring *int_val_str = __js_ToString(&int_val);
    bool isIndex = __jsstr_equal(name, int_val_str);
    memory_manager->RecallString(int_val_str);
    if (!__is_string(p)) {
      memory_manager->RecallString(name);
    }
    if (isIndex) {
      return idx;
    }
  }
  return MAX_ARRAY_INDEX;
}

void __set_generic_elem(__jsobject *arr, uint32_t index, __jsvalue *v) {
  __jsprop_desc elem = __new_value_desc(v, JSPROP_DESC_HAS_VWEC);
  __jsarr_internal_DefineElemProperty(arr, index, elem, true);
}

void __set_regular_elem(__jsvalue *arr, uint32_t index, __jsvalue *v) {
#ifndef RC_NO_MMAP
  memory_manager->UpdateGCReference(&arr[index + 1].s.payload.ptr, JsvalToMval(*v));
#else
  GCCheckAndUpdateRf(arr[index + 1].s.asbits, IsNeedRc(arr[index +1].tag), v->s.asbits, IsNeedRc(v->tag));
#endif
  arr[index + 1] = *v;
}

void __jsarr_internal_MoveElem(__jsobject *arr, uint32_t to_idx, uint32_t from_idx) {
  __jsvalue from_value;
  if (__jsobj_helper_HasPropertyAndGet(arr, from_idx, &from_value)) {
    __jsobj_internal_Put(arr, to_idx, &from_value, true);
  } else {
    __jsobj_internal_Delete(arr, to_idx);
  }
}

void __jsarr_internal_MoveElem(__jsobject *o, __jsvalue *arr, uint32_t to_idx, uint32_t from_idx) {
  __jsvalue from_val = __jsarr_GetRegularElem(o, arr, from_idx);
  if (!__is_none(&from_val)) {
    __set_regular_elem(arr, to_idx, &from_val);
  } else {
    __jsvalue none_val = __none_value();
    __set_regular_elem(arr, to_idx, &none_val);
  }
}

__jsstring *__jsarr_ElemToString(__jsvalue *elem) {
  if (__is_null_or_undefined(elem) || __is_none(elem)) {
    return __jsstr_get_builtin(JSBUILTIN_STRING_EMPTY);
  } else {
    return __js_ToString(elem);
  }
}

__jsstring *__jsarr_JoinOnce(__jsvalue *elem, __jsstring *cur, __jsstring *sep) {
  __jsstring *next = __jsarr_ElemToString(elem);
  __jsstring *r = __jsstr_concat_3(cur, sep, next);
  memory_manager->RecallString(cur);
  memory_manager->RecallString(next);
  return r;
}

__jsvalue __jsarr_GetRegularElem(__jsobject *o, __jsvalue *arr, uint32_t idx) {
  // uese idx + 1 because arr[0] is the length of this array
  MAPLE_JS_ASSERT(o->object_type == JSREGULAR_ARRAY);
  if (idx <= ARRAY_MAXINDEXNUM_INTERNAL) {
    __jsvalue elem = arr[idx + 1];
    if (!__is_none(&elem)) {
      return elem;
    }
  }
  __jsvalue result;
  __jsobject *proto = __jsobj_get_prototype(o);
  if (__jsobj_helper_HasPropertyAndGet(proto, idx, &result)) {
    return result;
  } else {
    return __none_value();
  }
}

__jsvalue __jsarr_GetElem(__jsobject *o, uint32_t idx) {
  MAPLE_JS_ASSERT(o->object_class == JSARRAY);
  if (o->object_type == JSREGULAR_ARRAY) {
    __jsvalue *array = o->shared.array_props;
    return __jsarr_GetRegularElem(o, array, idx);
  }
  __jsvalue result = __jsobj_internal_Get(o, idx);
  if (__is_undefined(&result)) {
    return __none_value();
  }
  return result;
}

__jsvalue *__jsarr_RegularRealloc(__jsvalue *arr, uint32_t old_len, uint32_t new_len) {
  uint32_t old_size = old_len + 1;
  uint32_t new_size = new_len + 1;
#ifndef RC_NO_MMAP
  uint32_t old_addr[old_size], new_addr[new_size];
#endif
  if (new_len < old_len) {
    for (uint32_t i = new_size; i < old_size; i++) {
#ifdef MACHINE64
      GCCheckAndDecRf(arr[i].s.asbits, IsNeedRc(arr[i].tag));
#else
      GCDecRf(arr[i].s.payload.ptr);
#endif
    }
  }
#ifndef RC_NO_MMAP
  for (uint32_t i = 0; i < old_size; i++) {
    old_addr[i] = (uint32_t)(&arr[i].s.payload.ptr);
  }
#endif
  __jsvalue *new_arr = (__jsvalue *)VMReallocGC(arr, (old_size) * sizeof(__jsvalue), (new_size) * sizeof(__jsvalue));
  if (new_len > old_len) {
    for (uint32_t i = old_len; i < new_len; i++) {
      __jsvalue none_val = __none_value();
      __set_regular_elem(new_arr, i, &none_val);
    }
  }
  new_arr[0] = __number_value(new_len);
#ifndef RC_NO_MMAP
  for (uint32_t i = 0; i < new_size; i++) {
    new_addr[i] = (uint32_t)(&new_arr[i].s.payload.ptr);
  }
  memory_manager->UpdateAddrMap(old_addr, new_addr, old_size, new_size);
#endif
  return new_arr;
}

// ecma 23.1.2.3 Array.of(..items)
__jsvalue __jsarr_pt_of(__jsvalue *this_array, __jsvalue *items, uint32_t size) {
  if (!__is_js_function(this_array)) {
    MAPLE_JS_ASSERT(false && "NIY: __jsarr_pt_of");
  }
  // create a new array
  return __object_value(__js_new_arr_elems_direct(items, size));
}

// ecma 23.1.2.1 Array.from(items, [mpfunc, [arg]])
// now only support one argument with array or string type
__jsvalue __jsarr_pt_from(__jsvalue *this_array, __jsvalue *args, uint32_t count) {
  if (count != 1) {
      MAPLE_JS_ASSERT(false && "NIY: __jsarr_pt_from, need one argument ");
  }
  uint32_t len;
  __jsvalue *argValue = &args[0];
  __jsobject *arr = NULL;
  // if item is undefined, throw exception
  if (__is_undefined(argValue)){
    MAPLE_JS_TYPEERROR_EXCEPTION();
  }

  if (__is_js_array(argValue)) {
    return *args;
  } else if (__is_js_object(argValue)) {
    // check the object is array-like object: property length
    __jsobject *argObj = __jsval_to_object(argValue);
    __jsvalue lenVal;
    if (__jsobj_helper_HasPropertyAndGet(argObj, JSBUILTIN_STRING_LENGTH, &lenVal)) {
      len = __js_ToUint32(&lenVal);
      arr = __js_new_arr_internal(len);
      __jsvalue *array_elems = arr->shared.array_props;
      // get value of property [0,...,len-1]
      for (uint32_t i = 0; i < len; i++) {
        __jsvalue itemVal = __jsobj_internal_Get(argObj, i);
        if (!__is_undefined(&itemVal)) {
          __set_regular_elem(array_elems, i, &itemVal);
        }
      }
      return __object_value(arr);
    } else {
      // return new created empty array
      return __object_value(__js_new_arr_internal(0));
    }
  } else {
    MAPLE_JS_ASSERT(false && "NIY: __jsarr_pt_from, unsupported args type");
  }
  return __object_value(arr);
}

// ecma 22.1.3.8 Array.prototype.find (predicate[,thisArg])
__jsvalue __jsarr_pt_find(__jsvalue *this_array, __jsvalue *arg_list, uint32_t argNum) {
  // ecma 22.1.3.8 step 1.
  __jsobject *o = __js_ToObject(this_array);
  __jsobj_helper_convert_to_generic(o);
  // ecma 22.1.3.8 step 2.
  uint32_t len = __jsobj_helper_get_length(o);
  // ecma 22.1.3.8 step 3.
  MAPLE_JS_ASSERT((argNum <= 2) && "__jsarr_pt_find");
  __jsvalue undefinedVal = __undefined_value();
  __jsvalue *callbackfn = argNum > 0 ? (&arg_list[0]) : (&undefinedVal);
  if (__is_none(callbackfn)) {
    MAPLE_JS_REFERENCEERROR_EXCEPTION();
  } else if (!__js_IsCallable(callbackfn)){
    MAPLE_JS_TYPEERROR_EXCEPTION();
  }
  // ecma 22.1.3.8 step 4.
  __jsvalue *t;
  if (argNum == 2) {
    t = &arg_list[1];
  } else {
    t = &undefinedVal;
  }
  // ecma 22.1.3.8 step 5-7.
  return __jsarr_helper_iter_method(this_array, callbackfn, t, len, JSARR_FIND);
}

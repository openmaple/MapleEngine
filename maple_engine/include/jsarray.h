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

#ifndef JSARRAY_H
#define JSARRAY_H
#include "jsvalue.h"
#include "jstycnv.h"
#include "jsobject.h"

#define SPACE_UNIT 5
// maximum index number use index mode
// otherwise use string to get/set property
#define ARRAY_MAXINDEXNUM_INTERNAL 0x10000

enum __jsarr_iter_type {
  JSARR_EVERY = 0,
  JSARR_SOME,
  JSARR_FOREACH,
  JSARR_MAP,
  JSARR_FILTER,
  JSARR_FIND,
};

// Helper function for internal use.
__jsobject *__js_new_arr_internal(uint32_t len);
void __jsarr_internal_DefineOwnProperty(__jsobject *a, __jsvalue *p, __jsprop_desc desc, bool throw_p);
void __jsarr_internal_DefineOwnPropertyByValue(__jsobject *a, uint32_t, __jsprop_desc desc, bool throw_p);

void __jsarr_internal_DefineLengthProperty(__jsobject *a, __jsprop_desc desc, bool throw_p);

void __jsarr_internal_DefineElemProperty(__jsobject *a, uint32_t index, __jsprop_desc desc, bool throw_p);
// __jsarr_getIndex(p) returns MAX_ARRAY_INDEX if p is not an index
uint32_t __jsarr_getIndex(__jsvalue *p);

void __jsarr_internal_MoveElem(__jsobject *arr, uint32_t to_idx, uint32_t from_idx);

void __jsarr_internal_MoveElem(__jsobject *o, __jsvalue *arr, uint32_t to_idx, uint32_t from_idx);

void __set_generic_elem(__jsobject *arr, uint32_t index, __jsvalue *v);
void __set_regular_elem(__jsvalue *arr, uint32_t index, __jsvalue *v);

__jsstring *__jsarr_ElemToString(__jsvalue *elem);

__jsstring *__jsarr_JoinOnce(__jsvalue *elem, __jsstring *r, __jsstring *sep);

// idx is guaranteed outside to be less than length
// equivalent to __jsobj_internal_Get, not __jsobj_internal_GetOwnProperty
__jsvalue __jsarr_GetRegularElem(__jsobject *o, __jsvalue *arr, uint32_t idx);

__jsvalue __jsarr_GetElem(__jsobject *o, uint32_t idx);

__jsvalue *__jsarr_RegularRealloc(__jsvalue *arr, uint32_t old_len, uint32_t new_len);

// ecma 15.4.2.1
__jsobject *__js_new_arr_elems(__jsvalue *items, uint32_t length);
// ecma 15.4.2.2
__jsobject *__js_new_arr_length(__jsvalue *l);
// ecma 15.4.3.2
__jsvalue __jsarr_isArray(__jsvalue *this_array, __jsvalue *arg);
// ecma 15.4.4.2
__jsvalue __jsarr_pt_toString(__jsvalue *this_array);
// ecma 15.4.4.3
__jsvalue __jsarr_pt_toLocaleString(__jsvalue *this_array);
// ecma 15.4.4.4
__jsvalue __jsarr_pt_concat(__jsvalue *this_array, __jsvalue *items, uint32_t size);
// ecma 15.4.4.5
__jsvalue __jsarr_pt_join(__jsvalue *this_array, __jsvalue *separator);
// ecma 15.4.4.6
__jsvalue __jsarr_pt_pop(__jsvalue *this_array);
// ecma 15.4.4.7
__jsvalue __jsarr_pt_push(__jsvalue *this_array, __jsvalue *items, uint32_t size);
// ecma 15.4.4.8
__jsvalue __jsarr_pt_reverse(__jsvalue *this_array);
// ecma 15.4.4.9
__jsvalue __jsarr_pt_shift(__jsvalue *this_array);
// ecma 15.4.4.10
__jsvalue __jsarr_pt_slice(__jsvalue *this_array, __jsvalue *start, __jsvalue *end);
// ecma 15.4.4.11
__jsvalue __jsarr_pt_sort(__jsvalue *this_array, __jsvalue *comparefn);
// ecma 15.4.4.12
__jsvalue __jsarr_pt_splice(__jsvalue *this_array, __jsvalue *items, uint32_t size);
// ecma 15.4.4.13
__jsvalue __jsarr_pt_unshift(__jsvalue *this_array, __jsvalue *items, uint32_t size);
// ecma 15.4.4.14
__jsvalue __jsarr_pt_indexOf(__jsvalue *this_array, __jsvalue *arg_list, uint32_t argNum);
// ecma 15.4.4.15
__jsvalue __jsarr_pt_lastIndexOf(__jsvalue *this_array, __jsvalue *arg_list, uint32_t argNum);
// ecma 15.4.4.16
__jsvalue __jsarr_pt_every(__jsvalue *this_array, __jsvalue *arg_list, uint32_t argNum);
// ecma 15.4.4.17
__jsvalue __jsarr_pt_some(__jsvalue *this_array, __jsvalue *arg_list, uint32_t argNum);
// ecma 15.4.4.18
__jsvalue __jsarr_pt_forEach(__jsvalue *this_array, __jsvalue *arg_list, uint32_t argNum);
// ecma 15.4.4.19
__jsvalue __jsarr_pt_map(__jsvalue *this_array, __jsvalue *arg_list, uint32_t argNum);
// ecma 15.4.4.20
__jsvalue __jsarr_pt_filter(__jsvalue *this_array, __jsvalue *arg_list, uint32_t argNum);
// ecma 15.4.4.21
__jsvalue __jsarr_pt_reduce(__jsvalue *this_array, __jsvalue *arg_list, uint32_t argNum);
// ecma 15.4.4.22
__jsvalue __jsarr_pt_reduceRight(__jsvalue *this_array, __jsvalue *arg_list, uint32_t argNum);
__jsvalue __jsarr_internal_reduce(__jsvalue *this_array, __jsvalue *arg_list, uint32_t argNum, bool right_flag);
// ecma 23.1.2.3 Array.of(..items)
__jsvalue __jsarr_pt_of(__jsvalue *arr, __jsvalue *items, uint32_t size);
// ecma 23.1.2.1 Array.from(items[,mapfn[,thisArg]])
__jsvalue __jsarr_pt_from(__jsvalue *arr, __jsvalue *args, uint32_t count);
// ecma 22.1.3.8 Array.prototype.find (predicate[,thisArg])
__jsvalue __jsarr_pt_find(__jsvalue *this_array, __jsvalue *arg_list, uint32_t argNum);
#endif

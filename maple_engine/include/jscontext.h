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

#ifndef JSCONTEXT_H
#define JSCONTEXT_H
#include "jsvalue.h"
#define UNCERTAIN_NARGS 0x7FFFFFFF
extern __jsvalue __js_Global_ThisBinding;
extern __jsvalue __js_ThisBinding;
extern bool __is_global_strict;

void __js_init_context(bool);
__jsvalue __js_entry_function(__jsvalue *this_arg, bool strict_p);
void __js_exit_function(__jsvalue *this_arg, __jsvalue old_this, bool strict_p);
__jsstring *__jsstr_get_builtin(__jsbuiltin_string_id id);
__jsobject *__jsobj_get_or_create_builtin(__jsbuiltin_object_id id);
#ifdef MEMORY_LEAK_CHECK
void __jsobj_release_builtin();
#endif
__jsvalue __jsop_call(__jsvalue *function, __jsvalue *this_arg, __jsvalue *arg_list, uint32_t arg_count);
void __jsop_print_item(__jsvalue value);
__jsvalue __jsop_add(__jsvalue *x, __jsvalue *y);
__jsvalue __jsop_object_mul(__jsvalue *x, __jsvalue *y);
__jsvalue __jsop_object_sub(__jsvalue *x, __jsvalue *y);
__jsvalue __jsop_object_div(__jsvalue *x, __jsvalue *y);
__jsvalue __jsop_object_rem(__jsvalue *x, __jsvalue *y);
bool __js_AbstractEquality(__jsvalue *x, __jsvalue *y, bool &);
bool __js_AbstractRelationalComparison(__jsvalue *x, __jsvalue *y, bool &, bool leftFirst = true);
bool __jsop_instanceof(__jsvalue *x, __jsvalue *y);
bool __jsop_in(__jsvalue *x, __jsvalue *y);
bool __jsop_stricteq(__jsvalue *x, __jsvalue *y);
bool __jsop_strictne(__jsvalue *x, __jsvalue *y);
// unary operations
__jsvalue __jsop_typeof(__jsvalue *v);
__jsvalue __jsop_new(__jsvalue *constructor, __jsvalue *this_arg, __jsvalue *arg_list, uint32_t nargs);

bool __js_ToBoolean(__jsvalue *v);
// Object.
__jsobject *__js_new_obj_obj_0();
__jsobject *__js_new_obj_obj_1(__jsvalue *v);
void __jsop_setprop(__jsvalue *o, __jsvalue *p, __jsvalue *v);
__jsvalue __jsop_getprop(__jsvalue *o, __jsvalue *p);
void __jsop_setprop_by_name(__jsvalue *o, __jsstring *p, __jsvalue *v, bool isStrict);
void __jsop_set_this_prop_by_name(__jsvalue *o, __jsstring *p, __jsvalue *v, bool noThrowTE = false);
void __jsop_init_this_prop_by_name(__jsvalue *o, __jsstring *name);
__jsvalue __jsop_getprop_by_name(__jsvalue *o, __jsstring *p);
__jsvalue __jsop_get_this_prop_by_name(__jsvalue *o, __jsstring *p);
__jsvalue __jsop_delprop(__jsvalue *o, __jsvalue *p);
void __jsop_initprop_by_name(__jsvalue *o, __jsstring *p, __jsvalue *v);
void __jsop_initprop_getter(__jsvalue *o, __jsvalue *p, __jsvalue *v);
void __jsop_initprop_setter(__jsvalue *o, __jsvalue *p, __jsvalue *v);
// Array
__jsobject *__js_new_arr_elems(__jsvalue *items, uint32_t len);
__jsobject *__js_new_arr_elems_direct(__jsvalue *items, uint32_t length);
__jsobject *__js_new_arr_length(__jsvalue *len);
uint32_t __jsop_length(__jsvalue *data);
// String
__jsvalue __js_new_string(uint16_t *data);
#endif

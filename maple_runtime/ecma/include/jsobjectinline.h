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

/// Only small inline functions for jsobject.
#ifndef JSOBJECTINLINE_H
#define JSOBJECTINLINE_H
#include <cstdlib>
#include <cstring>
#include "jsvalue.h"
#include "jstycnv.h"
#include "jsobject.h"
#include "jsfunction.h"
#include "jsvalueinline.h"
#include "vmmemory.h"
#include "securec.h"

static inline bool __has_get(__jsprop_desc desc) {
  return (desc.s.fields & JSPROP_HAS_GET) != 0;
}

static inline bool __has_set(__jsprop_desc desc) {
  return (desc.s.fields & JSPROP_HAS_SET) != 0;
}

static inline bool __has_get_or_set(__jsprop_desc desc) {
  return (desc.s.fields & (JSPROP_HAS_GET | JSPROP_HAS_SET)) != 0;
}

static inline bool __has_value(__jsprop_desc desc) {
  return (desc.s.fields & JSPROP_HAS_VALUE) != 0;
}

static inline bool __has_writable(__jsprop_desc desc) {
  return (desc.s.attr_writable & JSPROP_DESC_HAS_ATTR) != 0;
}

static inline bool __has_enumerable(__jsprop_desc desc) {
  return (desc.s.attr_enumerable & JSPROP_DESC_HAS_ATTR) != 0;
}

static inline bool __has_configurable(__jsprop_desc desc) {
  return (desc.s.attr_configurable & JSPROP_DESC_HAS_ATTR) != 0;
}

static inline bool __is_undefined_desc(__jsprop_desc desc) {
  MAPLE_JS_ASSERT((desc.s.fields & JSPROP_UNDEFINED) == 0 || desc.s.fields == JSPROP_UNDEFINED);
  return (desc.s.fields & JSPROP_UNDEFINED) != 0;
}

static inline bool __enumerable(__jsprop_desc desc) {
  MAPLE_JS_ASSERT(__has_enumerable(desc));
  return desc.s.attr_enumerable == JSPROP_DESC_ATTR_TRUE;
}

static inline bool __has_and_enumerable(__jsprop_desc desc) {
  return desc.s.attr_enumerable == JSPROP_DESC_ATTR_TRUE;
}

static inline bool __has_and_unenumerable(__jsprop_desc desc) {
  return desc.s.attr_enumerable == JSPROP_DESC_ATTR_FALSE;
}

static inline bool __writable(__jsprop_desc desc) {
  MAPLE_JS_ASSERT(__has_writable(desc));
  return desc.s.attr_writable == JSPROP_DESC_ATTR_TRUE;
}

static inline bool __has_and_writable(__jsprop_desc desc) {
  return desc.s.attr_writable == JSPROP_DESC_ATTR_TRUE;
}

static inline bool __has_and_unwritable(__jsprop_desc desc) {
  return desc.s.attr_writable == JSPROP_DESC_ATTR_FALSE;
}

static inline bool __configurable(__jsprop_desc desc) {
  MAPLE_JS_ASSERT(__has_configurable(desc));
  return desc.s.attr_configurable == JSPROP_DESC_ATTR_TRUE;
}

static inline bool __has_and_configurable(__jsprop_desc desc) {
  return desc.s.attr_configurable == JSPROP_DESC_ATTR_TRUE;
}

static inline bool __has_and_unconfigurable(__jsprop_desc desc) {
  return desc.s.attr_configurable == JSPROP_DESC_ATTR_FALSE;
}

static inline __jsobject *__get_get(__jsprop_desc desc) {
  return __has_get(desc) ? desc.named_accessor_property.get : NULL;
}

static inline __jsobject *__get_set(__jsprop_desc desc) {
  return __has_set(desc) ? desc.named_accessor_property.set : NULL;
}

static inline __jsvalue __get_value(__jsprop_desc desc) {
  //  MAPLE_JS_ASSERT(__has_value(desc));
  return desc.named_data_property.value;
}

static inline void __set_writable(__jsprop_desc *desc, bool writable) {
  desc->s.attr_writable = writable | JSPROP_DESC_HAS_ATTR;
}

static inline void __set_enumerable(__jsprop_desc *desc, bool enumerable) {
  desc->s.attr_enumerable = enumerable | JSPROP_DESC_HAS_ATTR;
}

static inline void __set_configurable(__jsprop_desc *desc, bool configurable) {
  desc->s.attr_configurable = configurable | JSPROP_DESC_HAS_ATTR;
}

static inline void __set_get(__jsprop_desc *desc, __jsobject *o) {
  desc->s.fields |= JSPROP_HAS_GET;
  desc->named_accessor_property.get = o;
}

static inline void __set_set(__jsprop_desc *desc, __jsobject *o) {
  desc->s.fields |= JSPROP_HAS_SET;
  desc->named_accessor_property.set = o;
}

static inline void __set_value(__jsprop_desc *desc, __jsvalue *v) {
  desc->s.fields |= JSPROP_HAS_VALUE;
  desc->named_data_property.value = *v;
}

static inline void __set_get_gc(__jsprop_desc *desc, __jsobject *o) {
#ifdef RC_NO_MMAP
  GCIncRf(o);
  GCDecRf(__get_get(*desc));
#endif
  __set_get(desc, o);
}

static inline void __set_set_gc(__jsprop_desc *desc, __jsobject *o) {
#ifdef RC_NO_MMAP
  GCIncRf(o);
  GCDecRf(__get_set(*desc));
#endif
  __set_set(desc, o);
}

static inline void __set_value_gc(__jsprop_desc *desc, __jsvalue *v) {
#ifndef RC_NO_MMAP
  UpdateGCReference(&desc->named_data_property.value.x.payload.ptr, JsvalToMval(*v));
#else
  GCCheckAndUpdateRf(desc->named_data_property.value.x.asbits, desc->named_data_property.value.ptyp,  v->x.asbits, v->ptyp);
#endif
  __set_value(desc, v);
}

static inline __jsprop_desc __new_init_desc() {
  __jsprop_desc d;
  d.named_data_property.value = __undefined_value();
  d.attrs = 0;
  __set_writable(&d, false);
  __set_enumerable(&d, false);
  __set_configurable(&d, false);
  return d;
}

static inline __jsprop_desc __new_empty_desc() {
  __jsprop_desc d;
  d.named_data_property.value = __undefined_value();
  d.attrs = 0;
  return d;
}

static inline __jsprop_desc __new__desc() {
  __jsprop_desc d;
  d.named_data_property.value = __undefined_value();
  d.attrs = 0;
  return d;
}

static inline __jsprop_desc __new_value_desc(__jsvalue *v, uint32_t attrs) {
  __jsprop_desc d;
  d.named_data_property.value = *v;
  d.attrs = attrs;
  return d;
}

static inline __jsprop_desc __new_set_desc(__jsobject *setter, uint32_t attrs) {
  __jsprop_desc d;
  d.named_accessor_property.get = NULL;
  d.named_accessor_property.set = setter;
  d.attrs = attrs;
  return d;
}

static inline __jsprop_desc __new_get_desc(__jsobject *getter, uint32_t attrs) {
  __jsprop_desc d;
  d.named_accessor_property.set = NULL;
  d.named_accessor_property.get = getter;
  d.attrs = attrs;
  return d;
}

static inline __jsprop_desc __undefined_desc() {
  __jsprop_desc d;
  d.attrs = (uint32_t)JSPROP_UNDEFINED << 24;
  d.named_data_property.value = __undefined_value();
  return d;
}

static inline bool __is_property(__jsprop *p, __jsstring *name) {
  return __jsstr_equal(p->n.name, name, true/* check name2num value*/);
}

static inline __jsobject *__create_object() {
  __jsobject *obj = (__jsobject *)VMMallocGC(sizeof(__jsobject), MemHeadJSObj);
  errno_t set_ret = memset_s(obj, sizeof(__jsobject), 0, sizeof(__jsobject));
  if (set_ret != EOK) {
    abort();
  }
#ifndef MARK_CYCLE_ROOTS
  memory_manager->AddObjListNode(obj);
#endif
  return obj;
}

static inline bool __is_boolean_object(__jsvalue *v) {
  return __is_js_object(v) && __jsval_to_object(v)->object_class == JSBOOLEAN;
}

static inline bool __is_number_object(__jsvalue *v) {
  return __is_js_object(v) && __jsval_to_object(v)->object_class == JSNUMBER;
}
static inline bool __is_double_object(__jsvalue *v) {
  return __is_js_object(v) && __jsval_to_object(v)->object_class == JSDOUBLE;
}
static inline bool __is_string_object(__jsvalue *v) {
  return __is_js_object(v) && __jsval_to_object(v)->object_class == JSSTRING;
}

#endif

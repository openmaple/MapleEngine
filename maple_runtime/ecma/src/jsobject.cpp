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

#include "mfunction.h"
#include "mshimdyn.h"
#include "jsvalueinline.h"
#include "jsobject.h"
#include "jsobjectinline.h"
#include "jsarray.h"
#include "jstycnv.h"
#include "jsfunction.h"
#include "jsnum.h"
#include "jsboolean.h"
#include "cmpl.h"
#include "mir_config.h"
#include "jsmath.h"
#include "json.h"
#include "jsdate.h"
#include "jsregexp.h"
#include "jsglobal.h"
#include "jsintl.h"

#if __clang_major__ >= 4
#pragma clang diagnostic ignored "-Waddress-of-packed-member"
#endif
// Helper function for object constructors.
void __jsobj_set_prototype(__jsobject *obj, __jsobject *proto_obj) {
  obj->proto_is_builtin = false;
  obj->prototype.obj = proto_obj;
  GCIncRf((void *)proto_obj);
}

static __jsprop *__jsobj_helper_get_property(__jsobject *obj, __jsstring *name, bool createBuiltin = true) {
#ifdef USE_PROP_MAP
  if (obj->prop_string_map != NULL) {
    std::map<__jsstring *, __jsprop *>::iterator it;
      it = obj->prop_string_map->find(name);
    __jsprop *p = NULL;
    if (it != obj->prop_string_map->end()) {
      p = it->second;
    } else { // name could be copied to a new string, find by name
      std::wstring w_name = __jsstr_to_wstring(name);
      for (it = obj->prop_string_map->begin(); it != obj->prop_string_map->end(); ++it) {
	if (__jsstr_to_wstring(it->first) == w_name) {
          p = it->second; // found by name
          break;
        }
      }
    }
    if (p) {
        if (__is_undefined_desc(p->desc)) {
          return NULL;
        }
        return p;
    }
  }
  if (obj->is_builtin && createBuiltin) {
    __jsprop *p = __create_builtin_property(obj, name);
    return p;
  }
#else
  __jsprop *p = obj->prop_list;
  while (p && p->isIndex)
    p = p->next;
  while (p) {
    assert(!p->isIndex && "shouldn't be index here");
    if (__is_property(p, name)) {
      if (__is_undefined_desc(p->desc))
        return NULL;
      return p;
    }
    p = p->next;
  }
  if (obj->is_builtin && createBuiltin) {
    return __create_builtin_property(obj, name);
  }
#endif
  return NULL;

}

static __jsprop *__jsobj_helper_get_propertyByValue(__jsobject *obj, uint32_t index, bool createBuiltin = true) {
#ifdef USE_PROP_MAP
  if (obj->prop_index_map == NULL && obj->object_class == JSSTRING && obj->shared.prim_string) {
    // lazy initailize properties for string
    __jsobj_initprop_fromString(obj, obj->shared.prim_string);
  }
  if (obj->prop_index_map != NULL) {
    std::map<uint32_t, __jsprop *>::iterator it;
    it = obj->prop_index_map->find(index);
    if (it != obj->prop_index_map->end()) {
      __jsprop *p = it->second;
      if (__is_undefined_desc(p->desc)) {
        return NULL;
      }
      return p;
    }
    if (obj->is_builtin && createBuiltin) {
      __jsprop *p = __create_builtin_property(obj, __js_NumberToString(index));
      return p;
    }
  }
  return NULL;
#else
  __jsprop *p = obj->prop_list;
  if (!p || !p->isIndex || p->n.index > index) {
    // the minimun index is greater than what is looking for, return null
    return NULL;
  }
  while (p) {
    if (!p->isIndex)
      return NULL; // exit early to reduce unnecessary comparison
    if (p->n.index == index) {
      if (__is_undefined_desc(p->desc))
        return NULL;
      return p;
    }
    p = p->next;
  }
  if (createBuiltin)
    return obj->is_builtin ?  __create_builtin_property(obj,__js_NumberToString(index)) : NULL;
  else
    return NULL;
#endif
}

// insert prop into propList by increasing order
static void InsertIndexProp(__jsprop *prop, __jsprop **propList, __jsobject *obj = NULL) {
#ifdef USE_PROP_MAP
  if (obj) {
    if (!*propList) {
      // very first entry
      (*propList) = prop;
      if (prop->isIndex) {
        if (obj->prop_index_map == NULL)
          obj->prop_index_map = new std::map<uint32_t, __jsprop *>();
        else
          assert(obj->prop_index_map->empty() && "prop_index_map should be empty at this time");
        (*(obj->prop_index_map))[prop->n.index] = prop;
      } else {
        if (obj->prop_string_map == NULL) {
          obj->prop_string_map = new std::map<__jsstring *, __jsprop *>();
        }
        else
          assert(obj->prop_string_map->empty() && "prop_string_map should be empty at this time");
        (*(obj->prop_string_map))[prop->n.name] = prop;
      }
      // The first entry's prev points to the list entry. The last entry's next is NULL
      prop->prev = prop;
      prop->next = nullptr;
      return;
    }

    if (prop->isIndex) {
      if (obj->prop_index_map == NULL || obj->prop_index_map->empty()) {
        if (obj->prop_index_map == NULL)
          obj->prop_index_map = new std::map<uint32_t, __jsprop *>();
        // make it first entry
        (*(obj->prop_index_map))[prop->n.index] = prop;
        prop->next = *propList;
        prop->prev = (*propList)->prev;
        (*propList)->prev = prop;
        *propList = prop;
        return;
      }
      std::map<uint32_t, __jsprop *>::iterator left, right;
      right = (*(obj->prop_index_map)).lower_bound(prop->n.index); // larger neighbor
      if (right == (*(obj->prop_index_map)).end()) { // no larger one in the list
        left = std::prev(right);
        prop->next = left->second->next;
        left->second->next = prop;
        prop->prev = left->second;
        if (prop->next)
          prop->next->prev = prop;
        else
          (*propList)->prev = prop; // last one
      } else {
        prop->next = right->second;
        prop->prev = right->second->prev;
        right->second->prev = prop;
        if (right == (*(obj->prop_index_map)).begin()) { //make as the first
          assert(*propList == right->second && "prop_list should be the first entry");
          *propList = prop;
        } else {
          prop->prev->next = prop;
        }
      }
      (*(obj->prop_index_map))[prop->n.index] = prop;
    } else { // !isIndex
      if (obj->prop_string_map == NULL || (*obj->prop_string_map).empty()) {
        if (obj->prop_string_map == NULL) {
          obj->prop_string_map = new std::map<__jsstring *, __jsprop *>();
        }
        (*(obj->prop_string_map))[prop->n.name] = prop;
        prop->prev = (*propList)->prev;
        (*propList)->prev->next = prop;
        (*propList)->prev = prop;
        return;
      }

      std::map<__jsstring *, __jsprop *>::iterator it;
      it = obj->prop_string_map->find(prop->n.name);
      if (it != obj->prop_string_map->end()) {
        __jsprop *old_prop = it->second;
        prop->next = old_prop->next;
        prop->prev = old_prop->prev;
        if (old_prop->next) // old_prop is not the last one
          old_prop->next->prev = prop;
        if (*propList == old_prop)
          *propList = prop;
        else {
          assert(old_prop->prev->next != nullptr && "prev should not be the last one");
          old_prop->prev->next = prop;
        }
        memory_manager->ManageProp(old_prop, RECALL);
        it->second = prop;
        return;
      }

      //append to the last
      __jsprop *prev_prop = (*propList)->prev;
      prev_prop->next = prop;
      prop->prev = prev_prop;
      (*propList)->prev = prop;
      prop->next = nullptr;

      (*(obj->prop_string_map))[prop->n.name] = prop;
    }
  }
  return;
#else
  __jsprop **prop_p = propList;
  if (!*prop_p) {
    (*prop_p) = prop;
    return;
  }
  if (!prop->isIndex) {
    // if (!(*prop_p)->isIndex) {
    //  prop->next = *prop_p;
    //  *prop_p = prop;
    //  return;
    // }
    // insert it to the next of last index prop;
    //while ((*prop_p)->isIndex && (*prop_p)->next && (*prop_p)->next->isIndex)
    //  prop_p = &((*prop_p)->next);
    // prop->next = (*prop_p)->next;
    // (*prop_p)->next = prop;
    // insert it to the last of the list.
    // TODO: give class __object a field to record the last __jsprop of prop_list to make it faster
     __jsprop *pre_prop = NULL;
    while (*prop_p) {
      __jsprop *current = *prop_p;
      // property may be marked as deleted
      if (!current->isIndex && __jsstr_equal(current->n.name, prop->n.name, false)) {
        // replace current with prop
        if (pre_prop) {
          pre_prop->next = prop;
          prop->next = current->next;
        } else {
          prop->next = (*prop_p)->next;
          (*prop_p) = prop;
        }
        return;
      }
      if (!current->next) {
        current->next = prop;
        return;
      }
      pre_prop = current;
      prop_p = &(current->next);
    }
  }
  if (!(*prop_p)->isIndex) {
    prop->next = (*prop_p);
    *prop_p = prop;
    return;
  }
  if ((*prop_p)->n.index > prop->n.index) { // *prop_p must be index;
    prop->next = (*prop_p);
    *prop_p = prop;
    return;
  }
  // *prop_p's value less than prop's value, then move to find the proper address

  while ((*prop_p)->n.index < prop->n.index && (*prop_p)->next && (*prop_p)->next->isIndex) {
    prop_p = &((*prop_p)->next);
  }

  prop->next = (*prop_p)->next;
  (*prop_p)->next = prop;
#endif
}

static __jsprop *__jsobj_helper_create_propertyByValue(__jsobject *obj, uint32_t index) {
  __jsprop *prop = (__jsprop *)VMMallocGC(sizeof(__jsprop), MemHeadJSProp, false);
  InitProp(prop, __new_empty_desc(), index);
  // assert(obj->object_class != JSARRAY && "shouldn't be a regular jsarray");
  InsertIndexProp(prop, &obj->prop_list, obj);
  return prop;
}

static __jsprop *__jsobj_helper_create_property(__jsobject *obj, __jsstring *name) {
  __jsprop *prop = (__jsprop *)VMMallocGC(sizeof(__jsprop), MemHeadJSProp, false);
  InitProp(prop, __new_empty_desc(), name);
  GCIncRf(prop->n.name);
  InsertIndexProp(prop, &obj->prop_list, obj);
  return prop;
}


typedef bool (*callback_fp)(__jsprop *, uint32_t, void *);
typedef bool (*condtion_fp)(__jsprop *);
// Iterator an object.
// Return the number of properties iterated.
uint32_t __jsobj_helper_for_each_property(__jsobject *obj, callback_fp callback, void *result,
                                          condtion_fp condition = NULL) {
  __jsprop *p = obj->prop_list;
  uint32_t n = 0;
  while (p) {
    if (!condition || condition(p)) {
      if (callback && !callback(p, n, result)) {
        return n++;
      }
      n++;
    }
    p = p->next;
  }
  return n;
}

void __jsobj_helper_DefineOwnPropertyByValue(__jsobject *obj, uint32_t index, __jsprop_desc d, bool throw_p) {
  // assert(obj->object_class !=JSARRAY && "nyi");
  if (obj->object_class == JSARRAY) {
    __jsarr_internal_DefineOwnPropertyByValue(obj, index, d, throw_p);
  } else {
    __jsobj_internal_DefineOwnPropertyByValue(obj, index, d, throw_p);
  }
}
void __jsobj_helper_DefineOwnProperty(__jsobject *obj, __jsstring *p, __jsprop_desc d, bool throw_p) {
  if (obj->object_class == JSARRAY) {
    __jsvalue p_v = __string_value(p);
    __jsarr_internal_DefineOwnProperty(obj, &p_v, d, throw_p);
  } else {
    __jsobj_internal_DefineOwnProperty(obj, p, d, throw_p);
  }
}

void __jsobj_helper_DefineOwnProperty(__jsobject *obj, __jsvalue *p, __jsprop_desc d, bool throw_p) {
  if (obj->object_class == JSARRAY) {
    __jsarr_internal_DefineOwnProperty(obj, p, d, throw_p);
  } else {
    __jsobj_internal_DefineOwnProperty(obj, p, d, throw_p);
  }
}

void __jsobj_helper_add_value_propertyByValue(__jsobject *obj, uint32_t index, __jsvalue *v, uint32_t attrs) {
  __jsprop_desc desc = __new_value_desc(v, attrs);
  __jsobj_helper_DefineOwnPropertyByValue(obj, index, desc, false);
}

// Helper function for add a value property.
void __jsobj_helper_add_value_property(__jsobject *obj, __jsvalue *name, __jsvalue *v, uint32_t attrs) {
  __jsprop_desc d = __new_value_desc(v, attrs);
  __jsobj_helper_DefineOwnProperty(obj, name, d, false);
}

void __jsobj_helper_add_value_property(__jsobject *obj, __jsstring *p, __jsvalue *v, uint32_t attrs) {
  __jsprop_desc desc = __new_value_desc(v, attrs);
  __jsobj_helper_DefineOwnProperty(obj, p, desc, false);
}

void __jsobj_helper_add_value_property(__jsobject *obj, __jsbuiltin_string_id id, __jsvalue *v, uint32_t attrs) {
  __jsobj_helper_add_value_property(obj, __jsstr_get_builtin(id), v, attrs);
}

__jsprop *__jsobj_helper_init_value_propertyByValue(__jsobject *obj, uint32_t index, __jsvalue *v, uint32_t attrs) {
  __jsprop *prop = __jsobj_helper_create_propertyByValue(obj, index);
  prop->desc = __new_value_desc(v, attrs);
  GCCheckAndIncRf(v->s.asbits, IsNeedRc((v->tag)));
  return prop;
}

__jsprop *__jsobj_helper_init_value_property(__jsobject *obj, __jsstring *p, __jsvalue *v, uint32_t attrs) {
  __jsprop *prop = __jsobj_helper_create_property(obj, p);
  prop->desc = __new_value_desc(v, attrs);
  GCCheckAndIncRf(v->s.asbits, IsNeedRc(v->tag));
  return prop;
}

__jsprop *__jsobj_helper_init_value_property(__jsobject *obj, __jsbuiltin_string_id id, __jsvalue *v, uint32_t attrs) {
  return __jsobj_helper_init_value_property(obj, __jsstr_get_builtin(id), v, attrs);
}

static __jsobject *bi_obj;
static __jsstring *bi_name;
__jsprop *__add_builtin_function_property(__jsbuiltin_string_id id, void *method, uint32_t attrs, bool create_all) {
  __jsobject *obj = bi_obj;
  __jsstring *name = bi_name;
  if (create_all) {
    __jsvalue v = __js_new_function((void *)method, NULL, attrs);
    __jsobj_helper_add_value_property(obj, id, &v, JSPROP_DESC_HAS_VWUEC);
    return NULL;
  } else if (__jsstr_equal_to_builtin(name, id)) {
    // ecma 20.2.3 : Function Prototype Object does not have a "prototype" property.
    bool skipprototype = (obj->is_builtin && (obj->builtin_id == JSBUILTIN_FUNCTIONPROTOTYPE ||
                                              obj->builtin_id == JSBUILTIN_STRINGPROTOTYPE ||
                                              obj->builtin_id == JSBUILTIN_ARRAYPROTOTYPE ||
                                              obj->builtin_id == JSBUILTIN_DATEPROTOTYPE ||
                                              obj->builtin_id == JSBUILTIN_REGEXPPROTOTYPE ||
                                              obj->builtin_id == JSBUILTIN_JSON ||
                                              obj->builtin_id == JSBUILTIN_OBJECTPROTOTYPE));
    __jsvalue v = __js_new_function((void *)method, NULL, attrs, -1, (!skipprototype));
    return __jsobj_helper_init_value_property(obj, id, &v, JSPROP_DESC_HAS_VWUEC);
  } else {
    return NULL;
  }
}

__jsprop *__add_builtin_value_property(__jsbuiltin_string_id id, __jsbuiltin_object_id builtin_obj_id,
                                       bool create_all) {
  __jsobject *obj = bi_obj;
  __jsstring *name = bi_name;
  __jsvalue v = __object_value(__jsobj_get_or_create_builtin(builtin_obj_id));
  uint32_t attrs;
  switch(builtin_obj_id) {
      case JSBUILTIN_DATECONSTRUCTOR:
          attrs = JSPROP_DESC_HAS_VWUEC;
          break;
      case JSBUILTIN_OBJECTCONSTRUCTOR:
          attrs = JSPROP_DESC_HAS_VUWUEC;
          break;
      case JSBUILTIN_REGEXPPROTOTYPE:
          attrs = id != JSBUILTIN_STRING_PROTOTYPE ? JSPROP_DESC_HAS_VUWUEC : JSPROP_DESC_HAS_VUWUEUC;
          break;
      default:
          attrs = JSPROP_DESC_HAS_VUWUEUC;
  }
  if (create_all) {
    __jsobj_helper_add_value_property(obj, id, &v, attrs);
    return NULL;
  } else if (__jsstr_equal_to_builtin(name, id)) {
    return __jsobj_helper_init_value_property(obj, id, &v, attrs);
  } else {
    return NULL;
  }
}

__jsprop *__add_builtin_value_property2(__jsbuiltin_string_id id, __jsbuiltin_object_id builtin_obj_id,  __jsvalue val,
                                       bool create_all) {
  __jsobject *obj = bi_obj;
  __jsstring *name = bi_name;
  if (create_all) {
    __jsobj_helper_add_value_property(obj, id, &val, JSPROP_DESC_HAS_VUWUEUC);
    return NULL;
  } else if (__jsstr_equal_to_builtin(name, id)) {
    return __jsobj_helper_init_value_property(obj, id, &val, JSPROP_DESC_HAS_VUWUEUC);
  } else {
    return NULL;
  }
}

__jsprop *add_builtin_accessor_property(__jsbuiltin_string_id id, __jsbuiltin_object_id builtin_obj_id,
                                       void* getter, uint32_t get_attr,
                                       void* setter, uint32_t set_attr,
                                       uint32_t attrs,
                                       bool create_all) {
  __jsobject *obj = bi_obj;
  __jsstring *name = bi_name;
  __jsobject *getObj = NULL;
  __jsobject *setObj = NULL;

  if (__jsstr_equal_to_builtin(name, id)) {
    // init property
    __jsprop *prop = __jsobj_helper_create_property(obj, __jsstr_get_builtin(id));
    prop->desc.attrs = attrs; // set user defined attrs
    MAPLE_JS_ASSERT(!__has_value(prop->desc) && !__has_writable(prop->desc)); // not have value

    __jsvalue f;
    if (getter) {
      f = __js_new_function((void *)getter, NULL, get_attr);
      __jsobject *fobj = __jsval_to_object(&f);
      __set_get(&(prop->desc), fobj);
    }
    if (setter) {
      f = __js_new_function((void *)setter, NULL, set_attr);
      __jsobject *fobj = __jsval_to_object(&f);
      __set_set(&(prop->desc), fobj);
    }
    return prop;
  }
  return NULL;
}

// check obj is global this
bool __is_globalthis(__jsobject *obj) {
  return (obj->builtin_id == JSBUILTIN_GLOBALOBJECT &&
          obj->object_class == JSGLOBAL &&
          obj->is_builtin);
}

__jsprop *__create_builtin_property(__jsobject *obj, __jsstring *name) {
#define ATTRS(nargs, length)                                                                      \
  ((uint32_t)(uint8_t)(nargs == UNCERTAIN_NARGS ? 1 : 0) << 24 | (uint32_t)(uint8_t)nargs << 16 | \
   (uint32_t)(uint8_t)length << 8 | JSFUNCPROP_NATIVE)

#define ADD_FUNCTION_PROPERTY(builtin_property_name_id, method, attrs)                                             \
  do {                                                                                                             \
    __jsprop *prop = __add_builtin_function_property(builtin_property_name_id, (void *)method, attrs, create_all); \
    if (prop)                                                                                                      \
      return prop;                                                                                                 \
  } while (0)

#define ADD_VALUE_PROPERTY(builtin_property_name_id, builtin_obj_id)                                     \
  do {                                                                                                   \
    __jsprop *prop = __add_builtin_value_property(builtin_property_name_id, builtin_obj_id, create_all); \
    if (prop)                                                                                            \
      return prop;                                                                                       \
  } while (0)

#define ADD_VALUE2_PROPERTY(builtin_property_name_id, builtin_obj_id, value)                                     \
  do {                                                                                                   \
    __jsprop *prop = __add_builtin_value_property2(builtin_property_name_id, builtin_obj_id, value, create_all); \
    if (prop)                                                                                            \
      return prop;                                                                                       \
  } while (0)

#define ADD_ACCESSOR_PROPERTY(builtin_property_name_id, builtin_obj_id, getter, get_attrs, setter, set_attrs, attrs) \
  do {                                                         \
    __jsprop *prop = add_builtin_accessor_property(builtin_property_name_id, builtin_obj_id, (void *)getter,   \
                                                   get_attrs, (void *)setter, set_attrs, attrs, create_all);          \
    if (prop) \
      return prop;                                               \
  } while (0)                 \

  bi_obj = obj;
  bi_name = name;
  bool create_all = false;
  if (name == NULL) {
    create_all = true;
  }
  switch (obj->builtin_id) {
    case JSBUILTIN_OBJECTCONSTRUCTOR:
      ADD_VALUE_PROPERTY(JSBUILTIN_STRING_PROTOTYPE, (JSBUILTIN_OBJECTPROTOTYPE));
      ADD_FUNCTION_PROPERTY(JSBUILTIN_STRING_GET_PROTOTYPE_OF_UL, __jsobj_getPrototypeOf, ATTRS(1, 1));
      ADD_FUNCTION_PROPERTY(JSBUILTIN_STRING_GET_OWN_PROPERTY_DESCRIPTOR_UL, __jsobj_getOwnPropertyDescriptor,
                            ATTRS(2, 2));
      ADD_FUNCTION_PROPERTY(JSBUILTIN_STRING_GET_OWN_PROPERTY_NAMES_UL, __jsobj_getOwnPropertyNames, ATTRS(1, 1));
      ADD_FUNCTION_PROPERTY(JSBUILTIN_STRING_CREATE, __jsobj_create, ATTRS(2, 2));
      ADD_FUNCTION_PROPERTY(JSBUILTIN_STRING_DEFINE_PROPERTY_UL, __jsobj_defineProperty, ATTRS(3, 3));
      ADD_FUNCTION_PROPERTY(JSBUILTIN_STRING_DEFINE_PROPERTIES_UL, __jsobj_defineProperties, ATTRS(2, 2));
      ADD_FUNCTION_PROPERTY(JSBUILTIN_STRING_SEAL, __jsobj_seal, ATTRS(1, 1));
      ADD_FUNCTION_PROPERTY(JSBUILTIN_STRING_FREEZE, __jsobj_freeze, ATTRS(1, 1));
      ADD_FUNCTION_PROPERTY(JSBUILTIN_STRING_PREVENT_EXTENSIONS_UL, __jsobj_preventExtensions, ATTRS(1, 1));
      ADD_FUNCTION_PROPERTY(JSBUILTIN_STRING_IS_SEALED_UL, __jsobj_isSealed, ATTRS(1, 1));
      ADD_FUNCTION_PROPERTY(JSBUILTIN_STRING_IS_FROZEN_UL, __jsobj_isFrozen, ATTRS(1, 1));
      ADD_FUNCTION_PROPERTY(JSBUILTIN_STRING_IS_EXTENSIBLE, __jsobj_isExtensible, ATTRS(1, 1));
      ADD_FUNCTION_PROPERTY(JSBUILTIN_STRING_KEYS, __jsobj_keys, ATTRS(1, 1));
      break;
    case JSBUILTIN_OBJECTPROTOTYPE:
      ADD_VALUE_PROPERTY(JSBUILTIN_STRING_CONSTRUCTOR, (JSBUILTIN_OBJECTCONSTRUCTOR));
      ADD_FUNCTION_PROPERTY(JSBUILTIN_STRING_TO_STRING_UL, __jsobj_pt_toString, ATTRS(0, 0));
      ADD_FUNCTION_PROPERTY(JSBUILTIN_STRING_TO_LOCALE_STRING_UL, __jsobj_pt_toLocaleString, ATTRS(0, 0));
      ADD_FUNCTION_PROPERTY(JSBUILTIN_STRING_VALUE_OF_UL, __jsobj_pt_valueOf, ATTRS(0, 0));
      ADD_FUNCTION_PROPERTY(JSBUILTIN_STRING_HAS_OWN_PROPERTY_UL, __jsobj_pt_hasOwnProperty, ATTRS(1, 1));
      ADD_FUNCTION_PROPERTY(JSBUILTIN_STRING_IS_PROTOTYPE_OF_UL, __jsobj_pt_isPrototypeOf, ATTRS(1, 1));
      ADD_FUNCTION_PROPERTY(JSBUILTIN_STRING_PROPERTY_IS_ENUMERABLE_UL, __jsobj_pt_propertyIsEnumerable, ATTRS(1, 1));
      break;
    case JSBUILTIN_FUNCTIONCONSTRUCTOR:
      ADD_VALUE_PROPERTY(JSBUILTIN_STRING_PROTOTYPE, (JSBUILTIN_FUNCTIONPROTOTYPE));
      break;
    case JSBUILTIN_FUNCTIONPROTOTYPE:
      ADD_VALUE_PROPERTY(JSBUILTIN_STRING_CONSTRUCTOR, (JSBUILTIN_FUNCTIONCONSTRUCTOR));
      ADD_FUNCTION_PROPERTY(JSBUILTIN_STRING_CALL, __jsfun_pt_call, ATTRS(UNCERTAIN_NARGS, 1));
      ADD_FUNCTION_PROPERTY(JSBUILTIN_STRING_APPLY, __jsfun_pt_apply, ATTRS(2, 2));
      ADD_FUNCTION_PROPERTY(JSBUILTIN_STRING_BIND, __jsfun_pt_bind, ATTRS(UNCERTAIN_NARGS, 1));
      ADD_FUNCTION_PROPERTY(JSBUILTIN_STRING_TO_STRING_UL, __jsfun_pt_tostring, ATTRS(0, 0));
      break;
    case JSBUILTIN_ARRAYCONSTRUCTOR:
      ADD_VALUE_PROPERTY(JSBUILTIN_STRING_PROTOTYPE, (JSBUILTIN_ARRAYPROTOTYPE));
      ADD_FUNCTION_PROPERTY(JSBUILTIN_STRING_IS_ARRAY_UL, __jsarr_isArray, ATTRS(1, 1));
      ADD_FUNCTION_PROPERTY(JSBUILTIN_STRING_OF_UL, __jsarr_pt_of, ATTRS(UNCERTAIN_NARGS, -1));
      ADD_FUNCTION_PROPERTY(JSBUILTIN_STRING_FROM_UL, __jsarr_pt_from, ATTRS(UNCERTAIN_NARGS, -1));
      break;
    case JSBUILTIN_ARRAYPROTOTYPE:
      if (create_all || __jsstr_equal_to_builtin(name, JSBUILTIN_STRING_LENGTH)) {
        __jsvalue v = __number_value(0);
        return __jsobj_helper_init_value_property(obj, JSBUILTIN_STRING_LENGTH, &v, JSPROP_DESC_HAS_VWUEUC);
      }
      ADD_VALUE_PROPERTY(JSBUILTIN_STRING_CONSTRUCTOR, (JSBUILTIN_ARRAYCONSTRUCTOR));
      ADD_FUNCTION_PROPERTY(JSBUILTIN_STRING_TO_STRING_UL, __jsarr_pt_toString, ATTRS(0, 0));
      ADD_FUNCTION_PROPERTY(JSBUILTIN_STRING_TO_LOCALE_STRING_UL, __jsarr_pt_toLocaleString, ATTRS(0, 0));
      ADD_FUNCTION_PROPERTY(JSBUILTIN_STRING_CONCAT, __jsarr_pt_concat, ATTRS(UNCERTAIN_NARGS, 1));
      ADD_FUNCTION_PROPERTY(JSBUILTIN_STRING_JOIN, __jsarr_pt_join, ATTRS(1, 1));
      ADD_FUNCTION_PROPERTY(JSBUILTIN_STRING_POP, __jsarr_pt_pop, ATTRS(0, 0));
      ADD_FUNCTION_PROPERTY(JSBUILTIN_STRING_PUSH, __jsarr_pt_push, ATTRS(UNCERTAIN_NARGS, 1));
      ADD_FUNCTION_PROPERTY(JSBUILTIN_STRING_REVERSE, __jsarr_pt_reverse, ATTRS(0, 0));
      ADD_FUNCTION_PROPERTY(JSBUILTIN_STRING_SHIFT, __jsarr_pt_shift, ATTRS(0, 0));
      ADD_FUNCTION_PROPERTY(JSBUILTIN_STRING_SLICE, __jsarr_pt_slice, ATTRS(2, 2));
      ADD_FUNCTION_PROPERTY(JSBUILTIN_STRING_SORT, __jsarr_pt_sort, ATTRS(1, 1));
      ADD_FUNCTION_PROPERTY(JSBUILTIN_STRING_SPLICE, __jsarr_pt_splice, ATTRS(UNCERTAIN_NARGS, 2));
      ADD_FUNCTION_PROPERTY(JSBUILTIN_STRING_UNSHIFT, __jsarr_pt_unshift, ATTRS(UNCERTAIN_NARGS, 1));
      ADD_FUNCTION_PROPERTY(JSBUILTIN_STRING_INDEX_OF_UL, __jsarr_pt_indexOf, ATTRS(UNCERTAIN_NARGS, 1));
      ADD_FUNCTION_PROPERTY(JSBUILTIN_STRING_LAST_INDEX_OF_UL, __jsarr_pt_lastIndexOf, ATTRS(UNCERTAIN_NARGS, 1));
      ADD_FUNCTION_PROPERTY(JSBUILTIN_STRING_EVERY, __jsarr_pt_every, ATTRS(UNCERTAIN_NARGS, 1));
      ADD_FUNCTION_PROPERTY(JSBUILTIN_STRING_SOME, __jsarr_pt_some, ATTRS(UNCERTAIN_NARGS, 1));
      ADD_FUNCTION_PROPERTY(JSBUILTIN_STRING_FOR_EACH_UL, __jsarr_pt_forEach, ATTRS(UNCERTAIN_NARGS, 1));
      ADD_FUNCTION_PROPERTY(JSBUILTIN_STRING_MAP, __jsarr_pt_map, ATTRS(UNCERTAIN_NARGS, 1));
      ADD_FUNCTION_PROPERTY(JSBUILTIN_STRING_FILTER, __jsarr_pt_filter, ATTRS(UNCERTAIN_NARGS, 1));
      ADD_FUNCTION_PROPERTY(JSBUILTIN_STRING_REDUCE, __jsarr_pt_reduce, ATTRS(UNCERTAIN_NARGS, 1));
      ADD_FUNCTION_PROPERTY(JSBUILTIN_STRING_REDUCE_RIGHT_UL, __jsarr_pt_reduceRight, ATTRS(UNCERTAIN_NARGS, 1));
      ADD_FUNCTION_PROPERTY(JSBUILTIN_STRING_FIND_UL, __jsarr_pt_find, ATTRS(UNCERTAIN_NARGS, 1));
      break;
    case JSBUILTIN_STRINGCONSTRUCTOR:
      ADD_VALUE_PROPERTY(JSBUILTIN_STRING_PROTOTYPE, (JSBUILTIN_STRINGPROTOTYPE));
      ADD_FUNCTION_PROPERTY(JSBUILTIN_STRING_FROM_CHAR_CODE_UL, __jsstr_fromCharCode, ATTRS(UNCERTAIN_NARGS, 1));
      break;
    case JSBUILTIN_STRINGPROTOTYPE:
      ADD_VALUE_PROPERTY(JSBUILTIN_STRING_CONSTRUCTOR, (JSBUILTIN_STRINGCONSTRUCTOR));
      ADD_FUNCTION_PROPERTY(JSBUILTIN_STRING_TO_STRING_UL, __jsstr_toString, ATTRS(0, 0));
      ADD_FUNCTION_PROPERTY(JSBUILTIN_STRING_VALUE_OF_UL, __jsstr_valueOf, ATTRS(0, 0));
      ADD_FUNCTION_PROPERTY(JSBUILTIN_STRING_CHAR_AT_UL, __jsstr_charAt, ATTRS(1, 1));
      ADD_FUNCTION_PROPERTY(JSBUILTIN_STRING_CHAR_CODE_AT_UL, __jsstr_charCodeAt, ATTRS(1, 1));
      ADD_FUNCTION_PROPERTY(JSBUILTIN_STRING_CONCAT, __jsstr_concat, ATTRS(UNCERTAIN_NARGS, 1));
      ADD_FUNCTION_PROPERTY(JSBUILTIN_STRING_INDEX_OF_UL, __jsstr_indexOf, ATTRS(2, 1));
      ADD_FUNCTION_PROPERTY(JSBUILTIN_STRING_LAST_INDEX_OF_UL, __jsstr_lastIndexOf, ATTRS(2, 1));
      ADD_FUNCTION_PROPERTY(JSBUILTIN_STRING_LOCALE_COMPARE_UL, __jsstr_localeCompare, ATTRS(2, 1));
      ADD_FUNCTION_PROPERTY(JSBUILTIN_STRING_SLICE, __jsstr_slice, ATTRS(2, 2));
      ADD_FUNCTION_PROPERTY(JSBUILTIN_STRING_SUBSTR, __jsstr_substr, ATTRS(2, 2));
      ADD_FUNCTION_PROPERTY(JSBUILTIN_STRING_SUBSTRING, __jsstr_substring, ATTRS(2, 2));
      ADD_FUNCTION_PROPERTY(JSBUILTIN_STRING_TO_LOWER_CASE_UL, __jsstr_toLowerCase, ATTRS(0, 0));
      ADD_FUNCTION_PROPERTY(JSBUILTIN_STRING_TO_LOCALE_LOWER_CASE_UL, __jsstr_toLocaleLowerCase, ATTRS(0, 0));
      ADD_FUNCTION_PROPERTY(JSBUILTIN_STRING_TO_UPPER_CASE_UL, __jsstr_toUpperCase, ATTRS(0, 0));
      ADD_FUNCTION_PROPERTY(JSBUILTIN_STRING_TO_LOCALE_UPPER_CASE_UL, __jsstr_toLocaleUpperCase, ATTRS(0, 0));
      ADD_FUNCTION_PROPERTY(JSBUILTIN_STRING_TRIM, __jsstr_trim, ATTRS(0, 0));
      ADD_FUNCTION_PROPERTY(JSBUILTIN_STRING_SPLIT, __jsstr_split, ATTRS(2, 2));
      ADD_FUNCTION_PROPERTY(JSBUILTIN_STRING_MATCH, __jsstr_match, ATTRS(1, 1));
      ADD_FUNCTION_PROPERTY(JSBUILTIN_STRING_SEARCH, __jsstr_search, ATTRS(1, 1));
      ADD_FUNCTION_PROPERTY(JSBUILTIN_STRING_REPLACE, __jsstr_replace, ATTRS(2, 2));
      break;
    case JSBUILTIN_BOOLEANCONSTRUCTOR:
      ADD_VALUE_PROPERTY(JSBUILTIN_STRING_PROTOTYPE, (JSBUILTIN_BOOLEANPROTOTYPE));
      break;
    case JSBUILTIN_BOOLEANPROTOTYPE:
      ADD_VALUE_PROPERTY(JSBUILTIN_STRING_CONSTRUCTOR, (JSBUILTIN_BOOLEANCONSTRUCTOR));
      ADD_FUNCTION_PROPERTY(JSBUILTIN_STRING_TO_STRING_UL, __jsboo_pt_toString, ATTRS(0, 0));
      ADD_FUNCTION_PROPERTY(JSBUILTIN_STRING_VALUE_OF_UL, __jsboo_pt_valueOf, ATTRS(0, 0));
      break;
    case JSBUILTIN_NUMBERCONSTRUCTOR:
      ADD_VALUE_PROPERTY(JSBUILTIN_STRING_PROTOTYPE, (JSBUILTIN_NUMBERPROTOTYPE));
      ADD_VALUE2_PROPERTY(JSBUILTIN_STRING_MAX_VALUE_U, (JSBUILTIN_MATH), __double_value(NumberMaxValue));
      ADD_VALUE2_PROPERTY(JSBUILTIN_STRING_MIN_VALUE_U, (JSBUILTIN_MATH), __double_value(NumberMinValue));
      ADD_VALUE2_PROPERTY(JSBUILTIN_STRING_POSITIVE_INFINITY_U, (JSBUILTIN_MATH), __number_value(0));
      ADD_VALUE2_PROPERTY(JSBUILTIN_STRING_NEGATIVE_INFINITY_U, (JSBUILTIN_MATH), __number_value(0));
      ADD_VALUE2_PROPERTY(JSBUILTIN_STRING_NAN, (JSBUILTIN_MATH), __number_value(0));
      ADD_FUNCTION_PROPERTY(JSBUILTIN_STRING_VALUE_OF_UL, __jsnum_pt_valueOf, ATTRS(0, 0));
      break;
    case JSBUILTIN_NUMBERPROTOTYPE:
      ADD_VALUE_PROPERTY(JSBUILTIN_STRING_CONSTRUCTOR, (JSBUILTIN_NUMBERCONSTRUCTOR));
      ADD_FUNCTION_PROPERTY(JSBUILTIN_STRING_TO_STRING_UL, __jsnum_pt_toString, ATTRS(1, 0));
      ADD_FUNCTION_PROPERTY(JSBUILTIN_STRING_TO_LOCALE_STRING_UL, __jsnum_pt_toLocaleString, ATTRS(0, 0));
      ADD_FUNCTION_PROPERTY(JSBUILTIN_STRING_VALUE_OF_UL, __jsnum_pt_valueOf, ATTRS(0, 0));
      ADD_FUNCTION_PROPERTY(JSBUILTIN_STRING_TO_FIXED_UL, __jsnum_pt_toFixed, ATTRS(1, 1));
      ADD_FUNCTION_PROPERTY(JSBUILTIN_STRING_TO_EXPONENTIAL_UL, __jsnum_pt_toExponential, ATTRS(1, 1));
      ADD_FUNCTION_PROPERTY(JSBUILTIN_STRING_TO_PRECISION_UL, __jsnum_pt_toPrecision, ATTRS(1, 1));
      break;
    case JSBUILTIN_MATH:
      ADD_FUNCTION_PROPERTY(JSBUILTIN_STRING_ABS, __jsmath_pt_abs, ATTRS(1, 1));
      ADD_FUNCTION_PROPERTY(JSBUILTIN_STRING_CEIL, __jsmath_pt_ceil, ATTRS(1, 1));
      ADD_FUNCTION_PROPERTY(JSBUILTIN_STRING_FLOOR, __jsmath_pt_floor, ATTRS(1, 1));
      ADD_FUNCTION_PROPERTY(JSBUILTIN_STRING_MAX, __jsmath_pt_max, ATTRS(UNCERTAIN_NARGS, 2));
      ADD_FUNCTION_PROPERTY(JSBUILTIN_STRING_MIN, __jsmath_pt_min, ATTRS(UNCERTAIN_NARGS, 2));
      ADD_FUNCTION_PROPERTY(JSBUILTIN_STRING_POW, __jsmath_pt_pow, ATTRS(2, 2));
      ADD_FUNCTION_PROPERTY(JSBUILTIN_STRING_ROUND, __jsmath_pt_round, ATTRS(1, 1));
      ADD_FUNCTION_PROPERTY(JSBUILTIN_STRING_SQRT, __jsmath_pt_sqrt, ATTRS(1, 1));
      ADD_FUNCTION_PROPERTY(JSBUILTIN_STRING_SIN, __jsmath_pt_sin, ATTRS(1, 1));
      ADD_FUNCTION_PROPERTY(JSBUILTIN_STRING_ASIN, __jsmath_pt_asin, ATTRS(1, 1));
      ADD_FUNCTION_PROPERTY(JSBUILTIN_STRING_COS, __jsmath_pt_cos, ATTRS(1, 1));
      ADD_FUNCTION_PROPERTY(JSBUILTIN_STRING_ACOS, __jsmath_pt_acos, ATTRS(1, 1));
      ADD_FUNCTION_PROPERTY(JSBUILTIN_STRING_TAN, __jsmath_pt_tan, ATTRS(1, 1));
      ADD_FUNCTION_PROPERTY(JSBUILTIN_STRING_ATAN, __jsmath_pt_atan, ATTRS(1, 1));
      ADD_FUNCTION_PROPERTY(JSBUILTIN_STRING_ATAN2, __jsmath_pt_atan2, ATTRS(2, 2));
      ADD_FUNCTION_PROPERTY(JSBUILTIN_STRING_LOG, __jsmath_pt_log, ATTRS(1, 1));
      ADD_FUNCTION_PROPERTY(JSBUILTIN_STRING_EXP, __jsmath_pt_exp, ATTRS(1, 1));
      ADD_FUNCTION_PROPERTY(JSBUILTIN_STRING_RANDOM, __jsmath_pt_random, ATTRS(0, 0));
      ADD_VALUE2_PROPERTY(JSBUILTIN_STRING_E_U, (JSBUILTIN_MATH), (__double_value(MathE)));
      ADD_VALUE2_PROPERTY(JSBUILTIN_STRING_LN10_U, (JSBUILTIN_MATH), (__double_value(MathLn10)));
      ADD_VALUE2_PROPERTY(JSBUILTIN_STRING_LN2_U, (JSBUILTIN_MATH), (__double_value(MathLn2)));
      ADD_VALUE2_PROPERTY(JSBUILTIN_STRING_LOG10E_U, (JSBUILTIN_MATH), (__double_value(MathLog10e)));
      ADD_VALUE2_PROPERTY(JSBUILTIN_STRING_LOG2E_U, (JSBUILTIN_MATH), (__double_value(MathLog2e)));
      ADD_VALUE2_PROPERTY(JSBUILTIN_STRING_PI_U, (JSBUILTIN_MATH), (__double_value(MathPi)));
      ADD_VALUE2_PROPERTY(JSBUILTIN_STRING_SQRT1_2_U, (JSBUILTIN_MATH), (__double_value(MathSqrt1_2)));
      ADD_VALUE2_PROPERTY(JSBUILTIN_STRING_SQRT2_U, (JSBUILTIN_MATH), (__double_value(MathSqrt2)));
      break;
    case JSBUILTIN_JSON:
      ADD_FUNCTION_PROPERTY(JSBUILTIN_STRING_TOSOURCE, __json_toSource, ATTRS(2, 2));
      ADD_FUNCTION_PROPERTY(JSBUILTIN_STRING_PARSE, __json_parse, ATTRS(2, 2));
      ADD_FUNCTION_PROPERTY(JSBUILTIN_STRING_STRINGIFY, __json_stringify, ATTRS(3, 3));
    case JSBUILTIN_REFERENCEERRORCONSTRUCTOR:
      ADD_VALUE_PROPERTY(JSBUILTIN_STRING_PROTOTYPE, (JSBUILTIN_REFERENCEERRORPROTOTYPE));
      break;
    case JSBUILTIN_REFERENCEERRORPROTOTYPE:
      ADD_VALUE_PROPERTY(JSBUILTIN_STRING_CONSTRUCTOR, (JSBUILTIN_REFERENCEERRORCONSTRUCTOR));
      ADD_FUNCTION_PROPERTY(JSBUILTIN_STRING_TO_STRING_UL, __js_referenceerror_pt_toString, ATTRS(0, 0));
      break;
    case JSBUILTIN_ERROR_CONSTRUCTOR:
      ADD_VALUE_PROPERTY(JSBUILTIN_STRING_PROTOTYPE, (JSBUILTIN_ERROR_PROTOTYPE));
      break;
    case JSBUILTIN_ERROR_PROTOTYPE:
      ADD_VALUE_PROPERTY(JSBUILTIN_STRING_CONSTRUCTOR, (JSBUILTIN_ERROR_CONSTRUCTOR));
      ADD_VALUE2_PROPERTY(JSBUILTIN_STRING_NAME, (JSBUILTIN_ERROR_CONSTRUCTOR), __string_value(__jsstr_get_builtin(JSBUILTIN_STRING_ERROR_UL)));
      ADD_VALUE2_PROPERTY(JSBUILTIN_STRING_MESSAGE, (JSBUILTIN_ERROR_CONSTRUCTOR), __string_value(__jsstr_get_builtin(JSBUILTIN_STRING_EMPTY)));
      ADD_FUNCTION_PROPERTY(JSBUILTIN_STRING_TO_STRING_UL, __jserror_pt_toString, ATTRS(0, 0));
      break;
    case JSBUILTIN_EVALERROR_CONSTRUCTOR:
      ADD_VALUE_PROPERTY(JSBUILTIN_STRING_PROTOTYPE, (JSBUILTIN_EVALERROR_PROTOTYPE));
      break;
    case JSBUILTIN_EVALERROR_PROTOTYPE:
      ADD_VALUE_PROPERTY(JSBUILTIN_STRING_CONSTRUCTOR, (JSBUILTIN_EVALERROR_CONSTRUCTOR));
      ADD_FUNCTION_PROPERTY(JSBUILTIN_STRING_TO_STRING_UL, __js_evalerror_pt_toString, ATTRS(0, 0));
      break;
    case JSBUILTIN_RANGEERROR_CONSTRUCTOR:
      ADD_VALUE_PROPERTY(JSBUILTIN_STRING_PROTOTYPE, (JSBUILTIN_RANGEERROR_PROTOTYPE));
      break;
    case JSBUILTIN_RANGEERROR_PROTOTYPE:
      ADD_VALUE_PROPERTY(JSBUILTIN_STRING_CONSTRUCTOR, (JSBUILTIN_RANGEERROR_CONSTRUCTOR));
      ADD_FUNCTION_PROPERTY(JSBUILTIN_STRING_TO_STRING_UL, __js_rangeerror_pt_toString, ATTRS(0, 0));
      break;
    case JSBUILTIN_SYNTAXERROR_CONSTRUCTOR:
      ADD_VALUE_PROPERTY(JSBUILTIN_STRING_PROTOTYPE, (JSBUILTIN_SYNTAXERROR_PROTOTYPE));
      break;
    case JSBUILTIN_SYNTAXERROR_PROTOTYPE:
      ADD_VALUE_PROPERTY(JSBUILTIN_STRING_CONSTRUCTOR, (JSBUILTIN_SYNTAXERROR_CONSTRUCTOR));
      ADD_VALUE2_PROPERTY(JSBUILTIN_STRING_NAME, (JSBUILTIN_SYNTAXERROR_CONSTRUCTOR), __string_value(__jsstr_get_builtin(JSBUILTIN_STRING_SYNTAX_ERROR_UL)));
      ADD_FUNCTION_PROPERTY(JSBUILTIN_STRING_TO_STRING_UL, __js_syntaxerror_pt_toString, ATTRS(0, 0));
      break;
    case JSBUILTIN_URIERROR_CONSTRUCTOR:
      ADD_VALUE_PROPERTY(JSBUILTIN_STRING_PROTOTYPE, (JSBUILTIN_URIERROR_PROTOTYPE));
      break;
    case JSBUILTIN_URIERROR_PROTOTYPE:
      ADD_VALUE_PROPERTY(JSBUILTIN_STRING_CONSTRUCTOR, (JSBUILTIN_URIERROR_CONSTRUCTOR));
      ADD_FUNCTION_PROPERTY(JSBUILTIN_STRING_TO_STRING_UL, __js_urierror_pt_toString, ATTRS(0, 0));
      break;
    case JSBUILTIN_TYPEERROR_CONSTRUCTOR:
      ADD_VALUE_PROPERTY(JSBUILTIN_STRING_PROTOTYPE, (JSBUILTIN_TYPEERROR_PROTOTYPE));
      break;
    case JSBUILTIN_TYPEERROR_PROTOTYPE:
      ADD_VALUE_PROPERTY(JSBUILTIN_STRING_CONSTRUCTOR, (JSBUILTIN_TYPEERROR_CONSTRUCTOR));
      ADD_VALUE2_PROPERTY(JSBUILTIN_STRING_NAME, (JSBUILTIN_TYPEERROR_CONSTRUCTOR), __string_value(__jsstr_get_builtin(JSBUILTIN_STRING_TYPE_ERROR_UL)));
      ADD_FUNCTION_PROPERTY(JSBUILTIN_STRING_TO_STRING_UL, __js_typeerror_pt_toString, ATTRS(0, 0));
      break;
    case JSBUILTIN_REGEXPCONSTRUCTOR:
      ADD_VALUE_PROPERTY(JSBUILTIN_STRING_PROTOTYPE, (JSBUILTIN_REGEXPPROTOTYPE));
      break;
    case JSBUILTIN_REGEXPPROTOTYPE:
      ADD_VALUE_PROPERTY(JSBUILTIN_STRING_CONSTRUCTOR, (JSBUILTIN_REGEXPCONSTRUCTOR));
      ADD_ACCESSOR_PROPERTY(JSBUILTIN_STRING_SOURCE, JSBUILTIN_REGEXPPROTOTYPE, __jsregexp_Source, ATTRS(0, 0), NULL, ATTRS(0,0), JSPROP_DESC_HAS_UVUWUEC);
      ADD_ACCESSOR_PROPERTY(JSBUILTIN_STRING_GLOBAL, JSBUILTIN_REGEXPPROTOTYPE, __jsregexp_Global, ATTRS(0, 0), NULL, ATTRS(0,0), JSPROP_DESC_HAS_UVUWUEC);
      ADD_ACCESSOR_PROPERTY(JSBUILTIN_STRING_IGNORECASE_UL, JSBUILTIN_REGEXPPROTOTYPE, __jsregexp_Ignorecase, ATTRS(0, 0), NULL, ATTRS(0,0), JSPROP_DESC_HAS_UVUWUEC);
      ADD_ACCESSOR_PROPERTY(JSBUILTIN_STRING_MULTILINE, JSBUILTIN_REGEXPPROTOTYPE, __jsregexp_Multiline, ATTRS(0, 0), NULL, ATTRS(0,0), JSPROP_DESC_HAS_UVUWUEC);
      ADD_FUNCTION_PROPERTY(JSBUILTIN_STRING_EXEC, __jsregexp_Exec, ATTRS(1, 1));
      ADD_FUNCTION_PROPERTY(JSBUILTIN_STRING_TEST, __jsregexp_Test, ATTRS(1, 1));
      ADD_FUNCTION_PROPERTY(JSBUILTIN_STRING_TO_STRING_UL, __jsregexp_ToString, ATTRS(0, 0));
      break;
    case JSBUILTIN_DATECONSTRUCTOR:
      ADD_VALUE_PROPERTY(JSBUILTIN_STRING_PROTOTYPE, (JSBUILTIN_DATEPROTOTYPE));
      ADD_FUNCTION_PROPERTY(JSBUILTIN_STRING_UTC_U, __jsdate_UTC, ATTRS(UNCERTAIN_NARGS, 7));
      ADD_FUNCTION_PROPERTY(JSBUILTIN_STRING_NOW, __jsdate_Now, ATTRS(0, 0));
      ADD_FUNCTION_PROPERTY(JSBUILTIN_STRING_PARSE, __jsdate_Parse, ATTRS(1, 1));
      break;
    case JSBUILTIN_DATEPROTOTYPE:
      ADD_VALUE_PROPERTY(JSBUILTIN_STRING_CONSTRUCTOR, (JSBUILTIN_DATECONSTRUCTOR));
      ADD_FUNCTION_PROPERTY(JSBUILTIN_STRING_HAS_OWN_PROPERTY_UL, __jsobj_pt_hasOwnProperty, ATTRS(1, 1));
      ADD_FUNCTION_PROPERTY(JSBUILTIN_STRING_GET_DATE_UL, __jsdate_GetDate, ATTRS(0, 0));
      ADD_FUNCTION_PROPERTY(JSBUILTIN_STRING_GET_DAY_UL, __jsdate_GetDay, ATTRS(0, 0));
      ADD_FUNCTION_PROPERTY(JSBUILTIN_STRING_GET_FULL_YEAR_UL, __jsdate_GetFullYear, ATTRS(0, 0));
      ADD_FUNCTION_PROPERTY(JSBUILTIN_STRING_GET_HOURS_UL, __jsdate_GetHours, ATTRS(0, 0));
      ADD_FUNCTION_PROPERTY(JSBUILTIN_STRING_GET_MILLISECONDS_UL, __jsdate_GetMilliseconds, ATTRS(0, 0));
      ADD_FUNCTION_PROPERTY(JSBUILTIN_STRING_GET_MINUTES_UL, __jsdate_GetMinutes, ATTRS(0, 0));
      ADD_FUNCTION_PROPERTY(JSBUILTIN_STRING_GET_MONTH_UL, __jsdate_GetMonth, ATTRS(0, 0));
      ADD_FUNCTION_PROPERTY(JSBUILTIN_STRING_GET_SECONDS_UL, __jsdate_GetSeconds, ATTRS(0, 0));
      ADD_FUNCTION_PROPERTY(JSBUILTIN_STRING_GET_TIME_UL, __jsdate_GetTime, ATTRS(0, 0));
      ADD_FUNCTION_PROPERTY(JSBUILTIN_STRING_GET_TIMEZONE_OFFSET_UL, __jsdate_GetTimezoneOffset, ATTRS(0, 0));
      ADD_FUNCTION_PROPERTY(JSBUILTIN_STRING_GET_UTC_DATE_UL, __jsdate_GetUTCDate, ATTRS(0, 0));
      ADD_FUNCTION_PROPERTY(JSBUILTIN_STRING_GET_UTC_DAY_UL, __jsdate_GetUTCDay, ATTRS(0, 0));
      ADD_FUNCTION_PROPERTY(JSBUILTIN_STRING_GET_UTC_FULL_YEAR_UL, __jsdate_GetUTCFullYear, ATTRS(0, 0));
      ADD_FUNCTION_PROPERTY(JSBUILTIN_STRING_GET_UTC_HOURS_UL, __jsdate_GetUTCHours, ATTRS(0, 0));
      ADD_FUNCTION_PROPERTY(JSBUILTIN_STRING_GET_UTC_MILLISECONDS_UL, __jsdate_GetUTCMilliseconds, ATTRS(0, 0));
      ADD_FUNCTION_PROPERTY(JSBUILTIN_STRING_GET_UTC_MINUTES_UL, __jsdate_GetUTCMinutes, ATTRS(0, 0));
      ADD_FUNCTION_PROPERTY(JSBUILTIN_STRING_GET_UTC_MONTH_UL, __jsdate_GetUTCMonth, ATTRS(0, 0));
      ADD_FUNCTION_PROPERTY(JSBUILTIN_STRING_GET_UTC_SECONDS_UL, __jsdate_GetUTCSeconds, ATTRS(0, 0));
      ADD_FUNCTION_PROPERTY(JSBUILTIN_STRING_SET_DATE_UL, __jsdate_SetDate, ATTRS(1, 1));
      ADD_FUNCTION_PROPERTY(JSBUILTIN_STRING_SET_FULL_YEAR_UL, __jsdate_SetFullYear, ATTRS(UNCERTAIN_NARGS, 3));
      ADD_FUNCTION_PROPERTY(JSBUILTIN_STRING_SET_HOURS_UL, __jsdate_SetHours, ATTRS(UNCERTAIN_NARGS, 4));
      ADD_FUNCTION_PROPERTY(JSBUILTIN_STRING_SET_MILLISECONDS_UL, __jsdate_SetMilliseconds, ATTRS(1, 1));
      ADD_FUNCTION_PROPERTY(JSBUILTIN_STRING_SET_MINUTES_UL, __jsdate_SetMinutes, ATTRS(UNCERTAIN_NARGS, 3));
      ADD_FUNCTION_PROPERTY(JSBUILTIN_STRING_SET_MONTH_UL, __jsdate_SetMonth, ATTRS(UNCERTAIN_NARGS, 2));
      ADD_FUNCTION_PROPERTY(JSBUILTIN_STRING_SET_SECONDS_UL, __jsdate_SetSeconds, ATTRS(UNCERTAIN_NARGS, 2));
      ADD_FUNCTION_PROPERTY(JSBUILTIN_STRING_SET_TIME_UL, __jsdate_SetTime, ATTRS(1, 1));
      ADD_FUNCTION_PROPERTY(JSBUILTIN_STRING_SET_UTC_DATE_UL, __jsdate_SetUTCDate, ATTRS(1, 1));
      ADD_FUNCTION_PROPERTY(JSBUILTIN_STRING_SET_UTC_FULL_YEAR_UL, __jsdate_SetUTCFullYear, ATTRS(UNCERTAIN_NARGS, 3));
      ADD_FUNCTION_PROPERTY(JSBUILTIN_STRING_SET_UTC_HOURS_UL, __jsdate_SetUTCHours, ATTRS(UNCERTAIN_NARGS, 4));
      ADD_FUNCTION_PROPERTY(JSBUILTIN_STRING_SET_UTC_MILLISECONDS_UL, __jsdate_SetUTCMilliseconds, ATTRS(1, 1));
      ADD_FUNCTION_PROPERTY(JSBUILTIN_STRING_SET_UTC_MINUTES_UL, __jsdate_SetUTCMinutes, ATTRS(UNCERTAIN_NARGS, 3));
      ADD_FUNCTION_PROPERTY(JSBUILTIN_STRING_SET_UTC_MONTH_UL, __jsdate_SetUTCMonth, ATTRS(UNCERTAIN_NARGS, 2));
      ADD_FUNCTION_PROPERTY(JSBUILTIN_STRING_SET_UTC_SECONDS_UL, __jsdate_SetUTCSeconds, ATTRS(UNCERTAIN_NARGS, 2));
      ADD_FUNCTION_PROPERTY(JSBUILTIN_STRING_TO_DATE_STRING_UL, __jsdate_ToDateString, ATTRS(0, 0));
      ADD_FUNCTION_PROPERTY(JSBUILTIN_STRING_TO_LOCALE_DATE_STRING_UL, __jsdate_ToLocaleDateString, ATTRS(0, 0));
      ADD_FUNCTION_PROPERTY(JSBUILTIN_STRING_TO_LOCALE_STRING_UL, __jsdate_ToLocaleString, ATTRS(0, 0));
      ADD_FUNCTION_PROPERTY(JSBUILTIN_STRING_TO_LOCALE_TIME_STRING_UL, __jsdate_ToLocaleTimeString, ATTRS(0, 0));
      ADD_FUNCTION_PROPERTY(JSBUILTIN_STRING_TO_STRING_UL, __jsdate_ToString, ATTRS(0, 0));
      ADD_FUNCTION_PROPERTY(JSBUILTIN_STRING_TO_TIME_STRING_UL, __jsdate_ToTimeString, ATTRS(0, 0));
      ADD_FUNCTION_PROPERTY(JSBUILTIN_STRING_TO_UTC_STRING_UL, __jsdate_ToUTCString, ATTRS(0, 0));
      ADD_FUNCTION_PROPERTY(JSBUILTIN_STRING_VALUE_OF_UL, __jsdate_ValueOf, ATTRS(0, 0));
      ADD_FUNCTION_PROPERTY(JSBUILTIN_STRING_TO_ISO_STRING_UL, __jsdate_ToISOString, ATTRS(0, 0));
      ADD_FUNCTION_PROPERTY(JSBUILTIN_STRING_TO_JSON_UL, __jsdate_ToJSON, ATTRS(1, 1));
      ADD_FUNCTION_PROPERTY(JSBUILTIN_STRING_GET_YEAR_UL, __jsdate_GetYear, ATTRS(0, 0));
      ADD_FUNCTION_PROPERTY(JSBUILTIN_STRING_SET_YEAR_UL, __jsdate_SetYear, ATTRS(1, 1));
      ADD_FUNCTION_PROPERTY(JSBUILTIN_STRING_TO_GMT_STRING_UL, __jsdate_ToGMTString, ATTRS(0, 0));
      break;
    case JSBUILTIN_GLOBALOBJECT:
      if (__is_globalthis(bi_obj)) {
        ADD_VALUE2_PROPERTY(JSBUILTIN_STRING_NAN, (JSBUILTIN_NAN), __nan_value());
        ADD_VALUE2_PROPERTY(JSBUILTIN_STRING_INFINITY_UL, (JSBUILTIN_INFINITY),  __number_infinity());
        ADD_VALUE2_PROPERTY(JSBUILTIN_STRING_UNDEFINED, (JSBUILTIN_UNDEFINED), __undefined_value());
        // 15.1.2 and 15.1.3  URI Handling and Function Properties of the Global Object
        ADD_FUNCTION_PROPERTY(JSBUILTIN_STRING_EVAL, __js_eval, ATTRS(0, 0));
        ADD_FUNCTION_PROPERTY(JSBUILTIN_STRING_PARSE_INT, __js_parseint, ATTRS(UNCERTAIN_NARGS, 2));
        ADD_FUNCTION_PROPERTY(JSBUILTIN_STRING_PARSE_FLOAT, __js_parsefloat, ATTRS(UNCERTAIN_NARGS, 0));
        ADD_FUNCTION_PROPERTY(JSBUILTIN_STRING_IS_NAN, __is_nan, ATTRS(1, 0));
        ADD_FUNCTION_PROPERTY(JSBUILTIN_STRING_IS_FINITE, __is_infinity, ATTRS(1, 0));
        ADD_FUNCTION_PROPERTY(JSBUILTIN_STRING_DECODE_URI, __js_decodeuri, ATTRS(UNCERTAIN_NARGS, 0));
        ADD_FUNCTION_PROPERTY(JSBUILTIN_STRING_DECODE_URI_COMPONENT, __js_decodeuricomponent, ATTRS(UNCERTAIN_NARGS, 0));
        ADD_FUNCTION_PROPERTY(JSBUILTIN_STRING_ENCODE_URI, __js_encodeuri, ATTRS(UNCERTAIN_NARGS, 0));
        ADD_FUNCTION_PROPERTY(JSBUILTIN_STRING_ENCODE_URI_COMPONENT, __js_encodeuricomponent, ATTRS(UNCERTAIN_NARGS, 1));
        // 15.1.4 Constructor Properties of the Global Object
        ADD_FUNCTION_PROPERTY(JSBUILTIN_STRING_OBJECT_UL, __js_new_obj, ATTRS(UNCERTAIN_NARGS, 1));
        ADD_FUNCTION_PROPERTY(JSBUILTIN_STRING_FUNCTION_UL, __js_new_functionN, ATTRS(UNCERTAIN_NARGS, 1));
        ADD_FUNCTION_PROPERTY(JSBUILTIN_STRING_ARRAY_UL, __js_new_arr, ATTRS(UNCERTAIN_NARGS, 1));
        ADD_FUNCTION_PROPERTY(JSBUILTIN_STRING_STRING_UL, __js_new_stringconstructor, ATTRS(UNCERTAIN_NARGS, 1));
        ADD_FUNCTION_PROPERTY(JSBUILTIN_STRING_BOOLEAN_UL, __js_new_booleanconstructor, ATTRS(UNCERTAIN_NARGS, 1));
        ADD_FUNCTION_PROPERTY(JSBUILTIN_STRING_NUMBER_UL, __js_new_numberconstructor, ATTRS(UNCERTAIN_NARGS, 1));
        ADD_FUNCTION_PROPERTY(JSBUILTIN_STRING_DATE_UL, __js_new_dateconstructor, ATTRS(UNCERTAIN_NARGS, 2));
        ADD_FUNCTION_PROPERTY(JSBUILTIN_STRING_REGEXP_UL, __js_new_regexp_obj, ATTRS(UNCERTAIN_NARGS, 7));
        ADD_FUNCTION_PROPERTY(JSBUILTIN_STRING_ERROR_UL, __js_new_error_obj, ATTRS(UNCERTAIN_NARGS, 1));
        ADD_FUNCTION_PROPERTY(JSBUILTIN_STRING_EVAL_ERROR_UL, __js_new_evalerror_obj, ATTRS(UNCERTAIN_NARGS, 1));
        ADD_FUNCTION_PROPERTY(JSBUILTIN_STRING_RANGE_ERROR_UL, __js_new_rangeerror_obj, ATTRS(UNCERTAIN_NARGS, 1));
        ADD_FUNCTION_PROPERTY(JSBUILTIN_STRING_REFERENCE_ERROR_UL, __js_new_reference_error_obj, ATTRS(UNCERTAIN_NARGS, 1));
        ADD_FUNCTION_PROPERTY(JSBUILTIN_STRING_SYNTAX_ERROR_UL, __js_new_syntaxerror_obj, ATTRS(UNCERTAIN_NARGS, 1));
        ADD_FUNCTION_PROPERTY(JSBUILTIN_STRING_TYPE_ERROR_UL, __js_new_type_error_obj, ATTRS(UNCERTAIN_NARGS, 1));
        ADD_FUNCTION_PROPERTY(JSBUILTIN_STRING_URI_ERROR_UL, __js_new_urierror_obj, ATTRS(UNCERTAIN_NARGS, 1));
        ADD_FUNCTION_PROPERTY(JSBUILTIN_STRING_MATH_UL, __js_new_math_obj, ATTRS(0, 1));
        ADD_FUNCTION_PROPERTY(JSBUILTIN_STRING_JSON_U, __js_new_json_obj, ATTRS(0, 1));
        ADD_FUNCTION_PROPERTY(JSBUILTIN_STRING_ESCAPE, __js_escape, ATTRS(1, 1));
      }
      break;
    case JSBUILTIN_INTL:
      ADD_VALUE_PROPERTY(JSBUILTIN_STRING_COLLATOR_UL, (JSBUILTIN_INTL_COLLATOR_CONSTRUCTOR));
      ADD_VALUE_PROPERTY(JSBUILTIN_STRING_NUMBERFORMAT_UL, (JSBUILTIN_INTL_NUMBERFORMAT_CONSTRUCTOR));
      ADD_VALUE_PROPERTY(JSBUILTIN_STRING_DATETIMEFORMAT_UL, (JSBUILTIN_INTL_DATETIMEFORMAT_CONSTRUCTOR));
      break;
    case JSBUILTIN_INTL_COLLATOR_CONSTRUCTOR:
      ADD_VALUE_PROPERTY(JSBUILTIN_STRING_PROTOTYPE, (JSBUILTIN_INTL_COLLATOR_PROTOTYPE));
      ADD_FUNCTION_PROPERTY(JSBUILTIN_STRING_SUPPORTED_LOCALES_OF, __jsintl_CollatorSupportedLocalesOf, ATTRS(UNCERTAIN_NARGS, 2));
      break;
    case JSBUILTIN_INTL_COLLATOR_PROTOTYPE:
      ADD_VALUE_PROPERTY(JSBUILTIN_STRING_CONSTRUCTOR, (JSBUILTIN_INTL_COLLATOR_CONSTRUCTOR));
      ADD_FUNCTION_PROPERTY(JSBUILTIN_STRING_COMPARE, __jsintl_CollatorCompare, ATTRS(2, 2));
      ADD_FUNCTION_PROPERTY(JSBUILTIN_STRING_RESOLVED_OPTIONS, __jsintl_CollatorResolvedOptions, ATTRS(0, 0));
      break;
    case JSBUILTIN_INTL_NUMBERFORMAT_CONSTRUCTOR:
      ADD_VALUE_PROPERTY(JSBUILTIN_STRING_PROTOTYPE, (JSBUILTIN_INTL_NUMBERFORMAT_PROTOTYPE));
      ADD_FUNCTION_PROPERTY(JSBUILTIN_STRING_SUPPORTED_LOCALES_OF, __jsintl_NumberFormatSupportedLocalesOf, ATTRS(UNCERTAIN_NARGS, 2));
      break;
    case JSBUILTIN_INTL_NUMBERFORMAT_PROTOTYPE:
      ADD_VALUE_PROPERTY(JSBUILTIN_STRING_CONSTRUCTOR, (JSBUILTIN_INTL_NUMBERFORMAT_CONSTRUCTOR));
      ADD_ACCESSOR_PROPERTY(JSBUILTIN_STRING_FORMAT, JSBUILTIN_INTL_NUMBERFORMAT_PROTOTYPE, __jsintl_NumberFormatFormat, ATTRS(1, 1), NULL, ATTRS(0,0), JSPROP_DESC_HAS_UVUWUEC);
      ADD_FUNCTION_PROPERTY(JSBUILTIN_STRING_RESOLVED_OPTIONS, __jsintl_NumberFormatResolvedOptions, ATTRS(0, 0));
      break;
    case JSBUILTIN_INTL_DATETIMEFORMAT_CONSTRUCTOR:
      ADD_VALUE_PROPERTY(JSBUILTIN_STRING_PROTOTYPE, (JSBUILTIN_INTL_DATETIMEFORMAT_PROTOTYPE));
      ADD_FUNCTION_PROPERTY(JSBUILTIN_STRING_SUPPORTED_LOCALES_OF, __jsintl_DateTimeFormatSupportedLocalesOf, ATTRS(UNCERTAIN_NARGS, 2));
      break;
    case JSBUILTIN_INTL_DATETIMEFORMAT_PROTOTYPE:
      ADD_VALUE_PROPERTY(JSBUILTIN_STRING_CONSTRUCTOR, (JSBUILTIN_INTL_DATETIMEFORMAT_CONSTRUCTOR));
      ADD_FUNCTION_PROPERTY(JSBUILTIN_STRING_FORMAT, __jsintl_DateTimeFormatFormat, ATTRS(1, 1));
      ADD_FUNCTION_PROPERTY(JSBUILTIN_STRING_RESOLVED_OPTIONS, __jsintl_DateTimeFormatResolvedOptions, ATTRS(0, 0));
      break;
    case JSBUILTIN_CONSOLE:
      ADD_FUNCTION_PROPERTY(JSBUILTIN_STRING_LOG, __jsconsole_pt_log, ATTRS(1, 1));
      break;
    case JSBUILTIN_ARRAYBUFFER_CONSTRUCTOR:
      ADD_VALUE_PROPERTY(JSBUILTIN_STRING_PROTOTYPE, (JSBUILTIN_ARRAYBUFFER_PROTOTYPE));
      break;
    case JSBUILTIN_ARRAYBUFFER_PROTOTYPE:
      ADD_VALUE_PROPERTY(JSBUILTIN_STRING_CONSTRUCTOR, (JSBUILTIN_ARRAYBUFFER_CONSTRUCTOR));
      break;
    case JSBUILTIN_DATAVIEW_CONSTRUCTOR:
      ADD_VALUE_PROPERTY(JSBUILTIN_STRING_PROTOTYPE, (JSBUILTIN_DATAVIEW_PROTOTYPE));
      break;
    case JSBUILTIN_DATAVIEW_PROTOTYPE:
      ADD_VALUE_PROPERTY(JSBUILTIN_STRING_CONSTRUCTOR, (JSBUILTIN_DATAVIEW_CONSTRUCTOR));
      ADD_FUNCTION_PROPERTY(JSBUILTIN_STRING_SETINT8, __jsdataview_pt_setInt8, ATTRS(UNCERTAIN_NARGS, 2));
      ADD_FUNCTION_PROPERTY(JSBUILTIN_STRING_GETINT8, __jsdataview_pt_getInt8, ATTRS(UNCERTAIN_NARGS, 2));
      ADD_FUNCTION_PROPERTY(JSBUILTIN_STRING_SETINT16, __jsdataview_pt_setInt16, ATTRS(UNCERTAIN_NARGS, 2));
      ADD_FUNCTION_PROPERTY(JSBUILTIN_STRING_GETINT16, __jsdataview_pt_getInt16, ATTRS(UNCERTAIN_NARGS, 2));
      ADD_FUNCTION_PROPERTY(JSBUILTIN_STRING_SETINT32, __jsdataview_pt_setInt32, ATTRS(UNCERTAIN_NARGS, 2));
      ADD_FUNCTION_PROPERTY(JSBUILTIN_STRING_GETINT32, __jsdataview_pt_getInt32, ATTRS(UNCERTAIN_NARGS, 2));
      ADD_FUNCTION_PROPERTY(JSBUILTIN_STRING_SETUINT8, __jsdataview_pt_setUint8, ATTRS(UNCERTAIN_NARGS, 2));
      ADD_FUNCTION_PROPERTY(JSBUILTIN_STRING_GETUINT8, __jsdataview_pt_getUint8, ATTRS(UNCERTAIN_NARGS, 2));
      ADD_FUNCTION_PROPERTY(JSBUILTIN_STRING_SETUINT16, __jsdataview_pt_setUint16, ATTRS(UNCERTAIN_NARGS, 2));
      ADD_FUNCTION_PROPERTY(JSBUILTIN_STRING_GETUINT16, __jsdataview_pt_getUint16, ATTRS(UNCERTAIN_NARGS, 2));
      ADD_FUNCTION_PROPERTY(JSBUILTIN_STRING_SETUINT32, __jsdataview_pt_setUint32, ATTRS(UNCERTAIN_NARGS, 2));
      ADD_FUNCTION_PROPERTY(JSBUILTIN_STRING_GETUINT32, __jsdataview_pt_getUint32, ATTRS(UNCERTAIN_NARGS, 2));
      break;
    default:
      break;
  }

#undef ADD_FUNCTION_PROPERTY
#undef ADD_VALUE_PROPERTY
#undef ADD_VALUE2_PROPERTY
  return NULL;
}

// ecma 8.10.1
// 1. If Desc is undefined, then return false.
// 2. If both Desc.[[Get]] and Desc.[[Set]] are absent, then return false.
// 3. Return true.
// Inline small function.
static inline bool __jsprop_desc_IsAccessorDescriptor(__jsprop_desc desc) {
  return __has_get_or_set(desc);
}

// ecma 8.10.2
// 1. If Desc is undefined, then return false.
// 2. If both Desc.[[Get]] and Desc.[[Set]] are absent, then return false.
// 3. Return true.
// Inline small function.
static inline bool __jsprop_desc_IsDataDescriptor(__jsprop_desc desc) {
  return __has_value(desc) || __has_writable(desc);
}

// ecma 8.10.3
// 1. If Desc is undefined, then return false.
// 2. If IsAccessorDescriptor(Desc) and IsDataDescriptor(Desc) are both false, then return true.
// 3. Return false.
// Inline small function.
static inline bool __jsprop_desc_IsGenericDescriptor(__jsprop_desc desc) {
  // desc.s.fields == 0 means Desc is defined but Desc.get/Desc.set/Desc.value are absent.
  return desc.s.fields == 0 && !__has_writable(desc);
}

// ecma 8.10.4
// Inline if call once.
static inline __jsvalue __jsprop_desc_FromPropertyDescriptor(__jsprop_desc desc) {
  // ecma 8.10.4 step 1.
  if (__is_undefined_desc(desc)) {
    return __undefined_value();
  }
  // ecma 8.10.4 step 2.
  __jsobject *obj = __js_new_obj_obj_0();
  // ecma 8.10.4 step 3.
  if (__jsprop_desc_IsDataDescriptor(desc)) {
    __jsvalue v = __get_value(desc);
    // ecma 8.10.4 step 3.a.
    __jsobj_helper_add_value_property(obj, JSBUILTIN_STRING_VALUE, &v, JSPROP_DESC_HAS_VWEC);
    // ecma 8.10.4 step 3.b.
    v = __boolean_value(__writable(desc));
    __jsobj_helper_add_value_property(obj, JSBUILTIN_STRING_WRITABLE, &v, JSPROP_DESC_HAS_VWEC);
    // ecma 8.10.4 step 4.
  } else {
    MAPLE_JS_ASSERT(__jsprop_desc_IsAccessorDescriptor(desc));
    // ecma 8.10.4 step 4.a.
    __jsobject *getter = __get_get(desc);
    __jsvalue v;
    if (getter) {
      v = __object_value(getter);
    } else {
      v = __undefined_value();
    }
    __jsobj_helper_add_value_property(obj, JSBUILTIN_STRING_GET, &v, JSPROP_DESC_HAS_VWEC);
    // ecma 8.10.4 step 4.b.
    __jsobject *setter = __get_set(desc);
    if (setter) {
      v = __object_value(__get_set(desc));
    } else {
      v = __undefined_value();
    }
    __jsobj_helper_add_value_property(obj, JSBUILTIN_STRING_SET, &v, JSPROP_DESC_HAS_VWEC);
  }
  // ecma 8.10.4 step 5.
  __jsvalue v = __boolean_value(__enumerable(desc));
  __jsobj_helper_add_value_property(obj, JSBUILTIN_STRING_ENUMERABLE, &v, JSPROP_DESC_HAS_VWEC);
  // ecma 8.10.4 step 6.
  v = __boolean_value(__configurable(desc));
  __jsobj_helper_add_value_property(obj, JSBUILTIN_STRING_CONFIGURABLE, &v, JSPROP_DESC_HAS_VWEC);
  // ecma 8.10.4 step 7.
  return __object_value(obj);
}

static __jsvalue __jsobj_internal_get_by_desc(__jsobject *obj, __jsprop_desc desc, __jsvalue *orgVal = NULL) {
  // ecma 8.12.3 step 2.
  if (__is_undefined_desc(desc)) {
    return __undefined_value();
  }
  // ecma 8.12.3 step 3.
  if (__jsprop_desc_IsDataDescriptor(desc)) {
    return __get_value(desc);
  } else {  // ecma 8.12.3 step 4.
    MAPLE_JS_ASSERT(__jsprop_desc_IsAccessorDescriptor(desc));
    // ecma 8.12.3 step 6.
    if (__has_get(desc)) {
      __jsobject *getter = __get_get(desc);
      if (!getter) {
        return __undefined_value();
      }
      __jsvalue this_arg = __object_value(obj);
      return __jsfun_internal_call(getter, &this_arg, NULL, 0, orgVal);
    } else { /* ecma 8.12.3 step 5/6. */
      return __undefined_value();
    }
  }
}

// For codesize, Combine HasProperty and Get.
// Return true if HasProperty return true, else return the false.
// If HasProperty return true, store the result of GET to *result;
bool __jsobj_helper_HasPropertyAndGet(__jsobject *obj, __jsstring *p, __jsvalue *result) {
  if (!__jsobj_internal_HasProperty(obj, p)) {
    return false;
  }
  *result = __jsobj_internal_Get(obj, p);
  return true;
}

bool __jsobj_helper_HasPropertyAndGet(__jsobject *obj, __jsbuiltin_string_id id, __jsvalue *result) {
  return __jsobj_helper_HasPropertyAndGet(obj, __jsstr_get_builtin(id), result);
}

bool __jsobj_helper_HasPropertyAndGet(__jsobject *obj, uint32_t index, __jsvalue *result) {
  __jsstring *p;
  if (index <= INT32_MAX) {
    p = __js_NumberToString((int32_t)index);
  } else {
    p = __js_DoubleToString(index);
  }
  bool res = __jsobj_helper_HasPropertyAndGet(obj, p, result);
  memory_manager->RecallString(p);
  return res;
}

// For codesize, Combine Get and Call without arguments.
bool __jsobj_helper_GetAndCall(__jsobject *obj, __jsbuiltin_string_id id, __jsvalue *result) {
  __jsvalue call = __jsobj_internal_Get(obj, id);
  __jsvalue this_object = __object_value(obj);
  if (__js_IsCallable(&call)) {
    *result = __jsfun_val_call(&call, &this_object, NULL, 0);
    return true;
  }
  return false;
}

// ecma 8.10.5
__jsprop_desc __jsprop_desc_ToPropertyDescriptor(__jsvalue *o) {
  // ecma 8.10.5 step 1.
  if (!__is_js_object(o)) {
    MAPLE_JS_TYPEERROR_EXCEPTION();
  }
  // ecma 8.10.5 step 2.
  __jsprop_desc desc = __new_empty_desc();
  __jsobject *obj = __jsval_to_object(o);
  __jsvalue result;
  // ecma 8.10.5 step 3.
  if (__jsobj_helper_HasPropertyAndGet(obj, JSBUILTIN_STRING_ENUMERABLE, &result)) {
    __set_enumerable(&desc, __js_ToBoolean(&result));
  }
  // ecma 8.10.5 step 4.
  if (__jsobj_helper_HasPropertyAndGet(obj, JSBUILTIN_STRING_CONFIGURABLE, &result)) {
    __set_configurable(&desc, __js_ToBoolean(&result));
  }
  // ecma 8.10.5 step 5.
  if (__jsobj_helper_HasPropertyAndGet(obj, JSBUILTIN_STRING_VALUE, &result)) {
    __set_value(&desc, &result);
  }
  // ecma 8.10.5 step 6.
  if (__jsobj_helper_HasPropertyAndGet(obj, JSBUILTIN_STRING_WRITABLE, &result)) {
    __set_writable(&desc, __js_ToBoolean(&result));
  }
  // ecma 8.10.5 step 7.
  if (__jsobj_helper_HasPropertyAndGet(obj, JSBUILTIN_STRING_GET, &result)) {
    if (!__is_undefined(&result) && !__is_js_function(&result)) {
      MAPLE_JS_TYPEERROR_EXCEPTION();
    }
    __jsobject *getter = __is_undefined(&result) ? NULL : __jsval_to_object(&result);
    __set_get(&desc, getter);
  }
  // ecma 8.10.5 step 8.
  if (__jsobj_helper_HasPropertyAndGet(obj, JSBUILTIN_STRING_SET, &result)) {
    if (!__is_undefined(&result) && !__is_js_function(&result)) {
      MAPLE_JS_TYPEERROR_EXCEPTION();
    }
    __jsobject *setter = __is_undefined(&result) ? NULL : __jsval_to_object(&result);
    __set_set(&desc, setter);
  }
  // ecma 8.10.5 step 9.
  if(__has_get_or_set(desc) && (__has_value(desc) || __has_writable(desc))) {
    MAPLE_JS_TYPEERROR_EXCEPTION();
  }
  // ecma 8.10.5 step 10.
  return desc;
}

__jsprop_desc __jsobj_internal_GetOwnPropertyByValue(__jsobject *o, uint32_t index) {
  __jsobj_helper_convert_to_generic(o);
  __jsprop *prop = __jsobj_helper_get_propertyByValue(o, index);
  // ecma 8.12.1 step 1: if o doesn't have an own property with name p, return
  // undefined.
  if (!prop) {
    return __undefined_desc();
  }
  // ecma 8.12.1 step 2~8
  // ??? Can we return the prop-desc directly.
  // It seems there's no difference from ecma 8.12.1 step 2~8.
  return prop->desc;
}
// ecma 8.12.1
__jsprop_desc __jsobj_internal_GetOwnProperty(__jsobject *o, __jsstring *p) {
  bool isNum;
  uint32 idxNum = __jsstr_is_numidx(p, isNum);
  if (isNum) {
    return __jsobj_internal_GetOwnPropertyByValue(o, idxNum);
  }
  __jsobj_helper_convert_to_generic(o);
  __jsprop *prop = __jsobj_helper_get_property(o, p);
  // ecma 8.12.1 step 1: if o doesn't have an own property with name p, return
  // undefined.
  if (!prop) {
    return __undefined_desc();
  }
  // ecma 8.12.1 step 2~8
  // ??? Can we return the prop-desc directly.
  // It seems there's no difference from ecma 8.12.1 step 2~8.
  return prop->desc;
}

// ecma 8.12.1
__jsprop_desc __jsobj_internal_GetOwnProperty(__jsobject *o, __jsbuiltin_string_id id) {
  return __jsobj_internal_GetOwnProperty(o, __jsstr_get_builtin(id));
}

// ecma 8.12.1
__jsprop_desc __jsobj_internal_GetOwnProperty(__jsobject *o, __jsvalue *p) {
  __jsstring *name = __js_ToString(p);
  if (!__is_string(p)) {
    GCIncRf(name);
  }
  __jsprop_desc desc = __jsobj_internal_GetOwnProperty(o, name);
  if (!__is_string(p)) {
    GCDecRf(name);
  }
  return desc;
}

// ecma 8.12.2
__jsprop_desc __jsobj_internal_GetProperty(__jsobject *o, __jsvalue *p) {
  // ecma 8.12.2 step 1.
  __jsprop_desc desc = __jsobj_internal_GetOwnProperty(o, p);
  // ecma 8.12.2 step 2.
  if (!__is_undefined_desc(desc)) {
    return desc;
  }
  // ecma 8.12.2 step 3.
  __jsobject *proto = __jsobj_get_prototype(o);
  // ecma 8.12.2 step 4.
  if (!proto) {
    return __undefined_desc();
  }
  // ecma 8.12.2 step 5.
  return __jsobj_internal_GetProperty(proto, p);
}

// TODO: merge with the up function
__jsprop_desc __jsobj_internal_GetPropertyByValue(__jsobject *o, uint32_t index) {
  __jsprop_desc desc = __jsobj_internal_GetOwnPropertyByValue(o, index);
  // ecma 8.12.2 step 2.
  if (!__is_undefined_desc(desc)) {
    return desc;
  }
  // ecma 8.12.2 step 3.
  __jsobject *proto = __jsobj_get_prototype(o);
  // ecma 8.12.2 step 4.
  if (!proto) {
    return __undefined_desc();
  }
  // ecma 8.12.2 step 5.
  return __jsobj_internal_GetPropertyByValue(proto, index);
}

// Fixme: Donot need this.
__jsprop_desc __jsobj_internal_GetProperty(__jsobject *o, __jsstring *p) {
  __jsvalue name = __string_value(p);
  return __jsobj_internal_GetProperty(o, &name);
}

// TODO: merge the following 2 functions
__jsvalue __jsobj_internal_GetByValue(__jsobject *obj, uint32_t index) {
  // Fast path.
  if (obj->object_type == JSGENERIC && obj->object_class == JSSTRING) {
    // access string using bracket notation, i.e. str[i]
    __jsstring *str = obj->shared.prim_string;
    uint32_t size = __jsstr_get_length(str);
    if ((int32_t)index < 0 || index >= (int32_t)size) {
      return __undefined_value();
    }
    __jsstring *str1 = __js_new_string_internal(1, true);
    __jsstr_set_char(str1, 0, __jsstr_get_char(str, index));
    return __string_value(str1);
  }
  if (obj->object_type == JSREGULAR_OBJECT) {
    __jsprop *prop = __jsobj_helper_get_propertyByValue(obj, index);
    if (prop) {
      return __get_value(prop->desc);
    }
    __jsobject *proto = __jsobj_get_prototype(obj);
    if (proto) {
      return __jsobj_internal_GetByValue(proto, index);
    }
    return __undefined_value();
  }
  if (obj->object_type == JSREGULAR_ARRAY) {
    if ((__jsbuiltin_string_id)index == JSBUILTIN_STRING_LENGTH) {
      return obj->shared.array_props[0];
    }
    __jsobject *proto = __jsobj_get_prototype(obj);
    if (proto) {
      return __jsobj_internal_GetByValue(proto, index);
    }
  }

  // Slow path.
  // ecma 8.12.3 step 1.
  __jsprop_desc desc = __jsobj_internal_GetPropertyByValue(obj, index);
  return __jsobj_internal_get_by_desc(obj, desc);
}

// ecma 8.12.3
__jsvalue __jsobj_internal_Get(__jsobject *obj, __jsstring *p) {
  // Fast path.
  if (obj->object_type == JSREGULAR_OBJECT) {
    __jsprop *prop = __jsobj_helper_get_property(obj, p);
    if (prop) {
      return __get_value(prop->desc);
    }
    __jsobject *proto = __jsobj_get_prototype(obj);
    if (proto) {
      return __jsobj_internal_Get(proto, p);
    }
    return __undefined_value();
  }
  if (obj->object_type == JSREGULAR_ARRAY) {
    if (__jsstr_equal_to_builtin(p, JSBUILTIN_STRING_LENGTH)) {
      return obj->shared.array_props[0];
    }
    __jsobject *proto = __jsobj_get_prototype(obj);
    if (proto) {
      return __jsobj_internal_Get(proto, p);
    }
  }

  // Slow path.
  // ecma 8.12.3 step 1.
  __jsprop_desc desc = __jsobj_internal_GetProperty(obj, p);
  return __jsobj_internal_get_by_desc(obj, desc);
}

// ecma 8.12.3
__jsvalue __jsobj_internal_Get(__jsobject *obj, __jsvalue *p) {
  if (obj->object_type == JSREGULAR_ARRAY) {
    MAPLE_JS_ASSERT(obj->prop_list == NULL);
    __jsvalue *array = obj->shared.array_props;
    uint32_t index = __jsarr_getIndex(p);
    if (index != MAX_ARRAY_INDEX && index <= ARRAY_MAXINDEXNUM_INTERNAL) {
      uint32_t length = __jsobj_helper_get_length(obj);
      if (index < length) {
        __jsvalue elem = __jsarr_GetRegularElem(obj, array, index);
        return __is_none(&elem) ? __undefined_value() : elem;
      } else {
        __jsobject *proto = __jsobj_get_prototype(obj);
        return __jsobj_internal_Get(proto, p);
      }
    } else {
      __jsstring *name = __js_ToString(p);
      if (__jsstr_equal_to_builtin(name, JSBUILTIN_STRING_LENGTH)) {
        if (!__is_string(p)) {
          memory_manager->RecallString(name);
        }
        return array[0];
      }
      __jsobject *proto = __jsobj_get_prototype(obj);
      if (proto) {
        __jsvalue res = __jsobj_internal_Get(proto, name);
        if (!__is_string(p)) {
          memory_manager->RecallString(name);
        }
        return res;
      }
    }
  }
  __jsstring *name = __js_ToString(p);
  bool isNum;
  uint32 idxNum = __jsstr_is_numidx(name, isNum);
  if (isNum) {
    return __jsobj_internal_GetByValue(obj, idxNum);
  } else {
    if (!__is_string(p)) {
      GCIncRf(name);
    }
    __jsvalue v = __jsobj_internal_Get(obj, name);
    if (!__is_string(p)) {
      GCDecRf(name);
    }
    return v;
  }
}

// Fixme: Donot need this.
__jsvalue __jsobj_internal_Get(__jsobject *o, __jsbuiltin_string_id id) {
  return __jsobj_internal_Get(o, __jsstr_get_builtin(id));
}

__jsvalue __jsobj_internal_Get(__jsobject *obj, uint32_t index) {
  if (obj->object_type == JSREGULAR_ARRAY) {
    MAPLE_JS_ASSERT(obj->prop_list == NULL);
    __jsvalue *array = obj->shared.array_props;
    uint32_t length = __jsobj_helper_get_length(obj);
    if (index < length && index <= ARRAY_MAXINDEXNUM_INTERNAL) {
      __jsvalue elem = __jsarr_GetRegularElem(obj, array, index);
      return __is_none(&elem) ? __undefined_value() : elem;
    }
  }
  __jsstring *name = __js_DoubleToString(index);
  __jsvalue v = __jsobj_internal_Get(obj, name);
  memory_manager->RecallString(name);
  return v;
}

static bool InheritedDescCanbePut(__jsprop_desc &inherited, __jsobject *o) {
  // ecma 8.12.3 step 6.
  if (__is_undefined_desc(inherited)) {
    return o->extensible;
  }
  // ecma 8.12.3 step 7.
  if (__jsprop_desc_IsAccessorDescriptor(inherited)) {
    // ecma 8.12.3 step 7.a.
    __jsobject *inherited_set = __get_set(inherited);
    if (!inherited_set) {
      return false;
    }
    // ecma 8.12.3 step 7.b.
    return true;
  }
  // ecma 8.12.3 step 8.
  else {
    // ecma 8.12.3 step 8.a.
    if (!o->extensible) {
      return false;
    }
    // ecma 8.12.3 step 8.b.
    return __writable(inherited);
  }
}

bool __jsobj_internal_CanPutByValue(__jsobject *o, uint32_t index) {
  // ecma 8.12.4 step 1.
  __jsprop_desc desc = __jsobj_internal_GetOwnPropertyByValue(o, index);
  // ecma 8.12.4 step 2.
  if (!__is_undefined_desc(desc)) {
    // ecma 8.12.4 step 2.a.
    if (__jsprop_desc_IsAccessorDescriptor(desc)) {
      // ecma 8.12.4 step 2.a.i.
      __jsobject *desc_set = __get_set(desc);
      if (!desc_set) {
        return false;
      }
      // ecma 8.12.4 step 2.a.ii.
      return true;
    }
    // ecma 8.12.4 step 2.b.
    return __writable(desc);
  }
  // ecma 8.12.4 step 3.
  __jsobject *proto = __jsobj_get_prototype(o);
  // ecma 8.12.4 step 4.
  if (!proto) {
    return o->extensible;
  }
  // ecma 8.12.3 step 5.
  __jsprop_desc inherited = __jsobj_internal_GetPropertyByValue(proto, index);
  return InheritedDescCanbePut(inherited, o);
}
// ecma 8.12.4
bool __jsobj_internal_CanPut(__jsobject *o, __jsstring *p, bool isStrict) {
  // ecma 8.12.4 step 1.
  __jsprop_desc desc = __jsobj_internal_GetOwnProperty(o, p);
  // ecma 8.12.4 step 2.
  if (!__is_undefined_desc(desc)) {
    // ecma 8.12.4 step 2.a.
    if (__jsprop_desc_IsAccessorDescriptor(desc)) {
      // ecma 8.12.4 step 2.a.i.
      __jsobject *desc_set = __get_set(desc);
      if (!desc_set) {
        if(isStrict)
            MAPLE_JS_TYPEERROR_EXCEPTION();
        return false;
      }
      // ecma 8.12.4 step 2.a.ii.
      return true;
    }
    // ecma 8.12.4 step 2.b.
    return __writable(desc);
  }
  // ecma 8.12.4 step 3.
  __jsobject *proto = __jsobj_get_prototype(o);
  // ecma 8.12.4 step 4.
  if (!proto) {
    return o->extensible;
  }
  // ecma 8.12.3 step 5.
  __jsprop_desc inherited = __jsobj_internal_GetProperty(proto, p);
  return InheritedDescCanbePut(inherited, o);
}

// Helper for __jsobj_internal_Put.
// Make a decide if a object can be put and do a fast put.
// 1. No own accessor property or inherited accessor property
// 2. No own unwritable data property or inherited unwritable data property
// 3. The EXTENSIBLE is true.
static inline bool __jsobj_helper_is_all_regular(__jsobject *obj) {
  while (obj) {
    if (obj->object_type != JSREGULAR_OBJECT || (obj->object_class != JSOBJECT && obj->object_class != JSERROR)) {
      return false;
    }
    obj = __jsobj_get_prototype(obj);
  }
  return true;
}

void __jsobj_internal_PutByValue(__jsobject *o, uint32_t index, __jsvalue *v, bool throw_p) {

  if (__jsobj_helper_is_all_regular(o)) {
    __jsprop *prop = __jsobj_helper_get_propertyByValue(o, index);
    if (prop) {
      __jsprop_desc desc = prop->desc;
      if (__writable(desc)) {
        __set_value_gc(&prop->desc, v);
        return;
      }
    } else {
      __jsobj_helper_init_value_propertyByValue(o, index, v, JSPROP_DESC_HAS_VWEC);
      return;
    }
  }
  // Slow path.
  // ecma 8.12.5 step 1.
  if (!__jsobj_internal_CanPutByValue(o, index)) {
    // ecma 8.12.5 step 1.a.
    if (throw_p) {
      MAPLE_JS_TYPEERROR_EXCEPTION();
    }
    // ecma 8.12.5 step 1.b.
    else {
      return;
    }
  }
  // ecma 8.12.5 step 2.
  __jsprop_desc own_desc = __jsobj_internal_GetOwnPropertyByValue(o, index);
  // ecma 8.12.5 step 3.
  if (__jsprop_desc_IsDataDescriptor(own_desc)) {
    __jsobj_helper_add_value_propertyByValue(o, index, v, JSPROP_DESC_HAS_V);
    return;
  }
  // ecma 8.12.5 step 4.
  __jsprop_desc desc = __jsobj_internal_GetPropertyByValue(o, index);
  // ecma 8.12.5 step 5.
  if (__jsprop_desc_IsAccessorDescriptor(desc)) {
    // ecma 8.12.5 step 5.a.
    __jsobject *setter = __get_set(desc);
    // ecma 8.12.5 step 5.b.
    if (setter) {
      __jsvalue obj = __object_value(o);
      __jsfun_internal_call(setter, &obj, v, 1);
    }
  }
  // ecma 8.12.5 step 6.
  else {
    __jsobj_helper_add_value_propertyByValue(o, index, v, JSPROP_DESC_HAS_VWEC);
  }
  // ecma 8.12.5 step 7.
  return;
}

// ecma 8.12.5
void __jsobj_internal_Put(__jsobject *o, __jsstring *p, __jsvalue *v, bool throw_p, bool isStrict) {
  bool isNum;
  uint32 idxNum = __jsstr_is_numidx(p, isNum);
  if (isNum) {
    __jsobj_internal_PutByValue(o, idxNum, v, throw_p);
    return;
  }
  //  Fast path.
  if (__jsobj_helper_is_all_regular(o)) {
    __jsprop *prop = __jsobj_helper_get_property(o, p);
    if (prop) {
      __jsprop_desc desc = prop->desc;
      if (__writable(desc)) {
        __set_value_gc(&prop->desc, v);
        return;
      }
    } else {
      __jsobj_helper_init_value_property(o, p, v, JSPROP_DESC_HAS_VWEC);
      return;
    }
  }
  // Slow path.
  // ecma 8.12.5 step 1.
  if (!__jsobj_internal_CanPut(o, p, isStrict)) {
    // ecma 8.12.5 step 1.a.
    if (throw_p) {
      MAPLE_JS_TYPEERROR_EXCEPTION();
    }
    // ecma 8.12.5 step 1.b.
    else {
      return;
    }
  }
  // ecma 8.12.5 step 2.
  __jsprop_desc own_desc = __jsobj_internal_GetOwnProperty(o, p);
  // ecma 8.12.5 step 3.
  if (__jsprop_desc_IsDataDescriptor(own_desc)) {
    __jsobj_helper_add_value_property(o, p, v, JSPROP_DESC_HAS_V);
    return;
  }
  // ecma 8.12.5 step 4.
  __jsprop_desc desc = __jsobj_internal_GetProperty(o, p);
  // ecma 8.12.5 step 5.
  if (__jsprop_desc_IsAccessorDescriptor(desc)) {
    // ecma 8.12.5 step 5.a.
    __jsobject *setter = __get_set(desc);
    // ecma 8.12.5 step 5.b.
    if (setter) {
      __jsvalue obj = __object_value(o);
      __jsfun_internal_call(setter, &obj, v, 1);
    }
  }
  // ecma 8.12.5 step 6.
  else {
    __jsobj_helper_add_value_property(o, p, v, JSPROP_DESC_HAS_VWEC);
  }
  // ecma 8.12.5 step 7.
  return;
}

// ecma 8.12.5
void __jsobj_internal_Put(__jsobject *obj, uint32_t index, __jsvalue *v, bool throw_p) {
  __jsobj_internal_PutByValue(obj, index, v, throw_p);
}

// ecma 8.12.6
bool __jsobj_internal_HasProperty(__jsobject *o, __jsstring *p) {
  // ecma 8.12.6 step 1.
  __jsprop_desc desc = __jsobj_internal_GetProperty(o, p);
  // ecma 8.12.6 step 2.
  if (__is_undefined_desc(desc)) {
    return false;
  }
  // ecma 8.12.6 step 3.
  return true;
}

bool __jsobj_internal_DeleteByValue(__jsobject *o, uint32 index, bool mark_as_deleted = false,
                                    bool throw_p = false) {
  __jsobj_helper_convert_to_generic(o);
#ifdef USE_PROP_MAP
  if (o->prop_index_map == NULL && o->object_class == JSSTRING && o->shared.prim_string) {
    // lazy initailize properties for string
    __jsobj_initprop_fromString(o, o->shared.prim_string);
  }
  if (!o->prop_index_map)
    return true;
  std::map<uint32_t, __jsprop *>::iterator it, prev;
  it = o->prop_index_map->find(index);
  if (it != o->prop_index_map->end()) {
    __jsprop *prop = it->second;
    __jsprop_desc desc = prop->desc;
    if (__has_and_configurable(desc)) {
      if (mark_as_deleted) {
        prop->desc = __undefined_desc();
      } else {
        if (it != o->prop_index_map->begin()) {
          prop->prev->next = prop->next;
          if (prop->next) // not the last one
            prop->next->prev = prop->prev;
          else // last one
            o->prop_list->prev = prop->prev;
          o->prop_index_map->erase(it);
        } else {
          // very first one
          o->prop_list = prop->next;
          if (o->prop_list) {
            o->prop_list->prev = prop->prev;
          }
          o->prop_index_map->erase(it);
        }
        memory_manager->ManageProp(prop, RECALL);
      }
      return true;
    } else if (throw_p) {
      MAPLE_JS_TYPEERROR_EXCEPTION();
    }
    return false;
  }
  return true;
#else
  __jsprop **prop_p = &o->prop_list;
  while (*prop_p) {
    __jsprop *prop = *prop_p;
    if (prop->isIndex && prop->n.index == index) {
      __jsprop_desc desc = prop->desc;
      if (__has_and_configurable(desc)) {
        if (mark_as_deleted) {
          prop->desc = __undefined_desc();
        } else {
          *prop_p = prop->next;
          memory_manager->ManageProp(prop, RECALL);
        }
        return true;
      } else if (throw_p) {
        MAPLE_JS_TYPEERROR_EXCEPTION();
      }
      return false;
    }
    prop_p = &(prop->next);
  }
  return true;
#endif
}

// ecma 8.12.7
// 1. Let desc be the result of calling the [[GetOwnProperty]] internal method of O with property name P.
// 2. If desc is undefined, then return true.
// 3. If desc.[[Configurable]] is true, then
// a. Remove the own property with name P from O.
// b. Return true.
// 4. Else if Throw, then throw a TypeError exception.
// 5. Return false.
// Combine the step 1 2 3 5.
bool __jsobj_internal_Delete(__jsobject *o, __jsstring *p, bool mark_as_deleted, bool throw_p) {
  __jsobj_helper_convert_to_generic(o);
#ifdef USE_PROP_MAP
  bool property_created = false;
  for (;;) {
    if (o->prop_string_map) {
      std::map<__jsstring *, __jsprop *>::iterator it, prev;
      it = o->prop_string_map->find(p);
      __jsprop *prop = NULL;
      if (it != o->prop_string_map->end()) {
        prop = it->second;
      } else {
        std::wstring w_name = __jsstr_to_wstring(p);
        for (it = o->prop_string_map->begin(); it != o->prop_string_map->end(); ++it) {
          if (__jsstr_to_wstring(it->first) == w_name) {
            prop = it->second;
            break;
          }
        }
      }
      if (prop) {
        __jsprop_desc desc = prop->desc;
        if (__has_and_configurable(desc)) {
          if (mark_as_deleted) {
            prop->desc = __undefined_desc();
          } else {
            if (o->prop_list == prop) {
              // very first one
              o->prop_list = prop->next;
              if (o->prop_list)
                o->prop_list->prev = prop->prev;
            } else {
              prop->prev->next = prop->next;
              if (prop->next) {
                prop->next->prev = prop->prev;
              }
              else {
                o->prop_list->prev = prop->prev; // the last one
              }
            }
            o->prop_string_map->erase(it);
            memory_manager->ManageProp(prop, RECALL);
          }
          return true;
        } else if (throw_p) {
          MAPLE_JS_TYPEERROR_EXCEPTION();
        }
        return false;
      }
    }
    // may not have created yet, create the property
    if (!property_created) {
      __create_builtin_property(o, p);
      property_created = true;
    } else {
      break;
    }
  }
  return true;
#else
  __jsprop **prop_p = &o->prop_list;
  // skip the index
  while ((*prop_p) && (*prop_p)->isIndex)
    prop_p = &((*prop_p)->next);

  bool property_created = false;
  for (;;) {
    while (*prop_p) {
      __jsprop *prop = *prop_p;
      if (__is_property(prop, p)) {
        __jsprop_desc desc = prop->desc;
        if (__has_and_configurable(desc)) {
          if (mark_as_deleted) {
            prop->desc = __undefined_desc();
          } else {
            *prop_p = prop->next;
            memory_manager->ManageProp(prop, RECALL);
          }
          return true;
        } else if (throw_p) {
          MAPLE_JS_TYPEERROR_EXCEPTION();
        }
        return false;
      }
      prop_p = &(prop->next);
    }
    // may not have created yet, create the property
    if (!property_created) {
      __create_builtin_property(o, p);
      property_created = true;
    } else {
      break;
    }
  }
  return true;
#endif
}

// ecma 8.12.7
bool __jsobj_internal_Delete(__jsobject *o, __jsvalue *p, bool mark_as_deleted, bool throw_p) {
  __jsstring *name = __js_ToString(p);
  if (!__is_string(p)) {
    GCIncRf(name);
  }
  bool res = __jsobj_internal_Delete(o, name, mark_as_deleted, throw_p);
  if (!__is_string(p)) {
    GCDecRf(name);
  }
  return res;
}

// ecma 8.12.7
bool __jsobj_internal_Delete(__jsobject *o, uint32_t index, bool mark_as_deleted, bool throw_p) {
  return __jsobj_internal_DeleteByValue(o, index, mark_as_deleted, throw_p);
}

// ecma 8.12.8
__jsvalue __object_internal_DefaultValue(__jsobject *o, __jstype hint) {
  __jsvalue result1, result2;
  __jsbuiltin_string_id first, second;
  bool first_defined, second_defined;
  MAPLE_JS_ASSERT(hint == JSTYPE_STRING || hint == JSTYPE_NUMBER || hint == JSTYPE_UNDEFINED);
  if (hint == JSTYPE_STRING) {
    first = JSBUILTIN_STRING_TO_STRING_UL;
    second = JSBUILTIN_STRING_VALUE_OF_UL;
  } else {
    first = JSBUILTIN_STRING_VALUE_OF_UL;
    second = JSBUILTIN_STRING_TO_STRING_UL;
  }
  if ((first_defined = __jsobj_helper_GetAndCall(o, first, &result1)))
    if (__is_primitive(&result1)) {
      return result1;
    }
  if ((second_defined = __jsobj_helper_GetAndCall(o, second, &result2)))
    if (__is_primitive(&result2)) {
      return result2;
    }
  // MAPLE_JS_EXCEPTION(false && "TypeError");
  if ((first_defined && second_defined && !__is_none(&result1) && !__is_none(&result2))) {
    MAPLE_JS_TYPEERROR_EXCEPTION();
    return __undefined_value();
  } else {
    return __undefined_value();
  }
}

void __jsobj_helper_convert_to_generic(__jsobject *obj) {
  switch (obj->object_type) {
    case JSGENERIC:
      return;
    case JSREGULAR_OBJECT:
      obj->object_type = JSGENERIC;
      return;
    case JSREGULAR_ARRAY: {
      MAPLE_JS_ASSERT(obj->prop_list == NULL);
      __jsvalue *array_props = obj->shared.array_props;
      obj->object_type = (uint8_t)JSGENERIC;
      uint32_t length;
      if (__is_number(&array_props[0])) {
        length = (uint32_t)__jsval_to_number(&array_props[0]);
      } else {
        MAPLE_JS_ASSERT(__is_double(&array_props[0]));
        double d = __jsval_to_double(&array_props[0]);
        MAPLE_JS_ASSERT(d <= UINT32_MAX);
        length = (uint32_t)d;
      }
      __jsvalue *elems = &array_props[1];
      bool enable_extensible = false;
      if (!obj->extensible) {
        obj->extensible = true;
        enable_extensible = true;
      }
      __jsobj_helper_add_value_property(obj, JSBUILTIN_STRING_LENGTH, &array_props[0], JSPROP_DESC_HAS_VWUEUC);
      // length <= MAX_ARRAYINDEX_NUM_INTERNAL is stored in array_props
      length = length > ARRAY_MAXINDEXNUM_INTERNAL ? ARRAY_MAXINDEXNUM_INTERNAL : length;
      for (uint32_t i = 0; i < length; i++) {
        if (!__is_none(&elems[i])) {
          __set_generic_elem(obj, i, &elems[i]);
        }
      }
      if (enable_extensible) {
        obj->extensible = false;
      }
      memory_manager->RecallArray_props(array_props);
      return;
    }
    default:
      return;
  }
}

static void __jsobj_helper_desc_attr_copy(__jsprop_desc *to, __jsprop_desc from, bool set_default) {
  // if (__jsprop_desc_IsGenericDescriptor(from) ||
  //     __jsprop_desc_IsDataDescriptor(from)) {
  if (!__jsprop_desc_IsAccessorDescriptor(from)) {
    if (__has_writable(from) || set_default) {
      __set_writable(to, __has_and_writable(from));
    }
  }
  if (__has_enumerable(from) || set_default) {
    __set_enumerable(to, __has_and_enumerable(from));
  }
  if (__has_configurable(from) || set_default) {
    __set_configurable(to, __has_and_configurable(from));
  }
}

static void __jsobj_helper_set_data_desc_gc(__jsprop_desc *to, __jsprop_desc from, bool set_default) {
  if (__has_value(from)) {
    __jsvalue v = __get_value(from);
    __set_value_gc(to, &v);
  } else {
    if (set_default) {
      __jsvalue v = __undefined_value();
      __set_value_gc(to, &v);
    }
  }
  __jsobj_helper_desc_attr_copy(to, from, set_default);
}

static void __jsobj_helper_set_access_desc_gc(__jsprop_desc *to, __jsprop_desc from, bool set_default) {
  if (__has_set(from)) {
    __set_set_gc(to, __get_set(from));
  } else {
    if (set_default) {
      __set_set_gc(to, NULL);
    }
  }

  if (__has_get(from)) {
    __set_get_gc(to, __get_get(from));
  } else if (set_default) {
    __set_get_gc(to, NULL);
  }

  __jsobj_helper_desc_attr_copy(to, from, set_default);
}

uint32_t __jsobj_helper_get_length(__jsobject *obj) {
  if (obj->object_type == JSREGULAR_ARRAY) {
    return (uint32_t)__jsval_to_number(&obj->shared.array_props[0]);
  }
  __jsvalue length;
  if (__jsobj_helper_HasPropertyAndGet(obj, JSBUILTIN_STRING_LENGTH, &length)) {
    return __js_ToUint32(&length);
  }
  return 0;
}

// get length property and convert to u64 value
uint64_t __jsobj_helper_get_lengthsize(__jsobject *obj) {
  __jsvalue length = __jsobj_helper_get_length_value(obj);
  if (!__is_undefined(&length)) {
    return __js_toLength(&length);
  }
  return 0;
}

__jsvalue __jsobj_helper_get_length_value(__jsobject *obj) {
  if (obj->object_type == JSREGULAR_ARRAY) {
    return obj->shared.array_props[0];
  }
  __jsvalue length;
  if (__jsobj_helper_HasPropertyAndGet(obj, JSBUILTIN_STRING_LENGTH, &length)) {
    return length;
  }
  return __undefined_value();
}

void __jsobj_helper_set_length(__jsobject *obj, uint64_t length, bool throw_p) {
  __jsvalue v = (length <= INT32_MAX) ? __number_value(length) : __double_value(length);
  __jsobj_internal_Put(obj, __jsstr_get_builtin(JSBUILTIN_STRING_LENGTH), &v, throw_p);
}


// TODO: merge with __jsobj_internal_DefineOwnProperty
void __jsobj_internal_DefineOwnPropertyByValue(__jsobject *o, uint32_t index, __jsprop_desc desc, bool throw_p) {
  __jsobj_helper_convert_to_generic(o);
  // ecma 8.12.9 step 1
  __jsprop_desc current = __jsobj_internal_GetOwnPropertyByValue(o, index);
  // ecma 8.12.9 step 2
  bool extensible = o->extensible;
  // ecma 8.12.9 step 3
  if (__is_undefined_desc(current) && !extensible) {
    MAPLE_JS_TYPEERROR_EXCEPTION();
  }
  // ecma 8.12.9 step 4
  if (__is_undefined_desc(current) && extensible) {
    __jsprop *prop = __jsobj_helper_create_propertyByValue(o, index);
    // ecma 8.12.9 step 4.a
    // if (__jsprop_desc_IsGenericDescriptor(desc) ||
    // __jsprop_desc_IsDataDescriptor(desc)) {
    if (!__jsprop_desc_IsAccessorDescriptor(desc)) {
      __jsobj_helper_set_data_desc_gc(&prop->desc, desc, true);
    } else { /* ecma 8.12.9 step 4.b */
      MAPLE_JS_ASSERT(__jsprop_desc_IsAccessorDescriptor(desc));
      __jsobj_helper_set_access_desc_gc(&prop->desc, desc, true);
    }
    return;
  }
  // ecma 8.12.9 step 5
  // if every field in Desc is absent.
  // This branch just do a fast check to see if the next steps are necessary.
  // But it is very cold, so disbale this check both for performance and
  // code-size.
  //
  // ecma 8.12.9 step 6
  // if every field in Desc also occurs in current and the value of every
  // field in Desc is the same value as the corresponding field in current.
  // This branch just do a fast check to see if the next steps are necessary.
  // But it is very cold, so disbale this check both for performance and
  // code-size.
  //
  // ecma 8.12.9 step 7
  if (__has_and_unconfigurable(current)) {
    if (__has_and_configurable(desc)) {
      MAPLE_JS_TYPEERROR_EXCEPTION();
    }
    if (__has_enumerable(desc)) {
      if (!__has_enumerable(current)) {
        MAPLE_JS_TYPEERROR_EXCEPTION();
      }
      if (__enumerable(current) != __enumerable(desc)) {
        MAPLE_JS_TYPEERROR_EXCEPTION();
      }
    }
  }
  // ecma 8.12.9 step 8
  if (__jsprop_desc_IsGenericDescriptor(desc)) {
    // Do nothing.
  } else {
    bool current_data_p = __jsprop_desc_IsDataDescriptor(current);
    bool desc_data_p = __jsprop_desc_IsDataDescriptor(desc);
    /* ecma 8.12.9 step 9 */
    if (current_data_p != desc_data_p) {
      // ecma 8.12.9 step 9.a
      if (__has_and_unconfigurable(current)) {
        MAPLE_JS_TYPEERROR_EXCEPTION();
      }
      // ecma 8.12.9 step 9.b
      // This implementation is slow but for smaller codesize.
      // Note this branch is cold.
      bool configurable = __has_and_configurable(current);
      bool enumerable = __has_and_enumerable(current);
      // Step1: set the rest of the property‘s attributes to their default values
      __jsprop_desc empty = __new_empty_desc();
      __jsobj_helper_set_data_desc_gc(&current, empty, true);
      __jsobj_helper_set_access_desc_gc(&current, empty, true);
      // Step2: Preserve the existing values of the converted property‘s [[Configurable]] and [[Enumerable]] attributes
      __set_configurable(&current, configurable);
      __set_enumerable(&current, enumerable);
      // Step3: Convert the property named P of object O from a data property to an accessor property.
      if (current_data_p) {
        current.s.fields &= ~JSPROP_HAS_VALUE;
        current.s.attr_writable = 0;
      }
      // Step4: Convert the property named P of object O from an accessor property to a data property.
      else {
        current.s.fields &= ~JSPROP_HAS_SET;
        current.s.fields &= ~JSPROP_HAS_GET;
      }
    } else /* ecma 8.12.9 step 10 */ if (current_data_p && desc_data_p) {
      // ecma 8.12.9 step 10.a
      if (__has_and_unconfigurable(current)) {
        // ecma 8.12.9 step 10.a.i
        if (__has_and_unwritable(current) && __has_and_writable(desc)) {
          MAPLE_JS_TYPEERROR_EXCEPTION();
        }
        // ecma 8.12.9 step 10.a.ii
        if (__has_and_unwritable(current))
          if (__has_value(desc) &&
              !__js_SameValue(&desc.named_data_property.value, &current.named_data_property.value)) {
            MAPLE_JS_TYPEERROR_EXCEPTION();
          }
      }
    } else { /* ecma 8.12.9 step 11 */
      MAPLE_JS_ASSERT(__jsprop_desc_IsAccessorDescriptor(current) && __jsprop_desc_IsAccessorDescriptor(desc));
      // ecma 8.12.9 step 11.a
      if (__has_and_unconfigurable(current)) {
        // ecma 8.12.9 step 11.a.i
        if (__has_set(desc) && (!__has_set(current) || __get_set(desc) != __get_set(current))) {
          MAPLE_JS_TYPEERROR_EXCEPTION();
        }
        // ecma 8.12.9 step 11.a.ii
        if (__has_get(desc) && (!__has_get(current) || __get_get(desc) != __get_get(current))) {
          MAPLE_JS_TYPEERROR_EXCEPTION();
        }
      }
    }
  }
  /* ecma 8.12.9 step 12 */
  __jsobj_helper_set_data_desc_gc(&current, desc, false);
  __jsobj_helper_set_access_desc_gc(&current, desc, false);
  __jsprop *prop = __jsobj_helper_get_propertyByValue(o, index);
  MIR_ASSERT(prop);
  prop->desc = current;
}
// ecma 8.12.9
void __jsobj_internal_DefineOwnProperty(__jsobject *o, __jsstring *p, __jsprop_desc desc, bool throw_p) {
  __jsobj_helper_convert_to_generic(o);
  // ecma 8.12.9 step 1
  __jsprop_desc current = __jsobj_internal_GetOwnProperty(o, p);
  // ecma 8.12.9 step 2
  bool extensible = o->extensible;
  bool isNum;
  uint32 idxNum = __jsstr_is_numidx(p, isNum);
  // ecma 8.12.9 step 3
  if (__is_undefined_desc(current) && !extensible) {
    MAPLE_JS_TYPEERROR_EXCEPTION();
  }
  // ecma 8.12.9 step 4
  if (__is_undefined_desc(current) && extensible) {
    __jsprop *prop;
    if (isNum) {
      prop = __jsobj_helper_create_propertyByValue(o, idxNum);
    } else {
      prop = __jsobj_helper_create_property(o, p);
    }
    // ecma 8.12.9 step 4.a
    // if (__jsprop_desc_IsGenericDescriptor(desc) ||
    // __jsprop_desc_IsDataDescriptor(desc)) {
    if (!__jsprop_desc_IsAccessorDescriptor(desc)) {
      __jsobj_helper_set_data_desc_gc(&prop->desc, desc, true);
    } else { /* ecma 8.12.9 step 4.b */
      MAPLE_JS_ASSERT(__jsprop_desc_IsAccessorDescriptor(desc));
      __jsobj_helper_set_access_desc_gc(&prop->desc, desc, true);
    }
    return;
  }
  // ecma 8.12.9 step 5
  // if every field in Desc is absent.
  // This branch just do a fast check to see if the next steps are necessary.
  // But it is very cold, so disbale this check both for performance and
  // code-size.
  //
  // ecma 8.12.9 step 6
  // if every field in Desc also occurs in current and the value of every
  // field in Desc is the same value as the corresponding field in current.
  // This branch just do a fast check to see if the next steps are necessary.
  // But it is very cold, so disbale this check both for performance and
  // code-size.
  //
  // ecma 8.12.9 step 7
  if (__has_and_unconfigurable(current)) {
    if (__has_and_configurable(desc)) {
      MAPLE_JS_TYPEERROR_EXCEPTION();
    }
    if (__has_enumerable(desc)) {
      if (!__has_enumerable(current)) {
        MAPLE_JS_TYPEERROR_EXCEPTION();
      }
      if (__enumerable(current) != __enumerable(desc)) {
        MAPLE_JS_TYPEERROR_EXCEPTION();
      }
    }
  }
  // ecma 8.12.9 step 8
  if (__jsprop_desc_IsGenericDescriptor(desc)) {
    // Do nothing.
  } else {
    bool current_data_p = __jsprop_desc_IsDataDescriptor(current);
    bool desc_data_p = __jsprop_desc_IsDataDescriptor(desc);
    /* ecma 8.12.9 step 9 */
    if (current_data_p != desc_data_p) {
      // ecma 8.12.9 step 9.a
      if (__has_and_unconfigurable(current)) {
        MAPLE_JS_TYPEERROR_EXCEPTION();
      }
      // ecma 8.12.9 step 9.b
      // This implementation is slow but for smaller codesize.
      // Note this branch is cold.
      bool configurable = __has_and_configurable(current);
      bool enumerable = __has_and_enumerable(current);
      // Step1: set the rest of the property‘s attributes to their default values
      __jsprop_desc empty = __new_empty_desc();
      __jsobj_helper_set_data_desc_gc(&current, empty, true);
      __jsobj_helper_set_access_desc_gc(&current, empty, true);
      // Step2: Preserve the existing values of the converted property‘s [[Configurable]] and [[Enumerable]] attributes
      __set_configurable(&current, configurable);
      __set_enumerable(&current, enumerable);
      // Step3: Convert the property named P of object O from a data property to an accessor property.
      if (current_data_p) {
        current.s.fields &= ~JSPROP_HAS_VALUE;
        current.s.attr_writable = 0;
      }
      // Step4: Convert the property named P of object O from an accessor property to a data property.
      else {
        current.s.fields &= ~JSPROP_HAS_SET;
        current.s.fields &= ~JSPROP_HAS_GET;
      }
    } else /* ecma 8.12.9 step 10 */ if (current_data_p && desc_data_p) {
      // ecma 8.12.9 step 10.a
      if (__has_and_unconfigurable(current)) {
        // ecma 8.12.9 step 10.a.i
        if (__has_and_unwritable(current) && __has_and_writable(desc)) {
          MAPLE_JS_TYPEERROR_EXCEPTION();
        }
        // ecma 8.12.9 step 10.a.ii
        if (__has_and_unwritable(current))
          if (__has_value(desc) &&
              !__js_SameValue(&desc.named_data_property.value, &current.named_data_property.value)) {
            MAPLE_JS_TYPEERROR_EXCEPTION();
          }
      }
    } else { /* ecma 8.12.9 step 11 */
      MAPLE_JS_ASSERT(__jsprop_desc_IsAccessorDescriptor(current) && __jsprop_desc_IsAccessorDescriptor(desc));
      // ecma 8.12.9 step 11.a
      if (__has_and_unconfigurable(current)) {
        // ecma 8.12.9 step 11.a.i
        if (__has_set(desc) && (!__has_set(current) || __get_set(desc) != __get_set(current))) {
          MAPLE_JS_TYPEERROR_EXCEPTION();
        }
        // ecma 8.12.9 step 11.a.ii
        if (__has_get(desc) && (!__has_get(current) || __get_get(desc) != __get_get(current))) {
          MAPLE_JS_TYPEERROR_EXCEPTION();
        }
      }
    }
  }
  /* ecma 8.12.9 step 12 */
  __jsobj_helper_set_data_desc_gc(&current, desc, false);
  __jsobj_helper_set_access_desc_gc(&current, desc, false);
  __jsprop *prop = isNum ? __jsobj_helper_get_propertyByValue(o, idxNum) :
                           __jsobj_helper_get_property(o, p);
  MIR_ASSERT(prop);
  prop->desc = current;
  return;
}

// ecma 8.12.9
void __jsobj_internal_DefineOwnProperty(__jsobject *o, __jsbuiltin_string_id id, __jsprop_desc desc, bool throw_p) {
  __jsobj_internal_DefineOwnProperty(o, __jsstr_get_builtin(id), desc, throw_p);
}

// ecma 8.12.9
void __jsobj_internal_DefineOwnProperty(__jsobject *o, __jsvalue *p, __jsprop_desc desc, bool throw_p) {
  __jsstring *name = __js_ToString(p);
  if (!__is_string(p)) {
    GCIncRf(name);
  }
  __jsobj_internal_DefineOwnProperty(o, name, desc, throw_p);
  if (!__is_string(p)) {
    GCDecRf(name);
  }
}

// Object.getPrototypeOf : ecma 15.2.3.2, update to 20.1.2.12
__jsvalue __jsobj_getPrototypeOf(__jsvalue *this_object, __jsvalue *o) {
  __jsobject *obj = __js_ToObject(o);
  __jsobject *proto = __jsobj_get_prototype(obj);
  if (proto) {
    return __object_value(proto);
  } else {
    return __null_value();
  }
}

// ecma 15.2.3.3
__jsvalue __jsobj_getOwnPropertyDescriptor(__jsvalue *this_object, __jsvalue *o, __jsvalue *p) {
  // ecma 20.1.2.8 step 1
  __jsobject *obj = __js_ToObject(o);
  // ecma 15.2.3.3 step 3
  __jsprop_desc desc = __jsobj_internal_GetOwnProperty(obj, p);
  // ecma 15.2.3.3 step 4
  return __jsprop_desc_FromPropertyDescriptor(desc);
}

// Walk function for __jsobj_getOwnPropertyNames.
// See ecma 15.2.3.4 for details.
bool __jsobj_walk_getOwnPropertyNames(__jsprop *prop, uint32_t n, void *result) {
  __jsobject *arr = (__jsobject *)result;
  __jsvalue name;
  if (prop->isIndex) {
    // __set_number(&name, prop->n.index);
    __set_string(&name, __js_NumberToString(prop->n.index));
  } else {
    name = __string_value(prop->n.name);
  }
  __set_regular_elem(arr->shared.array_props, n, &name);
  return true;
}

// Object.getOwnPropertyNames : ecma 15.2.3.4, update to 20.1.2.10
__jsvalue __jsobj_getOwnPropertyNames(__jsvalue *this_object, __jsvalue *o) {
  // ecma 20.1.2.11.1 step 1
  __jsobject *obj = __js_ToObject(o);
  __create_builtin_property(obj, NULL);
  __jsobj_helper_convert_to_generic(obj);
  uint32_t n = __jsobj_helper_for_each_property(obj, NULL, NULL);
  __jsobject *arr = __js_new_arr_internal(n);
  __jsobj_helper_for_each_property(obj, __jsobj_walk_getOwnPropertyNames, (void *)arr);
  return __object_value(arr);
}

// ecma 15.2.3.5
__jsvalue __jsobj_create(__jsvalue *this_object, __jsvalue *o, __jsvalue *properties) {
  // ecma 15.2.3.5 step 1.
  if (!__is_js_object(o) && !__is_null(o)) {
    MAPLE_JS_TYPEERROR_EXCEPTION();
  }
  // ecma 15.2.3.5 step 2.
  __jsobject *obj = __js_new_obj_obj_0();
  __jsvalue object = __object_value(obj);
  // ecma 15.2.3.5 step 3.
  if (!__is_null(o)) {
    __jsobj_set_prototype(obj, __jsval_to_object(o));
  } else {
    __jsobj_set_prototype(obj, NULL);
  }
  // ecma 15.2.3.5 step 4.
  if (properties && !__is_undefined(properties)) {
    __jsobj_defineProperties(this_object, &object, properties);
  }
  // ecma 15.2.3.5 step 5.
  return object;
}

// ecma 15.2.3.6
__jsvalue __jsobj_defineProperty(__jsvalue *this_object, __jsvalue *o, __jsvalue *p, __jsvalue *attributes) {
  // ecma 15.2.3.6 step 1.
  if (!__is_js_object(o)) {
    MAPLE_JS_TYPEERROR_EXCEPTION();
  }
  // ecma 15.2.3.6 step 3.
  __jsprop_desc desc = __jsprop_desc_ToPropertyDescriptor(attributes);
  // ecma 15.2.3.6 step 4.
  __jsobject *obj = __jsval_to_object(o);
  __jsobj_helper_DefineOwnProperty(obj, p, desc, true);
  // ecma 15.2.3.6 step 5.
  return *o;
}

// Walk function for __jsobj_defineProperties.
// See ecma 15.2.3.7 for details.
bool __jsobj_walk_defineProperties(__jsvalue *this_val, __jsprop *prop, void *result) {
  __jsprop_desc desc = prop->desc;
  __jsobject *obj = (__jsobject *)result;
  if (__has_and_enumerable(desc)) {
    if(__has_value(desc)) {
     __jsvalue desc_obj = __get_value(desc);
     desc = __jsprop_desc_ToPropertyDescriptor(&desc_obj);
    } else {
      // ecma 15.2.3.7.5.a
      __jsvalue desc_obj;
      if (!__has_get(desc)) {
        desc_obj = __undefined_value();
      } else {
        __jsobject* getter = __get_get(desc);
        desc_obj = getter ? __jsfun_internal_call(getter, this_val, NULL, 0) : __undefined_value();
      }
      desc = __jsprop_desc_ToPropertyDescriptor(&desc_obj);
    }
    if (prop->isIndex) {
      __jsobj_helper_DefineOwnPropertyByValue(obj, prop->n.index, desc, true);
    } else {
      __jsobj_helper_DefineOwnProperty(obj, prop->n.name, desc, true);
    }
  }
  return true;
}

// ecma 15.2.3.7
__jsvalue __jsobj_defineProperties(__jsvalue *this_object, __jsvalue *o, __jsvalue *properties) {
  // ecma 15.2.3.7 step 1.
  if (!__is_js_object(o)) {
    MAPLE_JS_TYPEERROR_EXCEPTION();
  }
  __jsobject *obj = __jsval_to_object(o);
  // ecma 15.2.3.7 step 2.
  __jsobject *props = __is_js_object(properties) ? __jsval_to_object(properties) : __js_ToObject(properties);
  GCIncRf(props);
  // ecma 15.2.3.7 step 3, 4, 5, 6
  __jsobj_helper_convert_to_generic(props);
  {
    __jsprop *p = props->prop_list;
    while (p) {
      __jsobj_walk_defineProperties(properties, p, obj);
      p = p->next;
    }
  }

  GCDecRf(props);
  return *o;
}

// Walk function for __jsobj_seal.
// See ecma 15.2.3.8 for details.
bool __jsobj_walk_seal(__jsprop *prop, uint32_t n, void *result) {
  if (__has_and_configurable(prop->desc)) {
    __set_configurable(&prop->desc, false);
  }
  return true;
}

// ecma 15.2.3.8
__jsvalue __jsobj_seal(__jsvalue *this_object, __jsvalue *o) {
  // ecma 20.1.2.20 step 1.
  if (!__is_js_object(o)) {
    return *o;
  }
  // ecma 15.2.3.8 step 2.
  __jsobject *obj = __jsval_to_object(o);
  __jsobj_helper_convert_to_generic(obj);
  __jsobj_helper_for_each_property(obj, __jsobj_walk_seal, NULL);
  // ecma 15.2.3.8 step 3.
  obj->extensible = false;
  // ecma 15.2.3.8 step 4.
  return *o;
}

// Walk function for __jsobj_freeze.
// See ecma 15.2.3.9 for details.
bool __jsobj_walk_freeze(__jsprop *prop, uint32_t n, void *result) {
  __jsprop_desc desc = prop->desc;
  if (__jsprop_desc_IsDataDescriptor(desc)) {
    if (__has_and_writable(desc)) {
      __set_writable(&prop->desc, false);
    }
  }
  if (__has_and_configurable(desc)) {
    __set_configurable(&prop->desc, false);
  }
  return true;
}

// ecma 15.2.3.9, update to 20.1.2.6
__jsvalue __jsobj_freeze(__jsvalue *this_object, __jsvalue *o) {
  // ecma 20.1.2.6 step 1.
  if (!__is_js_object(o)) {
    return *o;
  }
  // ecma 15.2.3.9 step 2.
  __jsobject *obj = __jsval_to_object(o);
  __jsobj_helper_convert_to_generic(obj);
  __jsobj_helper_for_each_property(obj, __jsobj_walk_freeze, NULL);
  // ecma 15.2.3.9 step 3.
  obj->extensible = false;
  // ecma 15.2.3.8 step 4.
  return *o;
}

// ecma 15.2.3.10
__jsvalue __jsobj_preventExtensions(__jsvalue *this_object, __jsvalue *o) {
  // ecma 15.2.3.10 step 1.
  if (!__is_js_object(o)) {
    // JSB3320, JSB3329, JSB3338, JSB3339, JSB3343
    if(__is_number(o) || __is_boolean(o) || __is_string(o) || __is_undefined(o) || __is_null(o))
      return *o;
    MAPLE_JS_TYPEERROR_EXCEPTION();
  }
  // ecma 15.2.3.10 step 2.
  __jsobject *obj = __jsval_to_object(o);
  __jsobj_helper_convert_to_generic(obj);
  obj->extensible = false;
  // ecma 15.2.3.10 step 3.
  return *o;
}

// Walk function for __jsobj_isSealed.
// See ecma 15.2.3.11 for details.
bool __jsobj_walk_isSealed(__jsprop *prop, uint32_t n, void *result) {
  __jsprop_desc desc = prop->desc;
  if (__has_and_configurable(desc)) {
    *(bool *)result = false;
    return false;
  }
  return true;
}

// ecma 15.2.3.11 and 20.1.2.16
__jsvalue __jsobj_isSealed(__jsvalue *this_object, __jsvalue *o) {
  // ecma 20.1.2.16 step 1
  if (!__is_js_object(o)) {
    return __boolean_value(true);
  }
  // ecma 15.2.3.11 step 2.
  __jsobject *obj = __jsval_to_object(o);
  __jsobj_helper_convert_to_generic(obj);
  bool result = true;
  __jsobj_helper_for_each_property(obj, __jsobj_walk_isSealed, &result);
  if (result == false) {
    return __boolean_value(false);
  }
  // ecma 15.2.3.11 step 3.
  if (!obj->extensible) {
    return __boolean_value(true);
  }
  // ecma 15.2.3.11 step 4.
  return __boolean_value(false);
}

// Walk function for __jsobj_isFrozen.
// See ecma 15.2.3.12 for details.
bool __jsobj_walk_isFrozen(__jsprop *prop, uint32_t n, void *result) {
  __jsprop_desc desc = prop->desc;
  if (__jsprop_desc_IsDataDescriptor(desc)) {
    if (__has_and_writable(desc)) {
      *(bool *)result = false;
      return false;
    }
  }
  if (__has_and_configurable(desc)) {
    *(bool *)result = false;
    return false;
  }
  return true;
}

// Object.isFrozen: update to 20.1.2.15
__jsvalue __jsobj_isFrozen(__jsvalue *this_object, __jsvalue *o) {
  // ecma 20.1.2.15  step 1.
  if (!__is_js_object(o)) {
    return __boolean_value(true);
  }
  // ecma 15.2.3.12 step 2.
  __jsobject *obj = __jsval_to_object(o);
  __jsobj_helper_convert_to_generic(obj);
  bool result = true;
  __jsobj_helper_for_each_property(obj, __jsobj_walk_isFrozen, &result);
  if (result == false) {
    return __boolean_value(false);
  }
  // ecma 15.2.3.12 step 3.
  if (!obj->extensible) {
    return __boolean_value(true);
  }
  // ecma 15.2.3.12 step 4.
  return __boolean_value(false);
}

// ecma 15.2.3.13
__jsvalue __jsobj_isExtensible(__jsvalue *this_object, __jsvalue *o) {
  // ecma 20.1.2.14 step 1.
  if (!__is_js_object(o)) {
    return __boolean_value(false);
  }
  // ecma 15.2.3.12 step 2.
  return __boolean_value(__jsval_to_object(o)->extensible);
}

// Walk function for __jsobj_keys.
// See ecma 15.2.3.14 for details.
bool __jsobj_walk_keys_condition(__jsprop *prop) {
  return __has_and_enumerable(prop->desc);
}

bool __jsobj_walk_keys(__jsprop *prop, uint32_t n, void *result) {
  __jsobject *arr = (__jsobject *)result;
  MAPLE_JS_ASSERT(__has_and_enumerable(prop->desc));
  __jsvalue name;
  if (prop->isIndex) {
    // __set_number(&name, prop->n.index);
    __set_string(&name, __js_NumberToString(prop->n.index));
  }
  else {
    name = __string_value(prop->n.name);
  }
  __set_regular_elem(arr->shared.array_props, n, &name);
  return true;
}

// ecma 15.2.3.14, update to 20.1.2.17
__jsvalue __jsobj_keys(__jsvalue *this_object, __jsvalue *o) {
  // ecma 20.1.2.17 step 1.
  __jsobject *obj = __js_ToObject(o);
  __jsobj_helper_convert_to_generic(obj);
  uint32_t n = __jsobj_helper_for_each_property(obj, NULL, NULL, __jsobj_walk_keys_condition);
  // ecma 15.2.3.14 step 3.
  __jsobject *arr = __js_new_arr_internal(n);
  __jsobj_helper_for_each_property(obj, __jsobj_walk_keys, (void *)arr, __jsobj_walk_keys_condition);
  // ecma 15.2.3.14 step 6.
  return __object_value(arr);
}

// Helper function.
// Only called once by __jsobj_pt_toString, inline for performance.
static inline __jsstring *__jsobj_helper_get_object_class_name(__jsobj_class c) {
  MAPLE_JS_ASSERT(c < JSOBJ_CLASS_LAST && "internal error.");
  MAPLE_JS_ASSERT(JSBUILTIN_STRING_LAST < 255 && "internal error.");
  static const char class_name[JSOBJ_CLASS_LAST] = {
    JSBUILTIN_STRING_GLOBAL,
    JSBUILTIN_STRING_OBJECT_UL,
    JSBUILTIN_STRING_FUNCTION_UL,
    JSBUILTIN_STRING_ARRAY_UL,
    JSBUILTIN_STRING_STRING_UL,
    JSBUILTIN_STRING_BOOLEAN_UL,
    JSBUILTIN_STRING_NUMBER_UL,
    JSBUILTIN_STRING_MATH_UL,
    JSBUILTIN_STRING_DATE_UL,
    JSBUILTIN_STRING_REGEXP_UL,
    JSBUILTIN_STRING_JSON_U,
    JSBUILTIN_STRING_ERROR_UL,
    JSBUILTIN_STRING_ARGUMENTS_UL,
  };
  return __jsstr_get_builtin((__jsbuiltin_string_id)class_name[c]);
}

// ecma 15.2.4.2
__jsvalue __jsobj_pt_toString(__jsvalue *this_object) {
  __jsstring *str;
  // ecma 15.2.4.2 step 1.
  if (__is_undefined(this_object)) {
    str = __jsstr_new_from_char("[object Undefined]");
  } else if (__is_null(this_object)) {
    // ecma 15.2.4.2 step 2.
    str = __jsstr_new_from_char("[object Null]");
  } else {
    // ecma 15.2.4.2 step 3.
    __jsobject *obj = __is_js_object(this_object) ? __jsval_to_object(this_object) : __js_ToObject(this_object);
    // ecma 15.2.4.2 step 4.
    __jsobj_class c = (__jsobj_class)obj->object_class;
    // ecma 15.2.4.2 step 5.
    __jsstring *prefix = __jsstr_new_from_char("[object ");
    __jsstring *class_name = __jsobj_helper_get_object_class_name(c);
    __jsstring *postfix = __jsstr_new_from_char("]");
    str = __jsstr_concat_3(prefix, class_name, postfix);
    // Free local strings.
    memory_manager->RecallString(prefix);
    memory_manager->RecallString(class_name);
    memory_manager->RecallString(postfix);
    if (!__is_js_object(this_object)) {
      memory_manager->ManageObject(obj, RECALL);
    }
  }
  return __string_value(str);
}

// ecma 15.2.4.3
__jsvalue __jsobj_pt_toLocaleString(__jsvalue *this_object) {
  // ecma 15.2.4.3 step 1.
  __jsobject *obj = __jsval_to_object(this_object);
  __jsvalue o = __object_value(obj);
  // ecma 15.2.4.3 step 2.
  __jsvalue to_string = __jsobj_internal_Get(obj, JSBUILTIN_STRING_TO_STRING_UL);
  // ecma 15.2.4.3 step 3.
  if (!__js_IsCallable(&to_string)) {
    MAPLE_JS_TYPEERROR_EXCEPTION();
  }
  // ecma 15.2.4.3 step 4.
  return __jsfun_internal_call(__jsval_to_object(&to_string), &o, NULL, 0);
}

// Object.prototype.valueof(), ecma 15.2.4.4 update to 20.1.3.7
__jsvalue __jsobj_pt_valueOf(__jsvalue *this_object) {
  __jsobject *obj = __js_ToObject(this_object);
  __jsvalue o = __object_value(obj);
  // ecma 15.2.4.4 step 3.
  return o;
}

// ecma 15.2.4.5
__jsvalue __jsobj_pt_hasOwnProperty(__jsvalue *this_object, __jsvalue *v) {
  // ecma 20.1.3.2 step 2.
  __jsobject *obj = __js_ToObject(this_object);
  // ecma 15.2.4.5 step 3.
  __jsprop_desc desc = __jsobj_internal_GetOwnProperty(obj, v);
  // ecma 15.2.4.5 step 4.
  if (__is_undefined_desc(desc)) {
    return __boolean_value(false);
  }
  // ecma 15.2.4.5 step 5.
  return __boolean_value(true);
}

// ecma 15.2.4.6
__jsvalue __jsobj_pt_isPrototypeOf(__jsvalue *this_object, __jsvalue *v) {
  // ecma 15.2.4.6 step 1.
  if (!__is_js_object(v)) {
    return __boolean_value(false);
  }
  // ecma 20.1.3.3 step 2.
  __jsobject *obj = __js_ToObject(this_object);
  // ecma 15.2.4.6 step 3.
  __jsobject *pt = __jsobj_get_prototype(__jsval_to_object(v));
  while (true) {
    // ecma 15.2.4.6 step 3.a.
    // ecma 15.2.4.6 step 3.b.
    if (!pt) {
      return __boolean_value(false);
    }
    // ecma 15.2.4.6 step 3.c.
    if (obj == pt) {
      return __boolean_value(true);
    }
    pt = __jsobj_get_prototype(pt);
  }
}

// ecma 15.2.4.7
__jsvalue __jsobj_pt_propertyIsEnumerable(__jsvalue *this_object, __jsvalue *v) {
  // ecma 20.1.3.4 step 2.
  __jsobject *obj = __js_ToObject(this_object);
  // ecma 15.2.4.7 step 3.
  __jsprop_desc desc = __jsobj_internal_GetOwnProperty(obj, v);
  // ecma 15.2.4.7 step 4.
  if (__is_undefined_desc(desc)) {
    return __boolean_value(false);
  }
  // ecma 15.2.4.7 step 5.
  return __boolean_value(__enumerable(desc));
}

// 15.3.5 Properties of Function Instances
// if object is function obj created by bind and p is caller or arguments
// throw TypeError
static void __jsop_check_func_nameprop(__jsobject *obj, __jsstring *pname) {
  MIR_ASSERT(obj);
  if (obj->object_class == JSFUNCTION &&
      obj->shared.fun &&
      (((obj->shared.fun->attrs & 0xff) & JSFUNCPROP_BOUND) ||
       // 15.3.5.4 step 2
       // If P is "caller" and v is a strict mode Function object, throw a TypeError exception.
       ((obj->shared.fun->attrs & 0xff) & JSFUNCPROP_STRICT)) &&
      (__jsstr_equal_to_builtin(pname, JSBUILTIN_STRING_CALLER) ||
       __jsstr_equal_to_builtin(pname, JSBUILTIN_STRING_ARGUMENTS))) {
      // throw typeerror if function is created by bind
    MAPLE_JS_TYPEERROR_EXCEPTION();
  }
  return;
}

void __jsop_setprop(__jsvalue *o, __jsvalue *p, __jsvalue *v) {
  __jsobject *obj = __is_js_object(o) ? __jsval_to_object(o) : __js_ToObject(o);
  MIR_ASSERT(obj);
  if (obj->object_type == JSREGULAR_ARRAY) {
    MAPLE_JS_ASSERT(obj->prop_list == NULL);
    __jsvalue *array = obj->shared.array_props;
    uint32_t index = __jsarr_getIndex(p);
    uint32_t length = __jsobj_helper_get_length(obj);
    if (index != MAX_ARRAY_INDEX) {
      if (index <= ARRAY_MAXINDEXNUM_INTERNAL) {
        if (index >= length) {
          obj->shared.array_props = __jsarr_RegularRealloc(array, length, index + 1);
          array = obj->shared.array_props;
        }
        __set_regular_elem(array, index, v);
        return;
      } else {
        MAPLE_JS_ASSERT(index > ARRAY_MAXINDEXNUM_INTERNAL && index < MAX_ARRAY_INDEX);
        // update array length
        if (index >= length) {
          obj->shared.array_props = __jsarr_RegularRealloc(array, length, ARRAY_MAXINDEXNUM_INTERNAL);
          obj->shared.array_props[0] = (index+1 < INT32_MAX) ? __number_value(index+1) :
                                                               __double_value(index+1);
        }
      }
    } else {
      __jsstring *pstr = __js_ToString(p);
      bool isLength = __jsstr_equal_to_builtin(pstr, JSBUILTIN_STRING_LENGTH);
      if (!__is_string(p)) {
        memory_manager->RecallString(pstr);
      }
      if (isLength) {
        if (__js_ToNumber(v) >= 0) {
          uint32_t old_len = __jsobj_helper_get_length(obj);
          uint32_t new_len = __js_ToUint32(v);
          if (new_len != old_len) {
            obj->shared.array_props = __jsarr_RegularRealloc(array, old_len, new_len);
            return;
          } else {
            return;
          }
        }
        MAPLE_JS_EXCEPTION(false && "RangeError!");
      }
    }
  }
  // property index range is [0, 0xffffffff]
  if (__is_number(p) && __jsval_to_number(p) >= 0) {
    int32_t num = __jsval_to_number(p);
    __jsobj_internal_PutByValue(obj, num, v, false);
  } else {
    __jsstring *name = __js_ToString(p);
    __jsobj_internal_Put(obj, name, v, false);
    if (!__is_string(p)) {
      memory_manager->RecallString(name);
    }
  }
  if (!__is_js_object(o)) {
    memory_manager->ManageObject(obj, RECALL);
  }
}

void __jsop_initprop(__jsvalue *o, __jsvalue *p, __jsvalue *v) {
  MAPLE_JS_ASSERT(__is_js_object(o));
  __jsobject *obj = __jsval_to_object(o);
  if((__is_number(p) && __jsval_to_number(p) >= 0) ||
     (__is_double(p) && (__jsval_to_double(p) <= UINT32_MAX) && (__jsval_to_double(p) >= 0))) {
    uint32_t pIdx = __is_number(p) ? __jsval_to_number(p) : __jsval_to_double(p);
    __jsobj_helper_init_value_propertyByValue(obj, pIdx, v, JSPROP_DESC_HAS_VWEC);
  } else {
    __jsstring *pstr = __js_ToString(p);
    __jsobj_helper_init_value_property(obj, pstr, v, JSPROP_DESC_HAS_VWEC);
  }
  return;
}

void __jsop_initprop_by_name(__jsvalue *o, __jsstring *p, __jsvalue *v) {
  MAPLE_JS_ASSERT(__is_js_object(o));
  __jsobject *obj = __jsval_to_object(o);
  //MAPLE_JS_ASSERT((!__jsobj_helper_get_property(obj, p)) && "Can not init same property.");
  __jsobj_helper_init_value_property(obj, p, v, JSPROP_DESC_HAS_VWEC);
  return;
}

void __jsop_initprop_getter(__jsvalue *o, __jsvalue *p, __jsvalue *v) {
  MAPLE_JS_ASSERT(__is_js_object(o));
  MAPLE_JS_ASSERT(__is_js_object(v));
  __jsobject *obj = __jsval_to_object(o);
  __jsobj_helper_convert_to_generic(obj);
  __jsprop_desc desc = __new_get_desc(__jsval_to_object(v), JSPROP_DESC_HAS_GEC);
  if (p->tag == JSTYPE_NUMBER)
    __jsobj_internal_DefineOwnPropertyByValue(obj, p->s.u32, desc, false);
  else
    __jsobj_internal_DefineOwnProperty(obj, p, desc, false);
}

void __jsop_initprop_setter(__jsvalue *o, __jsvalue *p, __jsvalue *v) {
  MAPLE_JS_ASSERT(__is_js_object(o));
  MAPLE_JS_ASSERT(__is_js_object(v));
  __jsobject *obj = __jsval_to_object(o);
  __jsobj_helper_convert_to_generic(obj);
  __jsprop_desc desc = __new_set_desc(__jsval_to_object(v), JSPROP_DESC_HAS_SEC);
  if (p->tag == JSTYPE_NUMBER)
    __jsobj_internal_DefineOwnPropertyByValue(obj, p->s.u32, desc, false);
  else
    __jsobj_internal_DefineOwnProperty(obj, p, desc, false);
}

__jsvalue __jsop_getprop(__jsvalue *o, __jsvalue *p) {
  __jsobject *obj = __is_js_object(o) ? __jsval_to_object(o) : __js_ToObject(o);
  __jsvalue v = __jsobj_internal_Get(obj, p);
  if (!__is_js_object(o)) {
    memory_manager->ManageObject(obj, RECALL);
  }
  return v;
}

__jsvalue __jsop_delprop(__jsvalue *o, __jsvalue *p, bool throw_p) {
  bool res;
  __jsobject *obj = __is_js_object(o) ? __jsval_to_object(o) : __js_ToObject(o);

  if (__is_number(p) &&  __jsval_to_number(p) >= 0) {
    int32_t num = __jsval_to_number(p);
    res = __jsobj_internal_DeleteByValue(obj, num, throw_p);
  } else {
    bool isNum;
    __jsstring *name = __js_ToString(p);
    uint32_t idxNum = __jsstr_is_numidx(name, isNum);
    if (isNum) {
      res = __jsobj_internal_DeleteByValue(obj, idxNum, throw_p);
    } else {
      res = __jsobj_internal_Delete(obj, p, true/*mark_as_deleted*/, throw_p);
    }
  }
  if (!__is_js_object(o)) {
    memory_manager->ManageObject(obj, RECALL);
  }
  return __boolean_value(res);
}

__jsvalue __jsobj_getprop_by_scalar(__jsvalue *o, __jsstring *p) {
  __jsobject *obj = __js_ToObject(o);
  __jsprop_desc desc = __jsobj_internal_GetProperty(obj, p);
  __jsvalue ret = __jsobj_internal_get_by_desc(obj, desc, o);
  memory_manager->ManageObject(obj, RECALL);
  return ret;
}

__jsvalue __jsop_getprop_by_name(__jsvalue *o, __jsstring *p) {
  if (__is_undefined(o))
    return *o;
  if (__is_js_object(o)) {
    __jsobject *obj = __jsval_to_object(o);
    __jsop_check_func_nameprop(obj, p);
    return __jsobj_internal_Get(obj, p);
  } else if (__is_string(o)) {
    __jsobject *proto = __jsobj_get_or_create_builtin(JSBUILTIN_STRINGPROTOTYPE);
    __jsprop_desc desc = __jsobj_internal_GetProperty(proto, p);
    __jsvalue ret = __jsobj_internal_get_by_desc(proto, desc, o);
    return ret;
  } else {
    return __jsobj_getprop_by_scalar(o, p);
  }
}

static __jsprop * __jsop_get_prop_jsobject(__jsobject *obj, __jsstring *name) {
  bool isNum;
  uint32 idxNum = __jsstr_is_numidx(name, isNum);
  __jsprop *p = nullptr;
  if (isNum) {
    p = __jsobj_helper_get_propertyByValue(obj, idxNum, false);
  } else {
    p = __jsobj_helper_get_property(obj, name, false);
  }
  return p;
}

// make things faster
__jsvalue __jsop_get_this_prop_by_name(__jsvalue *o,  __jsstring *name) {
  __jsobject *obj = __is_js_object(o) ? __jsval_to_object(o) : __js_ToObject(o);
  __jsprop *p = __jsobj_helper_get_property(obj, name, false);
  if (p) {
    return __jsobj_internal_get_by_desc(obj, p->desc);
  } else {
    __jsvalue ret;
    ret.s.asbits = 0;
    ret.tag = JSTYPE_NONE;
    return ret;
  }
}

// make things faster
void __jsop_set_this_prop_by_name(__jsvalue *o, __jsstring *name, __jsvalue *v, bool noThrowTE) {
  __jsobject *obj = __is_js_object(o) ? __jsval_to_object(o) : __js_ToObject(o);
  // if name is builtin object like NaN, undefined.. JS will throw a TypeError
  if (__is_global_strict && __jsstr_throw_typeerror(name) && !noThrowTE) {
    MAPLE_JS_TYPEERROR_EXCEPTION();
  }
  // search the prop first
  __jsprop *p = __jsop_get_prop_jsobject(obj, name);
  if (p) {
    __set_value_gc(&p->desc, v);
  } else {
    __jsobj_helper_init_value_property(obj, name, v, JSPROP_DESC_HAS_VWEC);
  }
}

void __jsop_init_this_prop_by_name(__jsvalue *o, __jsstring *name) {
  __jsobject *obj = __is_js_object(o) ? __jsval_to_object(o) : __js_ToObject(o);
  // __jsprop_desc desc = __new_init_desc();
  __jsprop *p = __jsop_get_prop_jsobject(obj, name);
  if (p) {
    p->desc = __new_init_desc();
  } else {
    __jsprop *prop = __jsobj_helper_create_property(obj, name);
    __jsvalue jsUndf = __undefined_value();
    prop->desc = __new_value_desc(&jsUndf, JSPROP_DESC_HAS_UVWEUC);
  }
}

void __jsop_setprop_by_name(__jsvalue *o, __jsstring *p, __jsvalue *v, bool isStrict) {
  __jsobject *obj = __is_js_object(o) ? __jsval_to_object(o) : __js_ToObject(o);
  // check property is valid
  __jsop_check_func_nameprop(obj, p);
  if (isStrict) {
    if (!obj->extensible) {
      MAPLE_JS_TYPEERROR_EXCEPTION();
    } else {
      //__jsprop *pp = __jsop_get_prop_jsobject(obj, p);
      //if (__jsstr_throw_typeerror(p)) {
      //  MAPLE_JS_TYPEERROR_EXCEPTION();
      //} else {
        __jsstring *pname = __jsstr_get_builtin(JSBUILTIN_STRING_CALLEE);
        if(__jsstr_compare(pname, p) == 0) {
            MAPLE_JS_TYPEERROR_EXCEPTION();
        }
        __jsprop_desc desc =  __jsobj_internal_GetProperty(obj, p);
        if (!__is_undefined_desc(desc) && __has_and_unwritable(desc)) {
          MAPLE_JS_TYPEERROR_EXCEPTION();
        }

      //}
    }
  }
  __jsobj_internal_Put(obj, p, v, false, isStrict);
  if (!__is_js_object(o)) {
    memory_manager->ManageObject(obj, RECALL);
  }
  return;
}

__jsvalue __jsop_delprop_by_name(__jsvalue *o, __jsstring *nameIndex) {
  __jsobject *obj = __js_ToObject(o);
  bool res = __jsobj_internal_Delete(obj, nameIndex);
  if (!__is_js_object(o)) {
    memory_manager->ManageObject(obj, RECALL);
  }
  return __boolean_value(res);
}

// ecma 15.11.4.4
__jsvalue __jserror_pt_toString_base(__jsvalue *this_object, __jsbuiltin_string_id id) {
  if (!__is_js_object(this_object)) {
    MAPLE_JS_TYPEERROR_EXCEPTION();
  }
  __jsobject *obj = __jsval_to_object(this_object);
  __jsvalue name = __jsobj_internal_Get(obj, JSBUILTIN_STRING_NAME);
  if (__is_undefined(&name)) {
    name = __string_value(__jsstr_get_builtin(id));
  } else {
    name = __string_value(__js_ToString(&name));
  }
  __jsvalue msg = __jsobj_internal_Get(obj, JSBUILTIN_STRING_MESSAGE);
  if (__is_undefined(&msg)) {
    msg = __string_value(__jsstr_get_builtin(JSBUILTIN_STRING_EMPTY));
  } else {
    msg = __string_value(__js_ToString(&msg));
  }
  __jsstring *nameStr = __jsval_to_string(&name);
  __jsstring *msgStr = __jsval_to_string(&msg);
  bool isNameEmpty = __jsstr_get_length(nameStr) == 0;
  bool isMsgEmpty = __jsstr_get_length(msgStr) == 0;
  if (isNameEmpty) {
    return msg;
  }
  if (isMsgEmpty) {
    return name;
  }
  return __string_value(__jsstr_concat_3(nameStr, __jsstr_new_from_char(": "), msgStr));
}


__jsvalue __jserror_pt_toString(__jsvalue *this_object) {
  return __jserror_pt_toString_base(this_object, JSBUILTIN_STRING_ERROR_UL);
}

__jsvalue __js_rangeerror_pt_toString(__jsvalue *this_object) {
  return __jserror_pt_toString_base(this_object, JSBUILTIN_STRING_RANGE_ERROR_UL);
}

__jsvalue __js_evalerror_pt_toString(__jsvalue *this_object) {
  return __jserror_pt_toString_base(this_object, JSBUILTIN_STRING_EVAL_ERROR_UL);
}

__jsvalue __js_referenceerror_pt_toString(__jsvalue *this_object) {
  return __jserror_pt_toString_base(this_object, JSBUILTIN_STRING_REFERENCE_ERROR_UL);
}

__jsvalue __js_typeerror_pt_toString(__jsvalue *this_object) {
  return __jserror_pt_toString_base(this_object, JSBUILTIN_STRING_TYPE_ERROR_UL);
}

__jsvalue __js_urierror_pt_toString(__jsvalue *this_object) {
  return __jserror_pt_toString_base(this_object, JSBUILTIN_STRING_URI_ERROR_UL);
}

__jsvalue __js_syntaxerror_pt_toString(__jsvalue *this_object) {
  return __jserror_pt_toString_base(this_object, JSBUILTIN_STRING_SYNTAX_ERROR_UL);
}

// string is array-like objecti, set its property by its string character
void __jsobj_initprop_fromString(__jsobject *obj, __jsstring *str) {
  uint32_t len = __jsstr_get_length(str);
  for (uint32_t i = 0; i < len; i++) {
    __jsstring *strc = __jsstr_extract(str, i, 1);
    __jsvalue v = __string_value(strc);
    __jsprop_desc desc = __new_value_desc(&v, JSPROP_DESC_HAS_VWEC);
    __jsprop *prop = __jsobj_helper_create_propertyByValue(obj, i);
    __jsobj_helper_set_data_desc_gc(&prop->desc, desc, true);
  }
  return;
}

__jsvalue __jsobj_GetValueFromPropertyByValue(__jsobject *obj, uint32_t index) {
  __jsprop_desc desc = __jsobj_internal_GetPropertyByValue(obj, index);
  if (__is_undefined_desc(desc) || !__jsprop_desc_IsDataDescriptor(desc)) {
    return __undefined_value();
  }else {
    return __get_value(desc);
  }
}

bool __jsPropertyIsWritable(__jsobject *obj, uint32_t index) {
  __jsprop_desc desc = __jsobj_internal_GetPropertyByValue(obj, index);
  return __has_and_writable(desc);
}

void __jsconsole_pt_log (__jsvalue *thisV, __jsvalue *v) {
  __jsop_print_item(*v);
  printf("\n");
}

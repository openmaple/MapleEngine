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
#include "jsvalueinline.h"
#include "jsobject.h"
#include "jsarray.h"
#include "jsobjectinline.h"
#include "jsiter.h"
#include "vmmemory.h"

__jsiterator *__jsop_valueto_iterator(__jsvalue *value, uint32_t flags) {
  __jsiterator *itr = (__jsiterator *)VMMallocNOGC(sizeof(__jsiterator));
  // if value is undefined, return an empty iterator
  if (__is_null_or_undefined(value)) {
    itr->obj = NULL;
    return itr;
  }
  MAPLE_JS_ASSERT(__is_js_object(value));
  itr->obj = __jsval_to_object(value);
  itr->flags = (uint8_t)flags;
  itr->isNew = true;

  __create_builtin_property(itr->obj, NULL);
  if (itr->obj->object_class == JSARRAY) {
    __jsobj_helper_convert_to_generic(itr->obj);
    uint32_t len = __jsobj_helper_get_length(itr->obj);
    if (len > 0) {
      itr->prop_cur.index = 0;
      itr->prop_flag = false;
      return itr;
    }
  }

  if (itr->obj->object_class == JSON
          || itr->obj->builtin_id == JSBUILTIN_NUMBERCONSTRUCTOR
          || itr->obj->builtin_id == JSBUILTIN_BOOLEANCONSTRUCTOR) {
    itr->obj = NULL; // should not be able to enumerate
  }
  itr->prop_cur.prop = nullptr;
  itr->prop_flag = true;
  return itr;
}

__jsvalue __jsop_iterator_next(void *_itr_obj) {
  __jsiterator *itr_obj = (__jsiterator *)_itr_obj;
  // handle empty iterator
  if (!itr_obj->obj) {
    return __string_value(__jsstr_get_builtin(JSBUILTIN_STRING_EMPTY));
  }
  if (!itr_obj->prop_flag) {
    uint32_t len = __jsobj_helper_get_length(itr_obj->obj);
    assert(len > 0 && "NYI");
    if (itr_obj->isNew) {
      itr_obj->prop_cur.index = 0;
      itr_obj->isNew = false;
    } else {
      itr_obj->prop_cur.index++;
    }
    bool isLastIndex = false;
    __jsstring *retName = NULL;
    while (itr_obj->prop_cur.index < len) {
      uint32_t index = itr_obj->prop_cur.index;
      __jsvalue idx_val = __number_value(index);
      __jsprop_desc elem = __jsobj_internal_GetProperty(itr_obj->obj, &idx_val);
      if (index == len - 1) {
        isLastIndex = true;
      }
      if (__is_undefined_desc(elem) || !__has_and_enumerable(elem)) {
        itr_obj->prop_cur.index++;
      } else {
        retName = __js_NumberToString((int32_t)index);
        break;
      }
    }
    if (isLastIndex) {
      // that means the next prop of itr_obj will be pointed to itr_obj->obj->prop_list
      itr_obj->prop_flag = true;
#ifdef USE_PROP_MAP
      itr_obj->prop_cur.prop = itr_obj->obj->prop_list->prev;
#else
      __jsprop *skipProp = itr_obj->obj->prop_list;
      itr_obj->prop_cur.prop = NULL;
      while (skipProp) {
        uint32_t breakIndx;
        if (skipProp->isIndex)
          breakIndx = skipProp->n.index;
        else {
          __jsvalue name = __string_value(skipProp->n.name);
          breakIndx = __jsarr_getIndex(&name);
        }
        if (breakIndx == MAX_ARRAY_INDEX) {
          break;
        }
        itr_obj->prop_cur.prop = skipProp;
        skipProp = skipProp->next;
      }
#endif
      itr_obj->isNew =  (itr_obj->prop_cur.prop != NULL) ? false : true;
    }
    if (retName)
      return __string_value(retName);
  } else {
    // get the next iterator
    if (itr_obj->isNew) {
      __jsobject *obj = itr_obj->obj;
      if (obj->prop_list) {
        itr_obj->prop_cur.prop = obj->prop_list;
      } else if (obj->proto_is_builtin) {
        // if object prototype is builtin, iterate its prototyp
        __jsobject *proto = __jsobj_get_prototype(obj);
        itr_obj->prop_cur.prop = proto->prop_list;
        itr_obj->flags |= JSITER_USEPROTOTYPE;
      }
      itr_obj->isNew = false;
    } else {
      itr_obj->prop_cur.prop = itr_obj->prop_cur.prop->next;
      // iterate prototype if has
      if (itr_obj->prop_cur.prop == NULL &&
          itr_obj->obj->proto_is_builtin &&
          ((itr_obj->flags & JSITER_USEPROTOTYPE) == 0)) {
        __jsobject *proto = __jsobj_get_prototype(itr_obj->obj);
        itr_obj->prop_cur.prop = proto->prop_list;
        itr_obj->flags |= JSITER_USEPROTOTYPE;
      }
    }
    do {
      while (itr_obj->prop_cur.prop) {
        __jsprop *cur = itr_obj->prop_cur.prop;
        if (itr_obj->obj->object_class == JSARRAY) {
          __jsvalue name = __string_value(cur->n.name);
          if (__jsarr_getIndex(&name) != MAX_ARRAY_INDEX) {
            itr_obj->prop_cur.prop = cur->next;
            continue;
          }
        }
        if (((!cur->isIndex) && (cur->n.name == NULL)) ||
            !__has_and_enumerable(cur->desc)) {
          itr_obj->prop_cur.prop = cur->next;
        } else if (!cur->isIndex) {
          __jsstring *name = cur->n.name;
          return __string_value(name);
        } else { // must be subscription array access;
          return __string_value(__js_NumberToString(cur->n.index));
        }
      }
      if (((itr_obj->flags & JSITER_USEPROTOTYPE) != 0) ||
          (!itr_obj->obj->proto_is_builtin)) {
        break;
      }
      if (itr_obj->obj->proto_is_builtin) {
        __jsobject *proto = __jsobj_get_prototype(itr_obj->obj);
        itr_obj->prop_cur.prop = proto->prop_list;
        itr_obj->flags |= JSITER_USEPROTOTYPE;
      }
    } while(1);
  }
  __jsstring *empty = __jsstr_get_builtin(JSBUILTIN_STRING_EMPTY);
  return __string_value(empty);
}

bool __jsop_more_iterator(void *_itr_obj) {
  __jsiterator *itr_obj = (__jsiterator *)_itr_obj;
  // handle empty iterator
  if (!itr_obj->obj) return false;
  if (itr_obj->prop_flag) {
    __jsprop *nextProp = NULL;
    if (itr_obj->obj->object_class == JSARRAY) {
      if (!itr_obj->isNew && !(itr_obj->prop_cur.prop)) return false;
      nextProp = itr_obj->isNew ? itr_obj->obj->prop_list : itr_obj->prop_cur.prop->next;
      if (!nextProp) {
        return false;
      }
      if (__jsstr_equal_to_builtin(nextProp->n.name, JSBUILTIN_STRING_LENGTH)) {
        nextProp = nextProp->next;
      }
    } else {
      if (itr_obj->isNew) {
        __jsobject *obj = itr_obj->obj;
        if (obj->prop_list) return true;
        if (obj->proto_is_builtin) {
          __jsobject *proto = __jsobj_get_prototype(obj);
          if (proto) {
            return (proto->prop_list != NULL);
          }
        }
        return false;
      }
      nextProp = itr_obj->prop_cur.prop;
      if (!nextProp)
        return false;
      nextProp = itr_obj->prop_cur.prop->next;
    }
    if (!nextProp) {
      return false;
    }
    return true;
  }
  return true;
}

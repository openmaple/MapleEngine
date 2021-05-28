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

#include "jsvalueinline.h"
#include "jsobject.h"
#include "jsobjectinline.h"
#include "jsfunction.h"
#include "jsarray.h"
#include "jsnum.h"
#include "jsboolean.h"
#include "jsregexp.h"
#include "jsdate.h"
#include "jsglobal.h"
#include "vmmemory.h"
#include "jsdate.h"
#include "jsintl.h"

__jsvalue __js_Global_ThisBinding;
__jsvalue __js_ThisBinding;
__jsvalue __js_OuterBinding;
bool __is_global_strict = false;

// Assume builtin-objects' names are reserved keywords.
static __jsobject *__jsbuiltin_objects[JSBUILTIN_LAST_OBJECT] = { NULL };

static __jsfunction *__create_builtin_constructor(__jsbuiltin_object_id id, uint16_t *length) {
  *length = 1; // default arguments' length
  void *fp = NULL;
  bool isConstructor = false;
  switch (id) {
    case JSBUILTIN_OBJECTCONSTRUCTOR:
      fp = (void *)__js_new_obj;
      isConstructor = true;
      break;
    case JSBUILTIN_ARRAYCONSTRUCTOR:
      fp = (void *)__js_new_arr;
      isConstructor = true;
      break;
    case JSBUILTIN_STRINGCONSTRUCTOR:
      fp = (void *)__js_new_stringconstructor;
      isConstructor = true;
      break;
    case JSBUILTIN_BOOLEANCONSTRUCTOR:
      fp = (void *)__js_new_booleanconstructor;
      isConstructor = true;
      break;
    case JSBUILTIN_NUMBERCONSTRUCTOR:
      fp = (void *)__js_new_numberconstructor;
      isConstructor = true;
      break;
    case JSBUILTIN_REGEXPCONSTRUCTOR:
      fp = (void *)__js_new_regexp_obj;
      isConstructor = true;
      *length = 2;
      break;
    case JSBUILTIN_DATECONSTRUCTOR:
      fp = (void *)__js_new_dateconstructor;
      isConstructor = true;
      *length = 7;
      break;
    case JSBUILTIN_FUNCTIONCONSTRUCTOR:
      fp = (void *)__js_new_function;
      isConstructor = true;
      break;
    case JSBUILTIN_FUNCTIONPROTOTYPE:
      fp = (void *)__js_empty;
      *length = 0;
      break;
    case JSBUILTIN_ERROR_CONSTRUCTOR:
      fp = (void *)__js_new_error_obj;
      isConstructor = true;
      break;
    case JSBUILTIN_EVALERROR_CONSTRUCTOR:
      fp = (void *)__js_new_evalerror_obj;
      isConstructor = true;
      break;
    case JSBUILTIN_RANGEERROR_CONSTRUCTOR:
      fp = (void *)__js_new_rangeerror_obj;
      isConstructor = true;
      break;
    case JSBUILTIN_SYNTAXERROR_CONSTRUCTOR:
      fp = (void *)__js_new_syntaxerror_obj;
      isConstructor = true;
      break;
    case JSBUILTIN_URIERROR_CONSTRUCTOR:
      fp = (void *)__js_new_urierror_obj;
      isConstructor = true;
      break;
    case JSBUILTIN_REFERENCEERRORCONSTRUCTOR:
      fp = (void *)__js_new_reference_error_obj;
      isConstructor = true;
      break;
    case JSBUILTIN_TYPEERROR_CONSTRUCTOR:
      fp = (void *)__js_new_type_error_obj;
      isConstructor = true;
      break;
    case JSBUILTIN_ISNAN:
      fp = (void *)__js_isnan;
      break;
    case JSBUILTIN_PARSEINT_CONSTRUCTOR:
      fp = (void *)__js_parseint;
      *length = 2;
      break;
    case JSBUILTIN_DECODEURI_CONSTRUCTOR:
      fp = (void *)__js_decodeuri;
      break;
    case JSBUILTIN_DECODEURICOMPONENT_CONSTRUCTOR:
      fp = (void *)__js_decodeuricomponent;
      break;
    case JSBUILTIN_PARSEFLOAT_CONSTRUCTOR:
      fp = (void *)__js_parsefloat;
      break;
    case JSBUILTIN_ISFINITE_CONSTRUCTOR:
      fp = (void *)__js_isfinite;
      break;
    case JSBUILTIN_ENCODEURI_CONSTRUCTOR:
      fp = (void *)__js_encodeuri;
      break;
    case JSBUILTIN_ENCODEURICOMPONENT_CONSTRUCTOR:
      fp = (void *)__js_encodeuricomponent;
      break;
    case JSBUILTIN_EVAL_CONSTRUCTOR:
      fp = (void *)__js_eval;
      break;
    case JSBUILTIN_INTL:
      fp = (void *)__js_IntlConstructor;
      break;
    case JSBUILTIN_INTL_COLLATOR_CONSTRUCTOR:
      fp = (void *)__js_CollatorConstructor;
      isConstructor = true;
      *length = 0;
      break;
    case JSBUILTIN_INTL_NUMBERFORMAT_CONSTRUCTOR:
      fp = (void *)__js_NumberFormatConstructor;
      isConstructor = true;
      break;
    case JSBUILTIN_INTL_DATETIMEFORMAT_CONSTRUCTOR:
      fp = (void *)__js_DateTimeFormatConstructor;
      isConstructor = true;
      break;
    default:
      return NULL;
  }
  __jsfunction *fun = (__jsfunction *)VMMallocGC(sizeof(__jsfunction), MemHeadJSFunc);
  fun->fp = fp;
  fun->attrs = 1 << 24 | 1 << 8 | JSFUNCPROP_NATIVE;
  if (isConstructor) {
    fun->attrs |= JSFUNCPROP_CONSTRUCTOR;
  }
  fun->env = NULL;
  return fun;
}

static const uint16_t __jsbuiltin_objects_info[JSBUILTIN_LAST_OBJECT] = {
  // OBJECT_CLASS, OBJECT_TYPE, PROTO_TYPE_ID
 [JSBUILTIN_GLOBALOBJECT] =                   JSGLOBAL << 12 | JSGENERIC << 8 | JSBUILTIN_OBJECTPROTOTYPE,
 [JSBUILTIN_OBJECTCONSTRUCTOR] =              JSFUNCTION << 12 | JSGENERIC << 8 | JSBUILTIN_FUNCTIONPROTOTYPE,
 [JSBUILTIN_OBJECTPROTOTYPE] =                JSOBJECT << 12 | JSREGULAR_OBJECT << 8 | JSBUILTIN_LAST_OBJECT,
 [JSBUILTIN_FUNCTIONCONSTRUCTOR] =            JSFUNCTION << 12 | JSGENERIC << 8 | JSBUILTIN_FUNCTIONPROTOTYPE,
 [JSBUILTIN_FUNCTIONPROTOTYPE] =              JSFUNCTION << 12 | JSGENERIC << 8 | JSBUILTIN_OBJECTPROTOTYPE,
 [JSBUILTIN_ARRAYCONSTRUCTOR] =               JSFUNCTION << 12 | JSGENERIC << 8 | JSBUILTIN_FUNCTIONPROTOTYPE,
 [JSBUILTIN_ARRAYPROTOTYPE] =                 JSARRAY << 12 | JSREGULAR_OBJECT << 8 | JSBUILTIN_OBJECTPROTOTYPE,
 [JSBUILTIN_STRINGCONSTRUCTOR] =              JSFUNCTION << 12 | JSGENERIC << 8 | JSBUILTIN_FUNCTIONPROTOTYPE,
 [JSBUILTIN_STRINGPROTOTYPE] =                JSSTRING << 12 | JSGENERIC << 8 | JSBUILTIN_OBJECTPROTOTYPE,
 [JSBUILTIN_BOOLEANCONSTRUCTOR] =             JSFUNCTION << 12 | JSGENERIC << 8 | JSBUILTIN_FUNCTIONPROTOTYPE,
 [JSBUILTIN_BOOLEANPROTOTYPE] =               JSBOOLEAN << 12 | JSGENERIC << 8 | JSBUILTIN_OBJECTPROTOTYPE,
 [JSBUILTIN_NUMBERCONSTRUCTOR] =              JSFUNCTION << 12 | JSGENERIC << 8 | JSBUILTIN_FUNCTIONPROTOTYPE,
 [JSBUILTIN_NUMBERPROTOTYPE] =                JSNUMBER << 12 | JSGENERIC << 8 | JSBUILTIN_OBJECTPROTOTYPE,
 [JSBUILTIN_EXPORTS] =                        JSOBJECT << 12 | JSGENERIC << 8 | JSBUILTIN_LAST_OBJECT,
 [JSBUILTIN_MODULE] =                         JSOBJECT << 12 | JSGENERIC << 8 | JSBUILTIN_LAST_OBJECT,
 [JSBUILTIN_MATH] =                           JSMATH << 12 | JSGENERIC << 8 | JSBUILTIN_OBJECTPROTOTYPE,
 [JSBUILTIN_JSON] =                           JSON << 12 | JSGENERIC << 8 | JSBUILTIN_OBJECTPROTOTYPE,
 [JSBUILTIN_ERROR_CONSTRUCTOR] =              JSFUNCTION << 12 | JSGENERIC << 8 | JSBUILTIN_FUNCTIONPROTOTYPE,
 [JSBUILTIN_ERROR_PROTOTYPE] =                JSOBJECT << 12 | JSREGULAR_OBJECT << 8 | JSBUILTIN_OBJECTPROTOTYPE,
 [JSBUILTIN_EVALERROR_CONSTRUCTOR] =          JSFUNCTION << 12 | JSGENERIC << 8 | JSBUILTIN_ERROR_CONSTRUCTOR,
 [JSBUILTIN_EVALERROR_PROTOTYPE] =            JSOBJECT << 12 | JSREGULAR_OBJECT << 8 | JSBUILTIN_OBJECTPROTOTYPE,
 [JSBUILTIN_RANGEERROR_CONSTRUCTOR] =         JSFUNCTION << 12 | JSGENERIC << 8 | JSBUILTIN_ERROR_CONSTRUCTOR,
 [JSBUILTIN_RANGEERROR_PROTOTYPE] =           JSOBJECT << 12 | JSREGULAR_OBJECT << 8 | JSBUILTIN_OBJECTPROTOTYPE,
 [JSBUILTIN_REFERENCEERRORCONSTRUCTOR] =      JSFUNCTION << 12 | JSGENERIC << 8 | JSBUILTIN_ERROR_CONSTRUCTOR,
 [JSBUILTIN_REFERENCEERRORPROTOTYPE] =        JSOBJECT << 12 | JSREGULAR_OBJECT << 8 | JSBUILTIN_OBJECTPROTOTYPE,
 [JSBUILTIN_SYNTAXERROR_CONSTRUCTOR] =        JSFUNCTION << 12 | JSGENERIC << 8 | JSBUILTIN_ERROR_CONSTRUCTOR,
 [JSBUILTIN_SYNTAXERROR_PROTOTYPE] =          JSOBJECT << 12 | JSREGULAR_OBJECT << 8 | JSBUILTIN_OBJECTPROTOTYPE,
 [JSBUILTIN_TYPEERROR_CONSTRUCTOR] =          JSFUNCTION << 12 | JSGENERIC << 8 | JSBUILTIN_ERROR_CONSTRUCTOR,
 [JSBUILTIN_TYPEERROR_PROTOTYPE] =            JSOBJECT << 12 | JSREGULAR_OBJECT << 8 | JSBUILTIN_ERROR_PROTOTYPE,
 [JSBUILTIN_URIERROR_CONSTRUCTOR] =           JSFUNCTION << 12 | JSGENERIC << 8 | JSBUILTIN_ERROR_CONSTRUCTOR,
 [JSBUILTIN_URIERROR_PROTOTYPE] =             JSOBJECT << 12 | JSREGULAR_OBJECT << 8 | JSBUILTIN_OBJECTPROTOTYPE,
 [JSBUILTIN_DATECONSTRUCTOR] =                JSFUNCTION << 12 | JSGENERIC << 8 | JSBUILTIN_FUNCTIONPROTOTYPE,
 [JSBUILTIN_DATEPROTOTYPE] =                  JSOBJECT << 12 | JSREGULAR_OBJECT << 8 | JSBUILTIN_OBJECTPROTOTYPE,
 [JSBUILTIN_ISNAN] =                          JSFUNCTION << 12 | JSGENERIC << 8 | JSBUILTIN_FUNCTIONPROTOTYPE,
 [JSBUILTIN_REGEXPCONSTRUCTOR] =              JSFUNCTION << 12 | JSGENERIC << 8 | JSBUILTIN_FUNCTIONPROTOTYPE,
 [JSBUILTIN_REGEXPPROTOTYPE] =                JSOBJECT << 12 | JSGENERIC << 8 | JSBUILTIN_OBJECTPROTOTYPE,
 [JSBUILTIN_NAN] =                            JSOBJECT << 12 | JSREGULAR_OBJECT << 8 | JSBUILTIN_LAST_OBJECT,
 [JSBUILTIN_INFINITY] =                       JSOBJECT << 12 | JSREGULAR_OBJECT << 8 | JSBUILTIN_LAST_OBJECT,
 [JSBUILTIN_UNDEFINED] =                      JSOBJECT << 12 | JSREGULAR_OBJECT << 8 | JSBUILTIN_LAST_OBJECT,
 [JSBUILTIN_PARSEINT_CONSTRUCTOR] =           JSFUNCTION << 12 | JSGENERIC << 8 | JSBUILTIN_FUNCTIONPROTOTYPE,
 [JSBUILTIN_DECODEURI_CONSTRUCTOR] =          JSFUNCTION << 12 | JSGENERIC << 8 | JSBUILTIN_FUNCTIONPROTOTYPE,
 [JSBUILTIN_DECODEURICOMPONENT_CONSTRUCTOR] = JSFUNCTION << 12 | JSGENERIC << 8 | JSBUILTIN_FUNCTIONPROTOTYPE,
 [JSBUILTIN_PARSEFLOAT_CONSTRUCTOR] =         JSFUNCTION << 12 | JSGENERIC << 8 | JSBUILTIN_FUNCTIONPROTOTYPE,
 [JSBUILTIN_ISFINITE_CONSTRUCTOR] =           JSFUNCTION << 12 | JSGENERIC << 8 | JSBUILTIN_FUNCTIONPROTOTYPE,
 [JSBUILTIN_ENCODEURI_CONSTRUCTOR] =          JSFUNCTION << 12 | JSGENERIC << 8 | JSBUILTIN_FUNCTIONPROTOTYPE,
 [JSBUILTIN_ENCODEURICOMPONENT_CONSTRUCTOR] = JSFUNCTION << 12 | JSGENERIC << 8 | JSBUILTIN_FUNCTIONPROTOTYPE,
 [JSBUILTIN_EVAL_CONSTRUCTOR] =               JSFUNCTION << 12 | JSGENERIC << 8 | JSBUILTIN_FUNCTIONPROTOTYPE,
 [JSBUILTIN_INTL] =                           JSOBJECT << 12 | JSREGULAR_OBJECT << 8 | JSBUILTIN_OBJECTPROTOTYPE,
 [JSBUILTIN_INTL_COLLATOR_CONSTRUCTOR] =      JSFUNCTION << 12 | JSGENERIC << 8 | JSBUILTIN_FUNCTIONPROTOTYPE,
 [JSBUILTIN_INTL_COLLATOR_PROTOTYPE] =        JSOBJECT << 12 | JSREGULAR_OBJECT << 8 << JSBUILTIN_OBJECTPROTOTYPE,
 [JSBUILTIN_INTL_NUMBERFORMAT_CONSTRUCTOR] =  JSFUNCTION << 12 | JSGENERIC << 8 | JSBUILTIN_FUNCTIONPROTOTYPE,
 [JSBUILTIN_INTL_NUMBERFORMAT_PROTOTYPE] =    JSOBJECT << 12 | JSREGULAR_OBJECT | JSBUILTIN_OBJECTPROTOTYPE,
 [JSBUILTIN_INTL_DATETIMEFORMAT_CONSTRUCTOR] =  JSFUNCTION << 12 | JSGENERIC << 8 | JSBUILTIN_FUNCTIONPROTOTYPE,
 [JSBUILTIN_INTL_DATETIMEFORMAT_PROTOTYPE] =   JSOBJECT << 12 | JSREGULAR_OBJECT << 8 | JSBUILTIN_OBJECTPROTOTYPE,
 [JSBUILTIN_CONSOLE] =                           JSOBJECT << 12 | JSGENERIC << 8 | JSBUILTIN_OBJECTPROTOTYPE,
};

// map builtin object to its builtin string
static const __jsbuiltin_string_id __jsbuiltin_object_map_string[JSBUILTIN_LAST_OBJECT] = {
 [JSBUILTIN_GLOBALOBJECT] =                   JSBUILTIN_STRING_EMPTY,
 [JSBUILTIN_OBJECTCONSTRUCTOR] =              JSBUILTIN_STRING_OBJECT_UL,
 [JSBUILTIN_OBJECTPROTOTYPE] =                JSBUILTIN_STRING_EMPTY,
 [JSBUILTIN_FUNCTIONCONSTRUCTOR] =            JSBUILTIN_STRING_FUNCTION_UL,
 [JSBUILTIN_FUNCTIONPROTOTYPE] =              JSBUILTIN_STRING_EMPTY,
 [JSBUILTIN_ARRAYCONSTRUCTOR] =               JSBUILTIN_STRING_ARRAY_UL,
 [JSBUILTIN_ARRAYPROTOTYPE] =                 JSBUILTIN_STRING_EMPTY,
 [JSBUILTIN_STRINGCONSTRUCTOR] =              JSBUILTIN_STRING_STRING_UL,
 [JSBUILTIN_STRINGPROTOTYPE] =                JSBUILTIN_STRING_EMPTY,
 [JSBUILTIN_BOOLEANCONSTRUCTOR] =             JSBUILTIN_STRING_BOOLEAN_UL,
 [JSBUILTIN_BOOLEANPROTOTYPE] =               JSBUILTIN_STRING_EMPTY,
 [JSBUILTIN_NUMBERCONSTRUCTOR] =              JSBUILTIN_STRING_NUMBER_UL,
 [JSBUILTIN_NUMBERPROTOTYPE] =                JSBUILTIN_STRING_EMPTY,
 [JSBUILTIN_EXPORTS] =                        JSBUILTIN_STRING_EXPORTS,  //EXPORTS
 [JSBUILTIN_MODULE] =                         JSBUILTIN_STRING_EMPTY,  // MODULE
 [JSBUILTIN_MATH] =                           JSBUILTIN_STRING_MATH_UL, // MATH
 [JSBUILTIN_JSON] =                           JSBUILTIN_STRING_JSON_U,
 [JSBUILTIN_ERROR_CONSTRUCTOR] =              JSBUILTIN_STRING_ERROR_UL,
 [JSBUILTIN_ERROR_PROTOTYPE] =                JSBUILTIN_STRING_EMPTY,
 [JSBUILTIN_EVALERROR_CONSTRUCTOR] =          JSBUILTIN_STRING_EVAL_ERROR_UL,
 [JSBUILTIN_EVALERROR_PROTOTYPE] =            JSBUILTIN_STRING_EMPTY,
 [JSBUILTIN_RANGEERROR_CONSTRUCTOR] =         JSBUILTIN_STRING_RANGE_ERROR_UL,
 [JSBUILTIN_RANGEERROR_PROTOTYPE] =           JSBUILTIN_STRING_EMPTY,
 [JSBUILTIN_REFERENCEERRORCONSTRUCTOR] =      JSBUILTIN_STRING_REFERENCE_ERROR_UL,
 [JSBUILTIN_REFERENCEERRORPROTOTYPE] =        JSBUILTIN_STRING_EMPTY,
 [JSBUILTIN_SYNTAXERROR_CONSTRUCTOR] =        JSBUILTIN_STRING_SYNTAX_ERROR_UL,
 [JSBUILTIN_SYNTAXERROR_PROTOTYPE] =          JSBUILTIN_STRING_EMPTY,
 [JSBUILTIN_TYPEERROR_CONSTRUCTOR] =          JSBUILTIN_STRING_TYPE_ERROR_UL,
 [JSBUILTIN_TYPEERROR_PROTOTYPE] =            JSBUILTIN_STRING_EMPTY,
 [JSBUILTIN_URIERROR_CONSTRUCTOR] =           JSBUILTIN_STRING_URI_ERROR_UL,
 [JSBUILTIN_URIERROR_PROTOTYPE] =             JSBUILTIN_STRING_EMPTY,
 [JSBUILTIN_DATECONSTRUCTOR] =                JSBUILTIN_STRING_DATE_UL,
 [JSBUILTIN_DATEPROTOTYPE] =                  JSBUILTIN_STRING_EMPTY,
 [JSBUILTIN_ISNAN] =                          JSBUILTIN_STRING_IS_NAN,
 [JSBUILTIN_REGEXPCONSTRUCTOR] =              JSBUILTIN_STRING_REGEXP_UL,
 [JSBUILTIN_REGEXPPROTOTYPE] =                JSBUILTIN_STRING_EMPTY,
 [JSBUILTIN_NAN] =                            JSBUILTIN_STRING_NAN,
 [JSBUILTIN_INFINITY] =                       JSBUILTIN_STRING_INFINITY_UL,
 [JSBUILTIN_UNDEFINED] =                      JSBUILTIN_STRING_EMPTY,
 [JSBUILTIN_PARSEINT_CONSTRUCTOR] =           JSBUILTIN_STRING_PARSE_INT,
 [JSBUILTIN_DECODEURI_CONSTRUCTOR] =          JSBUILTIN_STRING_DECODE_URI,
 [JSBUILTIN_DECODEURICOMPONENT_CONSTRUCTOR] = JSBUILTIN_STRING_DECODE_URI_COMPONENT,
 [JSBUILTIN_PARSEFLOAT_CONSTRUCTOR] =         JSBUILTIN_STRING_PARSE_FLOAT,
 [JSBUILTIN_ISFINITE_CONSTRUCTOR] =           JSBUILTIN_STRING_IS_FINITE,
 [JSBUILTIN_ENCODEURI_CONSTRUCTOR] =          JSBUILTIN_STRING_ENCODE_URI,
 [JSBUILTIN_ENCODEURICOMPONENT_CONSTRUCTOR] = JSBUILTIN_STRING_ENCODE_URI_COMPONENT,
 [JSBUILTIN_EVAL_CONSTRUCTOR] =               JSBUILTIN_STRING_EVAL,
 [JSBUILTIN_INTL] =                           JSBUILTIN_STRING_INTL_UL,
 [JSBUILTIN_INTL_COLLATOR_CONSTRUCTOR] =      JSBUILTIN_STRING_COLLATOR_UL,
 [JSBUILTIN_INTL_COLLATOR_PROTOTYPE] =        JSBUILTIN_STRING_EMPTY,
 [JSBUILTIN_INTL_NUMBERFORMAT_CONSTRUCTOR] =  JSBUILTIN_STRING_NUMBERFORMAT_UL,
 [JSBUILTIN_INTL_NUMBERFORMAT_PROTOTYPE] =    JSBUILTIN_STRING_EMPTY,
 [JSBUILTIN_INTL_DATETIMEFORMAT_CONSTRUCTOR] =  JSBUILTIN_STRING_DATETIMEFORMAT_UL,
 [JSBUILTIN_INTL_DATETIMEFORMAT_PROTOTYPE] =  JSBUILTIN_STRING_EMPTY,
 [JSBUILTIN_CONSOLE]                       =  JSBUILTIN_STRING_CONSOLE_UL,
};

__jsobject *__jsobj_get_or_create_builtin(__jsbuiltin_object_id id) {
  if (id >= JSBUILTIN_LAST_OBJECT) {
    return NULL;
  }
  if (__jsbuiltin_objects[id] != NULL) {
    return __jsbuiltin_objects[id];
  }
  __jsobject *obj = __create_object();
  __jsbuiltin_objects[id] = obj;
  GCIncRf((void *)obj);
  MAPLE_JS_ASSERT(JSBUILTIN_LAST_OBJECT < 255);
  uint16_t info = __jsbuiltin_objects_info[id];
  __jsobj_class cl = (__jsobj_class)(info >> 12);
  uint8_t proto_id = info & 0xff;
  if ((__jsbuiltin_object_id)proto_id == JSBUILTIN_LAST_OBJECT) {
    __jsobj_set_prototype(obj, NULL);
  } else {
    __jsobj_set_prototype(obj, (__jsbuiltin_object_id)proto_id);
  }
  obj->extensible = true;
  obj->object_class = cl;
  obj->object_type = info >> 8;
  obj->is_builtin = true;
  obj->builtin_id = id;
  // Create constructor if necessary;
  uint16_t arg_length;
  obj->shared.fun = __create_builtin_constructor(id, &arg_length);
  if (obj->shared.fun) {
    // init arguments' length
    __jsvalue length_value = __number_value(arg_length);
    // 20.2.2.1 Function.length, { [[Writable]]: false, [[Enumerable]]: false, [[Configurable]]: true }
    __jsobj_helper_init_value_property(obj, JSBUILTIN_STRING_LENGTH, &length_value, JSPROP_DESC_HAS_VUWUEC);
  }
  if ((id == JSBUILTIN_MODULE && !__jsbuiltin_objects[JSBUILTIN_EXPORTS]) ||
      (id == JSBUILTIN_EXPORTS && !__jsbuiltin_objects[JSBUILTIN_MODULE])) {
    // set module.exports = exports
    bool ismodule = (id == JSBUILTIN_MODULE);
    __jsvalue obj_value = __object_value(ismodule ? obj : __jsobj_get_or_create_builtin(JSBUILTIN_MODULE));
    __jsvalue str_value = __string_value(__jsstr_get_builtin(JSBUILTIN_STRING_EXPORTS));
    __jsvalue exports_value = __object_value(ismodule ? __jsobj_get_or_create_builtin(JSBUILTIN_EXPORTS) : obj);
    __jsop_setprop(&obj_value, &str_value, &exports_value);
  }
  if (__jsbuiltin_object_map_string[id] != JSBUILTIN_STRING_EMPTY) {
    // add the builtin object as global this' prop
    __jsvalue objV = __object_value(obj);
    // __jsobj_helper_add_value_property(__jsval_to_object(&__js_Global_ThisBinding), __jsbuiltin_object_map_string[id], &objV, JSPROP_DESC_HAS_VUWUEUC);
    __jsop_set_this_prop_by_name(&__js_Global_ThisBinding, __jsstr_get_builtin(__jsbuiltin_object_map_string[id]), &objV);
  }
  return obj;
}

#ifdef MEMORY_LEAK_CHECK
static bool __jsobj_check_builtin_circle(__jsobject *obj1, __jsobject *obj2) {
  __jsprop *prop = obj1->prop_list;
  while (prop) {
    __jsvalue v = prop->desc.named_data_property.value;
    if (__is_js_object(&v)) {
      if (__jsval_to_object(&v) == obj2) {
        return true;
      }
    }
    prop = prop->next;
  }
  return false;
}

static void __jsobj_delete_builtin_circle(__jsobject *obj1, __jsobject *obj2) {
  __jsprop *prop = obj1->prop_list;
  while (prop) {
    __jsvalue v = prop->desc.named_data_property.value;
    if (__is_js_object(&v) && __jsval_to_object(&v) == obj2) {
      __jsvalue undefined = __undefined_value();
      __set_value(&(prop->desc), &undefined);
      GCDecRf(obj2);
      return;
    }
    prop = prop->next;
  }
}

static void __jsobj_check_and_release_builtin_circle(__jsbuiltin_object_id id1, __jsbuiltin_object_id id2) {
  __jsobject *obj1 = __jsbuiltin_objects[id1];
  __jsobject *obj2 = __jsbuiltin_objects[id2];
  if (obj1 && obj2) {
    if (GCGetRf(obj1) == 1 && GCGetRf(obj2) == 1) {
      if (__jsobj_check_builtin_circle(obj1, obj2) && __jsobj_check_builtin_circle(obj2, obj1)) {
        __jsobj_delete_builtin_circle(obj1, obj2);
      }
    }
  }
}

// Help for memory leak check.
// Release all the builtin objects when exit the app.
void __jsobj_release_builtin() {
  for (int32_t i = 0; i < (int32_t)JSBUILTIN_LAST_OBJECT; i++) {
    if (__jsbuiltin_objects[i] != NULL) {
      GCDecRf((void *)__jsbuiltin_objects[i]);
    }
  }
  __jsobj_check_and_release_builtin_circle(JSBUILTIN_OBJECTCONSTRUCTOR, JSBUILTIN_OBJECTPROTOTYPE);
  __jsobj_check_and_release_builtin_circle(JSBUILTIN_FUNCTIONCONSTRUCTOR, JSBUILTIN_FUNCTIONPROTOTYPE);
  __jsobj_check_and_release_builtin_circle(JSBUILTIN_ARRAYCONSTRUCTOR, JSBUILTIN_ARRAYPROTOTYPE);
  __jsobj_check_and_release_builtin_circle(JSBUILTIN_STRINGCONSTRUCTOR, JSBUILTIN_STRINGPROTOTYPE);
  __jsobj_check_and_release_builtin_circle(JSBUILTIN_BOOLEANCONSTRUCTOR, JSBUILTIN_BOOLEANPROTOTYPE);
  __jsobj_check_and_release_builtin_circle(JSBUILTIN_NUMBERCONSTRUCTOR, JSBUILTIN_NUMBERPROTOTYPE);
}

#endif

void __js_init_context(bool isStrict) {
  // Init ThisBinding when entry global code.
  __js_ThisBinding = __object_value(__jsobj_get_or_create_builtin(JSBUILTIN_GLOBALOBJECT));
  __js_Global_ThisBinding = __js_ThisBinding;
  __is_global_strict = isStrict;
}

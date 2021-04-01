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

#include <cstdio>
#include "jsvalue.h"
#include "jsvalueinline.h"
#include "jsnum.h"
#include "jsstring.h"
#include "vmmemory.h"
#include "jstycnv.h"
#include "jsdate.h"

void __jsop_print_item(__jsvalue value) {
  switch (__jsval_typeof(&value)) {
    case JSTYPE_UNDEFINED:
    case JSTYPE_NONE:
      printf("undefined");
      break;
    case JSTYPE_NULL:
      printf("null");
      break;
    case JSTYPE_BOOLEAN: {
      const char *str = __jsval_to_boolean(&value) ? "true" : "false";
      printf("%s", str);
      break;
    }
    case JSTYPE_STRING: {
      __jsstring *str = __jsval_to_string(&value);
      __jsstr_print(str);
      break;
    }
    case JSTYPE_NUMBER:
      printf("%d", __jsval_to_number(&value));
      break;
    case JSTYPE_OBJECT: {
      // __jsvalue v = __jsobj_pt_toString(&value);
      __jsobject *obj = __jsval_to_object(&value);
      if (obj->object_class == JSNUMBER && obj->object_type == JSSPECIAL_NUMBER_OBJECT) {
        __jsstr_print(obj->shared.prim_string);
      } else if (obj->object_class == JSNUMBER || obj->object_class == JSBOOLEAN) {
        printf("%d", obj->shared.prim_number);
      } else if (obj->object_class == JSDOUBLE) {
        printf("%.16g", obj->shared.primDouble);
      } else if (obj->object_class == JSDATE) {
        __jsvalue v = __jsdate_ToString(&value);
        __jsop_print_item(v);
      } else {
        __jsvalue v = __js_ToPrimitive(&value, JSTYPE_UNDEFINED /* ??? */);
        __jsop_print_item(v);
        if(!__is_undefined(&v))
            memory_manager->RecallString(__jsval_to_string(&v));
      }
      break;
    }
    case JSTYPE_DOUBLE: {
      printf("%.16g", memory_manager->GetF64FromU32((value.s.payload.u32)));
      break;
    }
    case JSTYPE_NAN: {
      printf("NaN");
      break;
    }
    case JSTYPE_INFINITY: {
      if (__is_neg_infinity(&value))
        printf("-Infinity");
      else
        printf("Infinity");
      break;
    }
    default:
      MAPLE_JS_EXCEPTION(0 && "unexpect jsvalue type");
  }
}

__jsvalue __jsop_print(__jsvalue *array, uint32_t size) {
  for (uint32_t i = 0; i < size; i++) {
    __jsop_print_item(array[i]);
    if (i != size - 1)
        printf(" ");
  }
  printf("\n");
  return __undefined_value();
}

__jsvalue __js_error(__jsvalue *array, uint32_t size) {
  for (uint32_t i = 0; i < size; i++) {
    __jsop_print_item(array[i]);
    if (i != size - 1)
        printf(" ");
  }
  printf("\n");
  MIR_FATAL("__js_error reported");
  return __undefined_value();
}

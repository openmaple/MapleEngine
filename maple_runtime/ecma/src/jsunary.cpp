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

#include "jsvalue.h"
#include "jsvalueinline.h"
#include "jsstring.h"
#include "jstycnv.h"

// ecma 11.4.3 The typeof Operator
__jsvalue __jsop_typeof(__jsvalue *v) {
  __jsbuiltin_string_id id;
  switch (__jsval_typeof(v)) {
    case JSTYPE_NONE:
      id = JSBUILTIN_STRING_UNDEFINED;
      break;
    case JSTYPE_UNDEFINED:
      id = JSBUILTIN_STRING_UNDEFINED;
      break;
    case JSTYPE_NULL:
      id = JSBUILTIN_STRING_OBJECT;
      break;
    case JSTYPE_BOOLEAN:
      id = JSBUILTIN_STRING_BOOLEAN;
      break;
    case JSTYPE_NUMBER:
      id = (v->x.u64 == 0) ? JSBUILTIN_STRING_UNDEFINED : JSBUILTIN_STRING_NUMBER;
      break;
    case JSTYPE_DOUBLE:
    case JSTYPE_NAN:
    case JSTYPE_INFINITY:
      id = JSBUILTIN_STRING_NUMBER;
      break;
    case JSTYPE_STRING:
      id = JSBUILTIN_STRING_STRING;
      break;
    case JSTYPE_OBJECT:
      if (__is_js_function(v)) {
        id = JSBUILTIN_STRING_FUNCTION;
      } else {
        id = JSBUILTIN_STRING_OBJECT;
      }
      break;
    default:
      MAPLE_JS_EXCEPTION("unreachable!");
      id = JSBUILTIN_STRING_UNDEFINED;  // to suppress warning
  }
  return __string_value(__jsstr_get_builtin(id));
}

// ecma 11.4.4 Prefix Increment Operator
// ecma 11.4.5 Prefix Decrement Operator
// ecma 11.4.6 Unary + Operator
// Use __js_ToNumber

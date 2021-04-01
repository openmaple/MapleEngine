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
#include "jsobjectinline.h"
bool _jsboo_helper_get_boolean(__jsvalue *this_boolean) {
  if (!__is_boolean_object(this_boolean) && !__is_boolean(this_boolean)) {
    MAPLE_JS_TYPEERROR_EXCEPTION();
  }
  bool b;
  if (__is_boolean(this_boolean)) {
    b = __jsval_to_boolean(this_boolean);
  } else {
    __jsobject *obj = __jsval_to_object(this_boolean);
    b = obj->shared.prim_bool;
  }
  return b;
}

// ecma 15.6.4.2
__jsvalue __jsboo_pt_toString(__jsvalue *this_boolean) {
  bool b = _jsboo_helper_get_boolean(this_boolean);
  return __string_value(__jsstr_get_builtin(b ? JSBUILTIN_STRING_TRUE : JSBUILTIN_STRING_FALSE));
}

// ecma 15.6.4.3
__jsvalue __jsboo_pt_valueOf(__jsvalue *this_boolean) {
  return __boolean_value(_jsboo_helper_get_boolean(this_boolean));
}

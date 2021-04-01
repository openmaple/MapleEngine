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
#include "jsvalue.h"
#include "jsvalueinline.h"
#include "jsnum.h"
#include "jsstring.h"
#include "jsfunction.h"
__jsvalue __jsop_call(__jsvalue *function, __jsvalue *this_arg, __jsvalue *arg_list, uint32_t arg_count) {
  return __jsfun_val_call(function, this_arg, arg_list, arg_count);
}

// ecma 11.2.2 The new Operator
__jsvalue __jsop_new(__jsvalue *constructor, __jsvalue *this_arg, __jsvalue *arg_list, uint32_t nargs) {
  if (!__is_js_object(constructor) || !__is_js_function(constructor)) {
    MAPLE_JS_TYPEERROR_EXCEPTION();
  }
  __jsobject *f = __jsval_to_object(constructor);
  __jsfunction *fun = f->shared.fun;
  return __jsfun_intr_Construct(f, this_arg, arg_list, nargs);
}

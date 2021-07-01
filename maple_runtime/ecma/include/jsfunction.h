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

#ifndef JSFUNCTION_H
#define JSFUNCTION_H
#include "jsvalue.h"

#define JSFUNCPROP_STRICT ((uint8_t)0x01)
#define JSFUNCPROP_NATIVE ((uint8_t)0x02)
#define JSFUNCPROP_USERFUNC ((uint8_t)0x04)
#define JSFUNCPROP_BOUND ((uint8_t)0x08)
#define JSFUNCPROP_CONSTRUCTOR ((uint8_t)0x10)

// When the flag of a js-function is JSFUNCPROP_BOUND, the "fp" of
// the js-function piont to bound function object, and "env" piont to
// bound_this and arguments;
struct __jsfunction {
  void *fp;
  void *env;
  // Encode field:
  // |..8bits...|..8bits..|..8bits..|..8bits..|
  // | VARG_P   |  NARGS  |  LENGTH |  FLAG   |
  uint32_t attrs;
  int32_t fileIndex; // to indicate what files this function located for plugin
};

// ecma 13.2
__jsvalue __js_new_function(void *fp, void *env, uint32_t attrs, int32_t idx = -1, bool needpt = true);
// ecma 13.2.1
__jsvalue __jsfun_internal_call(__jsobject *f, __jsvalue *this_arg, __jsvalue *arg_list, uint32_t arg_count, __jsvalue *orig_arg = NULL);
// Helper function for internal use.
__jsvalue __jsfun_val_call(__jsvalue *function, __jsvalue *this_arg, __jsvalue *arg_list, uint32_t arg_count);
// ecma 13.2.2
__jsvalue __jsfun_intr_Construct(__jsobject *f, __jsvalue *this_arg, __jsvalue *arg_list, uint32_t arg_count);
// ecma 15.3.4.3
__jsvalue __jsfun_pt_apply(__jsvalue *function, __jsvalue *this_arg, __jsvalue *arg_array);
// ecma 15.3.4.4
__jsvalue __jsfun_pt_call(__jsvalue *function, __jsvalue *args, uint32_t arg_count);
// ecma 15.3.4.5
__jsvalue __jsfun_pt_bind(__jsvalue *function, __jsvalue *args, uint32_t arg_count);
bool __jsfun_internal_HasInstance(__jsobject *f, __jsvalue *v);
bool __js_Impl_HasInstance(__jsvalue *v);
__jsvalue __jsfun_internal_call_interp(__jsfunction *, __jsvalue *, uint32_t);

// ecma 15.3.2.1 new Function (p1, p2, … , pn, body)
__jsvalue __js_new_functionN(void *, __jsvalue *, uint32_t);
// ecma 15.3.4.2Function.prototype.toString
__jsvalue __jsfun_pt_tostring(__jsvalue *);
#endif

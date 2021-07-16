/*
 * Copyright (c) [2021] Futurewei Technologies Co.,Ltd.All rights reserved.
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

#include "jsobject.h"
#include "jsobjectinline.h"
#include "jsarray.h"
#include "jsmath.h"

// ECMA-402 1.0 10.1 The Intl.Collator Constructor
__jsvalue __js_CollatorConstructor(__jsvalue *this_arg, __jsvalue *arg_list,
                                   uint32_t nargs) {
  __jsobject *obj = __create_object();
  __jsobj_set_prototype(obj, JSBUILTIN_INTL_COLLATOR_PROTOTYPE);
  obj->object_class = JSINTL;
  obj->extensible = true;
  obj->object_type = JSREGULAR_OBJECT;
  obj->shared.intl = (__jsintl*) VMMallocGC(sizeof(__jsintl));
  obj->shared.intl->kind = JSINTL_COLLATOR;
  __jsvalue obj_val = __object_value(obj);

  __jsvalue locales = __undefined_value();
  __jsvalue options = __undefined_value();
  if (nargs == 0) {
    // Do nothing.
  } else if (nargs == 1) {
    locales = arg_list[0];
  } else if (nargs == 2) {
    locales = arg_list[0];
    options = arg_list[1];
  } else {
    MAPLE_JS_SYNTAXERROR_EXCEPTION();
  }
  InitializeCollator(&obj_val, &locales, &options);

  return obj_val;
}

// ECMA-402 1.0 10.1.1.1
// InitializeCollator(collator, locales, options)
void InitializeCollator(__jsvalue *collator, __jsvalue *locales,
                        __jsvalue *options) {
  // Step 1.
  __jsvalue prop = StrToVal("initializedIntlObject");
  __jsvalue v = __jsop_getprop(collator, &prop);
  if (!__is_undefined(&v)) {
    if (__is_boolean(&v) && __jsval_to_boolean(&v) == true) {
      MAPLE_JS_TYPEERROR_EXCEPTION();
    }
  }
  // Step 2.
  v = __boolean_value(true);
  __jsop_setprop(collator, &prop, &v);
  // Step 3.
  __jsvalue requested_locales = CanonicalizeLocaleList(locales);
  // Step 4.
  if (__is_undefined(options)) {
    // Step 4a.
    __jsobject *o = __js_new_obj_obj_0();
    *options = __object_value(o);
  }
}

// ECMA-402 1.0 10.2.2
// Intl.Collator.supportedLocalesOf(locales [, options])
__jsvalue __jsintl_CollatorSupportedLocalesOf(__jsvalue *locales,
                                              __jsvalue *arg_list,
                                              uint32_t nargs) {
  // Step 1.
  __jsvalue options;
  if (nargs == 1) {
    options = __undefined_value();
  } else {
    options = arg_list[1];
  }
  // Step 2.
  __jsvalue prop = StrToVal("availableLocales");
  __jsvalue collator_init; // TODO
  __jsvalue available_locales = __jsop_getprop(&collator_init, &prop);
  // Step 3.
  __jsvalue requested_locales = CanonicalizeLocaleList(locales);
  // Step 4.
  return SupportedLocales(&available_locales, &requested_locales, &options);
}

// ECMA-402 1.0 10.3.2
// Intl.Collator.prototype.compare
__jsvalue __jsintl_CollatorCompare(__jsvalue *this_arg, __jsvalue *arg_list,
                                   uint32_t nargs) {
  // TODO: not implemented yet.
  return __null_value();
}

// ECMA-402 1.0 10.3.3
// Intl.Collator.prototype.resolvedOptions()
__jsvalue __jsintl_CollatorResolvedOptions(__jsvalue *this_arg,
                                           __jsvalue *arg_list, uint32_t nargs) {
  // TODO: not implemented yet.
  return __null_value();
}

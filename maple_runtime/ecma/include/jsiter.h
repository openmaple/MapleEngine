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

#ifndef JSITER_H
#define JSITER_H

#include "jsvalue.h"
#include "jsobject.h"
#include "jsiter.h"

#define JSITER_ACTIVE 0x1000
#define JSITER_UNREUSABLE 0x2000
#define JSITER_ENUMERATE 0x1 /* for-in compatible hidden default iterator */
#define JSITER_FOREACH 0x2   /* return [key, value] pair rather than key */
#define JSITER_KEYVALUE 0x4  /* destructuring for-in wants [key, value] */
#define JSITER_OWNONLY 0x8   /* iterate over obj's own properties only */
#define JSITER_HIDDEN 0x10   /* also enumerate non-enumerable properties */
#define JSITER_USEPROTOTYPE 0x4000

struct __jsiterator {
  __jsobject *obj;
  union {
    uint32_t index;
    __jsprop *prop;
    __jsfast_prop *fast_prop;
  } prop_cur;
  uint32_t flags;
  bool prop_flag;  // true for prop, false for index
  bool isNew; // just newed, didn't iterated yet
};

__jsiterator *__jsop_valueto_iterator(__jsvalue *value, uint32_t flags);
__jsvalue __jsop_iterator_next(void *_itr_obj);
bool __jsop_more_iterator(void *_itr_obj);
#endif

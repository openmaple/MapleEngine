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
#include "jstycnv.h"
#include "vmmemory.h"
__jsvalue __jsmath_pt_abs(__jsvalue *this_math, __jsvalue *value);
__jsvalue __jsmath_pt_ceil(__jsvalue *this_math, __jsvalue *value);
__jsvalue __jsmath_pt_floor(__jsvalue *this_math, __jsvalue *value);
__jsvalue __jsmath_pt_max(__jsvalue *this_math, __jsvalue *items, uint32_t size);
__jsvalue __jsmath_pt_min(__jsvalue *this_math, __jsvalue *items, uint32_t size);
__jsvalue __jsmath_pt_pow(__jsvalue *this_math, __jsvalue *x, __jsvalue *y);
__jsvalue __jsmath_pt_round(__jsvalue *this_math, __jsvalue *value);
__jsvalue __jsmath_pt_sqrt(__jsvalue *this_math, __jsvalue *value);
__jsvalue __jsmath_pt_sin(__jsvalue *this_math, __jsvalue *value);
__jsvalue __jsmath_pt_asin(__jsvalue *this_math, __jsvalue *value);
__jsvalue __jsmath_pt_cos(__jsvalue *this_math, __jsvalue *value);
__jsvalue __jsmath_pt_acos(__jsvalue *this_math, __jsvalue *value);
__jsvalue __jsmath_pt_tan(__jsvalue *this_math, __jsvalue *value);
__jsvalue __jsmath_pt_atan(__jsvalue *this_math, __jsvalue *value);
__jsvalue __jsmath_pt_atan2(__jsvalue *this_math, __jsvalue *y, __jsvalue *x);
__jsvalue __jsmath_pt_log(__jsvalue *this_math, __jsvalue *value);
__jsvalue __jsmath_pt_exp(__jsvalue *this_math, __jsvalue *value);
__jsvalue __jsmath_pt_random(__jsvalue *this_math);

inline bool __is_int32_range(double t) {
  return t >= (int32_t)0x80000000 && t <= (int32_t)0x7fffffff;
}

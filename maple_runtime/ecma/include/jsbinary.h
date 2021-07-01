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

#ifndef JSBINARY_H
#define JSBINARY_H
// ecma 11.9.3
bool __js_AbstractEquality(__jsvalue *x, __jsvalue *y);
// ecma 11.9.6
// This algorithm differs from the SameValue Algorithm (9.12) in its
// treatment of signed zeroes and NaNs, So if the target does not support
// float just return __js_SameValue(x, y);
bool __js_StrictEquality(__jsvalue *x, __jsvalue *y);
#endif

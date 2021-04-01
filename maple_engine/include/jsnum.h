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
#include "jsstring.h"
#define ExponentBias 1023
#define ExponentShift 52

#define SignBit 0x8000000000000000ULL
#define SignificandBits 0x000fffffffffffffULL

double __js_toDecimal(uint16_t *start, uint16_t *end, int base, uint16_t **pos);
uint64_t __js_strtod(uint16_t *start, uint16_t *end, uint16_t **pos);
bool __js_chars2num(uint16_t *chars, int32_t length, uint64_t *result);
uint64_t __js_str2num(__jsstring *str);
double __js_str2num_base_x(__jsstring *str, int32_t base, bool& isNum);
__jsstring *__jsnum_integer_encode(int value, int base);
// ecma 15.7.4.2
__jsvalue __jsnum_pt_toString(__jsvalue *this_object, __jsvalue *radix);

// ecma 15.7.4.3
__jsvalue __jsnum_pt_toLocaleString(__jsvalue *this_object);

// ecma 15.7.4.4
__jsvalue __jsnum_pt_valueOf(__jsvalue *this_object);

// ecma 15.7.4.5
__jsvalue __jsnum_pt_toFixed(__jsvalue *this_number, __jsvalue *fracdigit);

// ecma 15.7.4.6
__jsvalue __jsnum_pt_toExponential(__jsvalue *this_number, __jsvalue *fractdigit);

// ecma 15.7.4.7
__jsvalue __jsnum_pt_toPrecision(__jsvalue *this_number, __jsvalue *precision);

int32_t __js_str2num2(__jsstring *, bool &, bool);
__jsvalue __js_str2double(__jsstring *, bool &);
__jsvalue __js_str2double2(__jsstring *, bool &);
bool __is_double_no_decimal(double);

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

#include <sstream>
#include <string>
#include <iomanip>
#include <cinttypes>
#include "jsnum.h"
#include "jsvalueinline.h"
#include "jsobjectinline.h"
#include <cstring>
#include <cstdbool>
#include <cstdlib>
#include <cstdio>
#include <cmath>
#include "jstycnv.h"
#include "jsstring.h"
#include "vmmemory.h"

double __js_toDecimal(uint16_t *start, uint16_t *end, int base, uint16_t **pos) {
  MAPLE_JS_ASSERT(start <= end);
  MAPLE_JS_ASSERT(2 <= base && base <= 36);

  uint16_t *cur = start;
  double d = 0;
  for (; cur < end; cur++) {
    int32_t digit;
    uint16_t c = *cur;
    if ('0' <= c && c <= '9') {
      digit = c - '0';
    } else if ('a' <= c && c <= 'z') {
      digit = c - 'a' + 10;
    } else if ('A' <= c && c <= 'Z') {
      digit = c - 'A' + 10;
    } else {
      break;
    }
    if (digit >= base) {
      break;
    }
    d = d * base + digit;
  }

  *pos = cur;
  return d;
}

double __js_wchartoDecimal(wchar_t *start, wchar_t *end, int base, wchar_t **pos) {
  MAPLE_JS_ASSERT(start <= end);
  MAPLE_JS_ASSERT(2 <= base && base <= 36);

  wchar_t *cur = start;
  double d = 0;
  for (; cur < end; cur++) {
    int32_t digit;
   wchar_t c = *cur;
    if ('0' <= c && c <= '9') {
      digit = c - '0';
    } else if ('a' <= c && c <= 'z') {
      digit = c - 'a' + 10;
    } else if ('A' <= c && c <= 'Z') {
      digit = c - 'A' + 10;
    } else {
      break;
    }
    if (digit >= base) {
      break;
    }
    d = d * base + digit;
  }

  *pos = cur;
  return d;
}

uint64_t __js_strtod(uint16_t *start, uint16_t *end, uint16_t **pos) {
  size_t i;
  char cbuf[64];
  char *cstr, *estr;
  uint64_t val;

  size_t length = end - start;

  /* Use cbuf to avoid malloc */
  if (length >= sizeof(cbuf)) {
    cstr = (char *)VMMallocNOGC(length + 1);
    if (!cstr) {
      return false;
    }
  } else {
    cstr = cbuf;
  }

  for (i = 0; i != length; i++) {
    if (start[i] >> 8) {
      break;
    }
    cstr[i] = (char)start[i];
  }
  cstr[i] = 0;

#if HAVE_STRTOD
  if(strncasecmp(cstr, "inf", 3) == 0) {
      val = 0;
      estr = cstr;
  } else
      val = strtod(cstr, &estr);
#else
  val = (uint64_t)0;
  estr = cstr;
#endif

  if (cstr != cbuf) {
    VMFreeNOGC(cstr, length + 1);
  }
  *pos = start + (estr - cstr);

  return val;
}

bool __js_chars2num(uint16_t *chars, int32_t length, uint64_t *result) {
  if (length == 0) {
    *result = 0;
    return true;
  }

  if (length == 1) {
    uint16_t c = chars[0];
    if ('0' <= c && c <= '9') {
      *result = c - '0';
    } else if (__js_isSpace(c)) {
      *result = 0;
    } else {
      return false;
    }
    return true;
  }

  uint16_t *start = chars;
  uint16_t *end = chars + length;
  uint16_t *cur = __js_skipSpace(start, end);
  uint16_t *endcur = NULL;
  uint64_t val;

  /* hex to decimal.  */
  if (cur[0] == '0' && (cur[1] == 'x' || cur[1] == 'X')) {
    val = __js_toDecimal(cur + 2, end, 16, &endcur);
  } else {
    val = __js_strtod(cur, end, &endcur);
  }
  if (val == 0x8000000000000000) //NaN
    return false;

  if (__js_skipSpace(endcur, end) != end) {
    return false;
  } else {
    *result = val;
    return true;
  }
}

static uint32_t inline __js_get_char_without_leading_spaces(__jsstring *str, wchar_t chars[], uint32_t len) {
  if (len == 0)
    return 0;
  uint32_t i, j = 0;
  bool found_non_space = false;
  if (!__jsstr_is_ascii(str)) { //UNICODE
    for (i = 0; i < len; i++) {
      wint_t c = (wint_t)__jsstr_get_char(str, i);
      if (!found_non_space) {
        if (!__is_UnicodeSpace(c)) {
          found_non_space = true;
          chars[j++] = c;
        }
      } else {
        chars[j++] = c;
      }
    }
  } else {
    for (i = 0; i < len; i++) {
      char c = __jsstr_get_char(str, i);
      if (!found_non_space) {
        if (!isspace(c) && c != (char)0xa0) {
          found_non_space = true;
          chars[j++] = (wchar_t)c;
        }
      } else {
        chars[j++] = (wchar_t)c;
      }
    }
  }
  chars[j] = '\0';
  return j;
}

__jsvalue __js_str2double(__jsstring *str, bool &isNum) {
  isNum = true;
  uint32_t len = __jsstr_get_length(str);
  wchar_t chars[len+1];
  len = __js_get_char_without_leading_spaces(str, chars, len);
  if (len == 0) {
    return __number_value(0);
  }
/*
  try {
    double n = atof(chars);
    return __double_value(n);
  } catch (...) {
    isNum = false;
    return __nan_value();
  }
*/
  if ((len == 1 && chars[0] == '0') ||
      (len == 2 && chars[0] == '+' && chars[1] == '0') ||
          (len == 3 &&
            chars[0] == '0' &&
            (chars[1] == 'x' || chars[1] == 'X') &&
            chars[2] == '0')) {
    return __number_value(0);
  } else if (len == 2 && chars[0] == '-' && chars[1] == '0')
    return __double_value(0); //-0

  double n;
  /* hex to decimal.*/
  if (len > 1 && (chars[0] == '0' && (chars[1] == 'x' || chars[1] == 'X'))) {
    wchar_t *cur = chars;
    wchar_t *end = chars + len;
    wchar_t *endcur = NULL;
    cur += 2;
    n = __js_wchartoDecimal(cur, end, 16, &endcur);
    isNum = (endcur == cur) ? false : true;
    return __double_value(n);
  }

  n = wcstod(chars, NULL);
  if (n == 0.0 || std::isnan(n)) {
    isNum = false;
    return __nan_value();
  } else if (std::isinf(n)) {
    if (n > 0)
      return __number_infinity();
    else
      return __number_neg_infinity();
  } else
  return __double_value(n);
}

// for parseFloat
__jsvalue __js_str2double2(__jsstring *str, bool &isNum) {
  isNum = true;
  uint32_t len = __jsstr_get_length(str);
  wchar_t chars[len+1];
  uint32_t i;
  len = __js_get_char_without_leading_spaces(str, chars, len);
  if (len == 0) {
    return __nan_value();  // ecma 15.1.2.3.3
  }

  //  Check Infinity
  if (wcsncmp(chars, L"Infinity", 8) == 0 ||
      wcsncmp(chars, L"+Infinity", 9) == 0) {
    return __number_infinity();
  } else if (wcsncmp(chars, L"-Infinity", 9) == 0) {
    return __number_neg_infinity();
  }

  bool firstDot = false;
  bool hasDigit = false;
  for (i = 0; i < len; i++) {
    if (i == 0 &&
        chars[i] != '+' &&
        chars[i] != '-' &&
        chars[i] != '.' &&
        !isdigit(chars[i])) {
      // 1st non blank char must be +/-/./[0..9]
      return __nan_value();
    }
    if (i == 1 &&
        chars[i] != '.' &&
        !isdigit(chars[i]) &&
        (chars[0] == '+' || chars[0] == '-')) {
      // if 1st char is +/- next char must be '.' or [0..9]
      return __nan_value();
    }
    if (chars[i] != '+' &&
        chars[i] != '-' &&
        chars[i] != '.' &&
        chars[i] != 'e' &&
        chars[i] != 'E' &&
        (chars[i] < '0' || chars[i] > '9')) {
      chars[i] = '\0';
      len = i;
      // stop parsing on chars outside allowed set
      break;
    }
    if (isdigit(chars[i])) {
      hasDigit = true;
    }
    if (chars[i] == 'e' || chars[i] == 'E') {
      if (!hasDigit) {
        // need at least 1 digit before exponent
        return __nan_value();
      }
      if (i+1 < len &&
          chars[i+1] != '+' &&
          chars[i+1] != '-' &&
          !isdigit(chars[i+1])) {
        // char after exponent must be +/-/[0..9]
        chars[i] = '\0';
        len = i;
        break;
      }
    }
    if (chars[i] == '.') {
      if (!firstDot) {
        firstDot = true;
      } else {
        chars[i] = '\0';
        len = i;
        break;
      }
    }
  }
  chars[i] = '\0';
/*
  try {
    double n = atof(chars);
    return __double_value(n);
  } catch (...) {
    isNum = false;
    return __nan_value();
  }
*/
  if (len == 0) {
    isNum = false;
    return __nan_value();
  }
  if (len == 1) {
    switch(chars[0]) {
      case '.':
      case '+':
      case '-':
        isNum = false;
        return __nan_value();
      default:
        break;
    }
  }
  double n = wcstod(chars, NULL);
  if (std::isnan(n)) {
    isNum = false;
    return __nan_value();
  } else
    return __double_value(n);
}

int32_t __js_str2num2(__jsstring *str, bool &isNum, bool isCvtNeg) {
  uint32_t len = __jsstr_get_length(str);
  if (len == 0) {
    isNum = false;
    return 0;
  }
  uint16_t chars[len];
  for (uint32_t i = 0; i < len; i++) {
    chars[i] = __jsstr_get_char(str, i);
  }
  if (!isCvtNeg && (chars[0] == '-' || (chars[0] == '0' && len > 1))) { // we would consider "-2", "02" to be string
    isNum = false;
    return 0;
  }
  uint64_t result;
  isNum = __js_chars2num(chars, len, &result);
  return result;
}

uint64_t __js_str2num(__jsstring *str) {
  uint32_t len = __jsstr_get_length(str);
  uint16_t chars[len];
  for (uint32_t i = 0; i < len; i++) {
    chars[i] = __jsstr_get_char(str, i);
  }
  uint64_t result;
  if (__js_chars2num(chars, len, &result)) {
    return result;
  }
  return 0;
}

double __js_str2num_base_x(__jsstring *str, int32_t base, bool &isNum) {
  uint32_t len = __jsstr_get_length(str);
  uint16_t chars[len];
  for (uint32_t i = 0; i < len; i++) {
    chars[i] = __jsstr_get_char(str, i);
  }
  uint16_t *start = chars;
  uint16_t *end = chars + len;
  uint16_t *cur = __js_skipSpace(start, end);
  uint16_t *endcur = NULL;
  double val;
  bool isNeg = false;

  if (cur[0] == '-') {
    isNeg = true;
    cur++;
  } else if (cur[0] == '+')
      cur++;
  /* hex to decimal.  */
  if (cur[0] == '0' && (cur[1] == 'x' || cur[1] == 'X')) {
    cur += 2;
    val = __js_toDecimal(cur, end, 16, &endcur);
  } else {
    val = __js_toDecimal(cur, end, base, &endcur);
  }
  isNum = (endcur == cur) ? false : true;
  if (isNeg && val != 0)
    return(-val);
  else
    return val;
}

__jsstring *__jsnum_integer_encode(int64_t value, int base) {
  __jsstring *str = NULL;
  int64_t val = value;
  long long clval = 1;
  int length = 0;
  if (value == 0) {
    length = 1;
  } else {
    while (clval <= (long long)value) {
      clval *= base;
      length++;
    }
  }

  char cbuf[64];
  char *buf;

  if (length > (int)sizeof(cbuf)) {
    buf = (char *)VMMallocNOGC((length + 1) * sizeof(char));
  } else {
    buf = cbuf;
  }
  MIR_ASSERT(buf);

  buf[length] = 0;
  int pos = length;
  do {
    int64_t quot = val / base;
    buf[--pos] = "0123456789abcdefghijklmnopqrstuvwxyz"[val - quot * base];
    val = quot;
  } while (val != 0);

  str = __jsstr_new_from_char(buf + pos);

  if (buf != cbuf) {
    VMFreeNOGC(buf, (length + 1) * sizeof(char));
  }

  return str;
}

static int __number_on_countu(int number, int count) {
    int result = 1;
    while(count-- > 0)
        result *= number;

    return result;
}


void __double_to_string(double f, char r[]) {
  long long int length, length2, i, number, position, sign;
  double number2;

  sign = -1;   // -1 == positive number
  if (f < 0)
  {
    sign = '-';
    f *= -1;
  }

  number2 = f;
  number = f;
  length = 0;  // Size of decimal part
  length2 = 0; // Size of tenth

  /* Calculate length2 tenth part */
  while( (number2 - (double)number) != 0.0 && !((number2 - (double)number) < 0.0) )
  {
    number2 = f * (__number_on_countu(10.0, length2 + 1));
    number = number2;

    length2++;
  }

  /* Calculate length decimal part */
  for (length = (f > 1) ? 0 : 1; f > 1; length++)
    f /= 10;

  position = length;
  length = length + 1 + length2;
  number = number2;
  if (sign == '-')
  {
    length++;
    position++;
  }

  for (i = length; i >= 0 ; i--)
  {
    if (i == (length))
        r[i] = '\0';
    else if(i == (position))
        r[i] = '.';
    else if(sign == '-' && i == 0)
        r[i] = '-';
    else
    {
        r[i] = (number % 10) + '0';
        number /=10;
    }
  }
}

// ecma 15.7.4.2
__jsvalue __jsnum_pt_toString(__jsvalue *this_number, __jsvalue *radix) {
  if (__is_double(this_number)) {
    return __string_value(__js_DoubleToString(__jsval_to_double(this_number)));
  }
  int base;
  if (__is_undefined(radix)) {
    base = 10;
  } else {
    base = __js_ToInt32(radix);
  }

  if (base < 2 || base > 36) {
    MAPLE_JS_RANGEERROR_EXCEPTION();
  }

  __jsstring *signstr, *istr, *ret;
  signstr = istr = ret = NULL;
  int64_t val;
#ifdef MACHINE64
  if (this_number->s.tag == JSTYPE_OBJECT) {
    __jsobject *obj = (__jsobject *)memory_manager->GetRealAddr(this_number->s.payload.obj);
    if(obj->object_class == JSNUMBER) {
      if (obj->object_type == JSSPECIAL_NUMBER_OBJECT) {
        // NaN or Infinity
        return __string_value(obj->shared.prim_string);
      }
      val = obj->shared.prim_number;
    } else if (obj->object_class == JSDOUBLE) {
      return __string_value(__js_DoubleToString(obj->shared.primDouble));
    } else {
      val = __js_ToNumber64(this_number);
    }
#else
  if (this_number->s.tag == JSTYPE_OBJECT && this_number->s.payload.obj->object_class == JSNUMBER) {
    val = this_number->s.payload.obj->shared.prim_number;
#endif
  } else {
    val = (int64_t)__js_ToNumber(this_number);
  }

  if (val < 0) {
    signstr = __jsstr_get_builtin(JSBUILTIN_STRING_MINUS_CHAR);
    val = 0 - val;
  } else {
    signstr = __jsstr_get_builtin(JSBUILTIN_STRING_EMPTY);
  }

  istr = __jsnum_integer_encode(val, base);
  ret = __jsstr_concat_2(signstr, istr);
  memory_manager->RecallString(istr);
  return __string_value(ret);
}

// ecma 15.7.4.3
// Locale-specific, implement it if really needed.
__jsvalue __jsnum_pt_toLocaleString(__jsvalue *this_number) {
  MAPLE_JS_ASSERT(false && "NIY: Number.prototype.toLocaleString");
  return __undefined_value();
}

// ecma 15.7.4.4
__jsvalue __jsnum_pt_valueOf(__jsvalue *this_number) {
  if (!__is_number_object(this_number) && ! __is_number(this_number)
                        && !__is_double_object(this_number) && !__is_double(this_number)) {
    MAPLE_JS_TYPEERROR_EXCEPTION();
  }
  if (__is_number(this_number) || __is_double(this_number)) {
    return *this_number;
  } else {
    __jsobject *obj = __jsval_to_object(this_number);
    if (__is_number_object(this_number)) {
      if (obj->object_type == JSSPECIAL_NUMBER_OBJECT) {
        // NaN or Infinity
        bool isNum;
        return __js_str2double(obj->shared.prim_string, isNum);
      }
      int32_t n = obj->shared.prim_number;
      return __number_value(n);
    } else {
      return __double_value(obj->shared.primDouble);
    }
  }
}

// use ostringstream::setprecision to format double value
static inline __jsstring* __jsdouble_toFixed(double val, int fracdigits) {
  std::ostringstream sObj;
  sObj << std::fixed;
  sObj << std::setprecision(fracdigits);
  sObj << val;
  return __jsstr_new_from_char(sObj.str().c_str());
}

// ecma 15.7.4.5, return a string Jsvalue
__jsvalue __jsnum_pt_toFixed(__jsvalue *this_number, __jsvalue *fracdigit) {
  MAPLE_JS_EXCEPTION((__is_nan(this_number) || __is_number_object(this_number) || __is_number(this_number) ||
                      __is_double_object(this_number) || __is_double(this_number)) && "TypeError!");

  int32_t f;
  if (__is_undefined(fracdigit)) {
    f = 0;
  } else {
    f = __js_ToInt32(fracdigit);
  }
  if (f < 0 || f > 20 || __is_infinity(fracdigit)) {
    // throw exception
    MAPLE_JS_RANGEERROR_EXCEPTION();
  }
  // if x is NaN, set string is NaN
  if (__is_nan(this_number)) {
    return __string_value(__jsstr_get_builtin(JSBUILTIN_STRING_NAN));
  }
  // TODO:: if this_number > 10^21, call toString() and return

  // use setprecision to format data
  double dval;
  __jsstring *signstr, *dstr, *ret;
  signstr = dstr = ret = NULL;

  // get double value from this_number
  if (__is_double(this_number)) {
    dval = memory_manager->GetF64FromU32(this_number->s.payload.u32);
  } else if (__is_number(this_number)) {
    dval = (double)__js_ToNumber(this_number);
  } else {
    // deal with object
    __jsobject *obj = __jsval_to_object(this_number);
    if (__is_double_object(this_number)) {
      dval = obj->shared.primDouble;
    } else if (obj->object_type == JSSPECIAL_NUMBER_OBJECT) {
      // NaN or Infinity
      return __string_value(__jsstr_get_builtin(JSBUILTIN_STRING_NAN));
    } else {
      dval = (double)obj->shared.prim_number;
    }
  }
  if (fabs(dval) >= 1e21)
    return __string_value(__js_DoubleToString(dval));

  // set sign
  if (dval < 0.0) {
    dval = 0.0 - dval;
    signstr = __jsstr_get_builtin(JSBUILTIN_STRING_MINUS_CHAR);
  } else {
    signstr = __jsstr_get_builtin(JSBUILTIN_STRING_EMPTY);
  }
  dstr = __jsdouble_toFixed(dval, f);
  ret = __jsstr_concat_2(signstr, dstr);
  memory_manager->RecallString(dstr);
  return __string_value(ret);
}

__jsvalue __jsnum_pt_toExponential(__jsvalue *this_number, __jsvalue *fractdigit) {
  MAPLE_JS_ASSERT(false && "NIY: Number.prototype.toExponential");
  return __undefined_value();
}

__jsvalue __jsnum_pt_toPrecision(__jsvalue *this_number, __jsvalue *precision) {
  MAPLE_JS_ASSERT(false && "NIY: Number.prototype.toPrecision");
  return __undefined_value();
}

bool __is_double_no_decimal(double x) {
  return (fabs(x - (double)(int64_t)x) < NumberMinValue);
}


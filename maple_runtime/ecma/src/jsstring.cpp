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
#include <cstring>
#include <cstdio>
#include <cmath>
#include <regex>
#include <ctype.h>
#include <string>
#include <codecvt>
#include "jsvalue.h"
#include "jsvalueinline.h"
#include "jsobject.h"
#include "jsobjectinline.h"
#include "jsstring.h"
#include "jsnum.h"
#include "vmmemory.h"
#include "jsarray.h"
#include "jsobject.h"
#include "jsregexp.h"

char NO_BREAK_SPACE = 0x00A0;
uint32_t BYTE_ORDER_MARK = 0xFEFF;
uint32_t LINE_SEPARATOR = 0x2028;
uint32_t PARAGRAPH_SEPARATOR = 0x2029;

// Helper functions for internal use.

// static inline bool __jsstr_is_const(__jsstring *str) {
//  return (((uint8_t *)str)[0] & JSSTRING_GEN) == 0;
// }

static inline bool __jsstr_is_builtin(__jsstring *str) {
  return (((uint8_t *)str)[0] & JSSTRING_BUILTIN) != 0;
}

static std::wstring __jsstr_to_wstring(__jsstring *s, int offset = 0) {
  int len = __jsstr_get_length(s) - offset;
  if (len <= 0) {
    std::wstring str32;
    return str32;
  }
  if (__jsstr_is_ascii(s)) {
    std::string str8;
    str8.assign(s->x.ascii + offset, len);
    std::wstring_convert<std::codecvt_utf8<wchar_t>,wchar_t> cv;
    return cv.from_bytes(str8);
  } else {
    wchar_t buf[len + 1];
    for (uint32_t i = 0; i < len; i++) {
      buf[i] = (wchar_t)((uint16_t *)s)[2 + i + offset];
    }
    buf[len] = 0;
    std::wstring str32;
    str32.assign(buf, (size_t)len);
    return str32;
  }
}

std::wstring whitespaces = L"\u0009\u000C\u0020\u00A0\u000B\u000A\u000D\u2028\u2029\u1680\u2000\u2001\u2002\u2003\u2004\u2005\u2006\u2007\u2008\u2009\u200A\u202F\u205F\u3000\uFEFF";
bool __is_UnicodeSpace(wchar_t c) {
  if (whitespaces.find(c) != std::wstring::npos)
    return true;
  else
    return false;
}

void __jsstr_set_char(__jsstring *str, uint32_t index, uint16_t ch) {
  if (__jsstr_is_ascii(str)) {
    ((uint8_t *)str)[4 + index] = (uint8_t)ch;
  } else {
    ((uint16_t *)str)[2 + index] = (uint16_t)ch;
  }
}

uint32_t __jsstr_get_bytesize(__jsstring *str) {
  uint32_t length = __jsstr_get_length(str);
  uint32_t uni_size = __jsstr_is_ascii(str) ? 1 : 2;
  return (uni_size * length + 4);
}

uint16_t __jsstr_get_char(__jsstring *str, uint32_t index) {
  if (__jsstr_is_ascii(str)) {
    uint8_t *chars;
    chars = &((uint8_t *)str)[4];
    return chars[index];
  } else {
    uint16_t *chars;
    chars = &((uint16_t *)str)[2];
    return chars[index];
  }
}

void __jsstr_print(__jsstring *str, FILE *stream) {
  uint32_t length = __jsstr_get_length(str);
  if(__jsstr_is_ascii(str))
      std::fprintf(stream, "%.*s", length, str->x.ascii);
  else {
      std::u16string u16str;
      u16str.assign(reinterpret_cast<std::u16string::const_pointer>(str->x.utf16), length);
      std::string u8str = std::wstring_convert<std::codecvt_utf8_utf16<char16_t>,
          char16_t>{}.to_bytes(u16str);
      std::fprintf(stream, "%s", u8str.c_str());
  }
}

void __jsstr_dump(__jsstring *str) {
#if MIR_FEATURE_FULL | MIR_DEBUG
  uint32_t length = __jsstr_get_length(str);
  if (__jsstr_is_ascii(str)) {
    printf("STRING_CLASS: ascii chars\n");
  } else {
    printf("STRING_CLASS: unicode chars\n");
  }
  printf("STRING_LENGTH: %d\n", length);
  printf("STRING_CONTEXT: ");
  __jsstr_print(str);
  printf("\n");
#endif  // MIR_FEATURE_FULL
}

__jsstring *__js_new_string_internal(uint32_t length, bool is_unicode) {
  MAPLE_JS_ASSERT(length < 0x10000 && "Donot support too long string");
  uint32_t unit_size = is_unicode ? 2 : 1;
  uint32_t total_size = (unit_size * length + 4);
  __jsstring *str = (__jsstring *)VMMallocGC(total_size, MemHeadJSString, false);
  __jsstring_type cl;
  if (is_unicode)
      cl = (__jsstring_type)(JSSTRING_UNICODE | JSSTRING_GEN);
  else
      cl = JSSTRING_GEN;
  str->kind = cl;
  str->builtin = (__jsbuiltin_string_id)0;
  str->length = (uint16_t)length;
  return str;
}

static const char *builtin_strings[JSBUILTIN_STRING_LAST] = {
#define JSBUILTIN_STRING_DEF(id, length, builtin_string) builtin_string,
#include "jsbuiltinstrings.inc.h"
#undef JSBUILTIN_STRING_DEF
};

__jsstring *__jsstr_get_builtin(__jsbuiltin_string_id id) {
  MAPLE_JS_ASSERT(id < JSBUILTIN_STRING_LAST);
  return (__jsstring *)builtin_strings[id];
}

__jsvalue __js_new_string(uint16_t *data) {
  return __string_value((__jsstring *)data);
}

bool __jsstr_equal_to_builtin(__jsstring *str, __jsbuiltin_string_id id) {
  if (__jsstr_is_builtin(str)) {
    return (__jsbuiltin_string_id)((uint8_t *)str)[1] == id;
  } else {
    return __jsstr_equal(str, __jsstr_get_builtin(id));
  }
}

bool __jsstr_ne(__jsstring *str1, __jsstring *str2) {
  return !__jsstr_equal(str1, str2);
}

bool __jsstr_equal(__jsstring *str1, __jsstring *str2, bool checknumber) {
  MAPLE_JS_ASSERT(str1 && str2);

  if (str1 == str2) {
    return true;
  }
  if (__jsstr_is_builtin(str1) && __jsstr_is_builtin(str2)) {
    return false;
  }

  // FIXME: might effect performance since it will cause some comparison by content of the string
  // if str1 and str2 are in different files, their content can be the same but address is different
  // that would cause plugin file down
  // if (__jsstr_is_const (str1) && __jsstr_is_const (str2))
  //  return str1 == str2;

  uint32_t length = __jsstr_get_length(str1);
  if (length != __jsstr_get_length(str2)) {
    return false;
  }
  for (uint32_t i = 0; i < length; i++) {
    if (__jsstr_get_char(str1, i) != __jsstr_get_char(str2, i)) {
      return false;
    }
  }

  // convert to double to compare the value
  if (checknumber) {
    bool isDouble;
    __jsvalue dlen1 = __js_str2double(str1, isDouble);
    if (isDouble) {
      __jsvalue dlen2 = __js_str2double(str2, isDouble);
      if (isDouble) {
        return (__jsval_to_double(&dlen1) == __jsval_to_double(&dlen2));
      }
    }
  }
  return true;
}

void __jsstr_copy_ascii(__jsstring *to, uint32_t to_index, __jsstring *from) {
  for (uint32_t i = 0; i < __jsstr_get_length(from); i++) {
    ((uint8_t *)to)[4 + to_index + i] = ((uint8_t *)from)[4 + i];
  }
}

void __jsstr_copy_unicode(__jsstring *to, uint32_t to_index, __jsstring *from) {
  for (uint32_t i = 0; i < __jsstr_get_length(from); i++) {
    ((uint16_t *)to)[2 + to_index + i] = ((uint16_t *)from)[2 + i];
  }
}

void __jsstr_copy_ascii_to_unicode(__jsstring *to, uint32_t to_index, __jsstring *from) {
  for (uint32_t i = 0; i < __jsstr_get_length(from); i++) {
    ((uint16_t *)to)[2 + to_index + i] = ((uint8_t *)from)[4 + i];
  }
}

void __jsstr_copy(__jsstring *to, uint32_t to_index, __jsstring *from) {
  for (uint32_t i = 0; i < __jsstr_get_length(from); i++) {
    __jsstr_set_char(to, to_index + i, __jsstr_get_char(from, i));
  }
}

__jsstring *__jsstr_extract(__jsstring *from, uint32_t from_index, uint32_t length) {
  __jsstring *res;
  if (__jsstr_is_ascii(from)) {
    res = __js_new_string_internal(length, false);
  } else {
    res = __js_new_string_internal(length, true);
  }
  for (uint32_t i = 0; i < length; i++) {
    __jsstr_set_char(res, i, __jsstr_get_char(from, from_index + i));
  }
  return res;
}

__jsstring *__jsstr_append_char(__jsstring *str, const __jschar ch) {
  uint32_t len = __jsstr_get_length(str);
  bool is_unicode;
  if (__jsstr_is_ascii(str)) {
    is_unicode = false;
  } else {
    is_unicode = true;
  }
  __jsstring *str1 = __js_new_string_internal(len + 1, is_unicode);
  __jsstr_copy(str1, 0, str);
  memory_manager->RecallString(str);
  __jsstr_set_char(str1, len, ch);
  return str1;
}

__jsstring *__jsstr_concat_2(__jsstring *left, __jsstring *right) {
  uint32_t leftlen = __jsstr_get_length(left);
  uint32_t rightlen = __jsstr_get_length(right);
  uint32_t wholelen = leftlen + rightlen;
  __jsstring *res;
  if (__jsstr_is_ascii(left) && __jsstr_is_ascii(right)) {
    res = __js_new_string_internal(wholelen, false);
    __jsstr_copy_ascii(res, 0, left);
    __jsstr_copy_ascii(res, leftlen, right);
  } else {
    res = __js_new_string_internal(wholelen, true);
    if (__jsstr_is_ascii(left))
      __jsstr_copy_ascii_to_unicode(res, 0, left);
    else
      __jsstr_copy_unicode(res, 0, left);
    if (__jsstr_is_ascii(right))
      __jsstr_copy_ascii_to_unicode(res, leftlen, right);
    else
      __jsstr_copy_unicode(res, leftlen, right);
  }
  return res;
}

__jsstring *__jsstr_concat_3(__jsstring *str1, __jsstring *str2, __jsstring *str3) {
  uint32_t len1 = __jsstr_get_length(str1);
  uint32_t len2 = __jsstr_get_length(str2);
  uint32_t len3 = __jsstr_get_length(str3);
  uint32_t wholelen = len1 + len2 + len3;
  __jsstring *res;
  if (__jsstr_is_ascii(str1) && __jsstr_is_ascii(str2) && __jsstr_is_ascii(str3)) {
    res = __js_new_string_internal(wholelen, false);
  } else {
    res = __js_new_string_internal(wholelen, true);
  }
  __jsstr_copy(res, 0, str1);
  __jsstr_copy(res, len1, str2);
  __jsstr_copy(res, len1 + len2, str3);
  return res;
}

int32_t __jsstr_compare(__jsstring *lhs, __jsstring *rhs) {
  uint32_t llength = __jsstr_get_length(lhs);
  uint32_t rlength = __jsstr_get_length(rhs);

  uint32_t n = llength < rlength ? llength : rlength;

  for (uint32_t i = 0; i < n; i++) {
    int32_t cmp = __jsstr_get_char(lhs, i) - __jsstr_get_char(rhs, i);
    if (cmp) {
      return cmp;
    }
  }

  return (int32_t)(llength - rlength);
}

void __jsstr_copy_char2string(__jsstring *str, const char *src) {
  uint32_t length = __jsstr_get_length(str);
  for (uint32_t i = 0; i < length; i++) {
    __jsstr_set_char(str, i, (uint16_t)src[i]);
  }
  return;
}

__jsstring *__jsstr_new_from_char(const char *ch) {
  uint32_t length = strlen(ch);
  __jsstring *str = __js_new_string_internal(length, false);
  __jsstr_copy_char2string(str, ch);
  return str;
}

bool __js_isSpace(uint16_t ch) {
  /*
   * IsSpace checks if some character is included in the merged set
   * of WhiteSpace and LineTerminator, specified by ES5 7.2 and 7.3.
   * We combined them, because in practice nearly every
   * calling function wants this, except some code in the tokenizer.
   *
   * We use a lookup table for ASCII-7 characters, because they are
   * very common and must be handled quickly in the tokenizer.
   * NO-BREAK SPACE is supposed to be the most common character not in
   * this range, so we inline this case, too.
   */
  /* Ecma 7.2/7.3 : Whitespace chars and Line Terminator chars. */
  if ((ch >= 9 && ch <= 13) || ch == 32 || ch == NO_BREAK_SPACE || ch == BYTE_ORDER_MARK || ch == LINE_SEPARATOR ||
      ch == PARAGRAPH_SEPARATOR || __is_UnicodeSpace(ch)) {
    return true;
  }

  // Ecma 7.2: Todo: Other category .Zs. Any other Unicode "space separator"
  return false;
}

uint16_t *__js_skipSpace(uint16_t *s, uint16_t *end) {
  MAPLE_JS_ASSERT(s <= end);
  while (s < end && __js_isSpace(*s)) {
    s++;
  }
  return s;
}

/*  Ecma 15.5.3.2 String.fromCharCode ( [ char0 [ , char1 [ , бн ] ] ] ) */
__jsvalue __jsstr_fromCharCode(__jsvalue *this_string, __jsvalue *array, uint32_t size) {
  __jsstring *str = __js_new_string_internal(size, true);
  for (uint32_t i = 0; i < size; i++) {
    __jsvalue value = array[i];
    __jsstr_set_char(str, i, __js_ToUint16(&value));
  }
  return __string_value(str);
}

// ecma 15.5.4.2
__jsvalue __jsstr_toString(__jsvalue *this_string) {
  if (__is_js_object(this_string)) {
    __jsobject *obj = __jsval_to_object(this_string);
    if (obj->object_class != JSSTRING) {
      MAPLE_JS_TYPEERROR_EXCEPTION();
    }
    if (obj->shared.prim_string == 0)
      return __string_value(__jsstr_get_builtin(JSBUILTIN_STRING_EMPTY));
    else
      return __string_value(obj->shared.prim_string);
  } else {
    if (!__is_string(this_string)) {
      MAPLE_JS_TYPEERROR_EXCEPTION();
    }
    return *this_string;
  }
}

// ecma 15.5.4.3
// for a String object, the toString method happens to return the same thing as the valueOf method
__jsvalue __jsstr_valueOf(__jsvalue *this_string) {
  return __jsstr_toString(this_string);
}

/*  Ecma 15.5.4.4 String.prototype.charAt (pos) */
__jsvalue __jsstr_charAt(__jsvalue *this_string, __jsvalue *pos) {
  // step 1:
  CheckObjectCoercible(this_string);
  // step 2:
  __jsstring *str = __js_ToString(this_string);
  // step 3:
  int32_t d = 0;
  if (!__is_nan(pos)) {
    d = __js_ToInteger(pos);
  }
  uint32_t size = __jsstr_get_length(str);
  // step 5:
  if (d < 0 || d >= (int32_t)size) {
    return __string_value(__jsstr_get_builtin(JSBUILTIN_STRING_EMPTY));
  }
  // step 6:
  __jsstring *str1 = __js_new_string_internal(1, true);
  __jsstr_set_char(str1, 0, __jsstr_get_char(str, (uint32_t)d));
  return __string_value(str1);
}

/*  Ecma 15.5.4.5 String.prototype.charCodeAt (pos) */
__jsvalue __jsstr_charCodeAt(__jsvalue *this_string, __jsvalue *idx) {
  // step 1:
  CheckObjectCoercible(this_string);
  // step 2:
  __jsstring *str = __js_ToString(this_string);
  __jsvalue v = {};
  // step 3:
  int32_t d = __js_ToInteger(idx);
  uint32_t size = __jsstr_get_length(str);
  // step 4:
  if (d < 0 || d >= (int32_t)size)
  // ecma return NaN here.
  {
    return __nan_value();
  }
  // step 5:
  __set_number(&v, (int32_t)(__jsstr_get_char(str, (uint32_t)d)));
  return v;
}

/*  Ecma 15.5.4.6 String.prototype.concat ( [ string1 [ , string2 [ , ... ] ] ] ) */
__jsvalue __jsstr_concat(__jsvalue *this_string, __jsvalue *arr, uint32_t size) {
  // step 1:
  CheckObjectCoercible(this_string);
  // step 2:
  __jsstring *s[size + 1];
  s[0] = __js_ToString(this_string);
  // step 3-6:
  if (size == 0) {
    return __string_value(s[0]);
  }
  uint32_t len = __jsstr_get_length(s[0]);
  for (uint32_t i = 0; i < size; i++) {
    __jsvalue val = __is_none(&arr[i]) ? __undefined_value() : arr[i];
    s[i + 1] = __js_ToString(&val);
    len += __jsstr_get_length(s[i + 1]);
  }
  __jsstring *str = __js_new_string_internal(len, true);
  len = 0;
  for (uint32_t i = 0; i <= size; i++) {
    uint32_t l = __jsstr_get_length(s[i]);
    for (uint32_t j = 0; j < l; j++) {
      __jschar c = __jsstr_get_char(s[i], j);
      __jsstr_set_char(str, len++, c);
    }
  }
  return __string_value(str);
}

// Ecma 15.5.4.7 String.prototype.indexOf (searchString, position)
__jsvalue __jsstr_indexOf(__jsvalue *this_string, __jsvalue *search, __jsvalue *pos) {
  // step 1:
  CheckObjectCoercible(this_string);
  // step 2:
  __jsstring *s = __js_ToString(this_string);
  // step 3:
  __jsstring *ss = __js_ToString(search);
  int32_t p = 0;
  // step 4:
  if (!__is_undefined(pos)) {
    p = __js_ToInt32(pos);
  }
  p = p > 0 ? p : 0;
  // step 5:
  uint32_t len = __jsstr_get_length(s);
  // step 6:
  uint32_t start = (uint32_t)p < len ? p : len;
  // step 7:
  uint32_t slen = __jsstr_get_length(ss);
  // step 8:
  uint32_t k = start;
  if (len >= slen) {
    for (k = start; k <= len - slen; k++) {
      uint32_t j = 0;
      for (; j < slen; j++) {
        if (__jsstr_get_char(s, k + j) != __jsstr_get_char(ss, j)) {
          break;
        }
      }
      if (j == slen) {
        return __number_value(k);
      }
    }
  }
  return __number_value(-1);
}

// Ecma 15.5.4.8 String.prototype.lastIndexOf (searchString, position)
__jsvalue __jsstr_lastIndexOf(__jsvalue *this_string, __jsvalue *search, __jsvalue *pos) {
  // step 1:
  CheckObjectCoercible(this_string);
  // step 2:
  __jsstring *s = __js_ToString(this_string);
  // step 3:
  __jsstring *ss = __js_ToString(search);
  // step 4:
  uint64_t p;
  // step 5: __js_ToInteger deal with NaN
  p = __js_ToInteger(pos);
  // step 6:
  uint32_t len = __jsstr_get_length(s);
  // step 7:
  p = p > 0 ? p : 0;
  uint32_t start = p < len ? p : len;
  // step 8:
  uint32_t slen = __jsstr_get_length(ss);
  // step 9:
  uint32_t k = start;
  int32_t res = -1;
  if (len >= slen) {
    for (k = start; k <= len - slen; k++) {
      uint32_t j = 0;
      for (; j < slen; j++) {
        if (__jsstr_get_char(s, k + j) != __jsstr_get_char(ss, j)) {
          break;
        }
      }
      if (j == slen) {
        res = k;
      }
    }
  }
  return __number_value(res);
}

// Ecma 15.5.4.9 String.prototype.localeCompare (that)
__jsvalue __jsstr_localeCompare(__jsvalue *this_string, __jsvalue *that) {
  // step 1:
  CheckObjectCoercible(this_string);
  // step 2:
  __jsstring *s = __js_ToString(this_string);
  // step 3:
  __jsstring *t = __js_ToString(that);

  // step 4;
  return __number_value(__jsstr_compare(s, t));
}

// Ecma 15.5.4.13 String.prototype.slice (start, end)
__jsvalue __jsstr_slice(__jsvalue *this_string, __jsvalue *start, __jsvalue *end) {
  // step 1:
  CheckObjectCoercible(this_string);
  // step 2:
  __jsstring *s = __js_ToString(this_string);
  // step 3:
  uint32_t len = __jsstr_get_length(s);
  ;
  // step 4:
  int32_t st;
  if (__is_infinity(start)) {
    if (__is_neg_infinity(start))
      st = 0;
    else
      st = len;
  } else
    st = __js_ToInteger(start);
  // step 5:
  int32_t en;
  if (__is_infinity(end)) {
    if (__is_neg_infinity(end))
      en = 0;
    else
      en = len;
  } else if (__is_undefined(end)) {
    en = len;
  } else {
    en = __js_ToInteger(end);
  }
  // step 6:
  uint32_t from;
  if (st < 0) {
    from = ((len + st) > 0) ? (len + st) : 0;
  } else {
    from = ((uint32_t)st > len) ? len : st;
  }
  // step 7:
  uint32_t to;
  if (en < 0) {
    to = ((len + en) > 0) ? (len + en) : 0;
  } else {
    to = ((uint32_t)en > len) ? len : en;
  }
  // step 8:
  uint32_t span = (to > from) ? (to - from) : 0;
  // step 9:
  __jsstring *str = __jsstr_extract(s, from, span);
  return __string_value(str);
}

static inline bool __is_regexp(__jsvalue *val) {
  return __is_js_object(val) && (__jsval_to_object(val))->object_class == JSREGEXP;
}

static inline void __jsstr_get_regexp_patten(std::wregex &pattern, __jsvalue *regexp, bool &global) {
  if (__is_regexp(regexp)) {
    __jsvalue val = __jsop_getprop_by_name(regexp, __jsstr_get_builtin(JSBUILTIN_STRING_SOURCE));
    __jsstring *pattern_str = __js_ToString(&val);

    val = __jsop_getprop_by_name(regexp, __jsstr_get_builtin(JSBUILTIN_STRING_GLOBAL));
    global = __js_ToBoolean(&val);

    val = __jsop_getprop_by_name(regexp, __jsstr_get_builtin(JSBUILTIN_STRING_IGNORECASE_UL));
    bool ignore_case = __js_ToBoolean(&val);

    std::regex::flag_type flags = std::regex::ECMAScript;
    if (ignore_case)
      flags |= std::regex::icase;
    pattern.assign((__jsstr_to_wstring(pattern_str)).c_str(), __jsstr_get_length(pattern_str), flags);
  }
}

// Ecma 15.5.4.14 SplitMatch
bool __jsstr_splitMatch(__jsstring *s, uint32_t *q, __jsvalue *separator, uint32_t *ret) {
  __jsstring *r = NULL;
  // step 1:
  if (__is_js_object(separator)) {
    __jsobject *obj = __jsval_to_object(separator);
    if (obj->object_class == JSREGEXP) {
      // step 2:
      std::wregex pattern;
      bool global;
      __jsstr_get_regexp_patten(pattern, separator, global);

      if (*q > __jsstr_get_length(s))
        return false;
      std::wstring str = __jsstr_to_wstring(s, *q);

      std::wsmatch sm;
      std::regex_search (str, sm, pattern);
      if (sm.size() > 0) {
        *ret = *q + sm.position(0) + sm.length(0);
        *q += sm.position(0);
        return true;
      } else {
        return false;
      }
    } else if (obj->object_class == JSSTRING) {
      r = obj->shared.prim_string;
    } else if (obj->object_class == JSOBJECT) {
      __jsvalue val = __object_internal_DefaultValue(obj, JSTYPE_STRING);
      if (__is_string(&val)) {
        r = __js_ToString(&val);
      }
      else {
        MAPLE_JS_TYPEERROR_EXCEPTION();
      }
    }
  } else {
    r = __js_ToString(separator);
  }
  uint32_t rl = __jsstr_get_length(r);
  // step 3:
  uint32_t sl = __jsstr_get_length(s);
  // step 4:
  if (*q + rl > sl)
    return false;
  // step 5:
  for (uint32_t i = 0; i < rl; i++) {
    __jschar sc = __jsstr_get_char(s, *q + i);
    __jschar rc = __jsstr_get_char(r, i);
    if (sc != rc) {
      return false;
    }
  }
  *ret = *q + rl;
  return true;
}

// Ecma 15.5.4.14 String.prototype.split (separator, limit)
__jsvalue __jsstr_split(__jsvalue *this_string, __jsvalue *separator, __jsvalue *limit) {
  // step 1:
  CheckObjectCoercible(this_string);
  // step 2:
  __jsstring *s = __js_ToString(this_string);
  // step 3:
  __jsobject *a = __js_new_arr_internal(0);
  // step 4:
  uint32_t len = 0;
  // step 5:
  uint32_t lim;
  if (__is_undefined(limit)) {
    lim = (uint32_t)0xFFFFFFFF;
  } else {
    lim = __js_ToInteger(limit);
  }
  // step 6:
  uint32_t sl = __jsstr_get_length(s);
  // step 7:
  uint32_t p = 0;
  // step 8:
  // step 9:
  if (lim == 0)
    return __object_value(a);
  // step 10:
  __jsvalue v = __string_value(s);
  __jsvalue nv = {};
  __set_number(&nv, 0);
  if (__is_undefined(separator)) {
    __jsobj_helper_add_value_property(a, __js_ToString(&nv), &v, JSPROP_DESC_HAS_VWEC);
    return __object_value(a);
  }
  // step 11:
  uint32_t e;
  uint32_t q = 0;
  if (sl == 0) {
    if (!__jsstr_splitMatch(s, &q, separator, &e))
      __jsobj_helper_add_value_property(a, __js_ToString(&nv), &v, JSPROP_DESC_HAS_VWEC);
    return __object_value(a);
  }
  // step 12:
  q = p;
  // step 13:
  uint32_t n = 0;
  __jsstring *t;
  while (q != sl) {
    if (!__jsstr_splitMatch(s, &q, separator, &e))
      q++;
    else {
      // step 13.c
      if (e == p)
        q++;
      else {
        // step 13.c.iii
        t = __js_new_string_internal(q - p, true);
        // step 13.c.iii.1
        for (uint32_t i = 0; i < q - p; i++) {
          __jsstr_set_char(t, i, __jsstr_get_char(s, i + p));
        }
        // step 13.c.iii.2
        __set_number(&nv, (int32_t)n);
        v = __string_value(t);
        __jsobj_helper_add_value_property(a, __js_ToString(&nv), &v, JSPROP_DESC_HAS_VWEC);
        // step 13.c.iii.3
        __jsobj_helper_set_length(a, ++n, true);
        // step 13.c.iii.4
        if (n == lim)
          return __object_value(a);
        // step 13.c.iii.5
        p = e;
        // step 13.c.iii.6
        // step 13.c.iii.7
        //TODO: cap of return state from __jsstr_splitMatch
        // step 13.c.iii.8
        q = p;
      }
    }
  }
  // step 14:
  t = __js_new_string_internal(q - p, true);
  for (uint32_t i = 0; i < q - p; i++) {
    __jsstr_set_char(t, i, __jsstr_get_char(s, i + p));
  }
  __set_number(&nv, (int32_t)n);
  // step 15:
  v = __string_value(t);
  __jsobj_helper_add_value_property(a, __js_ToString(&nv), &v, JSPROP_DESC_HAS_VWEC);
  __jsobj_helper_set_length(a, ++n, true);
  // step 16:
  return __object_value(a);
}

// Ecma 15.5.4.15 String.prototype.substring (start, end)
__jsvalue __jsstr_substring(__jsvalue *this_string, __jsvalue *start, __jsvalue *end) {
  // step 1:
  CheckObjectCoercible(this_string);
  // step 2:
  __jsstring *s = __js_ToString(this_string);
  // step 3:
  uint32_t len = __jsstr_get_length(s);
  // step 4:
  int32_t st;
  if (__is_infinity(start)) {
    if (__is_neg_infinity(start))
      st = 0;
    else
      st = len;
  } else {
    st = __js_ToInteger(start);
  }
  // step 5:
  int32_t en;
  if (__is_undefined(end)) {
    en = len;
  } else if (__is_infinity(end)) {
    if (__is_neg_infinity(end))
      en = 0;
    else
      en = len;
  } else {
    en = __js_ToInteger(end);
  }
  // step 6:
  uint32_t finalst = st > 0 ? st : 0;
  finalst = finalst > len ? len : finalst;
  // step 7:
  uint32_t finalen = en > 0 ? en : 0;
  finalen = finalen > len ? len : finalen;
  // step 8:
  uint32_t from = (finalst > finalen) ? finalen : finalst;
  // step 9:
  uint32_t to = (finalst > finalen) ? finalst : finalen;
  // step 10:
  uint32_t span = to - from;
  __jsstring *str = __jsstr_extract(s, from, span);
  return __string_value(str);
}

// Ecma 15.5.4.16 String.prototype.toLowerCase ( )
__jsvalue __jsstr_toLowerCase(__jsvalue *this_string) {
  // step 1:
  CheckObjectCoercible(this_string);
  // step 2:
  __jsstring *s = __js_ToString(this_string);
  // step 3
  uint32_t len = __jsstr_get_length(s);
  __jsstring *str = __js_new_string_internal(len, true);
  for (uint32_t i = 0; i < len; i++) {
    __jschar c = __jsstr_get_char(s, i);
    if (c < 256 && (char)c >= 'A' && (char)c <= 'Z') {
      __jsstr_set_char(str, i, c + 32);
    } else {
      __jsstr_set_char(str, i, c);
    }
  }
  // step 4
  return __string_value(str);
}

// Ecma 15.5.4.17 String.prototype.toLocaleLowerCase ( )
__jsvalue __jsstr_toLocaleLowerCase(__jsvalue *this_string) {
  return __jsstr_toLowerCase(this_string);
}

// Ecma 15.5.4.18 String.prototype.toUpperCase ( )
__jsvalue __jsstr_toUpperCase(__jsvalue *this_string) {
  // step 1:
  CheckObjectCoercible(this_string);
  // step 2:
  __jsstring *s = __js_ToString(this_string);
  // step 3
  uint32_t len = __jsstr_get_length(s);
  __jsstring *str = __js_new_string_internal(len, true);
  for (uint32_t i = 0; i < len; i++) {
    __jschar c = __jsstr_get_char(s, i);
    if (c < 256 && (char)c >= 'a' && (char)c <= 'z') {
      __jsstr_set_char(str, i, c - 32);
    } else {
      __jsstr_set_char(str, i, c);
    }
  }
  // step 4
  return __string_value(str);
}

// Ecma 15.5.4.19 String.prototype.toLocaleUpperCase ( )
__jsvalue __jsstr_toLocaleUpperCase(__jsvalue *this_string) {
  return __jsstr_toUpperCase(this_string);
}

// Ecma 15.5.4.20 String.prototype.trim ( )
__jsvalue __jsstr_trim(__jsvalue *this_string) {
  // step 1:
  CheckObjectCoercible(this_string);
  // step 2:
  __jsstring *s = __js_ToString(this_string);
  // step 3: remove leading and trailing white space
  uint32_t len = __jsstr_get_length(s);
  if (len == 0)
    return __string_value(s);

  uint32_t start, end;
  for (start = 0; start < len; start++) {
    __jschar c = __jsstr_get_char(s, start);
    if (!std::isspace(c) && !__is_UnicodeSpace(c)) {
      break;
    }
  }
  if (start == len) // become empty after trim
    return __string_value(__jsstr_get_builtin(JSBUILTIN_STRING_EMPTY));

  for (end = len - 1; end > start; end--) {
    __jschar c = __jsstr_get_char(s, end);
    if (!std::isspace(c) && !__is_UnicodeSpace(c)) {
      break;
    }
  }
  if (start == 0 && end == len - 1)
    return __string_value(s);

  __jsstring *str = __js_new_string_internal(end - start + 1, true);
  for (uint32_t i = start; i <= end; i++) {
    __jschar c = __jsstr_get_char(s, i);
    __jsstr_set_char(str, i - start, c);
  }
  // step 4
  return __string_value(str);
}

static inline void __jsstr_get_match_pattern(std::wregex &pattern, __jsvalue *regexp, bool &global) {
  __jsstring *pattern_str;
  if (__is_js_object(regexp)) {
    __jsobject *obj = __jsval_to_object(regexp);
    if (obj->object_class == JSREGEXP) {
      __jsstr_get_regexp_patten(pattern, regexp, global);
      return;
    } else {
      if (obj->object_class == JSSTRING) {
        pattern_str = obj->shared.prim_string;
      } else if (obj->object_class == JSOBJECT) {
        __jsvalue val = __object_internal_DefaultValue(obj, JSTYPE_STRING);
        pattern_str = __js_ToString(&val);
      }
    }
  } else {
     pattern_str = __js_ToString(regexp);
  }
  pattern.assign((__jsstr_to_wstring(pattern_str)).c_str(), __jsstr_get_length(pattern_str), std::regex::ECMAScript);
  return;
}

// Ecma 15.5.4.12 String.prototype.search ( )
__jsvalue __jsstr_search(__jsvalue *this_string, __jsvalue *regexp) {
  // step 1:
  CheckObjectCoercible(this_string);
  // step 2:
  __jsstring *s = __js_ToString(this_string);
  if (__jsstr_get_length(s) == 0 || __is_undefined(regexp))
    return __number_value(0);

  int32_t ret = -1;
  std::wregex pattern;
  bool global;
  __jsstr_get_match_pattern(pattern, regexp, global);
  std::wstring str = __jsstr_to_wstring(s);

  std::wsmatch sm;
  std::regex_search (str, sm, pattern);
  if (sm.size() > 0) {
    ret = sm.position(0);
  }
  return __number_value(ret);
}

static inline __jsvalue __jsstr_stdstr_to_value(std::wstring stdstr) {
    int32_t len = stdstr.length();
    if (len == 0)
      return __string_value(__jsstr_get_builtin(JSBUILTIN_STRING_EMPTY));

    __jsstring *str = __js_new_string_internal(len, true);
    for (uint32_t i = 0; i < len; i++) {
      __jsstr_set_char(str, i, (uint16_t)stdstr.at(i));
    }
    return __string_value(str);
}

// Memory allocation for jscre.
static void* RegExpAlloc(size_t size) {
  void *obj = VMMallocGC(size);
  MAPLE_JS_ASSERT(obj);
  return obj;
}

// Memory de-allocation for jscre.
static void RegExpFree(void *ptr) {
  // GC handles this.
}

static void __jsstr_get_regexp_property(__jsvalue *regexp, __jsstring *&js_pattern,
                                        bool &global, bool &ignorecase, bool &multiline,
                                        unsigned &num_captures) {

  __jsvalue source = __jsop_getprop_by_name(regexp,
                        __jsstr_get_builtin(JSBUILTIN_STRING_SOURCE));
  js_pattern = __js_ToString(&source);

  __jsvalue js_global = __jsop_getprop_by_name(regexp,
                        __jsstr_get_builtin(JSBUILTIN_STRING_GLOBAL));
  global = __js_ToBoolean(&js_global);

  __jsvalue js_ignorecase = __jsop_getprop_by_name(regexp,
                  __jsstr_get_builtin(JSBUILTIN_STRING_IGNORECASE_UL));
  ignorecase = __js_ToBoolean(&js_ignorecase);

  __jsvalue js_multiline = __jsop_getprop_by_name(regexp,
                      __jsstr_get_builtin(JSBUILTIN_STRING_MULTILINE));
  multiline = __js_ToBoolean(&js_multiline);
}

static int __jsstr_regexp_exec(__jsstring *js_subject, __jsstring *js_pattern,
                                bool global, bool ignorecase, bool multiline,
                                unsigned &num_captures, int &last_index,
                                std::vector<std::pair<int,int>> *vres_ret) {
  const char *error_message = NULL;
  dart::jscre::JSRegExp * regexp = RegExpCompile(js_pattern, ignorecase,
          multiline, &num_captures, &error_message, &RegExpAlloc, &RegExpFree);
  if (regexp == NULL) {
    if (strstr(error_message, "at end of pattern")) {
      return -1;
    } else {
      MAPLE_JS_SYNTAXERROR_EXCEPTION();
    }
  }

  if (last_index >=  __jsstr_get_length(js_subject))
    return 0;

  int start_offset = global ? last_index : 0;
  int *offsets = (int *) VMMallocGC((num_captures+1) * 3);
  int offset_count = (num_captures+1) * 3;

  int res = RegExpExecute(regexp, js_subject, start_offset, offsets,
                          offset_count);

  if (res == dart::jscre::JSRegExpErrorNoMatch ||
      res == dart::jscre::JSRegExpErrorHitLimit) {
    return -1;
  }

  for (int i = 0; i < res; i++) {
    vres_ret->push_back(std::make_pair(offsets[2 * i], offsets[2 * i + 1]));
    if (i == 0)
      last_index = offsets[2 * i + 1];
  }

  return 1;
}

// Ecma 15.5.4.10 String.prototype.match ( )
__jsvalue __jsstr_match(__jsvalue *this_string, __jsvalue *regexp) {
  // step 1:
  CheckObjectCoercible(this_string);
  __jsvalue val;
  if (__is_undefined(regexp)) {
    val = __js_new_regexp_obj(NULL, NULL, 0);
    regexp = &val;
  } else if (!__is_regexp(regexp)) {
    __jsstring *pattern_str;
    __jsvalue v;
    if (__is_js_object(regexp)) {
      __jsobject *obj = __jsval_to_object(regexp);
      v = __object_internal_DefaultValue(obj, JSTYPE_STRING);
      if (!__is_string(&v)) {
        pattern_str = __js_ToString(&v);
        v = __string_value(pattern_str);
      }
    } else {
      pattern_str = __js_ToString(regexp);
      v = __string_value(pattern_str);
    }
    val = __js_new_regexp_obj(NULL, &v, 1);
    regexp = &val;
  }
  // step 2:
  __jsstring *s = __js_ToString(this_string);

  __jsvalue js_global = __jsop_getprop_by_name(regexp,
                        __jsstr_get_builtin(JSBUILTIN_STRING_GLOBAL));
  bool global = __js_ToBoolean(&js_global);
  if (!global)
    return __jsregexp_Exec(regexp, this_string, 1);

  std::vector<std::pair<int,int>> vres_ret;
  __jsstring *js_pattern = NULL;
  bool ignorecase, multiline;
  unsigned num_captures;
  int last_index = 0;
  __jsstr_get_regexp_property(regexp, js_pattern, global, ignorecase, multiline, num_captures);

  int r;
  do  {
    r = __jsstr_regexp_exec(s, js_pattern, global, ignorecase, multiline, num_captures, last_index, &vres_ret);
  } while (r == 1 && global);

  if (r == -1 && vres_ret.size() == 0) {
    __jsvalue items[1];
    items[0] = __undefined_value();
    __jsobject *array_obj = __js_new_arr_elems(items, 1);
    return __object_value(array_obj);
  }
  // workaround: vres_ret may contain duplicates due to dart::jscre issue, delete dup
  std::sort(vres_ret.begin(), vres_ret.end());
  vres_ret.erase(std::unique(vres_ret.begin(), vres_ret.end()), vres_ret.end());

  int length = vres_ret.size();
  __jsvalue items[length];
  for (int i = 0; i < length; i++) {
    int start = vres_ret[i].first, end = vres_ret[i].second;
    if (start >= 0 && end - start >= 0) {
      __jsvalue subject_val = __string_value(s);
      __jsvalue start_val = __number_value(start);
      __jsvalue end_val = __number_value(end);
      __jsvalue str_val = __jsstr_substring(&subject_val, &start_val, &end_val);
      items[i] = str_val;
    } else {
      items[i] = __undefined_value();
    }
  }

  __jsobject *array_obj = __js_new_arr_elems(items, length);

  __jsvalue index = __number_value(vres_ret[0].first);
  __jsobj_helper_add_value_property(array_obj, JSBUILTIN_STRING_INDEX, &index,
                                    JSPROP_DESC_HAS_VWEC);

  __jsvalue input = __string_value(s);
  __jsobj_helper_add_value_property(array_obj, JSBUILTIN_STRING_INPUT, &input,
                                    JSPROP_DESC_HAS_VWEC);

  if (length > 0) {
    __jsvalue js_last_index = __number_value(vres_ret[length - 1].second);
    __jsobj_helper_add_value_property(array_obj, JSBUILTIN_STRING_LASTINDEX_UL,
                                    &js_last_index, JSPROP_DESC_HAS_VWUEUC);
  }

  return __object_value(array_obj);
}

static inline void __jsstr_get_replace_value(__jsvalue *func, __jsvalue *this_string, std::vector<std::pair<int,int>> &vres_ret, __jsvalue &val) {
  int j = 0;
  __jsvalue match_start;
  __jsvalue args[vres_ret.size() + 2];
  for (int i = 0; i < vres_ret.size(); i++) {
    int start = vres_ret[i].first;
    int end = vres_ret[i].second;
    if (i == 0) {
      match_start = __number_value(start);
    }
    // Argument 1 is the substring that matched
    __jsvalue subject_val = *this_string;
    __jsvalue start_val = __number_value(start);
    __jsvalue end_val = __number_value(end);
    __jsvalue str_val = __jsstr_substring(&subject_val, &start_val, &end_val);
    args[j++] = str_val;
  }
  // Argument m + 2 is the offset within string where the match occurred
  args[j++] = match_start;
  // Argument m + 3 is string
  args[j++] = *this_string;

  __jsobject *f = __jsval_to_object(func);
  __jsfunction *fun = f->shared.fun;
  if (fun == NULL || fun->attrs == 0) {
    val = __undefined_value();
  } else {
    val = __jsfun_val_call(func, this_string, &args[0], j);
  }
}

static inline bool __jsstr_get_replace_substitution(std::wstring &source, std::wstring &rep, std::vector<std::pair<int,int>> &vres_ret) {
  bool has_substitution = false;
  int start = vres_ret[0].first;
  int end = vres_ret[vres_ret.size() - 1].second;
  int i = 0;
  while (i < rep.size()) {
    if (rep[i] == L'$' && rep.size() - i >= 2) {
      switch (rep[i + 1]) {
        case L'$':
          rep = rep.substr(0, i) + rep.substr(i + 1, rep.size() - i - 1);
          i += 1;
          has_substitution = true;
          break;
        case L'&':
          // The matched substing
          rep = rep.substr(0, i) +
                source.substr(start, end - start) +
                rep.substr(i + 2, rep.size() - i - 2);
          i += end - start;
          has_substitution = true;
          break;
        case L'`':
          // The portion of string that precedes the matched substring
          rep = rep.substr(0, i) +
                source.substr(0, start) +
                rep.substr(i + 2, rep.size() - i - 2);
          i += start;
          has_substitution = true;
          break;
        case L'\'':
          // The portion of string that follows the matched substring
          rep = rep.substr(0, i) +
                source.substr(end, source.size() - end) +
                rep.substr(i + 2, rep.size() - i - 2);
          i += source.size() - end;
          has_substitution = true;
          break;
        default:
          if (rep[i + 1] > L'0' && rep[i + 1] <='9') {
            // $n
            int l = rep[i + 1] - L'0';
            int offset = 2;
            if (rep.size() - i >= 3 && rep[i + 2] >= L'0' && rep[i + 2] <='9' && vres_ret.size() > 9) {
              // $nn
              l = l * 10 + (rep[i + 2] - L'0');
              offset = 3;
            }
            if (l <= vres_ret.size()) {
              rep = rep.substr(0, i) +
                    source.substr(vres_ret[l].first, vres_ret[l].second - vres_ret[l].first) +
                    rep.substr(i + offset, rep.size() - i - offset);
              i += vres_ret[l].second - vres_ret[l].first;
            } else {
              rep = rep.substr(0, i) + rep.substr(i + offset, rep.size() - i - offset);
            }
            has_substitution = true;
          }
          break;
      }
    } else {
      i++;
    }
  }
  return has_substitution;
}

// Ecma 15.5.4.11 String.prototype.replace ( )
__jsvalue __jsstr_replace(__jsvalue *this_string, __jsvalue *search, __jsvalue *replace) {
  // step 1:
  CheckObjectCoercible(this_string);
  // step 2:
  __jsstring *s = __js_ToString(this_string);

  __jsstring *replace_str;
  if (__is_js_object(replace)) {
    __jsobject *obj = __jsval_to_object(replace);
    if (obj->object_class == JSSTRING) {
      replace_str = obj->shared.prim_string;
    } else {
      __jsvalue val;
      val = __object_internal_DefaultValue(obj, JSTYPE_STRING);
      if (__is_string(&val)) {
        replace_str = __js_ToString(&val);
      }
      else {
        MAPLE_JS_TYPEERROR_EXCEPTION();
      }
    }
  } else {
    replace_str = __js_ToString(replace);
  }

  if (!__is_regexp(search)) {
    __jsstring *pattern_str;
    __jsvalue v;
    if (__is_js_object(search)) {
      __jsobject *obj = __jsval_to_object(search);
      v = __object_internal_DefaultValue(obj, JSTYPE_STRING);
      if (!__is_string(&v)) {
        pattern_str = __js_ToString(&v);
        v = __string_value(pattern_str);
      }
    } else {
      pattern_str = __js_ToString(search);
      v = __string_value(pattern_str);
    }
    __jsvalue val = __js_new_regexp_obj(NULL, &v, 1);
    search = &val;
  }

  std::vector<std::pair<int,int>> vres_ret;
  __jsvalue match_start;
  std::wstring result = __jsstr_to_wstring(s);
  __jsstring *js_pattern = NULL;
  bool ignorecase, multiline;
  unsigned num_captures;
  bool global = false;
  bool need_substitute = true;
  int last_index = 0;
  int distance = 0;
  int r;
  __jsstr_get_regexp_property(search, js_pattern, global, ignorecase, multiline, num_captures);
  do  {
    r = __jsstr_regexp_exec(s, js_pattern, global, ignorecase, multiline, num_captures, last_index, &vres_ret);
    if (vres_ret.size() > 0) {
      if (__js_IsCallable(replace)) {
        __jsvalue val;
        __jsstr_get_replace_value(replace, this_string, vres_ret, val);
        replace_str = __js_ToString(&val);
      }

      std::wstring rep = __jsstr_to_wstring(replace_str);
      std::wstring source = __jsstr_to_wstring(s);
      if (need_substitute)
        need_substitute = __jsstr_get_replace_substitution(source, rep, vres_ret);
      result.replace(vres_ret[0].first + distance, vres_ret[0].second - vres_ret[0].first, rep);
      distance += rep.size() - (vres_ret[0].second - vres_ret[0].first);
      last_index = vres_ret[vres_ret.size() - 1].second + 1;

      // when 'search' is an empty string, put 'rep' to the end as well
      if (vres_ret[0].first == last_index - 1 && last_index >= __jsstr_get_length(s)) {
        result.replace(last_index + distance, 0, rep);
      }

      vres_ret.clear();
    }
  } while (r == 1 && global && last_index <= __jsstr_get_length(s));

  return __jsstr_stdstr_to_value(result);
}

uint32_t __jsstr_is_number(__jsstring *jsstr) {
  uint32_t length = __jsstr_get_length(jsstr);
  char arr[128];
  if (length > 128)
    return 0;
  uint32_t i;
  assert((jsstr->kind & JSSTRING_UNICODE) == 0); // TODO: Unicode
  for (i = 0; i < length; i++) {
    arr[i] = jsstr->x.ascii[i];
  }
  arr[i] = '\0';
  char *end = nullptr;
  double val = strtod(arr, &end);
  return !(end != &arr[0] && *end == '\0' && val != HUGE_VAL);
}

bool __jsstr_readonly_builtin(__jsbuiltin_string_id strId) {
  switch (strId) {
    case JSBUILTIN_STRING_NAN:
    case JSBUILTIN_STRING_INFINITY_UL:
    case JSBUILTIN_STRING_UNDEFINED_UL:
    case JSBUILTIN_STRING_NULL_UL:
    case JSBUILTIN_STRING_UNDEFINED:
    case JSBUILTIN_STRING_ARGUMENTS:
    case JSBUILTIN_STRING_CALLER:
    case JSBUILTIN_STRING_CALLEE:
      return true;
    default:
      return false;
  }
}

bool __jsstr_throw_typeerror(__jsstring *name) {
  if (!__jsstr_is_builtin(name)) {
    return false;
  }
  __jsbuiltin_string_id strId = (__jsbuiltin_string_id)((uint8_t *)name)[1];
  return __jsstr_readonly_builtin(strId);
}

// check string can be converted to number index property
uint32_t __jsstr_is_numidx(__jsstring *p, bool &isNumidx) {
  uint32_t u;
  isNumidx = false;
  uint32_t len = __jsstr_get_length(p);
  // TODO:: deal with unicode string
  if (len == 0 || !__jsstr_is_ascii(p)) {
    return 0;
  }
  char chars[len+1];
  uint32_t i;
  for (i = 0; i < len; i++) {
    chars[i] = (char)__jsstr_get_char(p, i);
  }
  chars[i] = '\0';
  // p is negative
  if (chars[0] == '-' || (chars[0] == '0' && len > 1)) {
    return 0;
  }
  // string is 0
  if ((len == 1 && chars[0] == '0') ||
      (len == 2 && chars[0] == '+' && chars[1] == '0') ||
      (len == 3 &&
       chars[0] == '0' &&
       (chars[1] == 'x' || chars[1] == 'X') &&
       chars[2] == '0')) {
    isNumidx = true;
    return 0;
  }

  double d = atof(chars);
  if (d == 0.0 || std::isnan(d) || std::isinf(d)) {
    return 0;
  }
  u = (uint32_t)d;
  if (u != d) {
    return 0;
  }
  isNumidx = true;
  return u;
}

/*
 * Copyright (C) [2021] Futurewei Technologies, Inc. All rights reserved.
 *
 * OpenArkCompiler is licensed under the Mulan Permissive Software License v2.
 * You can use this software according to the terms and conditions of the
 * MulanPSL - 2.0.
 * You may obtain a copy of MulanPSL - 2.0 at:
 *
 *   https://opensource.org/licenses/MulanPSL-2.0
 *
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY
 * KIND, EITHER
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
 * MERCHANTABILITY OR
 * FIT FOR A PARTICULAR PURPOSE.
 * See the MulanPSL - 2.0 for more details.
 */

#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <locale>
#include <codecvt>

#include "jsencode.h"
#include "jsnum.h"
#include "jsstring.h"
#include "jstycnv.h"
#include "jsvalue.h"
#include "jsvalueinline.h"
#include "vmmemory.h"

#include <iostream>
#include <string>

//#define DEBUG_E 1
//#define DEBUG_D 1

/*
 * ECMA 3, 15.1.3 URI Handling Function
 *
 */

/*
 * Convert one UCS-4 char and write it into a UTF-8 buffer, which must be at
 * least 4 bytes long.  Return the number of UTF-8 bytes of data written.
 */
int
__js_encode_Ucs4ToUtf8Char(uint8_t *utf8Buffer, uint32_t ucs4Char)
{
    int utf8Length = 1;

    // TODO: JS_ASSERT(ucs4Char <= 0x10FFFF);
    if (ucs4Char < 0x80) {
        *utf8Buffer = (uint8_t)ucs4Char;
    } else {
        int i;
        uint32_t a = ucs4Char >> 11;
        utf8Length = 2;
        while (a) {
            a >>= 5;
            utf8Length++;
        }
        i = utf8Length;
        while (--i) {
            utf8Buffer[i] = (uint8_t)((ucs4Char & 0x3F) | 0x80);
            ucs4Char >>= 6;
        }
        *utf8Buffer = (uint8_t)(0x100 - (1 << (8-utf8Length)) + ucs4Char);
    }
    return utf8Length;
}

static bool
Encode(__jsstring *src_js_str, char *result, const bool *unescapedSet,
       const bool *unescapedSet2, bool component)
{
    size_t length = __jsstr_get_length(src_js_str);
    if (length == 0) {
        result[0] = '\0';
        return true;
    }

    std::string strb;
    try{
      strb.reserve((length*3));  // Alloc string memory for 3x length for char expansion
    } catch( std::exception& ) { // frees automatically when strb goes out of scope
      return false;
      //return err_code;         // Eventually support err code on failure
    }

#ifdef DEBUG_E
  for (int k = 0; k < 10; k++){
    printf("{%c}", (char)__jsstr_get_char(src_js_str, k));
  }
#endif

    mjs_char hexBuf[4] = { '%', '0', '0', '0' };
    mjs_char c;
    for (size_t i = 0; i < length; i++) {
        c = __jsstr_get_char(src_js_str, i);
        if (c < 33 ) { // Investigate logic of component to unify
#ifdef DEBUG_E
          printf("Regular - {%c}{%d}", (char)c, component);
#endif
          strb += hexBuf[0];
          strb += hexchars[(unsigned char)c >> 4];
          strb += hexchars[(unsigned char)c & 0xF];
        } else
        if (c < 128 && (unescapedSet[c] || (unescapedSet2 && (component ^ unescapedSet2[c])))) {
#ifdef DEBUG_E
          printf("Unescaped - {%c}{%d}", (char)c, component);
#endif
          strb+=c;
        } else
        if (c < 128 ) {
#ifdef DEBUG_E
          printf("Regular - {%c}{%d}", (char)c, component);
#endif
          strb += hexBuf[0];
          strb += hexchars[(unsigned char)c >> 4];
          strb += hexchars[(unsigned char)c & 0xF];
        } else {
            if ((c <= 0xDFFF) && (c >= 0xDC00)) {
                MAPLE_JS_URIERROR_EXCEPTION();
                return false;
            }
            uint32_t v;
            if ((c > 0xDBFF) || (c < 0xD800)) {
                v = c;
            } else {
                i++;
                mjs_char c2 = __jsstr_get_char(src_js_str, i);
                if ((i == length) || ((c2 < 0xDC00) || (c2 > 0xDFFF))) {
                    MAPLE_JS_URIERROR_EXCEPTION();
                    return false;
                }
                v = ((c - 0xD800) << 10) + (c2 - 0xDC00) + 0x10000;
            }
            uint8_t utf8buf[4];
            size_t char_len = __js_encode_Ucs4ToUtf8Char(utf8buf, v);
            for (size_t j = 0; j < char_len; j++) {
                hexBuf[1] = hexchars[utf8buf[j] >> 4];
                hexBuf[2] = hexchars[utf8buf[j] & 0xf];
                strb+=hexBuf[0];
                strb+=hexBuf[1];
                strb+=hexBuf[2];
            }
        }
    }
    strcpy(result, strb.c_str());

    return true;
}
//Encode URI Elements
int CaEncode(__jsstring *src_js_str, const uint32_t js_str_sz, char *result,
             const int result_sz, bool component) {
  uint32_t i = 0, j = 0, mrk = 0;
  uint16_t char_c;

#ifdef DEBUG_E
  for (int k = 0; k < 10; k++){
    printf("{%c}", (char)__jsstr_get_char(src_js_str, k));
  }
#endif

  if (js_str_sz == 0 )
    return 100;
    //return -1; //ignore empty string

  for (i = 0, j = 0; i < js_str_sz; i++) {
    char_c = __jsstr_get_char(src_js_str, i);
    // -_.!~*'()  Unescaped characters
    if (isalnum(char_c) || char_c == '-' || char_c == '_' || char_c == '.' ||
        char_c == '*' || char_c == '!' || char_c == '~' || char_c == '\'' ||
        char_c == '(' || char_c == ')') {
      result[j++] = char_c;
      // ;,/?:@&=+$ Reserved characters  and the #  Number sign
    } else if((char_c >= 0xDC00) && (char_c <= 0xDFFF)) {
      MAPLE_JS_URIERROR_EXCEPTION();
    } else if (((!component) &&
                (char_c == ';' || char_c == ',' || char_c == '/' ||
                 char_c == '?' || char_c == ':' || char_c == '@' ||
                 char_c == '&' || char_c == '=' || char_c == '+' ||
                 char_c == '$' || char_c == '#'))) {
      result[j++] = char_c;
    } else if (char_c == ' ') { // Use literal encoding rather than '+'
      result[j++] = '%';
      result[j++] = '2';
      result[j++] = '0';
    } else if (char_c == '#' && component) {
      result[j++] = '%';
      result[j++] = '2';
      result[j++] = '3';
    } else if (char_c == '\\') {
      // Set escape on
      mrk = 1;
    } else if (mrk == 1) {
      if (char_c == '\\') {
        result[j++] = char_c;
        result[j++] = char_c;
      } else {
        result[j++] = '%';
        result[j++] = '0';
        if (char_c == 't')
          result[j++] = '9';
        else if (char_c == 'n')
          result[j++] = 'A';
        else if (char_c == 'v')
          result[j++] = 'B';
        else if (char_c == 'f')
          result[j++] = 'C';
        else if (char_c == 'r')
          result[j++] = 'D';
      }
      // Set escape off
      mrk = 0;
    } else {
      result[j++] = '%';
      result[j++] = hexchars[(unsigned char)char_c >> 4];
      result[j++] = hexchars[(unsigned char)char_c & 0xF];
    }
  }
  result[j++] = '\0';

#ifdef DEBUG_E
  printf("Info: data encoding [%s](%d,%zu)\n", result, i, strlen(result));
#endif

  if (i == 0)
    return 0;
  else if (i == js_str_sz)
    return j;
  return -2;
}

void debugprint(char debug_msg) {
   FILE *fileAddress;
   fileAddress = fopen("/tmp/debug.log", "a");

   if (fileAddress != NULL) {
       fputc (debug_msg,fileAddress);
    }
    fclose(fileAddress);
}


static bool
Decode(__jsstring *src_js_str, __jsstring *&result, const bool *reservedSet, const bool component )
{
    size_t length = __jsstr_get_length(src_js_str);
    if (length == 0) {
        result = __jsstr_get_builtin(JSBUILTIN_STRING_EMPTY);
        return true;
    }

    mjs_char c;
    bool highByteZero = true;
    bool cvtOk = true;
    std::string strb;
    std::u16string utf16;

    // replace '%xx' patterns with corresponding 8 bit binary
    int n;    // URI escape sequence length
    for (size_t i = 0; i < length; i++) {
        c = __jsstr_get_char(src_js_str, i);
        if (c == '%') {
            size_t start = i;
            if ((i + 2) >= length)
                goto report_bad_uri;
            if (!MJS_ISHEX(__jsstr_get_char(src_js_str, i+1)) || !MJS_ISHEX(__jsstr_get_char(src_js_str, i+2)))
                goto report_bad_uri;
            uint32_t B = MJS_UNHEX(__jsstr_get_char(src_js_str, i+1)) * 16 + MJS_UNHEX(__jsstr_get_char(src_js_str, i+2));

            if (B & 0x80) {

              // check for multi byte URI esacpe sequence
              n = 1;
              while (B & (0x80 >> n))             // count of 4 most sign. bits in 1st byte in sequence = seq. length
                  n++;
              if (n == 1 || n > 4)
                  goto report_bad_uri;
              if (i + 3 * (n - 1) >= length)
                  goto report_bad_uri;

              for (int j = 1; j <= n; j++) {
                  if (__jsstr_get_char(src_js_str, i) != '%')
                      goto report_bad_uri;
                  if (!MJS_ISHEX(__jsstr_get_char(src_js_str, i+1)) || !MJS_ISHEX(__jsstr_get_char(src_js_str, i+2)))
                      goto report_bad_uri;
                  B = MJS_UNHEX(__jsstr_get_char(src_js_str, i+1)) * 16 + MJS_UNHEX(__jsstr_get_char(src_js_str, i+2));
                  if (j>1 && (B & 0xC0) != 0x80)  // subsequent bytes must start with binary 10 i.e. 0b10xxxxxx
                      goto report_bad_uri;
                  i =  (j==n) ? i+2 : i+3;        // if last byte in sequence, adjust for i increment in outermost for loop
                  strb += B;
              }

            } else {
              // std ascii char - check for rsvd char
              if (!component && reservedSet && reservedSet[B]) {
                // if rsvd char, keep ascii string '%XX'
                strb += c;
                strb += __jsstr_get_char(src_js_str, i+1);
                strb += __jsstr_get_char(src_js_str, i+2);
              } else {
                strb += B;
              }
              i += 2;
              continue;
            }
        } else {
            strb+=c;
        }
    }

    try {
      utf16 = std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t>{}.from_bytes(strb.data());
    }
    catch (std::range_error) {
      cvtOk = false;
      MAPLE_JS_URIERROR_EXCEPTION();
    }
    if (cvtOk) {
      for (uint32_t i=0; i<utf16.length(); i++) {
        if (utf16.data()[i] >= 256) {
          highByteZero = false;
        }
      }
    } else {
      return false;
    }
    // allocate and setup return jsstring
    result = __js_new_string_internal(utf16.length(), !highByteZero);
    for (uint32_t i=0; i<utf16.length(); i++) {
      __jsstr_set_char(result, i, utf16[i]);
    }

    return true ;

  report_bad_uri:
    MAPLE_JS_URIERROR_EXCEPTION();
    /* FALL THROUGH */
    return false;
}


__jsstring *__jsop_encode_item(__jsstring *value, bool component) {
  uint32_t value_sz = __jsstr_get_length(value), result_sz;
  char char_result[value_sz * 3 + 1]; // Use the *3 since most encoding is
  int ret = 0;

#ifdef DEBUG_E
  printf("Info: encoding data input - [");
  __jsstr_print(value);
  printf("](%d)\n", value_sz);
#endif

  ret = Encode(value, char_result, js_uriUnescapeSet, js_uriReserveSet, component);
  __jsstring *result = __jsstr_new_from_char(char_result);
  result_sz = __jsstr_get_length(result);

  if (value_sz == 0 || result_sz == 0|| ret > 0) {
#ifdef DEBUG_D
    printf("Info: possible data encoding failure - [");
    __jsstr_print(result);
    printf("]\n");
#endif
  } else if (ret ==100) {
    return __jsstr_new_from_char("");
  }

#ifdef DEBUG_E
  printf("Info: encoding data result - [%s](%zu)\n", char_result, strlen(char_result));
#endif

  return result;
}

__jsstring *__jsop_decode_item(__jsstring *value, bool component) {
  __jsstring *result;
  bool ret = 0;

#ifdef DEBUG_D
  printf("Info: decoding data input - [");
  __jsstr_print(value);
  printf("](%d)\n", value_sz);
#endif

  ret = Decode(value, result, js_uriReserveSet, component);

  if (!ret ) {
#ifdef DEBUG_D
    printf("Info: possible data decoding failure - [");
    __jsstr_print(result);
    printf("]\n");
#endif
  }

#ifdef DEBUG_D
  printf("Info: decoding result - [");
  __jsstr_print(result);
  printf("](%d)(%d)\n", ret, __jsstr_get_length(result));
  debugprint('+');
  debugprint('+');
  debugprint('+');
  debugprint('\n');
#endif

  return result;
}

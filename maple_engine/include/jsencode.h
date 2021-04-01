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

/*
var set1 = ";,/?:@&=+$";  // Reserved Characters
var set2 = "-_.!~*'()";   // Unescaped Characters
var set3 = "#";           // Number Sign
var set4 = "ABC abc 123"; // Alphanumeric Characters + Space

console.log(encodeURI(set1)); // ;,/?:@&=+$
console.log(encodeURI(set2)); // -_.!~*'()
console.log(encodeURI(set3)); // #
console.log(encodeURI(set4)); // ABC%20abc%20123 (the space gets encoded as %20)

console.log(encodeURIComponent(set1)); // %3B%2C%2F%3F%3A%40%26%3D%2B%24
console.log(encodeURIComponent(set2)); // -_.!~*'()
console.log(encodeURIComponent(set3)); // %23
console.log(encodeURIComponent(set4)); // ABC%20abc%20123 (the space gets
encoded as %20)
*/

#define MJS_ISDEC(c)    ((((unsigned)(c)) - '0') <= 9)
#define MJS_UNDEC(c)    ((c) - '0')
#define MJS_ISHEX(c)    ((c) < 128 && isxdigit(c))
#define MJS_UNHEX(c)    (unsigned)(MJS_ISDEC(c) ? (c) - '0' : 10 + tolower(c) - 'a')
#define MJS_ISLET(c)    ((c) < 128 && isalpha(c))

static unsigned char hexchars[] = "0123456789ABCDEF";
typedef uint16_t mjs_char;

#define _X_ true
#define _0_ false

// Uri reserved chars + #:
static const bool js_uriReserveSet[] = {
/*      0    1    2    3    4    5    6    7    8    9  *//*
  0 */ _0_, _0_, _0_, _0_, _0_, _0_, _0_, _0_, _0_, _0_,  /*
  1 */ _0_, _0_, _0_, _0_, _0_, _0_, _0_, _0_, _0_, _0_,  /*
  2 */ _0_, _0_, _0_, _0_, _0_, _0_, _0_, _0_, _0_, _0_,  /*
  3 */ _0_, _0_, _0_, _0_, _0_, _X_, _X_, _0_, _X_, _0_,  /*  35: #,  36: $,  38: & 
  4 */ _0_, _0_, _0_, _X_, _X_, _0_, _0_, _X_, _0_, _0_,  /*  43: +,  44: ,,  47: / 
  5 */ _0_, _0_, _0_, _0_, _0_, _0_, _0_, _0_, _X_, _X_,  /*  58: :,  59: ;,  61: = 
  6 */ _0_, _X_, _0_, _X_, _X_, _0_, _0_, _0_, _0_, _0_,  /*  63: ?,  64: @   
  7 */ _0_, _0_, _0_, _0_, _0_, _0_, _0_, _0_, _0_, _0_,  /*
  8 */ _0_, _0_, _0_, _0_, _0_, _0_, _0_, _0_, _0_, _0_,  /*
  9 */ _0_, _0_, _0_, _0_, _0_, _0_, _0_, _0_, _0_, _0_,  /*
 10 */ _0_, _0_, _0_, _0_, _0_, _0_, _0_, _0_, _0_, _0_,  /*
 11 */ _0_, _0_, _0_, _0_, _0_, _0_, _0_, _0_, _0_, _0_,  /*
 12 */ _0_, _0_, _0_, _0_, _0_, _0_, _0_, _0_   };

// Uri unescaped chars:
static const bool js_uriUnescapeSet[] = {
/*      0    1    2    3    4    5    6    7    8    9  *//*
  0 */ _0_, _0_, _0_, _0_, _0_, _0_, _0_, _0_, _0_, _0_,  /* 
  1 */ _0_, _0_, _0_, _0_, _0_, _0_, _0_, _0_, _0_, _0_,  /* 
  2 */ _0_, _0_, _0_, _0_, _0_, _0_, _0_, _0_, _0_, _0_,  /* 
  3 */ _0_, _0_, _0_, _X_, _0_, _0_, _0_, _0_, _0_, _X_,  /*  33: !, 39: '
  4 */ _X_, _X_, _X_, _0_, _0_, _X_, _X_, _0_, _X_, _X_,  /*  40: (, 41: ), 42: *, 45: -, 46: .  * -  48: 0, 49: 1
  5 */ _X_, _X_, _X_, _X_, _X_, _X_, _X_, _X_, _0_, _0_,  /*  51 - 57: 2-9
  6 */ _0_, _0_, _0_, _0_, _0_, _X_, _X_, _X_, _X_, _X_,  /*  65 - 69: A-E
  7 */ _X_, _X_, _X_, _X_, _X_, _X_, _X_, _X_, _X_, _X_,  /*  70 - 79: F-O
  8 */ _X_, _X_, _X_, _X_, _X_, _X_, _X_, _X_, _X_, _X_,  /*  80 - 89: P-Y
  9 */ _X_, _0_, _0_, _0_, _0_, _X_, _0_, _X_, _X_, _X_,  /*  90: Z, 95: _, 97:  a, 98: b,  99: c
 10 */ _X_, _X_, _X_, _X_, _X_, _X_, _X_, _X_, _X_, _X_,  /*  100 - 109:  d - m
 11 */ _X_, _X_, _X_, _X_, _X_, _X_, _X_, _X_, _X_, _X_,  /*  110 - 119:  n - w
 12 */ _X_, _X_, _X_, _0_, _0_, _0_, _X_, _0_   };        //  120 - 122:  x - z, 126: ~

#undef _X_
#undef _0_

//
// ecma 15.X.X.X
__jsstring *__jsop_encode_item(__jsstring *value, bool component);

// ecma 15.X.X.X
__jsstring *__jsop_decode_item(__jsstring *value, bool component);

int __js_encode_Ucs4ToU8Char(uint8_t *utf8Buffer, uint32_t ucs4Char);

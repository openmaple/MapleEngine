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

#ifndef JSVALUE_H
#define JSVALUE_H
#include <cstdint>
#include <cstdbool>
#include <cassert>
#include <cmath>
#include "mir_config.h"
#include "mvalue.h"

#if MIR_FEATURE_FULL || MIR_DEBUG
#include <cstdio>
#endif  // MIR_FEATURE_FULL

#ifdef DEBUG_MAPLE
// replaced by MIR_ASSERT for simplicity.
#define MAPLE_JS_ASSERT(expr) MIR_ASSERT((expr))
#define MAPLE_JS_EXCEPTION(expr) MIR_ASSERT((expr) && "Should throw a JS exception, issue #553")
#else
#define MAPLE_JS_ASSERT(expr) MIR_ASSERT((expr))
#define MAPLE_JS_EXCEPTION(expr) MIR_ASSERT((expr) && "Should throw a JS exception, issue #553")
#endif
/*
#define MAPLE_JS_TYPEERROR_EXCEPTION() \
 do {                                  \
  throw "TypeError";                   \
 } while(0);                           \
 */

inline void MAPLE_JS_TYPEERROR_EXCEPTION() {
  throw "TypeError";
}

inline void MAPLE_JS_SYNTAXERROR_EXCEPTION() {
  throw "SyntaxError";
}

inline void MAPLE_JS_RANGEERROR_EXCEPTION() {
  throw "RangeError";
}
inline void MAPLE_JS_URIERROR_EXCEPTION() {
  throw "UriError";
}
inline void MAPLE_JS_REFERENCEERROR_EXCEPTION() {
  throw "ReferenceError";
}

enum __jstype : uint32_t {
  JSTYPE_INFINITY = 0x0,
  JSTYPE_NULL,
  JSTYPE_BOOLEAN,
  JSTYPE_NUMBER,
  JSTYPE_STRING,
  JSTYPE_OBJECT,
  JSTYPE_ENV,
  JSTYPE_UNKNOWN = 0x8,
  JSTYPE_UNDEFINED,
  JSTYPE_NONE,
  JSTYPE_NAN,
  // the following 3 tags are for address type that is used for jsvalue in memory. because mplbe lower always generate 64-bits value, we need a tag to mark the address type
  JSTYPE_SPBASE,
  JSTYPE_FPBASE,
  JSTYPE_GPBASE,
  JSTYPE_FUNCTION, // for native function that is only given its address
  JSTYPE_DOUBLE,
};

#define IS_DOUBLE(V)     ((V & 0x7FF0000000000000) != 0x7FF0000000000000)
#define NOT_DOUBLE(V)    ((V & 0x7FF0000000000000) == 0x7FF0000000000000)
#define IS_BOOLEAN(V)    ((V & 0x7FFF000000000000) == 0x7FF2000000000000)  //JSTYPE_BOOLEAN = 2
#define IS_NUMBER(V)     ((V & 0x7FFF000000000000) == 0x7FF3000000000000)  //JSTYPE_NUMBER = 3
#define IS_NONE(V)       ((V & 0x7FFF000000000000) == 0x7FFA000000000000)  //JSTYPE_NONE = 10
#define IS_NEEDRC(V)     (((uint64_t)V & 0x7FFC000000000000) == 0x7FF4000000000000)  //JSTYPE_STRING or JSTYPE_OBJECT or JSTYPE_ENV

#define NAN_BASE     0x7ff0
#define PAYLOAD_MASK 0x0000FFFFFFFFFFFF

#define mEncode(v) {\
  if (v.ptyp < JSTYPE_DOUBLE) {\
    v.x.c.type = v.ptyp | NAN_BASE;\
  }\
}

#define mDecodeValue(v) {\
  if (NOT_DOUBLE(v.x.u64)) {\
    v.x.u64 &= PAYLOAD_MASK;\
  }\
}

#define mDecodeType(v) {\
  if (NOT_DOUBLE(v.x.u64)) {\
    v.ptyp = v.x.c.type & ~NAN_BASE;\
  } else {\
    v.ptyp = JSTYPE_DOUBLE;\
  }\
}

#define mDecode(v) {\
  if (NOT_DOUBLE(v.x.u64)){\
    v.ptyp = v.x.c.type & ~NAN_BASE;\
    v.x.u64 &= PAYLOAD_MASK;\
  } else {\
    v.ptyp = JSTYPE_DOUBLE;\
  }\
}

enum __jsbuiltin_object_id : uint8_t {  // must in accordance with js_value.h:js_builtin_id in the front-end (js2mpl/include/jsvalue.h)
  JSBUILTIN_GLOBALOBJECT = 0,
  JSBUILTIN_OBJECTCONSTRUCTOR,
  JSBUILTIN_OBJECTPROTOTYPE,
  JSBUILTIN_FUNCTIONCONSTRUCTOR,
  JSBUILTIN_FUNCTIONPROTOTYPE,
  JSBUILTIN_ARRAYCONSTRUCTOR,
  JSBUILTIN_ARRAYPROTOTYPE,
  JSBUILTIN_STRINGCONSTRUCTOR,
  JSBUILTIN_STRINGPROTOTYPE,
  JSBUILTIN_BOOLEANCONSTRUCTOR,
  JSBUILTIN_BOOLEANPROTOTYPE,
  JSBUILTIN_NUMBERCONSTRUCTOR,
  JSBUILTIN_NUMBERPROTOTYPE,
  JSBUILTIN_EXPORTS,
  JSBUILTIN_MODULE,
  JSBUILTIN_MATH,
  JSBUILTIN_JSON,
  JSBUILTIN_ERROR_CONSTRUCTOR,
  JSBUILTIN_ERROR_PROTOTYPE,
  JSBUILTIN_EVALERROR_CONSTRUCTOR,
  JSBUILTIN_EVALERROR_PROTOTYPE,
  JSBUILTIN_RANGEERROR_CONSTRUCTOR,
  JSBUILTIN_RANGEERROR_PROTOTYPE,
  JSBUILTIN_REFERENCEERRORCONSTRUCTOR,
  JSBUILTIN_REFERENCEERRORPROTOTYPE,
  JSBUILTIN_SYNTAXERROR_CONSTRUCTOR,
  JSBUILTIN_SYNTAXERROR_PROTOTYPE,
  JSBUILTIN_TYPEERROR_CONSTRUCTOR,
  JSBUILTIN_TYPEERROR_PROTOTYPE,
  JSBUILTIN_URIERROR_CONSTRUCTOR,
  JSBUILTIN_URIERROR_PROTOTYPE,
  JSBUILTIN_DATECONSTRUCTOR,
  JSBUILTIN_DATEPROTOTYPE,
  JSBUILTIN_ISNAN,
  JSBUILTIN_REGEXPCONSTRUCTOR,
  JSBUILTIN_REGEXPPROTOTYPE,
  JSBUILTIN_NAN,
  JSBUILTIN_INFINITY,
  JSBUILTIN_UNDEFINED,
  JSBUILTIN_PARSEINT_CONSTRUCTOR,
  JSBUILTIN_DECODEURI_CONSTRUCTOR,
  JSBUILTIN_DECODEURICOMPONENT_CONSTRUCTOR,
  JSBUILTIN_PARSEFLOAT_CONSTRUCTOR,
  JSBUILTIN_ISFINITE_CONSTRUCTOR,
  JSBUILTIN_ENCODEURI_CONSTRUCTOR,
  JSBUILTIN_ENCODEURICOMPONENT_CONSTRUCTOR,
  JSBUILTIN_EVAL_CONSTRUCTOR,
  JSBUILTIN_INTL,
  JSBUILTIN_INTL_COLLATOR_CONSTRUCTOR,
  JSBUILTIN_INTL_COLLATOR_PROTOTYPE,
  JSBUILTIN_INTL_NUMBERFORMAT_CONSTRUCTOR,
  JSBUILTIN_INTL_NUMBERFORMAT_PROTOTYPE,
  JSBUILTIN_INTL_DATETIMEFORMAT_CONSTRUCTOR,
  JSBUILTIN_INTL_DATETIMEFORMAT_PROTOTYPE,
  JSBUILTIN_CONSOLE,
  JSBUILTIN_ARRAYBUFFER_CONSTRUCTOR,
  JSBUILTIN_ARRAYBUFFER_PROTOTYPE,
  JSBUILTIN_DATAVIEW_CONSTRUCTOR,
  JSBUILTIN_DATAVIEW_PROTOTYPE,
  JSBUILTIN_LAST_OBJECT,
};

typedef struct __jsobject __jsobject;
//typedef uint8_t __jsstring;

const double MathE = 2.718281828459045;
const double MathLn10 = 2.302585092994046;
const double MathLn2 = 0.6931471805599453;
const double MathLog10e = 0.4342944819032518;
const double MathLog2e = 1.4426950408889634;
const double MathPi = 3.141592653589793;
const double MathSqrt1_2 = 0.7071067811865476;
const double MathSqrt2 = 1.4142135623730951;
const double NumberMaxValue = 1.7976931348623157e+308;
const double NumberMinValue = 5e-324;

#define DOUBLE_ZERO_INDEX    0
#define MATH_E_INDEX    1
#define MATH_LN10_INDEX 2
#define MATH_LN2_INDEX  3
#define MATH_LOG10E_INDEX 4
#define MATH_LOG2E_INDEX 5
#define MATH_PI_INDEX 6
#define MATH_SQRT1_2_INDEX 7
#define MATH_SQRT2_INDEX 8
#define NUMBER_MAX_VALUE 9
#define NUMBER_MIN_VALUE 10
#define MATH_LAST_INDEX NUMBER_MIN_VALUE

#ifdef MACHINE64
typedef maple::MValue __jsvalue;
/*
struct __jsvalue {
  union {
    int32_t i32;
    uint32_t u32;
    uint32_t boo;
    void *ptr;
    void *str;
    __jsobject *obj;
    double f64;
    uint64_t asbits;
  } x;
  __jstype ptyp;
};
*/
#else
union __jsvalue {
  uint64_t asbits;
  struct {
    union {
      int32_t i32;
      uint32_t u32;
      uint32_t boo;
      __jsstring *str;
      __jsobject *obj;
      void *ptr;
    } payload;
    __jstype tag;
  } s;
};
#endif

void dumpJSValue(__jsvalue *jsval);
void dumpJSString(uint16_t *ptr);

#endif

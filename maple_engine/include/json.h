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

#ifndef JS_CHAR_H
#define JS_CHAR_H

#include "jsstring.h"

// Whitespace characters (ECMA-262 v5, Table 2)
#define JS_CHAR_TAB ((__jschar)0x0009)  /* tab */
#define JS_CHAR_VTAB ((__jschar)0x000B) /* vertical tab */
#define JS_CHAR_FF ((__jschar)0x000C)   /* form feed */
#define JS_CHAR_SP ((__jschar)0x0020)   /* space */
#define JS_CHAR_NBSP ((__jschar)0x00A0) /* no-break space */

// Json WhiteSpace
#define JS_CHAR_CR ((__jschar)0x000D)
#define JS_CHAR_LF ((__jschar)0x000A)

#define JS_CHAR_BS ((__jschar)0x0008) /* backspace */

#define JS_CHAR_DOUBLE_QUOTE ((__jschar)'"')  /* double quote */
#define JS_CHAR_SINGLE_QUOTE ((__jschar)'\'') /* single quote */
#define JS_CHAR_BACKSLASH ((__jschar)'\\')    /* reverse solidus (backslash) */

// Comment characters (ECMA-262 v5, 7.4)
#define JS_CHAR_SLASH ((__jschar)'/')    /* solidus */
#define JS_CHAR_ASTERISK ((__jschar)'*') /* asterisk */

// Identifier name characters (ECMA-262 v5, 7.6)
#define JS_CHAR_DOLLAR_SIGN ((__jschar)'$') /* dollar sign */
#define JS_CHAR_UNDERSCORE ((__jschar)'_')  /* low line (underscore) */

//   Punctuator characters (ECMA-262 v5, 7.7)
#define JS_CHAR_LEFT_BRACE ((__jschar)'{')   /* left nurly bracket */
#define JS_CHAR_RIGHT_BRACE ((__jschar)'}')  /* right curly bracket */
#define JS_CHAR_LEFT_PAREN ((__jschar)'(')   /* left parenthesis */
#define JS_CHAR_RIGHT_PAREN ((__jschar)')')  /* right parenthesis */
#define JS_CHAR_LEFT_SQUARE ((__jschar)'[')  /* left square bracket */
#define JS_CHAR_RIGHT_SQUARE ((__jschar)']') /* right square bracket */
#define JS_CHAR_DOT ((__jschar)'.')          /* dot */
#define JS_CHAR_SEMICOLON ((__jschar)';')    /* semicolon */
#define JS_CHAR_COMMA ((__jschar)',')        /* comma */
#define JS_CHAR_LESS_THAN ((__jschar)'<')    /* less-than sign */
#define JS_CHAR_GREATER_THAN ((__jschar)'>') /* greater-than sign */
#define JS_CHAR_EQUALS ((__jschar)'=')       /* equals sign */
#define JS_CHAR_PLUS ((__jschar)'+')         /* plus sign */
#define JS_CHAR_MINUS ((__jschar)'-')        /* hyphen-minus */
/* JS_CHAR_ASTERISK is defined above */
#define JS_CHAR_PERCENT ((__jschar)'%')     /* percent sign */
#define JS_CHAR_AMPERSAND ((__jschar)'&')   /* ampersand */
#define JS_CHAR_VLINE ((__jschar)'|')       /* vertical line */
#define JS_CHAR_CIRCUMFLEX ((__jschar)'^')  /* circumflex accent */
#define JS_CHAR_EXCLAMATION ((__jschar)'!') /* exclamation mark */
#define JS_CHAR_TILDE ((__jschar)'~')       /* tilde */
#define JS_CHAR_QUESTION ((__jschar)'?')    /* question mark */
#define JS_CHAR_COLON ((__jschar)':')       /* colon */

//    Uppercase ASCII letters
#define JS_CHAR_UPPERCASE_A ((__jschar)'A')
#define JS_CHAR_UPPERCASE_B ((__jschar)'B')
#define JS_CHAR_UPPERCASE_C ((__jschar)'C')
#define JS_CHAR_UPPERCASE_D ((__jschar)'D')
#define JS_CHAR_UPPERCASE_E ((__jschar)'E')
#define JS_CHAR_UPPERCASE_F ((__jschar)'F')
#define JS_CHAR_UPPERCASE_G ((__jschar)'G')
#define JS_CHAR_UPPERCASE_H ((__jschar)'H')
#define JS_CHAR_UPPERCASE_I ((__jschar)'I')
#define JS_CHAR_UPPERCASE_J ((__jschar)'J')
#define JS_CHAR_UPPERCASE_K ((__jschar)'K')
#define JS_CHAR_UPPERCASE_L ((__jschar)'L')
#define JS_CHAR_UPPERCASE_M ((__jschar)'M')
#define JS_CHAR_UPPERCASE_N ((__jschar)'N')
#define JS_CHAR_UPPERCASE_O ((__jschar)'O')
#define JS_CHAR_UPPERCASE_P ((__jschar)'P')
#define JS_CHAR_UPPERCASE_Q ((__jschar)'Q')
#define JS_CHAR_UPPERCASE_R ((__jschar)'R')
#define JS_CHAR_UPPERCASE_S ((__jschar)'S')
#define JS_CHAR_UPPERCASE_T ((__jschar)'T')
#define JS_CHAR_UPPERCASE_U ((__jschar)'U')
#define JS_CHAR_UPPERCASE_V ((__jschar)'V')
#define JS_CHAR_UPPERCASE_W ((__jschar)'W')
#define JS_CHAR_UPPERCASE_X ((__jschar)'X')
#define JS_CHAR_UPPERCASE_Y ((__jschar)'Y')
#define JS_CHAR_UPPERCASE_Z ((__jschar)'Z')

//    Lowercase ASCII letters
#define JS_CHAR_LOWERCASE_A ((__jschar)'a')
#define JS_CHAR_LOWERCASE_B ((__jschar)'b')
#define JS_CHAR_LOWERCASE_C ((__jschar)'c')
#define JS_CHAR_LOWERCASE_D ((__jschar)'d')
#define JS_CHAR_LOWERCASE_E ((__jschar)'e')
#define JS_CHAR_LOWERCASE_F ((__jschar)'f')
#define JS_CHAR_LOWERCASE_G ((__jschar)'g')
#define JS_CHAR_LOWERCASE_H ((__jschar)'h')
#define JS_CHAR_LOWERCASE_I ((__jschar)'i')
#define JS_CHAR_LOWERCASE_J ((__jschar)'j')
#define JS_CHAR_LOWERCASE_K ((__jschar)'k')
#define JS_CHAR_LOWERCASE_L ((__jschar)'l')
#define JS_CHAR_LOWERCASE_M ((__jschar)'m')
#define JS_CHAR_LOWERCASE_N ((__jschar)'n')
#define JS_CHAR_LOWERCASE_O ((__jschar)'o')
#define JS_CHAR_LOWERCASE_P ((__jschar)'p')
#define JS_CHAR_LOWERCASE_Q ((__jschar)'q')
#define JS_CHAR_LOWERCASE_R ((__jschar)'r')
#define JS_CHAR_LOWERCASE_S ((__jschar)'s')
#define JS_CHAR_LOWERCASE_T ((__jschar)'t')
#define JS_CHAR_LOWERCASE_U ((__jschar)'u')
#define JS_CHAR_LOWERCASE_V ((__jschar)'v')
#define JS_CHAR_LOWERCASE_W ((__jschar)'w')
#define JS_CHAR_LOWERCASE_X ((__jschar)'x')
#define JS_CHAR_LOWERCASE_Y ((__jschar)'y')
#define JS_CHAR_LOWERCASE_Z ((__jschar)'z')

//  ASCII decimal digits
#define JS_CHAR_0 ((__jschar)'0')
#define JS_CHAR_1 ((__jschar)'1')
#define JS_CHAR_2 ((__jschar)'2')
#define JS_CHAR_3 ((__jschar)'3')
#define JS_CHAR_4 ((__jschar)'4')
#define JS_CHAR_5 ((__jschar)'5')
#define JS_CHAR_6 ((__jschar)'6')
#define JS_CHAR_7 ((__jschar)'7')
#define JS_CHAR_8 ((__jschar)'8')
#define JS_CHAR_9 ((__jschar)'9')

extern bool lit_char_is_octal_digit(__jschar);
extern bool lit_char_is_hex_digit(__jschar);
extern uint32_t lit_char_hex_to_int(__jschar);

// Null character
#define JS_CHAR_NULL ((__jschar)'\0')

typedef enum {
  invalid_token,     /**< error token */
  end_token,         /**< end of stream reached */
  number_token,      /**< JS_CHAR number */
  string_token,      /**< JS_CHAR string */
  null_token,        /**< JS_CHAR null primitive value */
  true_token,        /**< JS_CHAR true primitive value */
  false_token,       /**< JS_CHAR false primitive value */
  left_brace_token,  /**< JS_CHAR left brace */
  right_brace_token, /**< JS_CHAR right brace */
  left_square_token, /**< JS_CHAR left square bracket */
  comma_token,       /**< JS_CHAR comma */
  colon_token        /**< JS_CHAR colon */
} __json_token_type;

typedef struct {
  __json_token_type type; /**< type of the current token */
  uint8_t *current;       /**< current position of the string processed by the parser */
  uint8_t *end;           /**< end of the string processed by the parser */
  union {
    struct {
      uint8_t *start; /**< when type is string_token, it contains the start of the string */
      uint32_t size;  /**< when type is string_token, it contains the size of the string */
    } str;
    int32_t number; /**< when type is number_token, it contains the value of the number */
  } u;
} __json_token;

typedef struct __json_node {
  __jsvalue value;
  struct __json_node *prev;
  struct __json_node *next;
} __json_node;

typedef struct __json_list {
  uint32_t count;
  struct __json_node *first;
  struct __json_node *last;
} __json_list;

typedef struct {
  __json_list *property_list;

  __json_list *stack;

  __jsstring *indent_str;
  __jsstring *gap_str;
  __jsobject *replacer_function;
} __json_stringify_context;

__jsvalue __json_toSource(__jsvalue *this_json, __jsvalue *text, __jsvalue *reviver);
__jsvalue __json_parse(__jsvalue *this_json, __jsvalue *text, __jsvalue *reviver);
__jsvalue __json_stringify(__jsvalue *this_json, __jsvalue *value, __jsvalue *replacer, __jsvalue *space);
__jsvalue __json_str(__jsvalue *key, __jsobject *holder, __json_stringify_context *context);
__jsstring *__json_quote(__jsstring *value);
__jsvalue __json_object(__jsobject *val, __json_stringify_context *context);
__jsvalue __json_array(__jsobject *val, __json_stringify_context *context);
#endif

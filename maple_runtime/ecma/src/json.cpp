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

#include "json.h"
#include "jsvalue.h"
#include "jsvalueinline.h"
#include "jsobject.h"
#include "jsobjectinline.h"
#include "jsarray.h"
#include "jstycnv.h"
#include "jsfunction.h"
#include "jsnum.h"
#include "jsboolean.h"
#include "cmpl.h"
#include "jsmath.h"
#include "vmmemory.h"
#include "securec.h"

bool __json_char_is_decimal_digit(__jschar c) {
  return (c >= JS_CHAR_0 && c <= JS_CHAR_9);
}

static void __js_list_append(__json_list *list, __jsvalue elem) {
  __json_node *node = (__json_node *)VMMallocNOGC(sizeof(__json_node));
  node->value = elem;
  GCCheckAndIncRf(elem.x.asbits, IsNeedRc(elem.ptyp));
  node->next = NULL;
  node->prev = list->last;
  if (list->count == 0) {
    list->first = node;
  } else {
    list->last->next = node;
  }
  list->last = node;
  list->count++;
}

static void __js_list_pop(__json_list *list) {
  MAPLE_JS_ASSERT(list->count > 0);
  __json_node *prev_node = list->last->prev;
  GCCheckAndDecRf(list->last->value.x.asbits, IsNeedRc((list->last->value.ptyp)));
  VMFreeNOGC(list->last, sizeof(__json_node));
  if (prev_node) {
    prev_node->next = NULL;
  }
  list->last = prev_node;
  if (list->count == 1) {
    list->first = NULL;
  }
  list->count--;
}

static bool __contains_value(__json_list *list, __jsvalue val) {
  __json_node *node = list->first;
  uint32_t i = 0;
  while (i < list->count) {
    __jsvalue tmp = node->value;
    if (__jsop_stricteq(&tmp, &val)) {
      return true;
    }
    node = node->next;
    i++;
  }
  return false;
}

__jsvalue __json_walk(__jsobject *reviver, __jsobject *holder, __jsvalue *name) {
  __jsvalue val = __jsobj_internal_Get(holder, name);
  if (__is_js_object(&val)) {
    __jsobject *obj = __jsval_to_object(&val);
    if (obj->object_class == JSARRAY) {
      uint32_t i = 0;
      uint32_t len = __jsobj_helper_get_length(obj);
      while (i < len) {
        __jsvalue id = __number_value(i);
        __jsvalue idx = __string_value(__js_ToString(&id));
        __jsvalue newelem = __json_walk(reviver, obj, &idx);
        if (__is_undefined(&newelem)) {
          __jsobj_internal_Delete(obj, i);
        } else {
          __jsobj_helper_add_value_property(obj, &idx, &newelem, JSPROP_DESC_HAS_VWEC);
        }
        i++;
      }
    } else {
      __jsvalue keys = __jsobj_keys(NULL, &val);
      __jsobject *arr = __jsval_to_object(&keys);
      MAPLE_JS_ASSERT(arr->object_class == JSARRAY);
      uint32_t len = __jsobj_helper_get_length(arr);
      uint32_t i = 0;
      for (i = 0; i < len; i++) {
        __jsvalue p = __jsarr_GetElem(arr, i);
        __jsvalue newelem = __json_walk(reviver, obj, &p);
        if (__is_undefined(&newelem)) {
          __jsobj_internal_Delete(obj, &p);
        } else {
          __jsobj_helper_add_value_property(obj, &p, &newelem, JSPROP_DESC_HAS_VWEC);
        }
      }
      memory_manager->ManageObject(arr, RECALL);
    }
  }
  __jsvalue arg_list[2];
  arg_list[0] = *name;
  arg_list[1] = val;
  __jsvalue holder_val = __object_value(holder);
  return __jsfun_internal_call(reviver, &holder_val, arg_list, 2);
}

static void __json_parse_string(__json_token *token) {
  uint8_t *current = token->current;
  token->u.str.start = current;

  uint32_t num = 0;
  while (*current != JS_CHAR_DOUBLE_QUOTE) {
    if (*current <= 0x1f) {
      return;
    }

    if (*current == JS_CHAR_BACKSLASH) {
      current++;
      switch (*current) {
        case JS_CHAR_DOUBLE_QUOTE:
        case JS_CHAR_SLASH:
        case JS_CHAR_BACKSLASH:
          *(current - 1) = JS_CHAR_NULL;
          num++;
          break;
        case JS_CHAR_LOWERCASE_B:
          *(current - 1) = JS_CHAR_NULL;
          num++;
          *current = JS_CHAR_BS;
          break;
        case JS_CHAR_LOWERCASE_F:
          *(current - 1) = JS_CHAR_NULL;
          num++;
          *current = JS_CHAR_FF;
          break;
        case JS_CHAR_LOWERCASE_N:
          *(current - 1) = JS_CHAR_NULL;
          num++;
          *current = JS_CHAR_LF;
          break;
        case JS_CHAR_LOWERCASE_R:
          *(current - 1) = JS_CHAR_NULL;
          *current = JS_CHAR_CR;
          num++;
          break;
        case JS_CHAR_LOWERCASE_T:
          *(current - 1) = JS_CHAR_NULL;
          *current = JS_CHAR_TAB;
          num++;
          break;
        case JS_CHAR_LOWERCASE_U: {
          char u[8];
          for (int i = 0; i < 4; i++) {
            uint8_t c = *(current + i + 1);
            if ((c >= '0' && c <= '9') || (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f'))
              u[i] = c;
            else
              MAPLE_JS_SYNTAXERROR_EXCEPTION();
          }
          u[4] = 0;
          wchar_t w = strtol(u, NULL, 16);
          if ( w <= 0xff) {
            *(current - 1) = JS_CHAR_NULL;
            *current = w;
            for (int i = 1; i <= 4; i++) {
              *(current + i) = JS_CHAR_NULL;
            }
            num += 5;
          } else {
            // TODO: w > 0xff, current whould be wchar_t
            MAPLE_JS_SYNTAXERROR_EXCEPTION();
          }
          current += 4;
          break;
        }
        default:
          return;
      }
    }
    current++;
  }

  token->u.str.size = (uint32_t)(current - token->u.str.start - num);
  token->current = current + 1;
  token->type = string_token;
}

static void __json_parse_number(__json_token *token) {
  uint8_t *current = token->current;
  uint8_t *start = current;

  if (*current == JS_CHAR_MINUS) {
    current++;
  }

  if (*current == JS_CHAR_0) {
    current++;
    if (__json_char_is_decimal_digit(*current)) {
      return;
    }
  } else if (__json_char_is_decimal_digit(*current)) {
    do {
      current++;
    } while (__json_char_is_decimal_digit(*current));
  }

  if (*current == JS_CHAR_LOWERCASE_E || *current == JS_CHAR_UPPERCASE_E) {
    current++;
    if (*current == JS_CHAR_PLUS || *current == JS_CHAR_MINUS) {
      current++;
    }

    if (!__json_char_is_decimal_digit(*current)) {
      return;
    }

    do {
      current++;
    } while (__json_char_is_decimal_digit(*current));
  }
  bool has_trailing_space = false;
  while (current != token->end && (*current == JS_CHAR_SP || *current == JS_CHAR_CR || *current == JS_CHAR_LF || *current == JS_CHAR_TAB)) {
    current++;
    has_trailing_space = true;
  }
  if (has_trailing_space && current != token->end && __json_char_is_decimal_digit(*current))
    MAPLE_JS_SYNTAXERROR_EXCEPTION();

  token->type = number_token;
  uint32_t length = (uint32_t)(current - start);
  __jsstring *str = __js_new_string_internal(length, false);
  for (uint32_t i = 0; i < length; i++) {
    __jsstr_set_char(str, i, (uint16_t)start[i]);
  }
  __jsvalue val = __string_value(str);
  token->u.number = __js_ToNumber(&val);
  token->current = current;
  memory_manager->RecallString(str);
}

/**
 * Compare the string with an ID.
 *
 * @return true if the match is successful
 */
static bool __json_check_id(uint8_t *string, const char *id) {
  MAPLE_JS_ASSERT(*string == *id);

  do {
    string++;
    id++;
    if (*id == JS_CHAR_NULL) {
      return true;
    }
  } while (*string == *id);
  return false;
}

/**
 * func name：__json_parse_next_token
 * function：Parse next token.The function fills the fields of the __json_token
 * argument：string pointer.
 */
static void __json_parse_next_token(__json_token *token) {
  uint8_t *current = token->current;
  token->type = invalid_token;

  /*
   * No need for end check since the string is zero terminated.
   */
  while (*current == JS_CHAR_SP || *current == JS_CHAR_CR || *current == JS_CHAR_LF || *current == JS_CHAR_TAB) {
    current++;
  }

  if (current == token->end) {
    token->type = end_token;
    return;
  }

  switch (*current) {
    case JS_CHAR_LEFT_BRACE: {
      token->type = left_brace_token;
      token->current = current + 1;
      return;
    }
    case JS_CHAR_RIGHT_BRACE: {
      token->type = right_brace_token;
      token->current = current + 1;
      return;
    }
    case JS_CHAR_LEFT_SQUARE: {
      token->type = left_square_token;
      token->current = current + 1;
      return;
    }
    case JS_CHAR_COMMA: {
      token->type = comma_token;
      token->current = current + 1;
      return;
    }
    case JS_CHAR_COLON: {
      token->type = colon_token;
      token->current = current + 1;
      return;
    }
    case JS_CHAR_DOUBLE_QUOTE: {
      token->current = current + 1;
      __json_parse_string(token);
      return;
    }
    case JS_CHAR_LOWERCASE_N: {
      if (__json_check_id(current, "null")) {
        token->type = null_token;
        token->current = current + 4;
        return;
      }
      break;
    }
    case JS_CHAR_LOWERCASE_T: {
      if (__json_check_id(current, "true")) {
        token->type = true_token;
        token->current = current + 4;
        return;
      }
      break;
    }
    case JS_CHAR_LOWERCASE_F: {
      if (__json_check_id(current, "false")) {
        token->type = false_token;
        token->current = current + 5;
        return;
      }
      break;
    }
    default: {
      if (*current == JS_CHAR_MINUS || __json_char_is_decimal_digit(*current)) {
        token->current = current;
        __json_parse_number(token);
        return;
      }
      break;
    }
  }
}

static bool __json_check_right_square_token(__json_token *token) {
  uint8_t *current = token->current;

  while (*current == JS_CHAR_SP || *current == JS_CHAR_CR || *current == JS_CHAR_LF || *current == JS_CHAR_TAB) {
    current++;
  }

  token->current = current;

  if (*current == JS_CHAR_RIGHT_SQUARE) {
    token->current = current + 1;
    return true;
  }
  return false;
}

__jsvalue __json_parse_value(__json_token *token) {
  __json_parse_next_token(token);

  switch (token->type) {
    case number_token: {
      int32_t number = token->u.number;
      return __number_value(number);
    }
    case string_token: {
      __jsstring *str = __js_new_string_internal(token->u.str.size, false);
      uint32_t i = 0;
      uint32_t j = 0;
      while (i < token->u.str.size) {
        uint8_t ch = *(token->u.str.start + j);
        if (ch != JS_CHAR_NULL) {
          __jsstr_set_char(str, i, ch);
          i++;
        }
        j++;
      }

      return __string_value(str);
    }
    case null_token:
      return __null_value();
    case true_token:
      return __boolean_value(true);
    case false_token:
      return __boolean_value(false);
    case left_brace_token: {
      bool parse_comma = false;
      __jsobject *obj = __js_new_obj_obj_0();

      while (true) {
        __json_parse_next_token(token);

        if (token->type == invalid_token)
          MAPLE_JS_SYNTAXERROR_EXCEPTION();

        if (token->type == right_brace_token) {
          return __object_value(obj);
        }

        if (parse_comma) {
          if (token->type != comma_token) {
            break;
          }
          __json_parse_next_token(token);
        }

        if (token->type != string_token) {
          break;
        }

        uint8_t *string_start = token->u.str.start;
        uint32_t string_size = token->u.str.size;
        __json_parse_next_token(token);

        if (token->type != colon_token) {
          break;
        }

        __jsvalue value = __json_parse_value(token);

        if (__is_undefined(&value)) {
          break;
        }

        __jsstring *name = __js_new_string_internal(string_size, false);
        GCIncRf(name);
        for (uint32_t i = 0; i < string_size; i++) {
          __jsstr_set_char(name, i, (uint16_t)(*(string_start + i)));
        }
        __jsobj_helper_add_value_property(obj, name, &value, JSPROP_DESC_HAS_VWEC);
        GCDecRf(name);
        parse_comma = true;
      }
      return __undefined_value();
    }
    case left_square_token: {
      bool parse_comma = false;
      uint32_t length = 0;

      __jsobject *arr = __js_new_arr_internal(0);
      while (true) {
        if (__json_check_right_square_token(token)) {
          return __object_value(arr);
        }

        if (parse_comma) {
          __json_parse_next_token(token);
          if (token->type != comma_token) {
            break;
          }
        }

        __jsvalue value = __json_parse_value(token);

        if (__is_undefined(&value)) {
          break;
        }

        __set_generic_elem(arr, length, &value);
        length++;
        parse_comma = true;
      }

      return __undefined_value();
    }
    default:
      MAPLE_JS_SYNTAXERROR_EXCEPTION();
  }
  return __undefined_value();
}

__jsvalue __json_toSource(__jsvalue *this_json, __jsvalue *text, __jsvalue *reviver) {
  assert(false&&"__json_toSource nyi");
}
// ecma 15.12.2
__jsvalue __json_parse(__jsvalue *this_json, __jsvalue *text, __jsvalue *reviver) {
  __jsstring *jtext = __js_ToString(text);
  //  MAPLE_JS_ASSERT(__jsstr_is_ascii(jtext));
  __json_token token;
  uint8_t *start = (uint8_t *)jtext + 4;
  token.current = start;
  token.end = start + __jsstr_get_length(jtext);
  __jsvalue res = __json_parse_value(&token);
  if (__js_IsCallable(reviver)) {
    __jsobject *root = __js_new_obj_obj_0();
    __jsstring *p = __jsstr_get_builtin(JSBUILTIN_STRING_EMPTY);
    __jsvalue name = __string_value(p);
    __jsobj_helper_add_value_property(root, JSBUILTIN_STRING_EMPTY, &res, JSPROP_DESC_HAS_VWEC);
    return __json_walk(__js_ToObject(reviver), root, &name);
  } else {
    return res;
  }

  return res;
}

// 15.12.3 stringify ( value [ , replacer [ , space ] ] )
__jsvalue __json_stringify(__jsvalue *this_json, __jsvalue *value, __jsvalue *replacer, __jsvalue *space) {
  __json_stringify_context context;
  // 1. Let stack be an empty List
  context.stack = (__json_list *)VMMallocGC(sizeof(__json_list), MemHeadJSList);
  errno_t set_ret = memset_s(context.stack, sizeof(__json_list), 0, sizeof(__json_list));
  if (set_ret != EOK) {
    MIR_FATAL("call memset_s failed in __json_stringify");
  }

  // 2
  context.indent_str = __jsstr_get_builtin(JSBUILTIN_STRING_EMPTY);
  // 3.
  context.property_list = (__json_list *)VMMallocGC(sizeof(__json_list), MemHeadJSList);
  errno_t set_ret1 = memset_s(context.property_list, sizeof(__json_list), 0, sizeof(__json_list));
  if (set_ret1 != EOK) {
    MIR_FATAL("call memset_s failed in __json_stringify");
  }
  context.replacer_function = NULL;
  // 4
  if (__is_js_object(replacer)) {
    // a
    if (__js_IsCallable(replacer)) {
      context.replacer_function = __js_ToObject(replacer);
    } else {
      __jsobject *obj = __jsval_to_object(replacer);
      if (obj->object_class == JSARRAY) {
        uint32_t length = __jsobj_helper_get_length(obj);
        uint32_t i = 0;
        __jsvalue v, item;
        __jsstring *str = NULL;
        for (i = 0; i < length; i++) {
          v = __jsarr_GetElem(obj, i);
          item = __undefined_value();
          if (__is_string(&v) || __is_number(&v)) {
            str = __js_ToString(&v);
            item = __string_value(str);
          } else if (__is_js_object(&v)) {
            __jsobject *obj1 = __jsval_to_object(&v);
            if (obj1->object_class == JSSTRING || obj1->object_class == JSNUMBER) {
              str = __js_ToString(&v);
              item = __string_value(str);
            }
          }
          if (__contains_value(context.property_list, item)) {
            if (!__is_string(&v)) {
              memory_manager->RecallString(str);
            }
            continue;
          }
          if (!__is_undefined(&item)) {
            __js_list_append(context.property_list, item);
          }
        }
      }
    }
  }
  // 5.
  if (__is_js_object(space)) {
    __jsobject *obj1 = __jsval_to_object(space);
    if (obj1->object_class == JSSTRING) {
      *space = __string_value(__js_ToString(space));
    } else if (obj1->object_class == JSNUMBER) {
      *space = __number_value(__js_ToNumber(space));
    }
  }
  // 6.
  if (__is_string(space)) {
    __jsstring *temp = __jsval_to_string(space);
    uint32_t len = __jsstr_get_length(temp);
    if (len > 10) {
      len = 10;
    }
    context.gap_str = __js_new_string_internal(len, false);
    __jsstr_copy(context.gap_str, 0, temp);
  } else {
    int32_t len = 0;
    if (__is_double(space)) {
      double v = __jsval_to_double(space);
      if (v > 0)
        len = (int32_t) floor(v);
    } else if (__is_number(space)) {
      len = __js_ToNumber(space);
    }
    if (len <= 0)
      context.gap_str = __jsstr_get_builtin(JSBUILTIN_STRING_EMPTY);
    else {
      if (len > 10) {
        *space = __number_value(10);
        len = 10;
      }
      context.gap_str = __js_new_string_internal(len, false);
      for (int32_t i = 0; i < len; i++) {
        __jsstr_set_char(context.gap_str, i, JS_CHAR_SP);
       }
    }
  }
  // 9
  __jsobject *wrapper = __js_new_obj_obj_0();
  GCIncRf(wrapper);
  __jsstring *empty = __jsstr_get_builtin(JSBUILTIN_STRING_EMPTY);
  __jsvalue name = __string_value(empty);
  __jsobj_helper_add_value_property(wrapper, JSBUILTIN_STRING_EMPTY, value, JSPROP_DESC_HAS_VWEC);
  __jsvalue ret = __json_str(&name, wrapper, &context);
  GCDecRf(wrapper);
  // Fixme: Need use inc-dec-rf pair.
  memory_manager->RecallString(context.gap_str);
  memory_manager->RecallList(context.property_list);
  memory_manager->RecallList(context.stack);
  return ret;
}

__jsvalue __json_str(__jsvalue *key, __jsobject *holder, __json_stringify_context *context) {
  // 1.
  __jsvalue value = __jsobj_internal_Get(holder, key);
  GCCheckAndIncRf(value.x.asbits, IsNeedRc((value.ptyp)));
  __jsstring *str;
  // 2.
  if (__is_js_object(&value)) {
    __jsobject *obj = __jsval_to_object(&value);
    // 2.a
    __jsvalue to_json = __jsobj_internal_Get(obj, JSBUILTIN_STRING_TO_JSON_UL);
    // 2.b
    if (__js_IsCallable(&to_json)) {
      __jsvalue temp = __jsfun_internal_call(__jsval_to_object(&to_json), &value, key, 1);
      GCCheckAndUpdateRf(value.x.asbits, IsNeedRc(value.ptyp), temp.x.asbits, IsNeedRc((temp.ptyp)));
      value = temp;
    }
  }
  // 3
  if (context->replacer_function) {
    __jsvalue args[2];
    args[0] = *key;
    args[1] = value;
    __jsvalue hold = __object_value(holder);
    __jsvalue temp = __jsfun_internal_call(context->replacer_function, &hold, args, 2);
    GCCheckAndUpdateRf(value.x.asbits, IsNeedRc(value.ptyp), temp.x.asbits, IsNeedRc((temp.ptyp)));
    value = temp;
  }
  // 4
  if (__is_js_object(&value)) {
    __jsobject *obj = __jsval_to_object(&value);
    __jsvalue temp = value;
    if (obj->object_class == JSNUMBER) {
      temp = __number_value(__js_ToNumber(&value));
    } else if (obj->object_class == JSSTRING) {
      temp = __string_value(__js_ToString(&value));
    } else if (obj->object_class == JSBOOLEAN) {
      temp = __boolean_value(obj->shared.prim_bool);
    }

    GCCheckAndUpdateRf(value.x.asbits, IsNeedRc(value.ptyp), temp.x.asbits, IsNeedRc((temp.ptyp)));
    value = temp;
  }

  __jsvalue res = __undefined_value();
  switch (__jsval_typeof(&value)) {
    case JSTYPE_NULL:
      str = __jsstr_new_from_char("null");
      res = __string_value(str);
      break;
    case JSTYPE_BOOLEAN:
      if (value.x.boo == true) {
        str = __jsstr_new_from_char("true");
      } else {
        str = __jsstr_new_from_char("false");
      }
      res = __string_value(str);
      break;
    case JSTYPE_STRING:
      str = __json_quote(__jsval_to_string(&value));
      res = __string_value(str);
      break;
    case JSTYPE_NUMBER:
    case JSTYPE_DOUBLE:
      str = __js_ToString(&value);
      res = __string_value(str);
      break;
    case JSTYPE_OBJECT:
      if (!__js_IsCallable(&value)) {
        __jsobject *obj = __jsval_to_object(&value);
        if (obj->object_class == JSARRAY) {
          res = __json_array(obj, context);
        } else {
          res = __json_object(obj, context);
        }
      }
      break;
    default:
      break;
  }
  GCCheckAndDecRf(value.x.asbits, IsNeedRc((value.ptyp)));
  return res;
}

__jsstring *__json_quote(uint32_t index) {
  __jsstring *product = __js_new_string_internal(1, false);
  __jsstr_set_char(product, 0, JS_CHAR_DOUBLE_QUOTE);
  __jsstring *str = __js_NumberToString(index);
  product = __jsstr_concat_2(product, str);
  return __jsstr_append_char(product, JS_CHAR_DOUBLE_QUOTE);
}

// The abstract operation Quote(value)
__jsstring *__json_quote(__jsstring *value) {
  __jsstring *product = __js_new_string_internal(1, false);
  __jsstr_set_char(product, 0, JS_CHAR_DOUBLE_QUOTE);
  // MAPLE_JS_ASSERT(__jsstr_is_ascii(value));
  uint32_t len = __jsstr_get_length(value);
  for (uint32_t i = 0; i < len; i++) {
    uint16_t c;
    if (__jsstr_is_ascii(value)) {
      c = ((uint8_t *)value)[4 + i];
    } else {
      c = ((uint16_t *)value)[2 + i];
    }
    if (c == JS_CHAR_DOUBLE_QUOTE || c == JS_CHAR_BACKSLASH) {
      product = __jsstr_append_char(product, JS_CHAR_BACKSLASH);
      product = __jsstr_append_char(product, c);
    } else if (c == JS_CHAR_BS || c == JS_CHAR_FF || c == JS_CHAR_LF || c == JS_CHAR_CR || c == JS_CHAR_TAB) {
      product = __jsstr_append_char(product, JS_CHAR_BACKSLASH);
      char abbrev = '\0';
      switch (c) {
        case JS_CHAR_BS:
          abbrev = 'b';
          break;
        case JS_CHAR_FF:
          abbrev = 'f';
          break;
        case JS_CHAR_LF:
          abbrev = 'n';
          break;
        case JS_CHAR_CR:
          abbrev = 'r';
          break;
        case JS_CHAR_TAB:
          abbrev = 't';
          break;
        default:
          MAPLE_JS_ASSERT(false);
          break;
      }
      product = __jsstr_append_char(product, (uint16_t)abbrev);
    } else if (c <= 0x1f || c >= 0x80) {
      product = __jsstr_append_char(product, JS_CHAR_BACKSLASH);
      product = __jsstr_append_char(product, JS_CHAR_LOWERCASE_U);
      char h[8];
      sprintf(h, "%04x", c);
      for (int i = 0; i < 4; i++) {
        product = __jsstr_append_char(product, h[i]);
      }
    } else {
      product = __jsstr_append_char(product, c);
    }
  }
  product = __jsstr_append_char(product, JS_CHAR_DOUBLE_QUOTE);
  return product;
}

// The abstract operation JO(value) serializes an object
__jsvalue __json_object(__jsobject *val, __json_stringify_context *context) {
  __jsvalue value = __object_value(val);
  // 1.
  if (__contains_value(context->stack, value)) {
    MAPLE_JS_TYPEERROR_EXCEPTION();
  }
  // 2
  __js_list_append(context->stack, value);
  // 3
  __jsstring *step_back = context->indent_str;
  // 4
  context->indent_str = __jsstr_concat_2(context->indent_str, context->gap_str);
  // 5. 6
  __json_list *k;
  if (context->property_list->count != 0) {
    k = context->property_list;
  } else {
    __jsvalue keys = __jsobj_keys(NULL, &value);
    __jsobject *obj = __jsval_to_object(&keys);
    k = (__json_list *)VMMallocGC(sizeof(__json_list), MemHeadJSList);
    errno_t ret1 = memset_s(k, sizeof(__json_list), 0, sizeof(__json_list));
    if (ret1 != EOK) {
      MIR_FATAL("call memset_s firstly failed in __json_object ");
    }
    for (uint32_t i = 0; i < __jsobj_helper_get_length(obj); i++) {
      __js_list_append(k, __jsarr_GetElem(obj, i));
    }
    memory_manager->ManageObject(obj, RECALL);
  }
  // 7
  __json_list *partial = (__json_list *)VMMallocGC(sizeof(__json_list), MemHeadJSList);
  errno_t ret2 = memset_s(partial, sizeof(__json_list), 0, sizeof(__json_list));
  if (ret2 != EOK) {
    MIR_ASSERT("call memset_s secondly failed in __json_object ");
  }
  // 08
  __json_node *node = k->first;
  uint32_t i = 0;
  while (i < k->count) {
    __jsvalue p = node->value;
    // 8.a
    __jsvalue str = __json_str(&p, val, context);
    // 8.b
    if (!__is_undefined(&str)) {
      // 8.b.i
      __jsstring *member = NULL;
      if (__jsval_typeof(&(node->value)) == JSTYPE_NUMBER) {
        member = __json_quote(__jsval_to_int32(&(node->value)));
      } else if (__jsval_typeof(&(node->value)) == JSTYPE_DOUBLE) {
        member = __json_quote(__jsval_to_double(&(node->value)));
      } else {
        member = __json_quote(__jsval_to_string(&(node->value)));
      }
      // 8.b.2
      member = __jsstr_append_char(member, JS_CHAR_COLON);
      // 8.b.3
      if (__jsstr_get_length(context->gap_str)) {
        member = __jsstr_append_char(member, JS_CHAR_SP);
      }
      // 8.b.iv
      __jsstring *str_val = __jsval_to_string(&str);
      __jsstring *tmp = __jsstr_concat_2(member, str_val);
      memory_manager->RecallString(member);
      memory_manager->RecallString(str_val);
      member = tmp;
      // 8.b.v
      __js_list_append(partial, __string_value(member));
    }
    i++;
    node = node->next;
  }
  if (context->property_list->count == 0) {
    memory_manager->RecallList(k);
  }
  __jsstring *final_str;
  // 9.
  if (partial->count == 0) {
    final_str = __js_new_string_internal(2, false);
    __jsstr_set_char(final_str, 0, JS_CHAR_LEFT_BRACE);
    __jsstr_set_char(final_str, 1, JS_CHAR_RIGHT_BRACE);
  } else {
    // 10.
    __jsstring *left_brace = __jsstr_get_builtin(JSBUILTIN_STRING_LEFT_BRACE_CHAR);
    __jsstring *right_brace = __jsstr_get_builtin(JSBUILTIN_STRING_RIGHT_BRACE_CHAR);
    // 10.a
    if (__jsstr_get_length(context->gap_str) == 0) {
      // 10.a.i
      __json_node *node = partial->first;
      __jsstring *property = __jsval_to_string(&(node->value));
      __jsstring *comma = __jsstr_get_builtin(JSBUILTIN_STRING_COMMA_CHAR);
      node = node->next;
      while (node) {
        __jsstring *item = __jsval_to_string(&(node->value));
        __jsstring *tmp = __jsstr_concat_3(property, comma, item);
        memory_manager->RecallString(property);
        property = tmp;
        node = node->next;
      }
      // 10.a.ii
      final_str = __jsstr_concat_3(left_brace, property, right_brace);
      memory_manager->RecallString(property);
    } else {
      // 10.b
      // 10.b.i
      uint32_t len = __jsstr_get_length(context->indent_str);
      __jsstring *sep = __js_new_string_internal(len + 2, false);
      __jsstr_set_char(sep, 0, JS_CHAR_COMMA);
      __jsstr_set_char(sep, 1, JS_CHAR_LF);
      __jsstr_copy(sep, 2, context->indent_str);
      // 10.b.ii
      __json_node *node = partial->first;
      __jsstring *property = __jsval_to_string(&(node->value));
      node = node->next;
      __jsstring *tmp;
      while (node) {
        __jsstring *item = __jsval_to_string(&(node->value));
        tmp = __jsstr_concat_3(property, sep, item);
        memory_manager->RecallString(property);
        property = tmp;
        node = node->next;
      }
      // 10.b.iii
      __jsstring *left = __jsstr_append_char(left_brace, JS_CHAR_LF);
      final_str = __jsstr_concat_3(left, context->indent_str, property);
      memory_manager->RecallString(left);
      tmp = __jsstr_append_char(final_str, JS_CHAR_LF);
      final_str = __jsstr_concat_3(tmp, step_back, right_brace);
      memory_manager->RecallString(tmp);
      memory_manager->RecallString(sep);
      memory_manager->RecallString(property);
    }
  }
  memory_manager->RecallList(partial);
  // 11.
  __js_list_pop(context->stack);
  memory_manager->RecallString(context->indent_str);
  context->indent_str = step_back;
  return __string_value(final_str);
}

// The abstract operation JA(value) serializes an array.
__jsvalue __json_array(__jsobject *val, __json_stringify_context *context) {
  __jsvalue value = __object_value(val);
  __jsstring *final_str;
  // 1.
  if (__contains_value(context->stack, value)) {
    MAPLE_JS_TYPEERROR_EXCEPTION();
  }
  // 2
  __js_list_append(context->stack, value);
  // 3
  __jsstring *step_back = context->indent_str;
  // 4
  context->indent_str = __jsstr_concat_2(context->indent_str, context->gap_str);
  // 5
  __json_list *partial = (__json_list *)VMMallocGC(sizeof(__json_list), MemHeadJSList);
  errno_t set_ret = memset_s(partial, sizeof(__json_list), 0, sizeof(__json_list));
  if (set_ret != EOK) {
    MIR_FATAL("call memset_s failed in __json_array");
  }
  // 6
  uint32_t len = __jsobj_helper_get_length(val);
  // 7
  uint32_t index = 0;
  // 8
  while (index < len) {
    __jsvalue id = __number_value(index);
    __jsstring *ind_str = __js_ToString(&id);
    __jsvalue ind = __string_value(ind_str);
    // 8.a
    GCIncRf(ind_str);
    __jsvalue strp = __json_str(&ind, val, context);
    GCDecRf(ind_str);
    // 8.b
    if (__is_undefined(&strp)) {
      __jsvalue null = __string_value(__jsstr_get_builtin(JSBUILTIN_STRING_NULL));
      __js_list_append(partial, null);
    } else {
      __js_list_append(partial, strp);
    }
    index++;
  }
  // 9
  if (partial->count == 0) {
    final_str = __js_new_string_internal(2, false);
    __jsstr_set_char(final_str, 0, JS_CHAR_LEFT_SQUARE);
    __jsstr_set_char(final_str, 1, JS_CHAR_RIGHT_SQUARE);
  } else {
    // 10.
    __jsstring *left_square = __jsstr_get_builtin(JSBUILTIN_STRING_LEFT_SQUARE_CHAR);
    __jsstring *right_square = __jsstr_get_builtin(JSBUILTIN_STRING_RIGHT_SQUARE_CHAR);
    // 10.a
    if (__jsstr_get_length(context->gap_str) == 0) {
      // 10.a.i
      __json_node *node = partial->first;
      __jsstring *property = __jsval_to_string(&(node->value));
      __jsstring *comma = __jsstr_get_builtin(JSBUILTIN_STRING_COMMA_CHAR);
      node = node->next;
      if (node) {
        MAPLE_JS_ASSERT(__is_string(&(node->value)));
        __jsstring *item = __jsval_to_string(&(node->value));
        property = __jsstr_concat_3(property, comma, item);
        node = node->next;
        while (node) {
          MAPLE_JS_ASSERT(__is_string(&(node->value)));
          __jsstring *item = __jsval_to_string(&(node->value));
          __jsstring *temp = __jsstr_concat_3(property, comma, item);
          memory_manager->RecallString(property);
          property = temp;
          node = node->next;
        }
        final_str = __jsstr_concat_3(left_square, property, right_square);
        memory_manager->RecallString(property);
      } else {
        // 10.a.ii
        final_str = __jsstr_concat_3(left_square, property, right_square);
      }
    } else {
      // 10.b
      // 10.b.i
      uint32_t len = __jsstr_get_length(context->indent_str);
      __jsstring *sep = __js_new_string_internal(len + 2, false);
      __jsstr_set_char(sep, 0, JS_CHAR_COMMA);
      __jsstr_set_char(sep, 1, JS_CHAR_LF);
      __jsstr_copy(sep, 2, context->indent_str);
      // 10.b.ii
      __json_node *node = partial->first;
      __jsstring *property = __jsval_to_string(&(node->value));
      node = node->next;
      for (uint32_t i = 1; i < partial->count; i++) {
        MAPLE_JS_ASSERT(__is_string(&(node->value)));
        __jsstring *item = __jsval_to_string(&(node->value));
        __jsstring *tmp = __jsstr_concat_3(property, sep, item);
        memory_manager->RecallString(property);
        property = tmp;
        node = node->next;
      }
      memory_manager->RecallString(sep);
      // 10.b.iii
      __jsstring *left = __jsstr_append_char(left_square, JS_CHAR_LF);
      __jsstring *tmp = __jsstr_concat_3(left, context->indent_str, property);
      memory_manager->RecallString(left);
      memory_manager->RecallString(property);
      final_str = __jsstr_append_char(tmp, JS_CHAR_LF);
      tmp = __jsstr_concat_3(final_str, step_back, right_square);
      memory_manager->RecallString(final_str);
      final_str = tmp;
    }
  }
  memory_manager->RecallList(partial);
  // 11.
  __js_list_pop(context->stack);
  // 12
  memory_manager->RecallString(context->indent_str);
  context->indent_str = step_back;
  // 13
  return __string_value(final_str);
}

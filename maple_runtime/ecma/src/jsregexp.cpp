/*
 * Copyright (c) [2021] Futurewei Technologies Co.,Ltd.All rights reserved.
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

#include "jsglobal.h"
#include "jsvalue.h"
#include "jsvalueinline.h"
#include "jsobject.h"
#include "jsobjectinline.h"
#include "jstycnv.h"
#include "jscontext.h"
#include "jsregexp.h"
#include "jsarray.h"
#include "vmmemory.h"

#define DEFAULT_REGEXP_PATTERN "(?:)"

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

// Convert char array to 2-byte integer array.
static inline void StrToUint16(const char *str, uint16_t *u16_str, int len) {
  for (int i = 0; i < len; i++) {
    u16_str[i] = str[i];
  }
}

// This is the case when RegExp literal is given, e.g., re = /ab+c/g.
__jsobject *__js_ToRegExp(__jsstring *jsstr) {
  MAPLE_JS_ASSERT(jsstr && "shouldn't be null");

  __jsobject *obj = __create_object();
  __jsobj_set_prototype(obj, JSBUILTIN_REGEXPPROTOTYPE);
  obj->object_class = JSREGEXP;
  obj->extensible = true;
  obj->object_type = JSREGULAR_OBJECT;

  __jsstring *js_pattern_flags = jsstr;

  // Find pattern from argument.
  int len = __jsstr_get_length(js_pattern_flags);
  const char slash[] = "/";
  __jsvalue search = __string_value(__jsstr_new_from_char(slash));
  __jsvalue pos = __number_value(0);
  __jsvalue str = __string_value(js_pattern_flags);
  __jsvalue js_i = __jsstr_indexOf(&str, &search, &pos);
  __jsvalue end = __number_value(len);
  __jsvalue js_j = __jsstr_lastIndexOf(&str, &search, &pos);
  int i = __js_ToNumber(&js_i);
  __jsvalue start = __number_value(i+1);
  __jsvalue pattern_val = __jsstr_substring(&str, &start, &js_j);
  __jsstring *js_pattern = __js_ToString(&pattern_val);

  // Find flags from argument.
  pos = __number_value(__js_ToNumber(&js_j)+1);
  __jsstring *js_flags;
  if (__js_ToNumber(&pos) >= len) {
    const char empty[] = "";
    js_flags = __jsstr_new_from_char(empty);
  } else {
    __jsvalue flags_val = __jsstr_substring(&str, &pos, &end);
    js_flags = __js_ToString(&flags_val);
  }

  // ES5 15.10.7.1 Init 'source' property.
  __jsvalue source;
  if (__jsstr_get_length(js_pattern) == 0)
    source = __string_value(__jsstr_new_from_char(DEFAULT_REGEXP_PATTERN));
  else {
    source = __string_value(js_pattern);
  }
  __jsobj_helper_init_value_property(obj, JSBUILTIN_STRING_SOURCE,
                                     &source, JSPROP_DESC_HAS_VUWUEUC);

  bool global = false, ignorecase = false, multiline = false;

  // Set flag options.
  CheckAndSetFlagOptions(js_flags, js_pattern, global, ignorecase, multiline);

  // ES5 15.10.7.2 Init 'global' property.
  __jsvalue js_global = __boolean_value(global);
  __jsobj_helper_init_value_property(obj, JSBUILTIN_STRING_GLOBAL,
                                     &js_global, JSPROP_DESC_HAS_VUWUEUC);

  // ES5 15.10.7.3 Init 'ignoreCase' property.
  __jsvalue js_ignorecase = __boolean_value(ignorecase);
  __jsobj_helper_init_value_property(obj, JSBUILTIN_STRING_IGNORECASE_UL,
                                     &js_ignorecase, JSPROP_DESC_HAS_VUWUEUC);

  // ES5 15.10.7.4 Init 'multiline' property.
  __jsvalue js_multiline = __boolean_value(multiline);
  __jsobj_helper_init_value_property(obj, JSBUILTIN_STRING_MULTILINE,
                                     &js_multiline, JSPROP_DESC_HAS_VUWUEUC);

  // ES5 15.10.7.5 Init 'lastIndex' property.
  int last_index = 0;
  __jsvalue js_last_index = __number_value(last_index);
  __jsobj_helper_init_value_property(obj, JSBUILTIN_STRING_LASTINDEX_UL,
                                     &js_last_index, JSPROP_DESC_HAS_VWUEUC);

  return obj;
}

// ES5 15.10.3 RegExp Constructor
__jsvalue __js_new_regexp_obj(__jsvalue *this_value, __jsvalue *arg_list,
                              uint32_t nargs) {
  __jsobject *obj = __create_object();
  __jsobj_set_prototype(obj, JSBUILTIN_REGEXPPROTOTYPE);
  obj->object_class = JSREGEXP;
  obj->extensible = true;
  obj->object_type = JSREGULAR_OBJECT;

  // Set default property values.
  std::string source;
  bool global = false, ignorecase = false, multiline = false;
  int last_index = 0;

  // Set default property values.
  __jsvalue js_source;
  __jsvalue js_global = __boolean_value(global);
  __jsvalue js_ignorecase = __boolean_value(ignorecase);
  __jsvalue js_multiline = __boolean_value(multiline);
  __jsvalue js_last_index = __number_value(last_index);

  const char empty[] = "";
  __jsstring *js_pattern = __jsstr_new_from_char(empty);
  __jsstring *js_flags = __jsstr_new_from_char(empty);

  if (nargs == 0) {
    // No pattern nor flags provided.
    // Do nothing here.
  } else if (nargs == 1) {
    if (__is_undefined(&arg_list[0])) {
      // Pattern is set at the end.
    } else if (__is_string(&arg_list[0])) {
      // Only pattern is provided.

      // Workaround for JSB5934, causing regression.
      //int len = js_pattern_input->length;
      //pattern_input[len] = '\0';
      //pattern.assign(pattern_input);
      js_pattern = __js_ToString(&arg_list[0]);
    } else if (__is_null(&arg_list[0])) {
      js_pattern = __js_ToString(&arg_list[0]);
    } else {
      // arg_list[0] is object.
      __jsobject *arg_obj = __jsval_to_object(&arg_list[0]);
      if (arg_obj->object_class == JSREGEXP) {
        // ES5 15.10.3.1 flags is undefined, then return R unchanged.
        return arg_list[0];
      } else {
        __jsvalue jsval = __js_ToPrimitive(&arg_list[0], JSTYPE_STRING);
        // TODO: Neeed toprocess flags also ?
        js_pattern = __js_ToString(&arg_list[0]);
      }
    }

  } else if (nargs == 2) {
    // Pattern and flags are provided.

    if (__is_undefined(&arg_list[1])) {
      // Handle rest of operations below.
    } else if (__is_js_object(&arg_list[1])) {
      __jsvalue jsval = __js_ToPrimitive(&arg_list[1], JSTYPE_STRING);
      // Check if arg_list[1] is undefined.
      if (__is_undefined(&jsval)) {
        MAPLE_JS_SYNTAXERROR_EXCEPTION();
      } else {
        js_flags = __js_ToString(&arg_list[1]);
      }
      CheckAndSetFlagOptions(js_flags, js_pattern,
                             global, ignorecase, multiline);

    } else if (!__is_string(&arg_list[1]) && !__is_string_object(&arg_list[1])) {
      MAPLE_JS_SYNTAXERROR_EXCEPTION();
    }

    if (__is_undefined(&arg_list[0])) {
      if (!__is_undefined(&arg_list[1])) {
        js_flags = __js_ToString(&arg_list[1]);

        CheckAndSetFlagOptions(js_flags, js_pattern,
                               global, ignorecase, multiline);
      }
    } else if (__is_string(&arg_list[0]) ||
               __is_null(&arg_list[0]) ||
               __is_boolean(&arg_list[0]) ||
               __is_number(&arg_list[0])) {
      js_pattern = __js_ToString(&arg_list[0]);

      js_flags = __js_ToString(&arg_list[1]);

      CheckAndSetFlagOptions(js_flags, js_pattern,
                             global, ignorecase, multiline);

    } else {  // arg_list[0] is object.
      __jsobject *arg_obj = __jsval_to_object(&arg_list[0]);

      // Check pattern first.
      if (arg_obj->object_class == JSREGEXP) {
        // ES5 15.10.3.1 flags is undefined.
        if (__is_undefined(&arg_list[1])) {
          // Flags is undefined, return this RegExp object.
          return arg_list[0];
        } else {
          // Extract pattern from 'source' property.
          js_source = __jsop_getprop_by_name(&arg_list[0],
                                __jsstr_get_builtin(JSBUILTIN_STRING_SOURCE));
          js_pattern = __js_ToString(&js_source);

          // Extract flags from other properties.
          js_global = __jsop_getprop_by_name(&arg_list[0],
                                __jsstr_get_builtin(JSBUILTIN_STRING_GLOBAL));
          js_ignorecase = __jsop_getprop_by_name(&arg_list[0],
                          __jsstr_get_builtin(JSBUILTIN_STRING_IGNORECASE_UL));
          js_multiline = __jsop_getprop_by_name(&arg_list[0],
                              __jsstr_get_builtin(JSBUILTIN_STRING_MULTILINE));
          js_last_index = __jsop_getprop_by_name(&arg_list[0],
                           __jsstr_get_builtin(JSBUILTIN_STRING_LASTINDEX_UL));
        }
      } else {
        __jsvalue jsval = __js_ToPrimitive(&arg_list[0], JSTYPE_STRING);
        js_pattern = __js_ToString(&jsval);
      }
      // Check flags.
      if (__is_string(&arg_list[1])) {
        js_flags = __js_ToString(&arg_list[1]);
      } else {
        // Object case.
        __jsvalue jsval = __js_ToPrimitive(&arg_list[1], JSTYPE_STRING);
        js_flags = __js_ToString(&jsval);
      }
      CheckAndSetFlagOptions(js_flags, js_pattern,
                             global, ignorecase, multiline);
    } // end of arg_list[0] is object.
  } else {  // nargs > 2
    MAPLE_JS_SYNTAXERROR_EXCEPTION();
  }

  // Set/init properties.
  if (nargs == 0 || __jsstr_get_length(js_pattern) == 0) {
    js_pattern = __jsstr_new_from_char(DEFAULT_REGEXP_PATTERN);
    js_source = __string_value(__jsstr_new_from_char(DEFAULT_REGEXP_PATTERN));
  } else {
    js_source = __string_value(js_pattern);
  } 
  __jsobj_helper_init_value_property(obj, JSBUILTIN_STRING_SOURCE,
                                     &js_source, JSPROP_DESC_HAS_VUWUEUC);
  js_global = __boolean_value(global);  
  __jsobj_helper_init_value_property(obj, JSBUILTIN_STRING_GLOBAL,
                                     &js_global, JSPROP_DESC_HAS_VUWUEUC);
  js_ignorecase = __boolean_value(ignorecase);
  __jsobj_helper_init_value_property(obj, JSBUILTIN_STRING_IGNORECASE_UL,
                                     &js_ignorecase, JSPROP_DESC_HAS_VUWUEUC);
  js_multiline = __boolean_value(multiline);
  __jsobj_helper_init_value_property(obj, JSBUILTIN_STRING_MULTILINE,
                                     &js_multiline, JSPROP_DESC_HAS_VUWUEUC);
  __jsobj_helper_init_value_property(obj, JSBUILTIN_STRING_LASTINDEX_UL,
                                       &js_last_index, JSPROP_DESC_HAS_VWUEUC);

  // Compile pattern and store 'source'.
  unsigned num_captures;
  const char *error_message = NULL;

  dart::jscre::JSRegExp * regexp = RegExpCompile(js_pattern, ignorecase,
          multiline, &num_captures, &error_message, &RegExpAlloc, &RegExpFree);
  if (regexp == NULL) {
    MAPLE_JS_SYNTAXERROR_EXCEPTION();
  }

  return __object_value(obj);
}

// ES5 15.10.6.2 RegExp.prototype.exec(string)
__jsvalue __jsregexp_Exec(__jsvalue *this_value, __jsvalue *value, uint32_t nargs) {
  // Check if this value is an object.
  if (!__is_js_object(this_value)) {
    MAPLE_JS_TYPEERROR_EXCEPTION();
  } 

  __jsobject *obj = __jsval_to_object(this_value);
  if (obj->object_class != JSREGEXP) {
    MAPLE_JS_TYPEERROR_EXCEPTION();
  }

  __jsstring *js_pattern;

  // Retrieve propereties from this RegExp object.
  bool global = false, ignorecase = false, multiline = false;

  __jsvalue source = __jsop_getprop_by_name(this_value, 
                        __jsstr_get_builtin(JSBUILTIN_STRING_SOURCE));
  js_pattern = __js_ToString(&source);

  __jsvalue js_global = __jsop_getprop_by_name(this_value,
                        __jsstr_get_builtin(JSBUILTIN_STRING_GLOBAL));
  global = __js_ToBoolean(&js_global);

  __jsvalue js_ignorecase = __jsop_getprop_by_name(this_value,
                  __jsstr_get_builtin(JSBUILTIN_STRING_IGNORECASE_UL));
  ignorecase = __js_ToBoolean(&js_ignorecase);

  __jsvalue js_multiline = __jsop_getprop_by_name(this_value,
                      __jsstr_get_builtin(JSBUILTIN_STRING_MULTILINE));
  multiline = __js_ToBoolean(&js_multiline);

  int last_index;
  __jsvalue js_last_index;
  js_last_index = __jsop_getprop_by_name(this_value,
                   __jsstr_get_builtin(JSBUILTIN_STRING_LASTINDEX_UL));
  last_index = __js_ToNumber(&js_last_index);

  if (last_index < 0)
    last_index = 0;

  unsigned num_captures;
  const char *error_message = NULL;

  // Compile RegExp pattern.
  dart::jscre::JSRegExp * regexp = RegExpCompile(js_pattern, ignorecase,
          multiline, &num_captures, &error_message, &RegExpAlloc, &RegExpFree);

  if (regexp == NULL) {
    if (strstr(error_message, "at end of pattern")) {
      return __null_value();
    } else {
      MAPLE_JS_SYNTAXERROR_EXCEPTION();
    }
  }

  // Prepare subject.
  __jsstring *js_subject;
  if (__is_js_object(value)) {
    __jsvalue jsval = __js_ToPrimitive(value, JSTYPE_STRING);
    js_subject = __js_ToString(&jsval);
  } else {
    js_subject = __js_ToString(value);
  }
  // TODO: Handle if js_subject is character class escape.
  // jscre cannot handle it correctly.
  // ES5 15.10.2.12

  if (global && last_index >= __jsstr_get_length(js_subject)) {
    // Set last index to this object.
    js_last_index = __number_value(0);
    __jsobj_helper_add_value_property(obj, JSBUILTIN_STRING_LASTINDEX_UL, 
                                      &js_last_index, JSPROP_DESC_HAS_VWUEUC);
    return __null_value();
  }

  int start_offset = global ? last_index : 0;
  int *offsets = (int *) VMMallocGC((num_captures+1) * 3);
  int offset_count = (num_captures+1) * 3;

  // Do RegExp execution.
  int res = RegExpExecute(regexp, js_subject, start_offset, offsets,
                          offset_count);

  if (res == dart::jscre::JSRegExpErrorNoMatch ||
      res == dart::jscre::JSRegExpErrorHitLimit) {
    return __null_value();
  }

  // Copy outputs from RegExp execution.
  std::vector<std::pair<int,int>> vres;
  for (int i = 0; i < res; i++) {
    vres.push_back(std::make_pair(offsets[2 * i], offsets[2 * i + 1]));
  }

  // Prepare for return object.
  int length = num_captures + 1;  // ES5 15.10.6.2 step. 17
  __jsvalue items[length];
  
  for (int i = 0; i < length; i++) {
    int start = vres[i].first, end = vres[i].second;
    if (i >= res) {
      items[i] = __undefined_value();
    } else if (start >= 0 && end - start >= 0) {
      __jsvalue subject_val = __string_value(js_subject);
      __jsvalue start_val = __number_value(start);
      __jsvalue end_val = __number_value(end);
      __jsvalue str_val = __jsstr_substring(&subject_val, &start_val, &end_val);
      items[i] = str_val;
    } else {
      items[i] = __undefined_value();
    }
  }
  __jsobject *array_obj = __js_new_arr_elems(items, length);

  // Set 'index' and 'input' property to return object.
  __jsvalue index = __number_value(vres[0].first);
  __jsobj_helper_add_value_property(array_obj, JSBUILTIN_STRING_INDEX, &index,
                                    JSPROP_DESC_HAS_VWEC);

  __jsvalue input = __string_value(js_subject);
  __jsobj_helper_add_value_property(array_obj, JSBUILTIN_STRING_INPUT, &input,
                                    JSPROP_DESC_HAS_VWEC);

  // Set 'last_index' to this object.
  js_last_index = __number_value(vres[0].second);
  __jsobj_helper_add_value_property(obj, JSBUILTIN_STRING_LASTINDEX_UL, 
                                    &js_last_index, JSPROP_DESC_HAS_VWUEUC);
  return __object_value(array_obj);
}

// ES5 15.10.6.3 RegExp.prototype.test(string)
__jsvalue __jsregexp_Test(__jsvalue *this_value, __jsvalue *value, 
                          uint32_t nargs) {
  if (!__is_js_object(this_value)) {
    MAPLE_JS_TYPEERROR_EXCEPTION();
  }
  __jsobject *obj = __jsval_to_object(this_value);
  if (obj->object_class != JSREGEXP) {
    MAPLE_JS_TYPEERROR_EXCEPTION();
  }

  __jsvalue match = __jsregexp_Exec(this_value, value, nargs);
  if (__is_null(&match)) {
    return __boolean_value(false);
  }
  return __boolean_value(true);
}

// ES5 15.10.6.4 RegExp.prototype.toString()
__jsvalue __jsregexp_ToString(__jsvalue *this_value) {

  bool global, ignorecase, multiline;

  // Add surrounding '/' to 'source'.
  const char slash[] = "/";
  __jsstring *js_slash = __jsstr_new_from_char(slash);

  __jsvalue source = __jsop_getprop_by_name(this_value, 
                        __jsstr_get_builtin(JSBUILTIN_STRING_SOURCE));
  __jsstring *js_source = __js_ToString(&source);

  __jsstring *js_properties = __jsstr_concat_2(js_slash, js_source);
  js_properties = __jsstr_concat_2(js_properties, js_slash);

  // Append flags.
  __jsvalue js_global = __jsop_getprop_by_name(this_value,
                        __jsstr_get_builtin(JSBUILTIN_STRING_GLOBAL));
  global = __js_ToBoolean(&js_global);
  if (global) {
    js_properties = __jsstr_append_char(js_properties, 'g');
  }
  __jsvalue js_ignorecase = __jsop_getprop_by_name(this_value,
                  __jsstr_get_builtin(JSBUILTIN_STRING_IGNORECASE_UL));
  ignorecase = __js_ToBoolean(&js_ignorecase);
  if (ignorecase) {
    js_properties = __jsstr_append_char(js_properties, 'i');
  }
  __jsvalue js_multiline = __jsop_getprop_by_name(this_value,
                      __jsstr_get_builtin(JSBUILTIN_STRING_MULTILINE));
  multiline = __js_ToBoolean(&js_multiline);
  if (multiline) {
   js_properties = __jsstr_append_char(js_properties, 'm');
  }

  return __string_value(js_properties);
}

void CheckAndSetFlagOptions(__jsstring *s, __jsstring *js_pattern,
                 bool& global, bool& ignorecase, bool& multiline) {

  if (__jsstr_get_length(s) == 0) {
    return;
  }
  const char undefined[] = "undefined";
  __jsstring *js_undefined = __jsstr_new_from_char(undefined);
  if (__jsstr_equal(s, js_undefined)) {
    if (__jsstr_get_length(js_pattern) == 0) {
      js_pattern = __jsstr_new_from_char(DEFAULT_REGEXP_PATTERN);
    }
    global = false;
    ignorecase = false;
    multiline = false;

    return;
  }
  // Check 'g', 'i', 'm' in flags (ES5 15.10.4.1)
  int count_g = 0, count_i = 0, count_m = 0, len = __jsstr_get_length(s);

  for (int i = 0; i < len; i++) {
    if (__jsstr_get_char(s, i) == 'g')
      count_g++;
    else if (__jsstr_get_char(s, i) == 'i')
      count_i++;
    else if (__jsstr_get_char(s, i) == 'm')
      count_m++;
    else {
      MAPLE_JS_SYNTAXERROR_EXCEPTION();
    }
  }
  if (count_g > 1 || count_i > 1 || count_m > 1) {
    MAPLE_JS_SYNTAXERROR_EXCEPTION();
  } else {
    global = count_g == 1 ? true : false;
    ignorecase = count_i == 1 ? true : false;
    multiline = count_m == 1 ? true : false;
  }
}

// Compile RegExp pattern.
dart::jscre::JSRegExp *RegExpCompile(__jsstring *js_pattern,
                                     bool ignorecase,
                                     bool multiline, 
                                     unsigned * num_captures, 
                                     const char **error_message, 
                                     dart::jscre::malloc_t *alloc_func, 
                                     dart::jscre::free_t *free_func) {

  int len = __jsstr_get_length(js_pattern);
  uint16_t pattern[len];
  if (__jsstr_is_ascii(js_pattern)) {
    StrToUint16(js_pattern->x.ascii, pattern, len);
  } else {
    memcpy(pattern, js_pattern->x.utf16, len * sizeof(uint16_t));
  }

  dart::jscre::JSRegExpIgnoreCaseOption case_option = ignorecase ?
      dart::jscre::JSRegExpIgnoreCase : dart::jscre::JSRegExpDoNotIgnoreCase;

  dart::jscre::JSRegExpMultilineOption multiline_option = multiline ?
      dart::jscre::JSRegExpMultiline : dart::jscre::JSRegExpSingleLine;

  dart::jscre::JSRegExp* regexp = 
      dart::jscre::jsRegExpCompile(pattern, len,
                                   case_option, multiline_option, num_captures,
                                   error_message, alloc_func, free_func);
  return regexp;
}

// Do RegExp execution.
int RegExpExecute(const dart::jscre::JSRegExp *re, __jsstring *js_subject,
                  int start_offset, int *offsets, int offset_count) {
  int len = __jsstr_get_length(js_subject);
  uint16_t subject[len];
  if (__jsstr_is_ascii(js_subject)) {
    StrToUint16(js_subject->x.ascii, subject, len);
  } else {
    memcpy(subject, js_subject->x.utf16, len*sizeof(uint16_t));
  }
  int res = dart::jscre::jsRegExpExecute(re, subject, len,
                                         start_offset, offsets, offset_count);

  return res;
}

// Getter for 'source' accessor property.
__jsvalue __jsregexp_Source(__jsvalue *this_arg) {
  return __jsop_getprop_by_name(this_arg, __jsstr_get_builtin(JSBUILTIN_STRING_SOURCE));
}

// Getter for 'global' accessor property.
__jsvalue __jsregexp_Global(__jsvalue *this_arg) {
  return __jsop_getprop_by_name(this_arg, __jsstr_get_builtin(JSBUILTIN_STRING_GLOBAL));
}

// Getter for 'ignoreCase' accessor property.
__jsvalue __jsregexp_Ignorecase(__jsvalue *this_arg) {
  return __jsop_getprop_by_name(this_arg, __jsstr_get_builtin(JSBUILTIN_STRING_IGNORECASE_UL));
}

// Getter for 'multiline' accessor property.
__jsvalue __jsregexp_Multiline(__jsvalue *this_arg) {
  return __jsop_getprop_by_name(this_arg, __jsstr_get_builtin(JSBUILTIN_STRING_MULTILINE));
}

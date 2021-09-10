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

#include <unicode/ucol.h>
#include "jsobject.h"
#include "jsobjectinline.h"
#include "jsarray.h"
#include "jsmath.h"
#include "jsintl.h"

// ECMA-402 1.0 10.1 The Intl.Collator Constructor
__jsvalue __js_CollatorConstructor(__jsvalue *this_arg, __jsvalue *arg_list,
                                   uint32_t nargs) {
  __jsobject *obj = __create_object();
  __jsobj_set_prototype(obj, JSBUILTIN_INTL_COLLATOR_PROTOTYPE);
  obj->object_class = JSINTL_COLLATOR;
  obj->extensible = true;
  obj->object_type = JSREGULAR_OBJECT;
  __jsvalue obj_val = __object_value(obj);

  __jsvalue locales = __undefined_value();
  __jsvalue options = __undefined_value();
  if (nargs == 0) {
    // Do nothing.
  } else if (nargs == 1) {
    locales = arg_list[0];
  } else if (nargs == 2) {
    locales = arg_list[0];
    options = arg_list[1];
  } else {
    MAPLE_JS_SYNTAXERROR_EXCEPTION();
  }
  InitializeCollator(&obj_val, &locales, &options);

  return obj_val;
}

// ECMA-402 1.0 10.1.1.1
// InitializeCollator(collator, locales, options)
void InitializeCollator(__jsvalue *this_collator, __jsvalue *locales,
                        __jsvalue *options) {
  // Step 1.
  __jsvalue p = StrToVal("initializedIntlObject");
  __jsvalue v = __jsop_getprop(this_collator, &p);
  if (!__is_undefined(&v)) {
    if (__is_boolean(&v) && __jsval_to_boolean(&v) == true) {
      MAPLE_JS_TYPEERROR_EXCEPTION();
    }
  }
  // Step 2.
  v = __boolean_value(true);
  __jsop_setprop(this_collator, &p, &v);
  // Step 3.
  __jsvalue requested_locales = CanonicalizeLocaleList(locales);
  // Step 4.
  if (__is_undefined(options)) {
    // Step 4a.
    __jsobject *o = __js_new_obj_obj_0();
    *options = __object_value(o);
  } else {
    // Step 5.
    *options = __object_value(__jsval_to_object(options));
  }
  // Step 6.
  __jsvalue property = StrToVal("usage");
  __jsvalue type = StrToVal("string");
  __jsvalue values = StrVecToVal({"sort", "search"});
  __jsvalue fallback = StrToVal("sort");
  __jsvalue u = GetOption(options, &property, &type, &values, &fallback);
  // Step 7.
  p = StrToVal("usage");
  __jsop_setprop(this_collator, &p, &u);
  // Step 8.
  __jsobject *collator_obj = __create_object();
  collator_obj->object_class = JSINTL_COLLATOR;
  collator_obj->extensible = true;
  collator_obj->object_type = JSREGULAR_OBJECT;
  __jsvalue collator = __object_value(collator_obj);

  InitializeCollatorProperties(&collator, &requested_locales, {"availableLocales", "relevantExtensionKeys", "sortLocaleData", "searchLocaleData"});

  // Step 9.
  // If 'u' is "sort", then let 'localeData' be the value of the [[sortLocaleData]]
  // internal property of 'Collator'; else let 'localeData' be the value of the
  // [searchLocaleData]] internal property of 'Collator'.
  __jsstring *u_str = __jsval_to_string(&u);
  std::string sort_str = "sort";
  __jsstring *sort = __jsstr_new_from_char(sort_str.c_str());
  __jsvalue locale_data = __undefined_value();
  if (__jsstr_equal(u_str, sort)) {
    p = StrToVal("sortLocaleData");
    locale_data = __jsop_getprop(&collator, &p);
  } else {
    p = StrToVal("searchLocaleData");
    locale_data = __jsop_getprop(&collator, &p);
  }
  // Step 10.
  __jsobject *opt_obj = __js_new_obj_obj_0();
  __jsvalue opt = __object_value(opt_obj);
  // Step 11.
  property = StrToVal("localeMatcher");
  type = StrToVal("string");
  values = StrVecToVal({"lookup", "best fit"});
  fallback = StrToVal("best fit");
  __jsvalue matcher = GetOption(options, &property, &type, &values, &fallback);
  // Step 12.
  p = StrToVal("localeMatcher");
  __jsop_setprop(&opt, &p, &matcher);
  // Step 13.
  // Unrolled operations based on Table 1.
  __jsvalue key = StrToVal("kn");
  property = StrToVal("numeric");
  type = StrToVal("boolean");
  values = __undefined_value();
  fallback = __undefined_value();
  __jsvalue value = GetOption(options, &property, &type, &values, &fallback);
  if (!__is_undefined(&value)) {
    value = __string_value(__jsval_to_string(&value));
  }
  __jsop_setprop(&opt, &key, &value);

  key = StrToVal("kf");
  property = StrToVal("caseFirst");
  type = StrToVal("string");
  values = StrVecToVal({"upper", "lower", "false"});
  fallback = __undefined_value();
  value = GetOption(options, &property, &type, &values, &fallback);
  __jsop_setprop(&opt, &key, &value);
  // Step 14.
  p = StrToVal("relevantExtensionKeys");
  __jsvalue relevant_extension_keys = __jsop_getprop(&collator, &p);
  // Step 15.
  p = StrToVal("availableLocales");
  __jsvalue available_locales = __jsop_getprop(&collator, &p);
  __jsvalue r = ResolveLocale(&available_locales, &requested_locales, &opt, &relevant_extension_keys, &locale_data);
  // Step 16.
  p = StrToVal("locale");
  v = __jsop_getprop(&r, &p);
  __jsop_setprop(this_collator, &p, &v);
  // Step 17.
  int i = 0;
  // Step 18.
  // Let 'len' be the result of calling the [[Get]] internal method of 'relevant_extension_keys'
  // with argument "length".
  __jsobject *relevant_extension_keys_object = __jsval_to_object(&relevant_extension_keys);
  int len = __jsobj_helper_get_length(relevant_extension_keys_object);
  // Step 19.
  while (i < len) {
    // Step 19a.
    __jsvalue key = __jsarr_GetElem(__jsval_to_object(&relevant_extension_keys), i);
    // Step 19b.
    __jsstring *key_str = __jsval_to_string(&key);
    std::string co = "co";
    __jsstring *co_str = __jsstr_new_from_char(co.c_str());
    if (__jsstr_equal(key_str, co_str)) {
      // Step 19b i.
      property = StrToVal("collation");
      // Step 19b ii.
      p = StrToVal("co");
      value = __jsop_getprop(&r, &p);
      // Step 19b iii.
      if (__is_null(&value)) {
        value = StrToVal("default");
      }
    } else {
      // Step 19c.
      // Unrolled operations based on Table 1.
      // Step 19c i.
      property = StrToVal("numeric");
      // Step 19c ii.
      value = __jsop_getprop(&r, &key);
      __jsstring *value_str = __jsval_to_string(&value);
      // Step 19c iii.
      std::string true_str = "true";
      __jsstring *true_jsstr = __jsstr_new_from_char(true_str.c_str());
      if (__jsstr_equal(value_str, true_jsstr)) {
        value = __boolean_value(true);
      } else {
        value = __boolean_value(false);
      }
    }
    // Step 19d.
    __jsop_setprop(this_collator, &property, &value);
    // Step 19e.
    i++;
  }
  // Step 20.
  property = StrToVal("sensitivity");
  type = StrToVal("string");
  values = StrVecToVal({"base", "accent", "case", "variant"});
  fallback = __undefined_value();
  __jsvalue s = GetOption(options, &property, &type, &values, &fallback);
  // Step 21.
  if (__is_undefined(&s)) {
    // Step 21a.
    if (__jsstr_equal(u_str, sort)) {
      s = StrToVal("variant");
    } else { // Step 21b.
      // Step 21b i.
      p = StrToVal("dataLocale");
      __jsvalue data_locale = __jsop_getprop(&r, &p);
      // Step 21b ii.
      __jsvalue data_locale_data = __jsop_getprop(&locale_data, &data_locale);
      // Step 21b iii.
      p = StrToVal("sensitivity");
      s = __jsop_getprop(&data_locale_data, &p);
    }
  }
  // Step 22.
  p = StrToVal("sensitivity");
  __jsop_setprop(this_collator, &p, &s);
  // Step 23.
  property = StrToVal("ignorePunctuation");
  type = StrToVal("boolean");
  values = __undefined_value();
  fallback = __undefined_value();
  __jsvalue ip = GetOption(options, &property, &type, &values, &fallback);
  // Step 24.
  p = StrToVal("ignorePunctuation");
  __jsop_setprop(this_collator, &p, &ip);
  // Step 25.
  p = StrToVal("boundCompare");
  v = __undefined_value();
  __jsop_setprop(this_collator, &p, &v);
  // Step 26.
  p = StrToVal("initializedCollator");
  v = __boolean_value(true);
  __jsop_setprop(this_collator, &p ,&v);
}

// ECMA-402 1.0 10.2.2
// Intl.Collator.supportedLocalesOf(locales [, options])
__jsvalue __jsintl_CollatorSupportedLocalesOf(__jsvalue *collator,
                                              __jsvalue *arg_list,
                                              uint32_t nargs) {
  // Step 1.
  // If' options' is not provided, then let 'options' be undefined.
  __jsvalue locales = arg_list[0];
  __jsvalue options;
  if (nargs == 1) {
    options = __undefined_value();
  } else {
    options = arg_list[1];
  }
  // Step 2.
  // Let 'availableLocales' be the value of the [[availableLocales]]
  // internal property of the standard built-in object that is the initial value
  // of Intl.Collator.
  __jsvalue available_locales = GetAvailableLocales();
  // Step 3.
  // Let 'requestedLocales' be the result of calling the CanonicalizeLocaleList
  // abstract operation with argument 'locales'.
  __jsvalue requested_locales = CanonicalizeLocaleList(&locales);
  // Step 4.
  return SupportedLocales(&available_locales, &requested_locales, &options);
}

__jsvalue CompareStrings(__jsvalue *collator, __jsvalue *x, __jsvalue *y) {
  __jsvalue p = StrToVal("sensitivity");
  __jsvalue v = __jsop_getprop(collator, &p);
  std::string sensitivity = ValToStr(&v);

  UColAttributeValue strength = UCOL_DEFAULT;
  UColAttributeValue case_level = UCOL_OFF;
  UColAttributeValue alternate = UCOL_DEFAULT;
  UColAttributeValue numeric = UCOL_OFF;
  UColAttributeValue normalization = UCOL_ON;
  UColAttributeValue case_first = UCOL_DEFAULT;

  if (sensitivity == "base") {
    strength = UCOL_PRIMARY;
  } else if (sensitivity == "accent") {
    strength = UCOL_SECONDARY;
  } else if (sensitivity == "case") {
    strength = UCOL_PRIMARY;
    case_level = UCOL_ON;
  } else if (sensitivity == "variant") {
    strength = UCOL_TERTIARY;
  } else {
    MAPLE_JS_ASSERT(false && "wrong sensitivity");
  }

  p = StrToVal("locale");
  v = __jsop_getprop(collator, &p);
  std::string locale_str = ValToStr(&v);

  UErrorCode status = U_ZERO_ERROR;
  UCollator *col = ucol_open(locale_str.c_str(), &status);
  if (U_FAILURE(status)) {
    MAPLE_JS_ASSERT(false && "Error in ucol_open()");
  }
  ucol_setAttribute(col, UCOL_STRENGTH, strength, &status);
  ucol_setAttribute(col, UCOL_CASE_LEVEL, case_level, &status);
  ucol_setAttribute(col, UCOL_ALTERNATE_HANDLING, alternate, &status);
  ucol_setAttribute(col, UCOL_NUMERIC_COLLATION, numeric, &status);
  ucol_setAttribute(col, UCOL_NORMALIZATION_MODE, normalization, &status);
  ucol_setAttribute(col, UCOL_CASE_FIRST, case_first, &status);

  if (U_FAILURE(status)) {
    ucol_close(col);
  }

  __jsstring *x_str = __jsval_to_string(x);
  __jsstring *y_str = __jsval_to_string(y);
  uint32_t x_len = __jsstr_get_length(x_str);
  uint32_t y_len = __jsstr_get_length(y_str);

  UChar *x_uchar = (UChar*)VMMallocGC(sizeof(UChar)*(x_len+1));
  UChar *y_uchar = (UChar*)VMMallocGC(sizeof(UChar)*(y_len+1));
  x_uchar[x_len] = '\0';
  y_uchar[y_len] = '\0';
  if (__jsstr_is_ascii(x_str)) {
    for (int i = 0; i < x_len; i++) {
      x_uchar[i] = x_str->x.ascii[i];
    }
  } else {
    for (int i = 0; i < x_len; i++) {
      x_uchar[i] = x_str->x.utf16[i];
    }
  }
  if (__jsstr_is_ascii(y_str)) {
    for (int i = 0; i < y_len; i++) {
      y_uchar[i] = y_str->x.ascii[i];
    }
  } else {
    for (int i = 0; i < y_len; i++) {
      y_uchar[i] = y_str->x.utf16[i];
    }
  }
  UCollationResult col_res = ucol_strcoll(col, x_uchar, x_len, y_uchar, y_len);
  int32_t res;
  switch (col_res) {
    case UCOL_LESS:
      res = -1;
      break;
    case UCOL_EQUAL:
      res = 0;
      break;
    case UCOL_GREATER:
      res = 1;
      break;
    default:
      MAPLE_JS_ASSERT(false && "Error in ucol_strcoll()");
  }
  return __number_value(res);
}

// ECMA-402 1.0 10.3.2
// Intl.Collator.prototype.compare
__jsvalue __jsintl_CollatorCompare(__jsvalue *collator, __jsvalue *x, __jsvalue *y) {
  // Check if 'collator' is a non-object value or is not initlized as a Collator.
  if (!__is_js_object(collator)) {
    MAPLE_JS_TYPEERROR_EXCEPTION();
  }
  __jsobject *collator_obj = __jsval_to_object(collator);
  if (collator_obj->object_class != JSINTL_COLLATOR) {
    MAPLE_JS_TYPEERROR_EXCEPTION();
  }

  // Step 1.
  __jsvalue p = StrToVal("boundCompare");
  __jsvalue bound_compare = __jsop_getprop(collator, &p);

  if (__is_undefined(&bound_compare)) {
#define ATTRS(nargs, length) \
  ((uint32_t)(uint8_t)(nargs == UNCERTAIN_NARGS ? 1: 0) << 24 | (uint32_t)(uint8_t)nargs << 16 | \
       (uint32_t)(uint8_t)length << 8 | JSFUNCPROP_NATIVE)
    __jsvalue f = __js_new_function((void*)CompareStrings, NULL, ATTRS(3, 2));

    *x = __string_value(__jsval_to_string(x));
    *y = __string_value(__jsval_to_string(y));

    __jsvalue args[] = { *collator, *x, *y };
    int arg_count = 1;

    __jsvalue this_binding_old = __js_ThisBinding;
    __js_ThisBinding = f;
    __jsvalue bc = __jsfun_pt_bind(&f, args, arg_count);
    __js_ThisBinding = this_binding_old;

    __jsop_setprop(collator, &p, &bc);
  }
  // Step 2.
  __jsvalue res = __jsop_getprop(collator, &p);
  return res;
}

// ECMA-402 1.0 10.3.3
// Intl.Collator.prototype.resolvedOptions()
__jsvalue __jsintl_CollatorResolvedOptions(__jsvalue *collator) {
  // Check if 'collator' is object.
  if (!__is_js_object(collator)) {
    MAPLE_JS_TYPEERROR_EXCEPTION();
  }
  // Check if 'collator' is an Intl.Collator object.
  __jsobject *collator_obj = __jsval_to_object(collator);
  if (collator_obj->object_class != JSINTL_COLLATOR) {
    MAPLE_JS_TYPEERROR_EXCEPTION();
  }

  __jsobject *col_obj = __create_object();
  __jsobj_set_prototype(col_obj, JSBUILTIN_INTL_COLLATOR_PROTOTYPE);
  col_obj->object_class = JSINTL_COLLATOR;
  col_obj->extensible = true;
  col_obj->object_type = JSREGULAR_OBJECT;
  __jsvalue col = __object_value(col_obj);

  std::vector<std::string> props = {"locale", "usage", "sensitivity",
                                    "ignorePunctuation", "collation"};
  __jsvalue p, v;
  for (int i = 0; i < props.size(); i++) {
    p = StrToVal(props[i]);
    v = __jsop_getprop(collator, &p);
    __jsop_setprop(&col, &p, &v);
  }
  // NOTE: is this really needed?
  p = StrToVal("initializedCollator");
  v = __boolean_value(true);
  __jsop_setprop(&col, &p, &v);

  return col;

}

void InitializeCollatorProperties(__jsvalue *collator, __jsvalue *locales, std::vector<std::string> properties) {
  __jsvalue p, v;
  for (int i = 0; i < properties.size(); i++) {
    if (properties[i] == "availableLocales") {
      p = StrToVal(properties[i]);
      v = GetAvailableLocales();

      __jsop_setprop(collator, &p, &v);
    } else if (properties[i] == "relevantExtensionKeys") {
      p = StrToVal(properties[i]);
      std::vector<std::string> values = {"co", "kn"};
      v = StrVecToVal(values);

      __jsop_setprop(collator, &p, &v);
    } else if (properties[i] == "sortLocaleData") {
      __jsobject *locale_object = __create_object();
      __jsvalue locale = __object_value(locale_object);
      locale_object->object_class = JSOBJECT;
      locale_object->extensible = true;
      locale_object->object_type = JSREGULAR_OBJECT;

      p = StrToVal("co");
      v = __null_value();
      __jsop_setprop(&locale, &p, &v); // Set 'co' to locale.

      p = StrToVal("kn");
      std::vector<std::string> vec = {"true", "false"};
      v = StrVecToVal(vec);
      __jsop_setprop(&locale, &p, &v); // Set 'kn' to locale.

      __jsobject *sort_locale_data_object = __create_object();
      __jsvalue sort_locale_data = __object_value(sort_locale_data_object);
      sort_locale_data_object->object_class = JSOBJECT;
      sort_locale_data_object->extensible = true;
      sort_locale_data_object->object_type = JSREGULAR_OBJECT;

      __jsobject *locales_object = __jsval_to_object(locales);
      uint32_t size = __jsobj_helper_get_length(locales_object);
      for (int j = 0; j < size; j++) {
        p = __jsarr_GetElem(locales_object, j);
        __jsvalue available_locales = GetAvailableLocales();
        p = BestAvailableLocale(&available_locales, &p);
        __jsop_setprop(&sort_locale_data, &p, &locale);
      }
      if (size == 0) {
        p = DefaultLocale();
        __jsop_setprop(&sort_locale_data, &p, &locale);
      }
      p = StrToVal(properties[i]);
      __jsop_setprop(collator, &p, &sort_locale_data);
    } else if (properties[i] == "searchLocaleData") {
      __jsobject *locale_object = __create_object();
      __jsvalue locale = __object_value(locale_object);
      locale_object->object_class = JSOBJECT;
      locale_object->extensible = true;
      locale_object->object_type = JSREGULAR_OBJECT;

      p = StrToVal("co");
      v = __null_value();
      __jsop_setprop(&locale, &p, &v); // Set 'co' to locale.

      p = StrToVal("kn");
      std::vector<std::string> vec = {"true", "false"};
      v = StrVecToVal(vec);
      __jsop_setprop(&locale, &p, &v); // Set 'kn' to locale.

      p = StrToVal("sensitivity");
      v = StrToVal("variant");
      __jsop_setprop(&locale, &p, &v); // Set 'sensitivity' to locale.

      __jsobject *search_locale_data_object = __create_object();
      __jsvalue search_locale_data = __object_value(search_locale_data_object);
      search_locale_data_object->object_class = JSOBJECT;
      search_locale_data_object->extensible = true;
      search_locale_data_object->object_type = JSREGULAR_OBJECT;

      __jsobject *locales_object = __jsval_to_object(locales);
      uint32_t size = __jsobj_helper_get_length(locales_object);
      for (int j = 0; j < size; j++) {
        p = __jsarr_GetElem(locales_object, j);
        __jsvalue available_locales = GetAvailableLocales();
        p = BestAvailableLocale(&available_locales, &p);
        __jsop_setprop(&search_locale_data, &p, &locale);
      }
      if (size == 0) {
        p = DefaultLocale();
        __jsop_setprop(&search_locale_data, &p, &locale);
      }
      p = StrToVal(properties[i]);
      __jsop_setprop(collator, &p, &search_locale_data);
    }
  }
}

std::vector<std::string> GetCollations() {
  char *locale = getenv("LANG");
  UErrorCode status = U_ZERO_ERROR;
  std::vector<std::string> vec;

  UEnumeration *values = ucol_getKeywordValuesForLocale("co", locale, false, &status);
  uint32_t count = uenum_count(values, &status);
  for (int i = 0; i < count; i++) {
    const char *col_value = uenum_next(values, nullptr, &status);
    assert(U_FAILURE(status) == false);

    if (strcmp(col_value, "standard") == 0 || strcmp(col_value, "search") == 0) continue;
    std::string s(col_value);
    vec.push_back(s);
  }
  //UCollator *col = ucol_open(locale, &status);
  //const char *default_collator = ucol_getType(col, &status);
  //assert(U_FAILURE(status) == false);
  //std::string s(default_collator);
  //ucol_close(col);
  //vec.insert(vec.begin(), s);

  return vec;
}

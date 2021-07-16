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

#include <algorithm>
#include "jsobject.h"
#include "jsobjectinline.h"
#include "jsarray.h"
#include "jsmath.h"

extern std::unordered_map<std::string,std::vector<std::string>> kDateTimeFormatComponents;

// ECMA-402 1.0 12.1 The Intl.DateTimeFormat Constructor
__jsvalue __js_DateTimeFormatConstructor(__jsvalue *this_arg,
                                        __jsvalue *arg_list, uint32_t nargs) {
  __jsobject *obj = __create_object();
  __jsobj_set_prototype(obj, JSBUILTIN_INTL_DATETIMEFORMAT_PROTOTYPE);
  obj->object_class = JSINTL;
  obj->extensible = true;
  obj->object_type = JSREGULAR_OBJECT;
  obj->shared.intl = (__jsintl*) VMMallocGC(sizeof(__jsintl));
  obj->shared.intl->kind = JSINTL_DATETIMEFORMAT;
  __jsvalue obj_val = __object_value(obj);

  __jsvalue locales = __undefined_value();
  __jsvalue options = __undefined_value();
  if (nargs == 0) {
    // Do nothing.
  } else if (nargs == 1) {
    locales = arg_list[0];
  } else if (nargs == 2) {
    locales = arg_list[1];
    options = arg_list[2];
  } else {
    MAPLE_JS_SYNTAXERROR_EXCEPTION();
  }
  InitializeDateTimeFormat(&obj_val, &locales, &options);

  return obj_val;
}

// ECMA-402 1.0 12.1.1.1
// InitializeDateTimeFormat(dateTimeFormat, locales, options)
void InitializeDateTimeFormat(__jsvalue *this_date_time_format, __jsvalue *locales,
                              __jsvalue *options) {
  // Step 1.
  __jsvalue prop = StrToVal("initializedIntlObject");
  if (!__is_undefined(this_date_time_format)) {
    __jsvalue init = __jsop_getprop(this_date_time_format, &prop);
    if (__is_boolean(&init) && __jsval_to_boolean(&init) == true) {
      MAPLE_JS_TYPEERROR_EXCEPTION();
    }
  }
  // Step 2.
  __jsvalue value = __boolean_value(true);
  __jsop_setprop(this_date_time_format, &prop, &value);
  // Step 3.
  __jsvalue requested_locales = CanonicalizeLocaleList(locales);
  // Step 4.
  __jsvalue required = StrToVal("any");
  __jsvalue default_val = StrToVal("date");
  *options = ToDateTimeOptions(options, &required, &default_val);
  // Step 5.
  __jsobject *opt_obj = __js_new_obj_obj_0();
  __jsvalue opt = __object_value(opt_obj);
  // Step 6.
  __jsvalue property = StrToVal("localMatcher");
  __jsvalue type = StrToVal("string");
  __jsvalue values = StrVecToVal({"lookup", "best fit"});
  __jsvalue fallback = StrToVal("best fit");
  __jsvalue matcher = GetOption(options, &property, &type, &values, &fallback);
  // Step 7.
  prop = StrToVal("localeMatcher");
  __jsop_setprop(&opt, &prop, &matcher);
  // Step 8.
  __jsobject *data_time_format_obj = __create_object();
  data_time_format_obj->object_class = JSINTL;
  data_time_format_obj->extensible = true;
  data_time_format_obj->object_type = JSREGULAR_OBJECT;
  data_time_format_obj->shared.intl = (__jsintl*) VMMallocGC(sizeof(__jsintl));
  data_time_format_obj->shared.intl->kind = JSINTL_DATETIMEFORMAT;
  __jsvalue date_time_format = __object_value(data_time_format_obj);

  InitProperty(&date_time_format, "availableLocales");
  InitProperty(&date_time_format, "relevantExtensionKeys");
  InitProperty(&date_time_format, "localeData");

  // Step 9.
  prop = StrToVal("localeData");
  __jsvalue locale_data = __jsop_getprop(&date_time_format, &prop);
  // Step 10.
  prop = StrToVal("availableLocales");
  __jsvalue available_locales = __jsop_getprop(&date_time_format, &prop);
  prop = StrToVal("relevantExtensionKeys");
  __jsvalue relevant_extension_keys = __jsop_getprop(&date_time_format, &prop);
  __jsvalue r = ResolveLocale(&available_locales, &requested_locales,
                              &opt, &relevant_extension_keys, &locale_data);
  // Step 11.
  prop = StrToVal("dateTimeFormat");
  value = __jsop_getprop(&r, &prop);
  __jsop_setprop(this_date_time_format, &prop, &value);
  // Step 12.
  prop = StrToVal("ca");
  value = __jsop_getprop(&r, &prop);
  prop = StrToVal("calendar");
  __jsop_setprop(this_date_time_format, &prop, &value);
  // Step 13.
  prop = StrToVal("nu");
  value = __jsop_getprop(&r, &prop);
  prop = StrToVal("numberingSystem");
  __jsop_setprop(this_date_time_format, &prop, &value);
  // Step 14.
  prop = StrToVal("dataLocale");
  __jsvalue data_locale = __jsop_getprop(&r, &prop);
  // Step 15.
  prop = StrToVal("timeZone");
  __jsvalue tz = __jsop_getprop(options, &prop);
  // Step 16.
  if (!__is_undefined(&tz)) {
    // Step 16a.
    std::string tz_str = ValToStr(&tz);
    // Step 16b.
    std::transform(tz_str.begin(), tz_str.end(), tz_str.begin(), ::toupper);
    if (tz_str != "UTC") {
      MAPLE_JS_RANGEERROR_EXCEPTION();
    }
  }
  // Step 17.
  prop = StrToVal("timeZone");
  __jsop_setprop(this_date_time_format, &prop, &tz);
  // Step 18.
  opt_obj = __js_new_obj_obj_0();
  opt = __object_value(opt_obj);
  // Step 19.
  auto it = kDateTimeFormatComponents.begin();
  while (it != kDateTimeFormatComponents.end()) {
    // Step 19a.
    std::string p = it->first;
    prop = StrToVal(p);
    // Step 19b.
    std::vector<std::string> v = it->second;
    __jsvalue type = StrToVal("string");
    __jsvalue values = StrVecToVal(v);
    __jsvalue fallback = __undefined_value();
    value = GetOption(options, &prop, &type, &values, &fallback);
    // Step 19c.
    __jsop_setprop(&opt, &prop, &value);
  }
  // Step 20.
  prop = StrToVal("dataLocale");
  __jsvalue data_locale_data = __jsop_getprop(&locale_data, &prop);
  // Step 21.
  prop = StrToVal("formats");
  __jsvalue formats = __jsop_getprop(&data_locale_data, &prop);
  // Step 22.
  property = StrToVal("formatMatcher");
  type = StrToVal("string");
  values = StrVecToVal({"basic", "best fit"});
  fallback = StrToVal("best fit");
  matcher = GetOption(options, &property, &type, &values, &fallback);
  // Step 23.
  std::string matcher_str = ValToStr(&matcher);
  __jsvalue best_format;
  if (matcher_str == "basic") {
    // Step 23a.
    best_format = BasicFormatMatcher(&opt, &formats);
  } else {
    // Step 24.
    best_format = BestFitFormatMatcher(&opt, &formats);
  }
  // Step 25.
  it = kDateTimeFormatComponents.begin();
  while (it != kDateTimeFormatComponents.end()) {
    // Step 25a.
    std::string p = it->first;
    prop = StrToVal(p);
    // Step 25b.
    __jsvalue p_desc = __jsobj_getOwnPropertyNames(&best_format, &prop);
    // Step 25c.
    if (!__is_undefined(&p_desc)) {
      // Step 25c i.
      __jsvalue p = __jsop_getprop(&best_format, &prop);
      // Step 25c ii.
      __jsop_setprop(this_date_time_format, &prop, &p);
    }
  }
  // Step 26.
  property = StrToVal("hour12");
  type = StrToVal("boolean");
  values = __undefined_value();
  fallback = __undefined_value();
  __jsvalue hr12 = GetOption(options, &property, &type, &values, &fallback);
  // Step 27.
  prop = StrToVal("hour");
  value = __jsop_getprop(this_date_time_format, &prop);
  __jsvalue pattern;
  if (!__is_null_or_undefined(&value)) {
    // Step 27a.
    if (__is_undefined(&hr12)) {
      prop = StrToVal("hour12");
      hr12 = __jsop_getprop(&data_locale_data, &prop);
    }
    // Step 27b.
    __jsop_setprop(this_date_time_format, &prop, &hr12);
    // Step 27c.
    if (__jsval_to_boolean(&hr12) == true) {
      // Step 27c i.
      prop = StrToVal("hourNo0");
      __jsvalue hour_no_0 = __jsop_getprop(&data_locale_data, &prop);
      // Step 27c ii.
      __jsop_setprop(this_date_time_format, &prop, &hour_no_0);
      // Step 27c iii.
      prop = StrToVal("pattern12");
      pattern = __jsop_getprop(&best_format, &prop);
    } else {
      prop = StrToVal("pattern");
      pattern = __jsop_getprop(&best_format, &prop);
    }
  } else {
    // Step 28.
    prop = StrToVal("pattern");
    pattern = __jsop_getprop(&best_format, &prop);
  }
  // Step 29.
  __jsop_setprop(this_date_time_format, &prop, &pattern);
  // Step 30.
  prop = StrToVal("boundFormat");
  value = __undefined_value();
  __jsop_setprop(this_date_time_format, &prop, &value);
  // Step 31.
  prop = StrToVal("initializedDateTimeFormat");
  value = __boolean_value(true);
  __jsop_setprop(this_date_time_format, &prop, &value);
}

// ECMA-402 1.0 12.1.1.1
__jsvalue ToDateTimeOptions(__jsvalue *options, __jsvalue *required, __jsvalue *defaults) {
  // Step 1.
  if (__is_undefined(options)) {
    *options = __null_value();
  }
  // Step 2.
  // Let 'create' be the standard built-in function object defined in ES5, 15.2.3.5.
  __jsobject *create_obj = __create_object();
  create_obj->object_class = JSFUNCTION;
  create_obj->extensible = true;
  __jsvalue create = __object_value(create_obj);
  // Step 3.
  // Let 'options' be the result of calling the [[Call]] internal method of 'create'
  // with 'undefined' as the 'this' value and an argument list containing the single
  // item 'options'.
  int len = 1;
  __jsvalue items[len];
  items[0] = *options;
  __jsobject *args_obj = __js_new_arr_elems_direct(items, len);
  __jsvalue args = __object_value(args_obj);
  __jsvalue this_arg = __undefined_value();
  *options = __jsfun_pt_call(&create, &args, 1); // TODO: failed here.
  // Step 4.
  __jsvalue need_defaults = __boolean_value(true);
  // Step 5.
  std::string required_str = ValToStr(required);
  std::vector<std::string> vprops;
  __jsvalue prop, value;
  if (required_str == "date" || required_str == "any") {
    // Step 5a.
    vprops = {"weekday", "year", "day"};
    for (int i = 0; i < vprops.size(); i++) {
      // Step 5a i.
      prop = StrToVal(vprops[i]);
      value = __jsop_getprop(options, &prop);
      if (!__is_undefined(&value)) {
        need_defaults = __boolean_value(false);
      }
    }
  }
  // Step 6.
  if (required_str == "time" || required_str == "any") {
    // Step 6a.
    vprops = {"hour", "minute", "second"};
    for (int i = 0; i < vprops.size(); i++) {
      // Step 6a i.
      prop = StrToVal(vprops[i]);
      value = __jsop_getprop(options, &prop);
      if (!__is_undefined(&value)) {
        need_defaults = __boolean_value(false);
      }
    }
  }
  // Step 7.
  std::string defaults_str = ValToStr(defaults);
  __jsvalue v;
  __jsprop_desc desc;
  if (__jsval_to_boolean(&need_defaults) == true &&
      (defaults_str == "date" || defaults_str == "all")) {
    // Step 7a.
    vprops = {"year", "month", "day"};
    for (int i = 0 ; i < vprops.size(); i++) {
      prop = StrToVal(vprops[i]);
      // Step 7a i.
      v = StrToVal("numeric");
      desc = __new_value_desc(&v, JSPROP_DESC_HAS_VWEC);
      __jsobj_internal_DefineOwnProperty(__jsval_to_object(options), &prop, desc, false);
    }
  }
  // Step 8.
  if (__jsval_to_boolean(&need_defaults) == true &&
      (defaults_str == "time" || defaults_str == "all'")) {
    // Step 8a.
    vprops = {"hour", "minute", "second"};
    for (int i = 0; i < vprops.size(); i++) {
      // Step 8a i.
      v = StrToVal("numeric");
      desc = __new_value_desc(&v, JSPROP_DESC_HAS_VWEC);
      __jsobj_internal_DefineOwnProperty(__jsval_to_object(options), &prop, desc, false);
    }
  }
  return *options;
}

// ECMA-402 1.0 12.1.1.1
__jsvalue BasicFormatMatcher(__jsvalue *options, __jsvalue *formats) {
  // Step 1.
  int removal_penalty = 120;
  // Step 2.
  int addition_penalty = 20;
  // Step 3.
  int long_less_penalty = 8;
  // Step 4.
  int long_more_penalty = 6;
  // Step 5.
  int short_less_penalty = 6;
  // Step 6.
  int short_more_penalty = 3;
  // Step 7.
  int best_score = (int)-INFINITY;
  // Step 8.
  __jsvalue best_format = __undefined_value();
  // Step 9.
  int i = 0;
  // Step 10.
  __jsvalue prop = StrToVal("length");
  __jsvalue len = __jsop_getprop(formats, &prop);
  // Step 11.
  while (i < __jsval_to_number(&len)) {
    // Step 11a.
    prop = StrToVal("formats");
    __jsvalue format = __jsop_getprop(formats, &prop);
    // Step 11b.
    int score = 0;
    // Step 11c.
    auto it = kDateTimeFormatComponents.begin();
    while (it != kDateTimeFormatComponents.end()) {
      // Step 11c i.
      std::string p = it->first;
      __jsvalue property = StrToVal(p);
      __jsvalue options_prop = __jsop_getprop(options, &property);
      // Step 11c ii.
      __jsvalue format_prop_desc = __jsobj_getOwnPropertyNames(formats, &property);
      // Step 11c iii.
      if (!__is_undefined(&format_prop_desc)) {
        // Step 11c iii 1.
        __jsvalue format_prop = __jsop_getprop(&format, &property);
        // Step 11c iv.
        if (__is_undefined(&options_prop) && !__is_undefined(&format_prop)) {
          score -= addition_penalty;
        } else if (!__is_undefined(&options_prop) && __is_undefined(&format_prop)) {
          // Step 11c v.
          score -= removal_penalty;
        } else {
          // Step 11c vi 1.
          std::vector<std::string> v = {"2-digit", "numeric", "narrow", "short", "long"};
          __jsvalue values = StrVecToVal(v);
          // Step 11c vi 2.
          __jsvalue options_prop_index = __jsarr_pt_indexOf(&values, &options_prop, 0);
          // Step 11c vi 3.
          __jsvalue format_prop_index = __jsarr_pt_indexOf(&values, &format_prop, 0);
          // Step 11c vi 4.
          int delta = std::max(std::min(__jsval_to_number(&format_prop_index) -
                                        __jsval_to_number(&options_prop_index), 2), -2);
          // Step 11c vi 5.
          if (delta == 2) {
            score -= long_more_penalty;
          } else if (delta == 1) {
            // Step 11c vi 6.
            score -= short_more_penalty;
          } else if (delta == -1) {
            // Step 11c vi 7.
            score -= short_less_penalty;
          } else if (delta == -2) {
            // Step 11c vi 8.
            score -= long_less_penalty;
          }
        }
      }
    }
    // Step 11d.
    if (score > best_score) {
      // Step 11d i.
      best_score = score;
      // Step 11d ii.
      best_format = format;
    }
    // Step 11e.
    i--;
  }
  // Step 12.
  return best_format;
}

__jsvalue BestFitFormatMatcher(__jsvalue *options, __jsvalue *formats) {
  return BasicFormatMatcher(options, formats);
}

// ECMA-402 1.0 12.2.2
// Intl.DateTimeFormat.supportedLocalesOf(locales [, options])
__jsvalue __jsintl_DateTimeFormatSupportedLocalesOf(__jsvalue *locales,
                                                    __jsvalue *arg_list,
                                                    uint32_t nargs) {
  // Step 1.
  __jsvalue options;
  if (nargs == 1) {
    options = __undefined_value();
  } else {
    options = arg_list[0];
  }
  // Step 2.
  __jsvalue prop = StrToVal("availableLocales");
  __jsvalue date_time_format; // TODO
  __jsvalue available_locales = __jsop_getprop(&date_time_format, &prop);
  // Step 3.
  __jsvalue requested_locales = CanonicalizeLocaleList(&options);
  // Step 4.
  return SupportedLocales(&available_locales, &requested_locales, &options);
}

// ECMA-402 1.0 12.3.2
// Intl.DateTimeFormat.prototype.format
__jsvalue __jsintl_DateTimeFormatFormat(__jsvalue *this_arg,
                                        __jsvalue *arg_list, uint32_t nargs) {
  // TODO: not implemented yet.
  return __null_value();
}

// ECMA-402 1.0 12.3.3
// Intl.DateTimeFormat.prototype.resolvedOptions()
__jsvalue __jsintl_DateTimeFormatResolvedOptions(__jsvalue *this_arg,
                                                 __jsvalue *arg_list,
                                                 uint32_t nargs) {
  // TODO: not implemented yet.
  return __null_value();
}

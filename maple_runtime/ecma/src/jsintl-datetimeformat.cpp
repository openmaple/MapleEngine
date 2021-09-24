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
#include <unicode/ucal.h>
#include <unicode/calendar.h>
#include <unicode/timezone.h>
#include <unicode/udat.h>
#include <unicode/udatpg.h>
#include <unicode/ustring.h>
#include "jsobject.h"
#include "jsobjectinline.h"
#include "jsarray.h"
#include "jsmath.h"
#include "jsdate.h"
#include "jsintl.h"

static const size_t INITIAL_STRING_BUFFER_SIZE = 32;

extern std::unordered_map<std::string,std::vector<std::string>> kDateTimeFormatComponents;

// ECMA-402 1.0 12.1 The Intl.DateTimeFormat Constructor
__jsvalue __js_DateTimeFormatConstructor(__jsvalue *this_arg,
                                        __jsvalue *arg_list, uint32_t nargs) {
  if (!__is_null(this_arg) && __is_js_object(this_arg)) {
    __jsvalue p = StrToVal("initializedIntlObject");
    __jsvalue v = __jsop_getprop(this_arg, &p);
    if (!__is_undefined(&v)) {
      if (__is_boolean(&v) && __jsval_to_boolean(&v) == true) {
        MAPLE_JS_TYPEERROR_EXCEPTION();
      }
    }
  }

  __jsobject *obj = __create_object();
  __jsobj_set_prototype(obj, JSBUILTIN_INTL_DATETIMEFORMAT_PROTOTYPE);
  obj->object_class = JSINTL_DATETIMEFORMAT;
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
  // Let 'options' be the result of calling the ToDateTimeOptions abstract operation
  // with argument 'options', "any", and "date".
  __jsvalue required = StrToVal("any");
  __jsvalue default_val = StrToVal("date");
  *options = ToDateTimeOptions(options, &required, &default_val);

  // Step 5.
  // Let 'opt' be a new Record.
  __jsobject *opt_obj = __js_new_obj_obj_0();
  __jsvalue opt = __object_value(opt_obj);

  // Additional check if 'localeMatcher' is null (JSI9502).
  __jsvalue p = StrToVal("localeMatcher");
  __jsvalue v = __jsop_getprop(options, &p);
  if (__is_null(&v)) {
    MAPLE_JS_RANGEERROR_EXCEPTION();
  }
  // Step 6.
  __jsvalue property = StrToVal("localMatcher");
  __jsvalue type = StrToVal("string");
  __jsvalue values = StrVecToVal({"lookup", "best fit"});
  __jsvalue fallback = StrToVal("best fit");
  __jsvalue matcher = GetOption(options, &property, &type, &values, &fallback);
  // Step 7.
  // Set opt.[[localeMatcher]] to 'matcher'.
  prop = StrToVal("localeMatcher");
  __jsop_setprop(&opt, &prop, &matcher);
  // Step 8.
  __jsobject *data_time_format_obj = __create_object();
  data_time_format_obj->object_class = JSINTL_DATETIMEFORMAT;
  data_time_format_obj->extensible = true;
  data_time_format_obj->object_type = JSREGULAR_OBJECT;
  __jsvalue date_time_format = __object_value(data_time_format_obj);

  InitializeDateTimeFormatProperties(&date_time_format, &requested_locales, options);

  // Step 9.
  // Let 'localeData' be the value of the [[localeData]] internal property of
  // 'DateTimeFormat'.
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
  // Set the [[locale]] internal property of 'dataTimeFormat' to the value of
  // r.[[locale]].
  prop = StrToVal("locale");
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
  // Let 'dataLocale' be the value of r.[[dataLocale]].
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
    tz = StrToVal(tz_str);
  }
  // Step 17.
  prop = StrToVal("timeZone");
  __jsop_setprop(this_date_time_format, &prop, &tz);
  // Step 18.
  // Let 'opt' be a new Record.
  opt_obj = __js_new_obj_obj_0();
  opt = __object_value(opt_obj);
  // Step 19.
  // For each row of Table 3, except the header row, do:
  for (auto it = kDateTimeFormatComponents.begin(); it != kDateTimeFormatComponents.end(); it++) {
    // Step 19a.
    // Let 'prop' be the name given in the Property column of the row.
    std::string p = it->first;
    prop = StrToVal(p);
    // Step 19b.
    // Let 'value' be the result of calling the GetOption abstract operation,
    // passing as argument 'options', the name given in the Property column
    // of the row, "string", a List containing the string given in the Values
    // column of the row, and undefined.
    std::vector<std::string> v = it->second;
    __jsvalue type = StrToVal("string");
    __jsvalue values = StrVecToVal(v);
    __jsvalue fallback = __undefined_value();
    value = GetOption(options, &prop, &type, &values, &fallback);
    // Step 19c.
    // Set opt.[[<prop>]] to 'value'.
    __jsop_setprop(&opt, &prop, &value);
  }
  // Step 20.
  // Let 'dataLocaleData' be the result of calling the [[Get]] internal
  // method of 'localeData' with argument 'dataLocale'.
  // Note: 'dataLocale' from Step 14.
  __jsvalue data_locale_data = __jsop_getprop(&locale_data, &data_locale);

  // Step 21.
  // Let 'formats' be the result of calling the [[Get]] internal method of
  // 'dataLocaleData' with argument "formats".
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
  // For each row in Table 3, except the headerrow, do
  for (auto it = kDateTimeFormatComponents.begin(); it != kDateTimeFormatComponents.end(); it++) {
    // Step 25a.
    std::string p = it->first;
    prop = StrToVal(p);
    // Step 25b.
    // Let 'pDesc' be  the result of calling the [[GetOwnProperty]] internal method of
    // 'bestFormat' with argument 'prop'.
    __jsobject *best_format_obj = __jsval_to_object(&best_format);
    __jsprop_desc p_desc = __jsobj_internal_GetOwnProperty(best_format_obj, &prop);
    // Step 25c.
    // If 'pDesc' is not undefined, then
    if (!__is_undefined_desc(p_desc)) {
      // Step 25c i.
      // Let 'p' be the rersult of calling the [[Get]] internal method of 'bestFormat'
      // with argument 'prop'.
      __jsvalue p = __jsop_getprop(&best_format, &prop);
      // Step 25c ii.
      // Set the [[<prop>]] internal property of 'dateTimeFormat' to 'p'.
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
    prop = StrToVal("hour");
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
  } else {
    *options = __object_value(__jsval_to_object(options));
  }
  // Step 2.
  // Let 'create' be the standard built-in function object defined in ES5, 15.2.3.5.
  // Step 3.
  // Let 'options' be the result of calling the [[Call]] internal method of 'create'
  // with 'undefined' as the 'this' value and an argument list containing the single
  // item 'options'.
  __jsvalue this_object = __undefined_value();
  __jsvalue properties = __undefined_value();
  *options = __jsobj_create(&this_object, options, &properties);

  // Step 4.
  __jsvalue need_defaults = __boolean_value(true);
  // Step 5.
  std::string required_str = ValToStr(required);
  std::vector<std::string> vprops;
  __jsvalue prop, value;
  if (required_str == "date" || required_str == "any") {
    // Step 5a.
    vprops = {"weekday", "year", "month", "day"};
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
      (defaults_str == "time" || defaults_str == "all")) {
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
  float best_score = -INFINITY;
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
    __jsobject *formats_object = __jsval_to_object(formats);
    //__jsvalue format = __jsop_getprop(formats, &prop);
    __jsvalue format = __jsarr_GetElem(formats_object, i);
    // Step 11b.
    float score = 0;
    // Step 11c.
    // For each 'property' shown in Table 3:
    for (auto it = kDateTimeFormatComponents.begin(); it != kDateTimeFormatComponents.end(); it++) {
      // Step 11c i.
      // Let 'optionsProp' be options.[[<property>]].
      std::string p = it->first;
      __jsvalue property = StrToVal(p);
      __jsvalue options_prop = __jsop_getprop(options, &property);
      // Step 11c ii.
      // Let 'formatPropDesc' be the result of caling the [[GetOwnProperty]]
      // internal method of 'format' with argument 'property'.
      //__jsvalue format_prop_desc = __jsobj_internal_GetProperty(formats, &property);
      __jsobject *format_object = __jsval_to_object(&format);
      __jsprop_desc format_prop_desc = __jsobj_internal_GetProperty(format_object, &property);
      __jsvalue format_prop = __undefined_value();
      // Step 11c iii.
      // If 'formatPropDesc' is not undefined, then
      if (!__is_undefined_desc(format_prop_desc)) {
        // Step 11c iii 1.
        // Let 'formatProp' be the result of calling the [[Get]] internal method
        // of 'format' with argument 'property'.
        format_prop = __jsop_getprop(&format, &property);
      }
      // Step 11c iv.
      // If 'optionsProp' is undefined and 'formatProp' is not undefined, then
      // decrease 'score' by 'additionalPenalty'.
      if (__is_undefined(&options_prop) && !__is_undefined(&format_prop)) {
        score -= addition_penalty;
      } else if (!__is_undefined(&options_prop) && __is_undefined(&format_prop)) {
        // Step 11c v.
        // Else if 'optionsProp' is not undefined and 'formatProp' is undefined,
        // then decrease 'score' by 'removePenalty'.
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
    // Step 11d.
    if (score > best_score) {
      // Step 11d i.
      best_score = score;
      // Step 11d ii.
      best_format = format;
    }
    // Step 11e.
    i++;
  }
  // Step 12.
  return best_format;
}

__jsvalue BestFitFormatMatcher(__jsvalue *options, __jsvalue *formats) {
  return BasicFormatMatcher(options, formats);
}

// ECMA-402 1.0 12.2.2
// Intl.DateTimeFormat.supportedLocalesOf(locales [, options])
__jsvalue __jsintl_DateTimeFormatSupportedLocalesOf(__jsvalue *date_time_format,
                                                    __jsvalue *arg_list,
                                                    uint32_t nargs) {
  // Step 1.
  __jsvalue locales = __undefined_value(), options = __undefined_value();
  if (nargs == 1) {
    locales = arg_list[0];
  } else if (nargs == 2) {
    locales = arg_list[0];
    options = arg_list[1];
  }
  // Step 2.
  __jsvalue available_locales = GetAvailableLocales();
  // Step 3.
  __jsvalue requested_locales = CanonicalizeLocaleList(&locales);
  // Step 4.
  return SupportedLocales(&available_locales, &requested_locales, &options);
}

// ECMA-402 1.0 12.3.2
__jsvalue ToLocalTime(__jsvalue *date, __jsvalue *calendar, __jsvalue *time_zone) {
  // Step 1. Apply calendrical calculations on date for the given calendar and time zone
  // to produce weekday, era, year, month, day, hour, minute, second, and inDST values.
  int64_t t = (int64_t)__jsval_to_double(date);
  std::string cal = ValToStr(calendar);
  int64_t tz;
  if (cal == "gregory") {
    UErrorCode status = U_ZERO_ERROR;
    UnicodeString tz_str;
    // check if 'time_zone' is not defined.
    if (!__is_undefined(time_zone)) {
      tz_str = UnicodeString::fromUTF8(ValToStr(time_zone));
    } else {
      tz_str = UnicodeString::fromUTF8("UTC");
    }
    TimeZone *timezone = TimeZone::createTimeZone(tz_str);
    Calendar *cal = Calendar::createInstance(status);
    UDate cur_date = cal->getNow();
    int32_t std_offset, dst_offset;
    timezone->getOffset(cur_date, true, std_offset, dst_offset, status);
    int64_t time_zone_offset = (int64_t) dst_offset/(1000*60*60);
    tz = t + time_zone_offset;
  } else {
    MAPLE_JS_ASSERT(false && "NIY: ToLocalTime");
  }
  // Step 2. Return a Record with fields [[weekday]], [[era]], [[year]], [[month]], [[day]],
  // [[hour]], [[minute]], [[second]], and [[inDST]], each with the corresponding calculated
  // value.
  __jsvalue p, v;

  __jsobject *time_object = __create_object();
  __jsvalue time_val = __object_value(time_object);
  time_object->object_class = JSOBJECT;
  time_object->extensible = true;
  time_object->object_type = JSREGULAR_OBJECT;

  p = StrToVal("weekday");
  v = __double_value(WeekDay(tz));
  __jsop_setprop(&time_val, &p, &v);

  p = StrToVal("era");
  int64_t year = YearFromTime(tz);
  v = (year < 0) ? StrToVal("BC") : StrToVal("AD");
  __jsop_setprop(&time_val, &p, &v);

  p = StrToVal("year");
  v = __double_value(YearFromTime(tz));
  __jsop_setprop(&time_val, &p, &v);

  p = StrToVal("month");
  v = __double_value(MonthFromTime(tz));
  __jsop_setprop(&time_val, &p, &v);

  p = StrToVal("day");
  v = __double_value(DateFromTime(tz));
  __jsop_setprop(&time_val, &p ,&v);

  p = StrToVal("hour");
  v = __double_value(HourFromTime(tz));
  __jsop_setprop(&time_val, &p, &v);

  p = StrToVal("minute");
  v = __double_value(MinFromTime(tz));
  __jsop_setprop(&time_val, &p, &v);

  p = StrToVal("second");
  v = __double_value(SecFromTime(tz));
  __jsop_setprop(&time_val, &p, &v);

  p = StrToVal("inDST");
  // TODO: inDST handling

  return time_val;
}

// ECMA-402 1.0 12.3.2
// FormatDateTime returns a String value representing 'x' (interpreted as time
// value) according to the effective locale and the formatting options of
// 'dateTimeFormat'.
__jsvalue FormatDateTime(__jsvalue *date_time_format, __jsvalue *x) {
  __jsvalue prop, value;
  // Step 1.
  if (__is_infinity(x) || __is_neg_infinity(x)) {
    MAPLE_JS_RANGEERROR_EXCEPTION();
  }
  // Step 2.
  // Let 'locale' be the value of the [[locale]] internal property of 'dateTimeFormat'.
  prop = StrToVal("locale");
  __jsvalue locale = __jsop_getprop(date_time_format, &prop);
  // Step 3.
  // Let 'nf' be the result of creating a new NumberFormat object
  // as if by the expression new Intl.NumberFormat([locale], {useGrouping: false}).
  __jsobject *locales_obj = __js_new_arr_elems_direct(&locale, 1);
  __jsvalue locales = __object_value(locales_obj);
  __jsobject *options_obj = __js_new_obj_obj_0();
  __jsvalue options = __object_value(options_obj);
  prop = StrToVal("useGrouping");
  value = __boolean_value(false);
  __jsop_setprop(&options, &prop, &value);
  __jsvalue args[] = {locales, options};

  __jsvalue undefined = __undefined_value();
  __jsvalue nf = __js_NumberFormatConstructor(&undefined, args, 2);

  // Step 4.
  // Let 'nf2' be the result of creating a new NumberFormat object
  // as if by the expression new Intl.NumberFormat([locale],
  // {minimumIntegerDigits: 2, useGrouping: false}).
  options_obj = __js_new_obj_obj_0();
  options = __object_value(options_obj);
  prop = StrToVal("minimumIntegerDidigts");
  value = __number_value(2);
  __jsop_setprop(&options, &prop, &value);
  prop = StrToVal("useGrouping");
  value = __boolean_value(false);
  __jsop_setprop(&options, &prop, &value);

  __jsvalue nf2 = __js_NumberFormatConstructor(&undefined, args, 2);

  // Step 5.
  prop = StrToVal("calendar");
  __jsvalue calendar = __jsop_getprop(date_time_format, &prop);
  prop = StrToVal("timeZone");
  __jsvalue time_zone = __jsop_getprop(date_time_format, &prop);
  __jsvalue tm = ToLocalTime(x, &calendar, &time_zone);

  // Step 6.
  prop = StrToVal("pattern");
  __jsvalue result = __jsop_getprop(date_time_format, &prop);

  // Replace operation step 7 to 8 (described in the specification) with ICU.
#if 0
  // Step 7.
  __jsvalue pm;
  for (auto it = kDateTimeFormatComponents.begin(); it != kDateTimeFormatComponents.end(); it++) {
    std::string property = it->first;
    std::vector<std::string> values = it->second;
    // Step 7a i.
    // Let 'p' b the name given in the Property column of the row.
    __jsvalue p = StrToVal(property);
    // Step 7a ii.
    // Let 'f' be the value of the [[<p>]] internal property of 'dateTimeFormat'.
    __jsvalue f = __jsop_getprop(date_time_format, &p);
    if (__is_undefined(&f)) continue; // Skip undefined value for a given property.

    // Step 7a iii.
    // Let 'v' be the value of tm.[[<p>]].
    __jsvalue v = __jsop_getprop(&tm, &p);
    // Step 7a iv.
    if (ValToStr(&p) == "year" && __jsval_to_number(&v) <= 0) {
      v = __number_value(1 - __jsval_to_number(&v));
    }
    // Step 7a v.
    if (ValToStr(&p) == "month") {
      v = __number_value(__jsval_to_number(&v)+1);
    }
    // Step 7a vi.
    __jsvalue p2 = StrToVal("hour12");
    __jsvalue v2 = __jsop_getprop(date_time_format, &p2);
    if (ValToStr(&p) == "hour" && __jsval_to_boolean(&v2) == true) {
      // Step 7a vi 1.
      v = __number_value(__jsval_to_number(&v) % 12);
      // Step 7a vi 2.
      v2 = __jsop_getprop(&tm, &p);
      if (__jsval_to_number(&v) == __jsval_to_number(&v2)) {
        pm = __boolean_value(false);
      } else {
        pm = __boolean_value(true);
      }
      // Step 7a vi 3.
      p2 = StrToVal("hourNo0");
      v2 = __jsop_getprop(date_time_format, &p2);
      if (__jsval_to_number(&v) == 0 && __jsval_to_boolean(&v2) == true) {
        v = __number_value(12);
      }
    }
    // Step 7a vii.
    __jsvalue fv;
    std::string f_str = ValToStr(&f);
    if (f_str == "numeric") {
      // Step 7a vii 1.
      fv = FormatNumber(&nf, &v);
    } else if (f_str == "2-digit") { // Step 7a viii.
      // Step 7a viii 1.
      fv = FormatNumber(&nf2, &v);
      __jsstring *fv_str = __jsval_to_string(&fv);
      // step 7a viii 2.
      int len = __jsstr_get_length(fv_str);
      if (len < 2) {
        __jsvalue start = __number_value(len-2), end = __undefined_value();
        fv = __jsstr_substring(&fv, &start, &end);
      }
    } else if (f_str == "narrow" || f_str == "short" || f_str == "long") { // Step 7a ix
      MAPLE_JS_ASSERT(false && "FIY: FormatDateTime()");
    }
    __jsvalue search = StrToVal("{" + ValToStr(&p) + "}");
    __jsstr_replace(&result, &search, &fv);
  }
  // Step 8.
  prop = StrToVal("hour12");
  value = __jsop_getprop(date_time_format, &prop);
  __jsvalue fv;
  if (__jsval_to_boolean(&value)) {
    // Step 8a.
    if (__jsval_to_boolean(&pm)) {
      MAPLE_JS_ASSERT(false && "FIY: FormatDateTime()");
    }
    // Step 8b.
    __jsvalue search = StrToVal("{ampm}");
    __jsstr_replace(&result, &search, &fv);
  }
#endif

  // 'result' contains pattern.
  std::string pat = ValToStr(&result);
  UChar *pattern = (UChar*)VMMallocGC(sizeof(UChar)*(pat.length()+1));
  for (int i = 0; i < pat.length(); i++) {
    pattern[i] = pat[i];
  }
  std::string tz;
  if (!__is_undefined(&time_zone)) {
    tz = ValToStr(&time_zone);
  } else {
    tz = "UTC"; // TODO: check
  }
  const UChar *timezone = (UChar *)tz.c_str();
  uint32_t timezone_len = tz.length();
  uint32_t pattern_len = u_strlen(pattern);
  UErrorCode status = U_ZERO_ERROR;
  std::string locale_name = ValToStr(&locale);

  UDateFormat *df = udat_open(UDAT_PATTERN, UDAT_PATTERN, locale_name.c_str(), timezone, timezone_len, pattern, pattern_len, &status);
  if (U_FAILURE(status)) {
    MAPLE_JS_ASSERT(false && "Error in udat_open()");
  }
  // Convert to Gregorian calendar.
  UCalendar *cal = const_cast<UCalendar*>(udat_getCalendar(df));
  const double start_of_time = -8.64e15;
  ucal_setGregorianChange(cal, start_of_time, &status);

  double time = __jsval_to_double(x);
  UChar *chars = (UChar*)VMMallocGC(sizeof(UChar)*(INITIAL_STRING_BUFFER_SIZE+1));
  int32_t size = udat_format(df, time, chars, INITIAL_STRING_BUFFER_SIZE, nullptr, &status);
  if (status == U_BUFFER_OVERFLOW_ERROR) {
    status = U_ZERO_ERROR;
    VMReallocGC(chars, sizeof(UChar)*(INITIAL_STRING_BUFFER_SIZE+1), sizeof(UChar)*(size+1));
    udat_format(df, time, chars, size, nullptr, &status);
  }
  if (U_FAILURE(status)) {
    MAPLE_JS_ASSERT(false && "Error in udat_format()");
  }
  char *res = (char*)VMMallocGC(sizeof(char)*64);
  u_austrcpy(res, chars);
  std::string res_str(res);
  result = StrToVal(res_str);

  udat_close(df);

  // Step 9.
  return result;
}

// ECMA-402 1.0 12.3.2
// Intl.DateTimeFormat.prototype.format
__jsvalue __jsintl_DateTimeFormatFormat(__jsvalue *date_time_format, __jsvalue *date) {
  // Step 1.
  // If the [[boundFormat]] internal property of this DateTimeFormat object is
  // undefined, then:
  __jsvalue p = StrToVal("boundFormat");
  __jsvalue bound_format = __jsop_getprop(date_time_format, &p);
  if (__is_undefined(&bound_format)) {
    // Step 1a.
    // Let 'F' be a Function object, and the length property set to 0,
    // that takes the argument 'date'.

    // Step 1a i.
    // If 'date' is not provided or is undefined, then let 'x' be the result
    // as if by the expression Date.now() where Date.now is the standard built-in
    // function.
    __jsvalue x;
    if (__is_undefined(date)) {
      __jsvalue undefined = __undefined_value();
      x = __jsdate_Now(&undefined);
    } else {
      // Step 1a ii.
      // Else let 'x' be ToNumber(date).
      x = __number_value(__jsval_to_number(date));
    }
    // Step 1a iii.
    // Return the result of calling the FormatDateTime abstract operation with argument
    // this and x.
    __jsvalue args[] = {*date_time_format, x};
#define ATTRS(nargs, length) \
  ((uint32_t)(uint8_t)(nargs == UNCERTAIN_NARGS ? 1: 0) << 24 | (uint32_t)(uint8_t)nargs << 16 | \
     (uint32_t)(uint8_t)length << 8 | JSFUNCPROP_NATIVE)
    __jsvalue f = __js_new_function((void*)FormatDateTime, NULL, ATTRS(2, 1));
    int len = 1;

    // Temporary workaround to avoid 'strict' constraint inside __jsfun_pt_bind()
    // function. Save old value of __js_ThisBinding.
    __jsvalue this_binding_old = __js_ThisBinding;
    __js_ThisBinding = f;
    // Step 1c. Let 'bf' be the result of calling the [[Call]] internal method
    // of 'bind' with 'F' as the this value and an argument list containing
    // the single item this.
    __jsvalue bf = __jsfun_pt_bind(&f, args, len);
    __js_ThisBinding = this_binding_old;

    // Step 1d.
    // Set the [[boundFormat]] internal property of this DateTimeFormat object to 'bf'.
    __jsop_setprop(date_time_format, &p, &bf);
  }
  // Step 2.
  // Return the value of the [[boundFormat]] internal property of this DateTimeFormat
  // object.
  __jsvalue res = __jsop_getprop(date_time_format, &p);
  return res;
}

void ResolveICUPattern(__jsvalue *date_time_format, __jsvalue *pattern) {
  int i = 0;
  uint32_t len = __jsstr_get_length(__jsval_to_string(pattern));
  std::string pat = ValToStr(pattern);
  __jsvalue p, v;

  while (i < len) {
    char c = pat[i++];
    if (c == '\'') {
      while (i < len && pat[i] != '\'')
        i++;
      i++;
    } else {
      int count = 1;
      while (i < len && pat[i] == c) {
        i++;
        count++;
      }
      std::string v;
      switch (c) {
      case 'G':
      case 'E':
      case 'z':
      case 'v':
      case 'V':
        if (count <= 3)
          v = "short";
        else if (count == 4)
          v = "long";
        else
          v = "narrow";
        break;
      case 'y':
      case 'd':
      case 'h':
      case 'H':
      case 'm':
      case 's':
      case 'k':
      case 'K':
        if (count == 2)
          v = "2-digit";
        else
          v = "numeric";
        break;
      case 'M':
      case 'L':
        if (count == 1)
          v = "numeric";
        else if (count == 2)
          v = "2-digit";
        else if (count == 3)
          v = "short";
        else if (count == 4)
          v = "long";
        else
          v = "narrow";
        break;
      default:
        // skip other characters in pattern.
        break;
      }

      // Set value to date_time_format.
      bool valid_char = true;
      if (c == 'E')
        p = StrToVal("weekday");
      else if (c == 'G')
        p = StrToVal("era");
      else if (c == 'y')
        p = StrToVal("year");
      else if (c == 'M' || c == 'L')
        p = StrToVal("month");
      else if (c == 'd')
        p = StrToVal("day");
      else if (c == 'h' || c == 'H' || c == 'k' || c == 'K')
        p = StrToVal("hour");
      else if (c == 'm')
        p = StrToVal("minute");
      else if (c == 's')
        p = StrToVal("second");
      else if (c == 'z' || c == 'v' || c == 'V')
        p = StrToVal("timeZoneName");
      else {
        valid_char = false;
      }

      __jsvalue value;
      if (valid_char) {
        value = StrToVal(v);
        __jsop_setprop(date_time_format, &p, &value);
      }

      // Set 'hour12'.
      if (c == 'h' || c == 'K') {
        value = __boolean_value(true);
      } else if (c == 'H' || c == 'k') {
        value = __boolean_value(false);
      }
      p = StrToVal("hour12");
      __jsop_setprop(date_time_format, &p, &value);
    }
  } // end of while
}

// ECMA-402 1.0 12.3.3
// Intl.DateTimeFormat.prototype.resolvedOptions()
__jsvalue __jsintl_DateTimeFormatResolvedOptions(__jsvalue *date_time_format) {
  __jsobject *dtf_obj = __create_object();
  __jsobj_set_prototype(dtf_obj, JSBUILTIN_INTL_DATETIMEFORMAT_PROTOTYPE);
  dtf_obj->object_class = JSINTL_DATETIMEFORMAT;
  dtf_obj->extensible = true;
  dtf_obj->object_type = JSREGULAR_OBJECT;
  __jsvalue dtf = __object_value(dtf_obj);

  std::vector<std::string> props = {"locale", "calendar", "numberingSystem",
                                    "timeZone", "weekday", "era", "year",
                                    "month", "day", "hour", "minute",
                                    "second", "timezoneName",
                                    "hour12", "hourNo0"};
  __jsvalue p, v;
  for (int i = 0; i < props.size(); i++) {
    p = StrToVal(props[i]);
    v = __jsop_getprop(date_time_format, &p);
    if (!__is_undefined(&v))
      __jsop_setprop(&dtf, &p, &v);
  }
  // Handle "pattern".
  p = StrToVal("pattern");
  v = __jsop_getprop(date_time_format, &p);
  ResolveICUPattern(&dtf, &v);

  // NOTE: is this really needed?
  p = StrToVal("initializedNumberFormat");
  v = __boolean_value(true);
  __jsop_setprop(&dtf, &p, &v);

  return dtf;
}

//void InitializeDateTimeFormatProperties(__jsvalue *date_time_format, __jsvalue *locales, __jsvalue *options, std::vector<std::string> properties) {
void InitializeDateTimeFormatProperties(__jsvalue *date_time_format, __jsvalue *locales, __jsvalue *options) {
  __jsvalue p, v;
  __jsvalue locale, locale_data;
  std::string locale_name;

  p = StrToVal("availableLocales");
  v = GetAvailableLocales();
  __jsop_setprop(date_time_format, &p, &v);

  p = StrToVal("relevantExtensionKeys");
  std::vector<std::string> values = {"ca", "nu"};
  v = StrVecToVal(values);
  __jsop_setprop(date_time_format, &p, &v);

  // Set 'localeData'.
  __jsobject *locale_object = __create_object();
  locale = __object_value(locale_object);
  locale_object->object_class = JSOBJECT;
  locale_object->extensible = true;
  locale_object->object_type = JSREGULAR_OBJECT;

  // [[localeData]][locale]['nu']
  std::vector<std::string> vec = GetNumberingSystems();
  v = StrVecToVal(vec);
  p = StrToVal("nu");
  __jsop_setprop(&locale, &p, &v);  // Set "nu" to locale.

  // [[localeData]][locale]['ca']
  vec = GetAvailableCalendars();
  v = StrVecToVal(vec);
  p = StrToVal("ca");
  __jsop_setprop(&locale, &p, &v);  // Set "ca" to locale.

  __jsobject *locale_data_object = __create_object();
  locale_data = __object_value(locale_data_object);
  locale_data_object->object_class = JSOBJECT;
  locale_data_object->extensible = true;
  locale_data_object->object_type = JSREGULAR_OBJECT;

  __jsobject *locales_object = __jsval_to_object(locales); // locales from function arguments.
  uint32_t size = __jsobj_helper_get_length(locales_object);
  for (int j = 0; j < size; j++) {
    p = __jsarr_GetElem(locales_object, j);
    // locale should be included in available_locales.
    __jsvalue available_locales = GetAvailableLocales();
    p = BestAvailableLocale(&available_locales, &p);

    // Check if 'p' is undefined (JSI9460).
    if (__is_undefined(&p)) {
      p = DefaultLocale();
    }
    __jsop_setprop(&locale_data, &p, &locale);

    locale_name = ValToStr(&p);
  }
  if (size == 0) {
    // When locale is not specified, use default one.
    p = DefaultLocale();
    __jsop_setprop(&locale_data, &p, &locale);
  }

  p = StrToVal("localeData");
  // Set localeData to date_time_format object.
  __jsop_setprop(date_time_format, &p, &locale_data);

  // Set 'format'.
  // Create a single format for en-US by default,
  // and make it as a single entry of "formats" array.
  __jsobject *format_object = __create_object();
  __jsvalue format = __object_value(format_object);
  format_object->object_class = JSOBJECT;
  format_object->extensible = true;
  format_object->object_type = JSREGULAR_OBJECT;

  p = StrToVal("pattern");
  MAPLE_JS_ASSERT(__is_null(&locale) == false);
  std::string pat = GetDateTimeStringSkeleton(&locale, options);
  v = GetICUPattern(locale_name, pat);
  __jsop_setprop(&format, &p, &v);

  p = StrToVal("pattern12");
  v = StrToVal("{hour}:{minute}:{second}:{ampm}");
  __jsop_setprop(&format, &p, &v);

  __jsobject *formats_array = __js_new_arr_elems_direct(&format, 1);
  __jsvalue formats = __object_value(formats_array);

  // Set 'formats" to localeData.
  assert(__is_null(&locale) == false);
  p = StrToVal("formats");
  __jsop_setprop(&locale, &p, &formats);

  // Set 'timeZone' to date_time_format.
  p = StrToVal("timeZone");
  v = __undefined_value();
  __jsop_setprop(date_time_format, &p, &v);
}

std::vector<std::string> GetAvailableCalendars() {
  char *locale = getenv("LANG");
  UErrorCode status = U_ZERO_ERROR;
  std::vector<std::string> vec;

  UEnumeration *values = ucal_getKeywordValuesForLocale("ca", locale, false, &status);
  uint32_t count = uenum_count(values, &status);
  for (int i = 0; i < count; i++) {
    const char *cal_value = uenum_next(values, nullptr, &status);
    assert(U_FAILURE(status) == false);
    std::string s = ToBCP47CalendarName(cal_value);
    vec.push_back(s);
  }
  UCalendar *cal = ucal_open(nullptr, 0, locale, UCAL_DEFAULT, &status);
  const char *default_calendar = ucal_getType(cal, &status);
  assert(U_FAILURE(status) == false);
  std::string s = ToBCP47CalendarName(default_calendar);
  ucal_close(cal);
  vec.insert(vec.begin(), s);

  return vec;
}

// ICU returns old style values, so we need to convert them to BCP47 style ones.
std::string ToBCP47CalendarName(const char* name) {
  char s1[] = "ethiopic-amete-alem", s2[] = "gregorian", s3[] = "islamic-civil";
  std::string res;
  if (strcmp(name, s1) == 0) {
    res = "ethiopic";
  } else if (strcmp(name, s2) == 0) {
    res = "gregory";
  } else if (strcmp(name, s3) == 0) {
    res = "islamicc";
  } else {
    res = std::string(name);
  }
  return res;
}

std::string GetDateTimeStringSkeleton(__jsvalue *locale, __jsvalue *options) {
  std::string pattern = "";
  __jsvalue p, v;
  p = StrToVal("weekday");
  v = __jsop_getprop(options, &p);
  if (!__is_undefined(&v)) {
    std::string weekday = ValToStr(&v);
    if (weekday == "narrow") {
      pattern += "EEEEE";
    } else if (weekday == "short") {
      pattern += "E";
    } else if (weekday == "long") {
      pattern += "EEEE";
    }
  }
  p = StrToVal("era");
  v = __jsop_getprop(options, &p);
  if (!__is_undefined(&v)) {
    std::string era = ValToStr(&v);
    if (era == "narrow") {
      pattern += "GGGGG";
    } else if (era == "short") {
      pattern += "G";
    } else if (era == "long") {
      pattern += "GGGG";
    }
  }
  p = StrToVal("year");
  v = __jsop_getprop(options, &p);
  if (!__is_undefined(&v)) {
    std::string year = ValToStr(&v);
    if (year == "2-digit") {
      pattern += "yy";
    } else if (year == "numeric") {
      pattern += "y";
    }
  }
  p = StrToVal("month");
  v = __jsop_getprop(options, &p);
  if (!__is_undefined(&v)) {
    std::string month = ValToStr(&v);
    if (month == "2-digit") {
      pattern += "MM";
    } else if (month == "numeric") {
      pattern += "M";
    } else if (month == "narrow") {
      pattern += "MMMMM";
    } else if (month == "short") {
      pattern += "MMM";
    } else if (month == "long") {
      pattern += "MMMM";
    }
  }
  p = StrToVal("day");
  v = __jsop_getprop(options, &p);
  if (!__is_undefined(&v)) {
    std::string day = ValToStr(&v);
    if (day == "2-digit") {
      pattern += "dd";
    } else if (day == "numeric") {
      pattern += "d";
    }
  }
  std::string hour_pattern_char = "j";
  p = StrToVal("hour12");
  v = __jsop_getprop(options, &p);
  if (!__is_undefined(&v)) {
    if (__jsval_to_boolean(&v) == true) {
      hour_pattern_char = "h";
    } else {
      hour_pattern_char = "H";
    }
  }
  p = StrToVal("hour");
  v = __jsop_getprop(options, &p);
  if (!__is_undefined(&v)) {
    std::string hour = ValToStr(&v);
    if (hour == "2-digit") {
      pattern += hour_pattern_char + hour_pattern_char;
    } else if (hour == "numeric") {
      pattern += hour_pattern_char;
    }
  }
  p = StrToVal("minute");
  v=  __jsop_getprop(options, &p);
  if (!__is_undefined(&v)) {
    std::string minute = ValToStr(&v);
    if (minute == "2-digit") {
      pattern += "mm";
    } else if (minute == "numeric") {
      pattern += "m";
    }
  }
  p = StrToVal("second");
  v = __jsop_getprop(options, &p);
  if (!__is_undefined(&v)) {
    std::string second = ValToStr(&v);
    if (second == "2-digit") {
      pattern += "ss";
    } else if (second == "numeric") {
      pattern += "s";
    }
  }
  p = StrToVal("timeZoneName");
  v = __jsop_getprop(options, &p);
  if (!__is_undefined(&v)) {
    std::string timezone_name = ValToStr(&v);
    if (timezone_name == "short") {
      pattern += "z";
    } else if (timezone_name == "long") {
      pattern += "zzzz";
    }
  }
  return pattern;
}

const char* ToICULocale(std::string& locale) {
  replace(locale.begin(), locale.end(), '-', '_');
  return locale.c_str();
}

__jsvalue GetICUPattern(std::string& locale, std::string& skeleton) {
  UErrorCode status = U_ZERO_ERROR;
  UDateTimePatternGenerator *gen = udatpg_open(ToICULocale(locale), &status);
  if (U_FAILURE(status)) {
    MAPLE_JS_ASSERT(false && "Error in udatpg_open()");
  }

  uint32_t skeleton_len = skeleton.length();
  UChar skel[skeleton_len];
  for (int i = 0; i < skeleton_len; i++) {
    skel[i] = skeleton[i];
  }
  int32_t size = udatpg_getBestPattern(gen, skel, skeleton_len, nullptr, 0, &status);
  if (U_FAILURE(status) && status != U_BUFFER_OVERFLOW_ERROR) {
    MAPLE_JS_ASSERT(false && "Error in udatpg_getBestPattern()");
  }
  UChar *pattern = (UChar*) VMMallocGC(sizeof(UChar)*(size+1));
  pattern[size] = '\0';
  if (!pattern)
    MAPLE_JS_ASSERT(false && "Error in VMMAllocGC()");

  status = U_ZERO_ERROR;
  udatpg_getBestPattern(gen, skel, skeleton_len, pattern, size, &status);
  if (U_FAILURE(status)) {
    MAPLE_JS_ASSERT(false && "Error in udatpg_getBestPattern()");
  }
  char dst[64];
  u_austrcpy(dst, pattern);
  std::string res(dst);

  udatpg_close(gen);

  return StrToVal(dst);
}

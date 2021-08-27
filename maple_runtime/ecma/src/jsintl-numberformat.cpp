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

#include <unicode/numsys.h>
#include "jsvalueinline.h"
#include "jsvalue.h"
#include "jsobject.h"
#include "jsobjectinline.h"
#include "jsarray.h"
#include "jsmath.h"
#include "jsintl.h"

extern __jsvalue __js_ThisBinding;
extern std::vector<std::pair<std::string,uint16_t>> kNumberingSystems;

std::map<std::string,int> kCurrencyDigits = {
    {"BHD", 3},
    {"BIF", 0},
    {"BYR", 0},
    {"CLF", 0},
    {"CLP", 0},
    {"DJF", 0},
    {"IQD", 3},
    {"GNF", 0},
    {"ISK", 0},
    {"JOD", 3},
    {"JPY", 0},
    {"KMF", 0},
    {"KRW", 0},
    {"KWD", 3},
    {"LYD", 3},
    {"OMR", 3},
    {"PYG", 0},
    {"RWF", 0},
    {"TND", 3},
    {"UGX", 0},
    {"UYI", 0},
    {"VND", 0},
    {"VUV", 0},
    {"XAF", 0},
    {"XOF", 0},
    {"XPF", 0},
};

// ECMA-402 1.0 11.1.3.1 The Intl.NumberFormat Constructor
__jsvalue __js_NumberFormatConstructor(__jsvalue *this_arg, __jsvalue *arg_list,
                                       uint32_t nargs) {
  __jsobject *obj = __create_object();
  __jsobj_set_prototype(obj, JSBUILTIN_INTL_NUMBERFORMAT_PROTOTYPE);
  obj->object_class = JSINTL;
  obj->extensible = true;
  obj->object_type = JSREGULAR_OBJECT;
  __jsvalue obj_val = __object_value(obj);

  __jsvalue prop = StrToVal("InitializedIntlObject");
  __jsvalue value = __boolean_value(false);
  __jsop_setprop(&obj_val, &prop, &value);

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
  InitializeNumberFormat(&obj_val, &locales, &options);

  return obj_val;
}

// ECMA-402 1.0 11.1.1.1
// InitializeNumberFormat(numberFormat, locales, options)
void InitializeNumberFormat(__jsvalue *this_number_format, __jsvalue *locales,
                            __jsvalue *options) {
  // Step 1.
  __jsvalue prop = StrToVal("initializedIntlObject");
  if (!__is_undefined(this_number_format)) {
    __jsvalue init = __jsop_getprop(this_number_format, &prop);
    if (__is_boolean(&init) && __jsval_to_boolean(&init) == true) {
      MAPLE_JS_TYPEERROR_EXCEPTION();
    }
  }
  // Step 2.
  __jsvalue value = __boolean_value(true);
  __jsop_setprop(this_number_format, &prop, &value);
  // Step 3.
  __jsvalue requested_locales = CanonicalizeLocaleList(locales);
  // Step 4.
  __jsobject *options_obj;
  if (__is_undefined(options)) {
    options_obj = __js_new_obj_obj_0();
    *options = __object_value(options_obj);
  }
  // Step 6.
  __jsobject *opt_obj = __js_new_obj_obj_0();
  __jsvalue opt = __object_value(opt_obj);
  // Step 7.
  prop = StrToVal("localeMatcher");
  __jsvalue type = StrToVal("string");
  __jsvalue values = StrVecToVal({"lookup", "best fit"});
  __jsvalue fallback = StrToVal("best fit");
  __jsvalue matcher = GetOption(options, &prop, &type, &values, &fallback);
  // Step 8.
  // Set opt.[[localeMatcher]] to 'matcher'.
  prop = StrToVal("localeMatcher");
  __jsop_setprop(&opt, &prop, &matcher);
  // Step 9.
  // Let 'NumberFormat' be the standard built-in object that is the initial value
  // of Intl.NumberFormat.
  __jsobject *number_format_obj = __create_object();
  number_format_obj->object_class = JSINTL;
  number_format_obj->extensible = true;
  number_format_obj->object_type = JSREGULAR_OBJECT;
  __jsvalue number_format = __object_value(number_format_obj);

  InitializeNumberFormatProperties(&number_format, &requested_locales,
      {"availableLocales", "relevantExtensionKeys", "localeData"});

  // Step 10.
  // Let 'localeData' be the value of the [[localeData]] internal property of
  // Intl.NumberFormat.
  prop = StrToVal("localeData");
  __jsvalue locale_data = __jsop_getprop(&number_format, &prop);
  // Step 11.
  // Let 'r' be the result of calling the 'ResolveLocale' abstract operation
  // (defined in 9.2.5) with the [[availableLocales]] internal property of 'NumberFormat',
  // 'requestedLocales', 'opt', the [[relevantExtensionKeys]] internal property of
  // 'NumberFormat', and 'localeData'.
  prop = StrToVal("availableLocales");
  __jsvalue available_locales = __jsop_getprop(&number_format, &prop);
  prop = StrToVal("relevantExtensionKeys");
  __jsvalue relevant_extension_keys = __jsop_getprop(&number_format, &prop);
  __jsvalue r = ResolveLocale(&available_locales, &requested_locales,
                              &opt, &relevant_extension_keys, &locale_data);
  // NOTE: r.[[locale]] is set by ResolveLocale() call.
  // Step 12.
  // Set the [[locale]] internal property of 'numberFormat' to the value of
  // r.[[locale]].
  prop = StrToVal("locale");
  value = __jsop_getprop(&r, &prop);
  __jsop_setprop(this_number_format, &prop, &value);
  // Step 13.
  // Set the [[numberingSystem]] internal property of 'numberFormat' to the value 
  // of r.[[nu]].
  prop = StrToVal("nu");
  value = __jsop_getprop(&r, &prop);
  prop = StrToVal("numberingSystem");
  __jsop_setprop(this_number_format, &prop, &value);
  // Step 14.
  // Let 'dataLocale' be the value of r.[[dataLocale]].
  prop = StrToVal("dataLocale");
  __jsvalue data_locale = __jsop_getprop(&r, &prop);
  // Step 15.
  prop = StrToVal("style");
  type = StrToVal("string");
  values = StrVecToVal({"decimal", "percent", "currency"});
  fallback = StrToVal("decimal");
  __jsvalue s = GetOption(options, &prop, &type, &values, &fallback);
  // Step 16.
  prop = StrToVal("style");
  __jsop_setprop(this_number_format, &prop, &s);
  // Step 17.
  prop = StrToVal("currency");
  type = StrToVal("string");
  values = __undefined_value();
  fallback = __undefined_value();
  __jsvalue c = GetOption(options, &prop, &type, &values, &fallback);
  // Step 18.
  if (!__is_undefined(&c) && !IsWellFormedCurrencyCode(&c)) {
    MAPLE_JS_RANGEERROR_EXCEPTION();
  }
  // Step 19.
  __jsstring *s_jsstr = __jsval_to_string(&s);
  std::string currency_str = "currency";
  __jsstring *currency = __jsstr_new_from_char(currency_str.c_str());
  if (__jsstr_equal(s_jsstr, currency) && __is_undefined(&c)) {
    MAPLE_JS_TYPEERROR_EXCEPTION();
  }
  // Step 20.
  __jsvalue c_digits;
  if (__jsstr_equal(s_jsstr, currency)) {
    // Step 20a.
    c = __jsstr_toUpperCase(&c);
    // Step 20b.
    prop = StrToVal("currency");
    __jsop_setprop(this_number_format, &prop, &c);
    // Step 20c.
    c_digits = CurrencyDigits(&c);
  }
  // Step 21.
  prop = StrToVal("currencyDisplay");
  type = StrToVal("string");
  values = StrVecToVal({"code", "symbol", "name"});
  fallback = StrToVal("symbol");
  __jsvalue cd = GetOption(options, &prop, &type, &values, &fallback);
  // Step 22.
  if (__jsstr_equal(s_jsstr, currency)) {
    prop = StrToVal("currencyDisplay");
    __jsop_setprop(this_number_format, &prop, &cd);
  }
  // Step 23.
  prop = StrToVal("minimumIntegerDigits");
  __jsvalue minimum = __number_value(1);
  __jsvalue maximum = __number_value(21);
  fallback = __number_value(1);
  __jsvalue mnid = GetNumberOption(options, &prop, &minimum, &maximum, &fallback);
  // Step 24.
  prop = StrToVal("minimumIntegerDigits");
  __jsop_setprop(this_number_format, &prop, &mnid);
  // Step 25.
  __jsvalue mnfd_default;
  if (__jsstr_equal(s_jsstr, currency)) {
    mnfd_default = c_digits;
  } else {
    mnfd_default = __number_value(0);
  }
  // Step 26.
  prop = StrToVal("minimumFractionDigits");
  minimum = __number_value(0);
  maximum = __number_value(20);
  fallback = mnfd_default;
  __jsvalue mnfd = GetNumberOption(options, &prop, &minimum, &maximum, &fallback);
  // Step 27.
  prop = StrToVal("minimumFractionDigits");
  __jsop_setprop(this_number_format, &prop, &mnfd);
  // Step 28.
  __jsvalue mxfd_default;
  std::string percent_str = "percent";
  __jsstring *percent = __jsstr_new_from_char(percent_str.c_str());
  if (__jsstr_equal(s_jsstr, currency)) {
    mxfd_default = __jsmath_pt_max(&mnfd, &c_digits, 1);
  } else if (__jsstr_equal(s_jsstr, percent)) {
    __jsvalue zero = __number_value(0);
    mxfd_default = __jsmath_pt_max(&mnfd, &zero, 1);
  } else {
    __jsvalue three = __number_value(3);
    mxfd_default = __jsmath_pt_max(&mnfd, &three, 1);
  }
  // Step 29.
  prop = StrToVal("maximumFractionDigits");
  maximum = mnfd;
  minimum = __number_value(20);
  fallback = mxfd_default;
  __jsvalue mxfd = GetNumberOption(options, &prop, &minimum, &maximum, &fallback);
  // Step 30.
  prop = StrToVal("maximumFractionDigits");
  __jsop_setprop(this_number_format, &prop, &mxfd);
  // Step 31.
  prop = StrToVal("minimumSignificantDigits");
  __jsvalue mnsd = __jsop_getprop(options, &prop);
  // Step 32.
  prop = StrToVal("maximumSignificantDigits");
  __jsvalue mxsd = __jsop_getprop(options, &prop);
  // Step 33.
  if (!__is_undefined(&mnsd) || !__is_undefined(&mxsd)) {
    // Step 33a.
    prop = StrToVal("minimumSignificantDigits");
    minimum = __number_value(1);
    maximum = __number_value(21);
    fallback = __number_value(1);
    mnsd = GetNumberOption(options, &prop, &minimum, &maximum, &fallback);
    // Step 33b.
    prop = StrToVal("maximumSignificantDigits");
    minimum = mnsd;
    maximum = __number_value(21);
    fallback = __number_value(21);
    mxsd = GetNumberOption(options, &prop, &minimum, &maximum, &fallback);
    // Step 33c.
    prop = StrToVal("minimumSignificantDigits");
    __jsop_setprop(this_number_format, &prop, &mnsd);
    prop = StrToVal("maximumSignificantDigits");
    __jsop_setprop(this_number_format, &prop, &mxsd);
  }
  // Step 34.
  prop = StrToVal("useGrouping");
  type = StrToVal("boolean");
  values = __undefined_value();
  fallback = __boolean_value(true);
  __jsvalue g = GetOption(options, &prop, &type, &values, &fallback);
  // Step 35.
  prop = StrToVal("useGrouping");
  __jsop_setprop(this_number_format, &prop, &g);
  // Step 36.
  // Let 'dataLocaleData' be the result of calling the [[Get]] internal method
  // of 'localeData' with argument 'dataLocale'.
  __jsvalue data_locale_data = __undefined_value();
  if (!__is_undefined(&locale_data)) {
    data_locale_data = __jsop_getprop(&locale_data, &data_locale);
  }
  // Step 37.
  prop = StrToVal("patterns");
  __jsvalue patterns = __undefined_value();
  if (!__is_undefined(&data_locale_data)) {
    patterns = __jsop_getprop(&data_locale_data, &prop);
  }
#if 0
  // Step 38.
  MAPLE_JS_ASSERT(__is_js_object(&patterns)); // TODO: not checked yet.
  // initial Intl.NumberFormat, locale_data, patterns not set.
  // Step 39.
  prop = s;
  __jsvalue style_patterns;
  if (!__is_undefined(&patterns)) {
    style_patterns = __jsop_getprop(&patterns, &prop);
  } else {
    style_patterns = __undefined_value();
  }
  // Step 40.
  prop = StrToVal("positivePattern");
  if (!__is_undefined(&style_patterns)) {
    value = __jsop_getprop(&style_patterns, &prop);
  } else {
    value = __undefined_value();
  }
  __jsop_setprop(this_number_format, &prop, &value);
  // Step 41.
  prop = StrToVal("negativePattern");
  if (!__is_undefined(&style_patterns)) {
    value = __jsop_getprop(&style_patterns, &prop);
  } else {
    value = __undefined_value();
  }
  __jsop_setprop(this_number_format, &prop, &value);
#endif
  // Step 42.
  prop = StrToVal("boundFormat");
  value = __undefined_value();
  __jsop_setprop(this_number_format, &prop, &value);
  // Step 43.
  prop = StrToVal("initializedNumberFormat");
  value = __boolean_value(true);
  __jsop_setprop(this_number_format, &prop, &value);
}

// ECMA-402 1.0 11.1.1.1
// If the ISO 4217 currency and funds code list contains 'currency' as an
// alphabetic code, then return the minor unit value corresponding to the
// 'currency' from the list; else return 2.
__jsvalue CurrencyDigits(__jsvalue *currency) {
  __jsstring *currency_str = __jsval_to_string(currency);
  for (auto it = kCurrencyDigits.begin(); it != kCurrencyDigits.end(); it++) {
    std::string str = it->first;
    __jsstring *cur = __jsstr_new_from_char(str.c_str());
    if (__jsstr_equal(currency_str, cur)) {
      return __number_value(it->second);
    }
  }
  return __number_value(2);
}

// ECMA-402 1.0 11.2.2
// Intl.NumberFormat.supportedLocalesOf(locales [, options])
__jsvalue __jsintl_NumberFormatSupportedLocalesOf(__jsvalue *number_format,
                                                  __jsvalue *locales,
                                                  uint32_t nargs) {
  // Step 1.
  __jsvalue options;
  if (nargs == 1) {
    options = __undefined_value();
  } else {
    options = locales[1];
  }
  // Step 2.
  __jsvalue available_locales = GetAvailableLocales();
  // Step 3.
  __jsvalue requested_locales = CanonicalizeLocaleList(locales);
  // Step 4.
  return SupportedLocales(&available_locales, &requested_locales, &options);
}

// ECMA-402 11.3.2
// FormatNumber(numberFormat, x)
// returns a String value representing x according to the effective locale and the formatting options
// of numberFormat.
__jsvalue FormatNumber(__jsvalue *number_format, __jsvalue *x_val) {
  // Step 1.
  __jsvalue negative = __boolean_value(false);
  __jsvalue n = __undefined_value();
  // Step 2.
  // If the result of isFinite(x) is false, then
  if (__is_infinity(x_val) || __is_neg_infinity(x_val)) {
    // Step 2a.
    // If x is NaN, then let 'n' be an ILD String value indicating the NaN value.
    if (__is_nan(x_val)) {
      std::string s = "NaN";
      n = __string_value(__jsstr_new_from_char(s.c_str()));
    } else { // Step 2b.
      // Step 2b i.
      // Let 'n' be an ILD String value indicating infinity.
      __jsstring *s = __js_new_string_internal(1, true);
      __jsstr_set_char(s, 0, 0x221E);
      n = __string_value(s);
      // Step 2b ii.
      // If x < 0, then let 'negative' be true.
      if (__jsval_to_double(x_val) < 0 || __is_neg_infinity(x_val)) { // handling '-Infinity' as well.
        negative = __boolean_value(true);
      }
    }
  } else { // Step 3.
    // Step 3a.
    // If x < 0, then
    double x = __jsval_to_double(x_val);
    if (x < 0 || __is_negative_zero(x_val)) {  // handling '-0' as well.
      // Step 3a i.
      // Let 'negative' is true,
      negative = __boolean_value(true);
      // Step 3a ii.
      x = -x;
      *x_val = __double_value(x);
    }
    // Step 3b.
    __jsvalue prop = StrToVal("style");
    __jsvalue value = __jsop_getprop(number_format, &prop);
    std::string value_str = ValToStr(&value);
    if (value_str == "percent") {
      x = 100 * x;
      *x_val = __double_value(x);
    }
    // Step 3c.
    // If the [[minimumSignificantDigits]] and [[maximumSignificantDigits]]
    // internal properties of 'numberFormat' are present
    prop = StrToVal("minimumSignificantDigits");
    __jsvalue min_sd = __jsop_getprop(number_format, &prop);
    prop = StrToVal("maximumSignificantDigits");
    __jsvalue max_sd = __jsop_getprop(number_format, &prop);
    if (!__is_undefined(&min_sd) && !__is_undefined(&max_sd)) {
      // Step 3c i.
      n = ToRawPrecision(x_val, &min_sd, &max_sd);
    } else {  // Step 3d.
      // Step 3d i.
      prop = StrToVal("minimumIntegerDigits");
      __jsvalue mnid = __jsop_getprop(number_format, &prop);
      prop = StrToVal("minimumFractionDigits");
      __jsvalue mnfd = __jsop_getprop(number_format, &prop);
      prop = StrToVal("maximumFractionDigits");
      __jsvalue mxfd = __jsop_getprop(number_format, &prop);
      n = ToRawFixed(x_val, &mnid, &mnfd, &mxfd);
    }
    // Step 3e.
    prop = StrToVal("numberingSystem");
    value = __jsop_getprop(number_format, &prop);
    std::string nf = ValToStr(&value);
    bool found = false;
    int nf_index;
    for (nf_index = 0; nf_index < kNumberingSystems.size(); nf_index++) {
      if (kNumberingSystems[nf_index].first == nf) {
        found = true;
        break;
      }
    }
    if (found) {
      // Step 3e i.
      uint16_t d[10];
      uint16_t start;
      if (kNumberingSystems[nf_index].first != "hanidec") {
        start = kNumberingSystems[nf_index].second;
        for (int i = 0; i < 10; i++) {
          d[i] = start + i;
        }
      } else { // Case for 'hanidec'.
        // From Table 2 - Numbering Systems.
        uint16_t d[] = {0x3007, 0x4E00, 0x4E8C, 0x4E09, 0x56DB,
                        0x4E94, 0x516D, 0x4E03, 0x516B, 0x4E5D};
        start = d[0];
      }
      // Step 3e ii.
      // Replace each 'digit' in 'n' with the value of digits[digit].
      std::string n_str = ValToStr(&n);
      __jsstring *n_jsstr = __jsstr_new_from_char(n_str.c_str());
      uint32_t size = __jsstr_get_length(n_jsstr);
      for (int i = 0; i < size; i++) {
        uint16_t d = __jsstr_get_char(n_jsstr, i) - start;
        __jsstr_set_char(n_jsstr, i, start + d);
      }
      n = __string_value(n_jsstr);
    } else {
      // Step 3f-3h.
      MAPLE_JS_ASSERT(false && "NIY: FormatNumber()");
    }
  }
  // Step 4.
  // If 'negative' is 'true', then let 'result' be the value of the [[negativePattern]]
  // internal property of 'numberFormat'; else let 'result' be the value of
  // [[possitivePattern]] internal property of 'numberFormat'.
  // FIXME: Use ICU or follow pattern from spec?
  __jsvalue result = __undefined_value();
  // Check the first char of 'n' if it is '-'.
  __jsstring *n_str = __jsval_to_string(&n);
  if (__jsval_to_boolean(&negative) && __jsstr_get_char(n_str, 0) != '-') {
    //std::string n_str = ValToStr(&n);
    //__jsstring *n_jsstr = __jsstr_new_from_char(n_str.c_str());
    std::string neg_sign = "-";
    __jsstring *neg_sign_str = __jsstr_new_from_char(neg_sign.c_str());
    __jsstring *n_str = __jsval_to_string(&n);
    __jsstring *n_signed_str = __jsstr_concat_2(neg_sign_str, n_str);
    __jsvalue n_signed = __string_value(n_signed_str);
    result = n_signed;
  } else {
    result = n;
  }
  // Step 5.
  // Replace the substring "{number}" within 'result' with 'n'.
  __jsvalue search = StrToVal("{number}");
  __jsstr_replace(&result, &search, &n);
  // Step 6.
  // If the value of the [[style]] internal property of 'numberFormat' is "currency", then:
  __jsvalue p = StrToVal("style");
  __jsvalue v = __jsop_getprop(number_format, &p);
  if (ValToStr(&v) == "currency") {
    // Step 6a.
    // Let 'currency' be the value of the [[currency]] internal property of 'numberFormat'.
    p = StrToVal("currency");
    __jsvalue currency = __jsop_getprop(number_format, &p);
    // Step 6b.
    // If the value of the [[currencyDisplay]] internal property of 'numberFormat' is "code",
    // then let 'cd' be 'currency'.
    p = StrToVal("currencyDisplay");
    v = __jsop_getprop(number_format, &p);
    __jsvalue cd;
    if (ValToStr(&v) == "code") {
      p = StrToVal("cd");
      cd = currency;
    } else if (ValToStr(&v) == "symbol") {
      // Step 6c.
      // Else if the value of the [[currencyDisplay]] internal property of 'numberFormat' is
      // "symbol", then let 'cd' be an ILD string representing 'currency' in short form.
      // If the implementation does not have such a representation of 'currency', then use
      // 'currency' itself.
      cd = StrToVal("currency");

    } else if (ValToStr(&v) == "name") {
      // Step 6d.
      // Else if the value of the [[currencyDisplay]] internal property of 'numberFormat' is
      // "name", then let 'cd' be an ILD string representing 'currency' in long form.
      // If the implementation does not have such a representation of 'currency', then use
      // 'currency' itself.
      cd = StrToVal("currency");
    }
    // Step 6e.
    // Replace the substring "{currency}" within 'result' with 'cd'.
    search = StrToVal("{currency}");
    __jsstr_replace(&result, &search, &cd);

  }
  // Step 7.
  return result;
}

__jsvalue ToRawPrecision(__jsvalue *x_val, __jsvalue *min_precision, __jsvalue *max_precision) {
#if 0
  // Step 1.
  int p = __jsval_to_number(max_precision);
  // Step 2.
  std::string m;
  int e = 0;
  int x = __jsval_to_number(x_val);
  if (x == 0) {
    // Step 2a.
    m = std::string(p, '0');
    // Step 2b.
    e = 0;
  } else { // Step 3.

  }
#endif
  return *x_val;
}

// Used in 11.3.2 Intl.NumberFormat.prototype.format.
__jsvalue ToRawFixed(__jsvalue *x_val, __jsvalue *min_integer_val, __jsvalue *min_fraction_val,
                     __jsvalue *max_fraction_val) {
  // Step 1.
  int f = __jsval_to_number(max_fraction_val);
  // Step 2.
  // Let 'n' be an integer for which the exact mathematical value of n/10**f-x
  // is as close to zero as possible. If there are two such 'n', pick the larger n.
  double x = __jsval_to_double(x_val);
  int n = x * pow(10, f);
  // Step 3.
  std::string m;
  if (n == 0) {
    m = "0";
  } else {
    m = std::to_string(n);
  }
  // Step 4.
  int i;
  if (f != 0) {
    // Step 4a.
    int k = m.length();
    // Step 4b.
    if (k <= f) {
      // Step 4b i.
      std::string z = std::string(f + 1 - k, '0');
      // Step 4b ii.
      m = z + m;
      // Step 4b iii.
      k = f + 1;
    }
    // Step 4c.
    std::string a = m.substr(0, k - f);
    std::string b = m.substr(k - f);
    // Step 4d.
    m = a + "." + b;
    // Step 4e.
    i = a.length();
  } else { // Step 5.
    i = m.length();
  }
  // Step 6.
  int max_fraction = __jsval_to_number(max_fraction_val);
  int min_fraction = __jsval_to_number(min_fraction_val);
  int cut = max_fraction - min_fraction;
  // Step 7.
  while (cut > 0 && m.back() == '0') {
    // Step 7a.
    m.pop_back();
    // Step 7b.
    cut--;
  }
  // Step 8.
  if (m.back() == '.') {
    // Step 8a.
    m.pop_back();
  }
  // Step 9.
  int min_integer = __jsval_to_number(min_integer_val);
  if (i < min_integer) {
    // Step 9a.
    std::string z = std::string(min_integer, '0');
    m = z + m;
  }
  return __string_value(__jsstr_new_from_char(m.c_str()));
}

// ECMA-402 1.0 11.3.2
// Intl.NumberFormat.prototype.format
__jsvalue __jsintl_NumberFormatFormat(__jsvalue *number_format, __jsvalue *value) {
  // Step 1.
  __jsvalue prop = StrToVal("boundFormat");
  __jsvalue bound_format = __jsop_getprop(number_format, &prop);
  __jsvalue f = __undefined_value();
  if (__is_undefined(&bound_format)) {
    // Step 1a.
#define ATTRS(nargs, length) \
  ((uint32_t)(uint8_t)(nargs == UNCERTAIN_NARGS ? 1: 0) << 24 | (uint32_t)(uint8_t)nargs << 16 | \
   (uint32_t)(uint8_t)length << 8 | JSFUNCPROP_NATIVE)
    f = __js_new_function((void*)FormatNumber, NULL, ATTRS(2, 1));
    // Step 1b- 1c.
    // Let bf b the result of calling the [[Call]] internal method of 'bind' with F
    // as 'this' value and an argument list containing the single item 'this'.
    __jsvalue args[] = {*number_format, *value};
    int len = sizeof(args)/sizeof(__jsvalue);

    // Temporary workaround to avoid 'strict' constraint inside __jsfun_pt_bind() function.
    // Save old value of __js_ThisBinding.
    __jsvalue this_binding_old = __js_ThisBinding;
    __js_ThisBinding = f;
    __jsvalue bf = __jsfun_pt_bind(&f, args, len);
    // Restore old value of __js_ThisBinding.
    __js_ThisBinding = this_binding_old;

    // Step 1d.
    // Set the [[boundformat]] internal property of this NumberFormat object to 'bf'.
    __jsop_setprop(number_format, &prop, &bf);
  }
  // Step 2.
  // Return the value of the [[boundFormat]] internal property of this NumberFormat object.
  __jsvalue res = __jsop_getprop(number_format, &prop);
  return res;
}

// ECMA-402 1.0 11.3.3
// Intl.NumberFormat.prototype.resolvedOptions()
// returns a new object whose properties and attributes are set as if constructed
// by an object literal assigning to each of the following properties the value of
// corresponding internal property of this NumberFormat object: locale, numberingSystem,
// style, currency, currencyDisplay, minimumIntegerDigits, minimumFractionDigits,
// maximumFractionDigits, minimumSigficantDigits, maximumSignificantDigits, and
// useGrouping.
__jsvalue __jsintl_NumberFormatResolvedOptions(__jsvalue *number_format) {
  __jsobject *nf_obj = __create_object();
  __jsobj_set_prototype(nf_obj, JSBUILTIN_INTL_NUMBERFORMAT_PROTOTYPE);
  nf_obj->object_class = JSINTL;
  nf_obj->extensible = true;
  nf_obj->object_type = JSREGULAR_OBJECT;
  __jsvalue nf = __object_value(nf_obj);

  std::vector<std::string> props = {"locale", "numberingSystem", "style",
                                    "currency", "currencyDisplay",
                                    "minimumIntegerDigits",
                                    "minimumFractionDigits",
                                    "maximumFractionDigits",
                                    "minimumSignificantDigits",
                                    "maximumSignificantDigits",
                                    "useGrouping"};
  __jsvalue p, v;
  for (int i = 0; i < props.size(); i++) {
    p = StrToVal(props[i]);
    v = __jsop_getprop(number_format, &p);
    __jsop_setprop(&nf, &p, &v);
  }
  // NOTE: is this really needed?
  p = StrToVal("initializedNumberFormat");
  v = __boolean_value(true);
  __jsop_setprop(&nf, &p, &v);

  return nf;
}

void InitializeNumberFormatProperties(__jsvalue *number_format, __jsvalue *locales,
                                     std::vector<std::string> properties) {
  __jsvalue p, v;
  for (int i = 0; i < properties.size(); i++) {
    if (properties[i] == "availableLocales") {
      p = StrToVal(properties[i]);
      v = GetAvailableLocales();

      __jsop_setprop(number_format, &p, &v);

    } else if (properties[i] == "relevantExtensionKeys") {
      p = StrToVal(properties[i]);
      std::vector<std::string> values = {"nu"};
      v = StrVecToVal(values);

      __jsop_setprop(number_format, &p, &v);

    } else if (properties[i] == "localeData") {
      // [[localeData]][locale]['nu']
      __jsobject *locale_object = __create_object();
      __jsvalue locale = __object_value(locale_object);
      locale_object->object_class = JSOBJECT;
      locale_object->extensible = true;
      locale_object->object_type = JSREGULAR_OBJECT;

      // Add default system numbering system as the first element of vec.
      std::vector<std::string> vec = GetNumberingSystems();
      v = StrVecToVal(vec);
      p = StrToVal("nu");
      __jsop_setprop(&locale, &p, &v);  // Set "nu" to locale.

      __jsobject *locale_data_object = __create_object();
      __jsvalue locale_data = __object_value(locale_data_object);
      locale_data_object->object_class = JSOBJECT;
      locale_data_object->extensible = true;
      locale_data_object->object_type = JSREGULAR_OBJECT;

      __jsobject *locales_object = __jsval_to_object(locales);
      uint32_t size = __jsobj_helper_get_length(locales_object);
      for (int j = 0; j < size; j++) {
        p = __jsarr_GetElem(locales_object, j);
        // locale should be includled in available_locales.
        __jsvalue available_locales = GetAvailableLocales();
        p = BestAvailableLocale(&available_locales, &p);
        __jsop_setprop(&locale_data, &p, &locale); // Set locale to localeData.
      }
      if (size == 0) { // when locale is not specified, use default one.
        p = DefaultLocale();
        __jsop_setprop(&locale_data, &p, &locale);
      }

      p = StrToVal(properties[i]);
      __jsop_setprop(number_format, &p, &locale_data); // Set localeData to number_format object.
    }
  }
}

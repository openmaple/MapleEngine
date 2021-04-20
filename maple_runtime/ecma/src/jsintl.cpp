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

#include "jsintl.h"
#include "jsvalueinline.h"
#include "jsvalue.h"
#include "jsobject.h"
#include "jsobjectinline.h"
#include "jsarray.h"
#include "jsregexp.h"
#include "jsmath.h"

// ECMA-402 1.0 8 The Intl Object
__jsvalue __js_IntlConstructor(__jsvalue *this_arg, __jsvalue *arg_list,
                            uint32_t nargs) {
  __jsobject *obj = __create_object();
  obj->object_class = JSINTL;
  obj->extensible = true; 
  obj->object_type = JSREGULAR_OBJECT;

  return __object_value(obj);
}

// ECMA-402 1.0 10.1 The Intl.Collator Constructor
__jsvalue __js_CollatorConstructor(__jsvalue *this_arg, __jsvalue *arg_list,
                                   uint32_t nargs) {
  __jsobject *obj = __create_object();
  obj->object_class = JSINTL;
  obj->extensible = true;
  obj->object_type = JSREGULAR_OBJECT;

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
  InitializeCollator(obj, &locales, &options);

  return __object_value(obj);
}

// ECMA-402 1.0 11.1 The Intl.NumberFormat Constructor
__jsvalue __js_NumberFormatConstructor(__jsvalue *this_arg, __jsvalue *arg_list,
                                       uint32_t nargs) {
  __jsobject *obj = __create_object();
  obj->object_class = JSINTL;
  obj->extensible = true;
  obj->object_type = JSREGULAR_OBJECT;
  obj->shared.intl = (__jsintl*) VMMallocGC(sizeof(__jsintl));

  obj->shared.intl->initialized_intl_object = false;

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
  InitializeNumberFormat(obj, &locales, &options);

  return __object_value(obj);
}

// ECMA-402 1.0 12.1 The Intl.DateTimeFormat Constructor
__jsvalue __js_DateTimeFormatConstructor(__jsvalue *this_arg,
                                        __jsvalue *arg_list, uint32_t nargs) {
  __jsobject *obj = __create_object();
  obj->object_class = JSINTL;
  obj->extensible = true;
  obj->object_type = JSREGULAR_OBJECT;

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
  InitializeDateTimeFormat(obj, &locales, &options);

  return __object_value(obj);
}

// ECMA-402 1.0 6.2.3
// CanonicalizeLanguageTag(locale)
__jsvalue CanonicalizeLanguageTag(__jsstring *tag) {
  // TODO: not implemented yet.
  return __null_value();
}

// ECMA-402 1.0 6.2.2
// IsStructurallyValidLanguageTag(locale)
bool IsStructurallyValidLanguageTag(__jsstring *tag) {
  __jsvalue tag_val = __string_value(tag);

  std::string alpha = "[a-zA-Z]";
  std::string digit = "[0-9]";
  std::string alpha_num = "(" + alpha + "|" + digit + ")";
  std::string regular = "(art-lojban|cel-gaulish|no-bok|no-nyn|zh-guoyu|zh-hakka|zh-min|zh-min-nan|zh-xiang)";
  std::string irregular = "(en-GB-oed|i-ami|i-bnn|i-default|i-enochian|i-hak|i-klingon|i-lux|i-mingo|i-navajo|i-pwn|i-tao|i-tay|i-tsu|sgn-BE-FR|sgn-BE-NL|sgn-CH-DE)";
  std::string singleton = "(" + digit + "|[A-WY-Za-wy-z])";
  std::string grandfathered = "(" + irregular + "|" + regular + ")";
  std::string private_use = "(x(-[a-z0-9]{1,8})+)";
  std::string extension = "(" + singleton + "(-" + alpha_num + "{2,8})+)";
  std::string ext_lang = "(" + alpha + "{3}(-" + alpha + "{3}){0,2})";
  std::string language = "(" + alpha + "{2,3}(-" + ext_lang + ")?|" + alpha + "{4}|"
                         + alpha + "{5,8})";
  std::string variant = "(" + alpha_num + "{5,8}|(" + digit + alpha_num + "{3}))";
  std::string region = "(" + alpha + "{2}|" + digit + "{3})";
  std::string script = "(" + alpha + "{4})";
  std::string lang_tag = language + "(-" + script + ")?(-" + region + ")?(-"
                         + variant + ")*(-" + extension + ")*(-" + private_use
                         + ")?";
  // For language tag.
  std::string language_tag = "^(" + lang_tag + "|" + private_use + "|"
                             + grandfathered + ")$";
  __jsstring *language_tag_str = __jsstr_new_from_char(language_tag.c_str());
  __jsvalue language_tag_re = __string_value(language_tag_str);

  // For duplicate singleton.
  std::string duplicate_singleton = "-" + singleton + "-(.*-)?\\1(?!" + alpha_num
                                    + ")";
  __jsstring *duplicate_singleton_str = __jsstr_new_from_char(duplicate_singleton.c_str());
  __jsvalue duplicate_singleton_re = __string_value(duplicate_singleton_str);
  
  // For duplicate variant.
  std::string duplicate_variant = "(" + alpha_num + "{2,8}-)+" + variant + "-("
                                  + alpha_num + "{2,8}-)*\\3(?!" + alpha_num + ")";
  __jsstring *duplicate_variant_str = __jsstr_new_from_char(duplicate_variant.c_str());
  __jsvalue duplicate_variant_re = __string_value(duplicate_variant_str);

  __jsvalue res = __jsregexp_Test(&language_tag_re, &tag_val);
  if (__jsval_to_boolean(&res) == false)
    return false;

  std::string separator = "/-x-/";
  __jsstring *separator_str = __jsstr_new_from_char(separator.c_str());
  __jsvalue separator_val = __string_value(separator_str);
  __jsvalue undefined_val = __undefined_value();
  res = __jsstr_split(&tag_val, &separator_val, &undefined_val);
  __jsobject *res_obj = __jsval_to_object(&res);
  __jsvalue res0 = __jsarr_GetElem(res_obj, 0);
  
  __jsvalue duplicate_singleton_res = __jsregexp_Test(&duplicate_singleton_re, &res0);
  __jsvalue duplicate_variant_res = __jsregexp_Test(&duplicate_variant_re, &res0);

  return !__jsval_to_boolean(&duplicate_singleton_res) 
         && !__jsval_to_boolean(&duplicate_variant_res);
}

// ECMA-402 1.0 6.3.1
// IsWellFormedCurrencyCode(currency)
bool IsWellFormedCurrencyCode(__jsvalue *currency) {
  // Step 2.
  __jsvalue normalized = __jsstr_toUpperCase(currency);
  __jsstring *normalized_str = __jsval_to_string(&normalized);
  // Step 3.
  uint32_t len = __jsstr_get_length(normalized_str);
  if (len != 3) {
    return false;
  }
  if (__jsstr_is_ascii(normalized_str)) {
    for (int i = 0; i < len; i++) {
      uint16_t c = __jsstr_get_char(normalized_str, i);
      if (c < 'A' || c > 'Z') 
        return false;
    }
  } else {
    for (int i = 0; i < len; i++) {
      uint16_t c = __jsstr_get_char(normalized_str, i);
      if (c < 0x0041 || c > 0x005a)
        return false;
    }
  }
  return true;
}

// ECMA-402 1.0 9.2.1
// CanonicalizeLocaleList(locales)
__jsvalue CanonicalizeLocaleList(__jsvalue *locales) {
  // Step 1.
  if (__is_undefined(locales)) {
    __jsobject *arr = __js_new_arr_internal(0);
    return __object_value(arr);
  }
  // Step 2.
  __jsvalue seen = __object_value(__js_new_arr_internal(0));

  // Step 3.
  __jsvalue locales2 = *locales;
  if (__is_string(locales)) {
    __jsobject *arr_obj = __js_new_arr_elems(locales, 1);
    locales2 = __object_value(arr_obj);
  }

  // Step 4.
  __jsobject *o = __jsval_to_object(&locales2);

  // Step 5.
  __jsvalue len_value = __jsobj_helper_get_length_value(o);

  // Step 6.
  uint32_t len = __jsval_to_uint32(&len_value);

  // Step 7.
  uint32_t k = 0;

  // Step 8.
  while (k < len) {
    // Step 8a.
    __jsstring *p_k = __js_NumberToString(k);
    // Step 8b.
    __jsvalue k_value;
    bool k_present = __jsobj_helper_HasPropertyAndGet(o, p_k, &k_value);
    // Step 8c.
    if (k_present) {
      // Step 8c ii.
      if (!(__is_string(&k_value) || __is_js_object(&k_value))) {
        MAPLE_JS_TYPEERROR_EXCEPTION();
      }
      // Step 8c iii.
      __jsstring *tag = __jsval_to_string(&k_value);
      // Step 8c iv.
      if (!IsStructurallyValidLanguageTag(tag)) {
        MAPLE_JS_RANGEERROR_EXCEPTION();
      }
      // Step 8c v.
      __jsvalue tag2 = CanonicalizeLanguageTag(tag);
      // step 8c vi.
      __jsvalue idx = __jsarr_pt_indexOf(&seen, &tag2, 0);
      if (__js_ToNumber(&idx) == -1) {
        seen = __jsarr_pt_concat(&seen, &tag2, 1);
      }
      // Step 8d.
      k++;
    }
  }
  // Step 9.
  return seen;
}

// ECMA-402 1.0 9.2.2
// BestAvailableLocale(availableLocales, locale)
__jsvalue BestAvailableLocale(__jsvalue *available_locales, __jsvalue *locale) {
  // TODO: not implemented yet.
  return __undefined_value();
}

// ECMA-402 1.0 9.2.3
// LookupMatcher(availableLocales, requestedLocales)
__jsvalue LookupMatcher(__jsvalue *available_locales, __jsvalue *requested_locales) {
  // TODO: not implemented yet.
  return __undefined_value();
}

// ECMA-402 1.0 9.2.4
// BestFitMatcher(availableLocales, requestedLocales)
__jsvalue BestFitMatcher(__jsvalue *available_locales, __jsvalue *requested_locales) {
  // TODO: not impemented yet.
  return __null_value();
}

// ECMA-402 1.0 9.2.5
// ResolveLocale(availableLocales, requestedLocales, options, relevantExtensionKeys,
//               localeData)
__jsvalue ResolveLocale(__jsvalue *available_locales, __jsvalue *requested_locales,
                        std::map<std::string,__jsvalue>& options, __jsvalue *relevant_extension_keys,
                        __jsvalue *locale_data) {
  // TODO: not implemented yet.
  return __null_value();
}

// ECMA-402 1.0 9.2.6
// LookupSupportedLocales(availableLocales, requestedLocales)
__jsvalue LookupSupportedLocales(__jsvalue *available_locales,
                                __jsvalue *requested_locales) {
  // TODO: not implemented yet.
  return __null_value();
}

// ECMA-402 1.0 9.2.7
// BestFitSupportedLocales(availableLocales, requestedLocales)
__jsvalue BestFitSupportedLocales(__jsvalue *available_locales,
                                  __jsvalue *requested_locales) {
  // TODO: not implemented yet.
  return __null_value();
}

// ECMA-402 1.0 9.2.8
// SupportedLocales(availableLocales, requestedLocales, options)
__jsvalue SupportedLocales(__jsvalue *available_locales,
                           __jsvalue *requested_locales, __jsvalue *options) {
  // TODO: not implemented yet.
  return __null_value();
}

// ECMA-402 1.0 9.2.9
// GetOption(options, property, type, values, fallback)
__jsvalue GetOption(__jsvalue *options, std::string property, std::string type,
                    __jsvalue *values, __jsvalue *fallback) {
  // Step 1.
  __jsobject *obj = __jsval_to_object(options);
  __jsvalue value = obj->shared.intl->map[property];
  // Step 2.
  if (!__is_undefined(&value)) {
    // Step 2a.
    MAPLE_JS_ASSERT(type == "boolean" || type == "string");
    // Step 2b/2c.
    /*
    if (type == "boolean") {
      value = __jsval_to_boolean(&value);
    } else if (type == "string") {
      value = __jsval_to_string(&value);
    }
    */
    // Step 2d.
    if (!__is_undefined(values)) {
      __jsvalue idx = __jsarr_pt_indexOf(values, &value, 0);
      if (__jsval_to_number(&idx) != -1) {
        MAPLE_JS_RANGEERROR_EXCEPTION();
      }
      return value;
    }
  }
  // Step 3.
  return *fallback;
}

// ECMA-402 1.0 9.2.10
// GetNumberOption(options, property, minimum, maximum, fallback)
__jsvalue GetNumberOption(__jsvalue *options, __jsvalue *property,
                          __jsvalue *minimum, __jsvalue *maximum,
                          __jsvalue *fallback) {
  // TODO: not implemented yet.
  return __null_value();
}

// ECMA-402 1.0 10.1.1.1
// InitializeCollator(collator, locales, options)
void InitializeCollator(__jsobject *obj, __jsvalue *locales,
                        __jsvalue *options) {
  // TODO: not implemented yet.
}

// ECMA-402 1.0 10.2.2
// Intl.Collator.supportedLocalesOf(locales [, options])
__jsvalue __jsintl_CollatorSupportedLocalesOf(__jsvalue *this_arg,
                                              __jsvalue *arg_list,
                                              uint32_t nargs) {
  // TODO: not implemented yet.
  return __null_value();
}

// ECMA-402 1.0 10.3.2
// Intl.Collator.prototype.compare
__jsvalue __jsintl_CollatorCompare(__jsvalue *this_arg, __jsvalue *arg_list,
                                   uint32_t nargs) {
  // TODO: not implemented yet.
  return __null_value();
}

// ECMA-402 1.0 10.3.3
// Intl.Collator.prototype.resolvedOptions()
__jsvalue __jsintl_CollatorResolvedOptions(__jsvalue *this_arg,
                                           __jsvalue *arg_list, uint32_t nargs) {
  // TODO: not implemented yet.
  return __null_value();
}

// ECMA-402 1.0 11.1.1.1
// InitializeNumberFormat(numberFormat, locales, options)
void InitializeNumberFormat(__jsobject *obj, __jsvalue *locales,
                            __jsvalue *options) {
  // Step 1.
  if (obj->shared.intl->initialized_intl_object == true)
    MAPLE_JS_TYPEERROR_EXCEPTION();
  // Step 2.
  obj->shared.intl->initialized_intl_object = true;
  // Step 3.
  //__jsvalue requested_locales = CanonicalizeLocaleList(locales);
}

// ES-402 1.0 11.1.1.1.1
__jsvalue CurrencyDigits(__jsvalue *currency) {
  // TODO: not implemented yet.
  return __null_value();
}

// ECMA-402 1.0 11.2.2
// Intl.NumberFormat.supportedLocalesOf(locales [, options])
__jsvalue __jsintl_NumberFormatSupportedLocalesOf(__jsvalue *this_arg,
                                                  __jsvalue *arg_list,
                                                  uint32_t nargs) {
  // TODO: not implemented yet.
  return __null_value();
}

// ECMA-402 1.0 11.3.2
// Intl.NumberFormat.prototype.format
__jsvalue __jsintl_NumberFormatFormat(__jsvalue *this_arg, __jsvalue *arg_list,
                                      uint32_t nargs) {
  // TODO: not implemented yet.
  return __null_value();
}

// ECMA-402 1.0 11.3.3
// Intl.NumberFormat.prototype.resolvedOptions()
__jsvalue __jsintl_NumberFormatResolvedOptions(__jsvalue *this_arg,
                                               __jsvalue *arg_list,
                                               uint32_t nargs) {
  // TODO: not implemented yet.
  return __null_value();
}

// ECMA-402 1.0 12.1.1.1
// InitializeDateTimeFormat(dateTimeFormat, locales, options)
void InitializeDateTimeFormat(__jsobject *obj, __jsvalue *locales,
                              __jsvalue *options) {
  // TODO: not implemented yet.
}

// ECMA-402 1.0 12.2.2
// Intl.DateTimeFormat.supportedLocalesOf(locales [, options])
__jsvalue __jsintl_DateTimeFormatSupportedLocalesOf(__jsvalue *this_arg,
                                                    __jsvalue *arg_list,
                                                    uint32_t nargs) {
  // TODO: not implemented yet.
  return __null_value();
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

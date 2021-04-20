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

#ifndef JSINTL_H
#define JSINTL_H

#include <unordered_map>
#include "jsvalue.h"
#include "jsstring.h"

enum __jsintl_type : uint8_t {
  JSINTL_COLLATOR = 0x01,
  JSINTL_NUMBERFORMAT = 0x02,
  JSINTL_DATETIMEFORMAT = 0x04,
};

typedef struct {
  __jsintl_type kind  : 8;
  bool initialized_intl_object;
  bool initialized_number_format;
  std::unordered_map<std::string, __jsvalue> map;
} __jsintl;

__jsvalue __js_IntlConstructor(__jsvalue *this_arg, __jsvalue *arg_list,
                               uint32_t nargs);
__jsvalue __js_CollatorConstructor(__jsvalue *this_arg, __jsvalue *arg_list,
                                   uint32_t nargs);
__jsvalue __js_NumberFormatConstructor(__jsvalue *this_arg, __jsvalue *arg_list,
                                       uint32_t nargs);
__jsvalue __js_DateTimeFormatConstructor(__jsvalue *this_arg,
                                         __jsvalue *arg_list, uint32_t nargs);

void InitializeCollator(__jsobject *obj, __jsvalue *locales, __jsvalue *options);
void InitializeNumberFormat(__jsobject *obj, __jsvalue *locales,
                            __jsvalue *options);
__jsvalue CurrencyDigits(__jsvalue *currency);
void InitializeDateTimeFormat(__jsobject *obj, __jsvalue *locales,
                              __jsvalue *options);
__jsvalue CanonicalizeLanguageTag(__jsstring *tag);
bool IsStructurallyValidLanguageTag(__jsstring *tag);
bool IsWellFormedCurrencyCode(__jsvalue *currency);
__jsvalue BestAvailableLocale(__jsvalue *available_locales, __jsvalue *locale);
__jsvalue LookupMatcher(__jsvalue *available_locales, __jsvalue *requested_locales);
__jsvalue BestFitMatcher(__jsvalue *available_locales, __jsvalue *requested_locales);
__jsvalue ResolveLocale(__jsvalue *available_locales, __jsvalue *requested_locales,
                        std::unordered_map<std::string,__jsvalue>& options, __jsvalue *relevant_extension_keys,
                        __jsvalue *locale_data);
__jsvalue LookupSupportedLocales(__jsvalue *available_locales,
                                __jsvalue *requested_locales);
__jsvalue BestFitSupportedLocales(__jsvalue *available_locales,
                                  __jsvalue *requested_locales);
__jsvalue SupportedLocales(__jsvalue *available_locales,
                           __jsvalue *requested_locales, __jsvalue *options);
__jsvalue GetOption(__jsvalue *options, std::string property, std::string type,
                    __jsvalue *values, __jsvalue *fallback); 
__jsvalue GetNumberOption(__jsvalue *options, __jsvalue *property,
                          __jsvalue *minimum, __jsvalue *maximum,
                                                    __jsvalue *fallback);

__jsvalue __jsintl_CollatorSupportedLocalesOf(__jsvalue *this_arg,
                                              __jsvalue *arg_list,
                                              uint32_t nargs);
__jsvalue __jsintl_CollatorCompare(__jsvalue *this_arg, __jsvalue *arg_list,
                                   uint32_t nargs);
__jsvalue __jsintl_CollatorResolvedOptions(__jsvalue *this_arg,
                                           __jsvalue *arg_list, uint32_t nargs);
__jsvalue __jsintl_NumberFormatSupportedLocalesOf(__jsvalue *this_arg,
                                                  __jsvalue *arg_list,
                                                  uint32_t nargs);
__jsvalue __jsintl_NumberFormatFormat(__jsvalue *this_arg, __jsvalue *arg_list,
                                      uint32_t nargs);
__jsvalue __jsintl_NumberFormatResolvedOptions(__jsvalue *this_arg,
                                               __jsvalue *arg_list,
                                               uint32_t nargs);
__jsvalue __jsintl_DateTimeFormatSupportedLocalesOf(__jsvalue *this_arg,
                                                    __jsvalue *arg_list,
                                                    uint32_t nargs);
__jsvalue __jsintl_DateTimeFormatFormat(__jsvalue *this_arg,
                                        __jsvalue *arg_list, uint32_t nargs);
__jsvalue __jsintl_DateTimeFormatResolvedOptions(__jsvalue *this_arg,
                                                 __jsvalue *arg_list,
                                                 uint32_t nargs);

#endif // JSINTL_H

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

#include <vector>
#include <string>
#include <unordered_map>
#include "jsvalue.h"
#include "jsstring.h"

// Helper
__jsvalue StrToVal(std::string str);
__jsvalue StrVecToVal(std::vector<std::string> strs);
std::string ValToStr(__jsvalue *value);
std::vector<std::string> VecValToVecStr(__jsvalue *value);

// Constructor
__jsvalue __js_IntlConstructor(__jsvalue *this_arg, __jsvalue *arg_list,
                               uint32_t nargs);
__jsvalue __js_CollatorConstructor(__jsvalue *this_arg, __jsvalue *arg_list,
                                   uint32_t nargs);
__jsvalue __js_NumberFormatConstructor(__jsvalue *this_arg, __jsvalue *arg_list,
                                       uint32_t nargs);
__jsvalue __js_DateTimeFormatConstructor(__jsvalue *this_arg,
                                         __jsvalue *arg_list, uint32_t nargs);

// Common
__jsvalue CanonicalizeLanguageTag(__jsstring *tag);
__jsvalue CanonicalizeLocaleList(__jsvalue *locales);
bool IsStructurallyValidLanguageTag(__jsstring *locale);
bool IsWellFormedCurrencyCode(__jsvalue *currency);
__jsvalue BestAvailableLocale(__jsvalue *available_locales, __jsvalue *locale);
__jsvalue LookupMatcher(__jsvalue *available_locales, __jsvalue *requested_locales);
__jsvalue BestFitMatcher(__jsvalue *available_locales, __jsvalue *requested_locales);
__jsvalue ResolveLocale(__jsvalue *available_locales, __jsvalue *requested_locales,
                        __jsvalue *options, __jsvalue *relevant_extension_keys,
                        __jsvalue *locale_data);
__jsvalue LookupSupportedLocales(__jsvalue *available_locales,
                                __jsvalue *requested_locales);
__jsvalue BestFitSupportedLocales(__jsvalue *available_locales,
                                  __jsvalue *requested_locales);
__jsvalue SupportedLocales(__jsvalue *available_locales,
                           __jsvalue *requested_locales, __jsvalue *options);
__jsvalue GetOption(__jsvalue *options, __jsvalue *property, __jsvalue *type,
                    __jsvalue *values, __jsvalue *fallback);
__jsvalue GetNumberOption(__jsvalue *options, __jsvalue *property,
                          __jsvalue *minimum, __jsvalue *maximum,
                          __jsvalue *fallback);
__jsvalue DefaultLocale();
__jsvalue GetAvailableLocales();
std::vector<std::string> GetNumberingSystems();


// NumberFormat
void InitializeNumberFormat(__jsvalue *number_format, __jsvalue *locales,
                            __jsvalue *options);
void InitializeNumberFormatProperties(__jsvalue *number_format, __jsvalue *locales,
                                     std::vector<std::string> properties);
__jsvalue CurrencyDigits(__jsvalue *currency);
__jsvalue __jsintl_NumberFormatSupportedLocalesOf(__jsvalue *this_arg,
                                                  __jsvalue *arg_list,
                                                  uint32_t nargs);
__jsvalue __jsintl_NumberFormatFormat(__jsvalue *number_format, __jsvalue *value);
__jsvalue __jsintl_NumberFormatResolvedOptions(__jsvalue *number_format);
__jsvalue ToDateTimeOptions(__jsvalue *options, __jsvalue *required, __jsvalue *defaults);
__jsvalue BasicFormatMatcher(__jsvalue *options, __jsvalue *formats);
__jsvalue BestFitFormatMatcher(__jsvalue *options, __jsvalue *formats);
__jsvalue FormatNumber(__jsvalue *number_format, __jsvalue *x);
__jsvalue ToRawPrecision(__jsvalue *x, __jsvalue *min_sd, __jsvalue *max_sd);
__jsvalue ToRawFixed(__jsvalue *x, __jsvalue *min_integer,
                     __jsvalue *min_fraction,__jsvalue *max_fraction);

// Collator
void InitializeCollator(__jsvalue *collator, __jsvalue *locales, __jsvalue *options);
void InitializeCollatorProperties(__jsvalue *collator, __jsvalue *locales, std::vector<std::string> propertie);
__jsvalue __jsintl_CollatorSupportedLocalesOf(__jsvalue *this_arg,
                                              __jsvalue *arg_list,
                                              uint32_t nargs);
__jsvalue __jsintl_CollatorCompare(__jsvalue *this_collator, __jsvalue *x, __jsvalue *y);
__jsvalue CompareStrings(__jsvalue *collator, __jsvalue *x, __jsvalue *y);
__jsvalue __jsintl_CollatorResolvedOptions(__jsvalue *collator);
std::vector<std::string> GetCollations();

// DateTimeFormat
void InitializeDateTimeFormat(__jsvalue *date_time_format, __jsvalue *locales,
                              __jsvalue *options);
void InitializeDateTimeFormatProperties(__jsvalue *date_time_format, __jsvalue *locales, __jsvalue *options);
__jsvalue __jsintl_DateTimeFormatSupportedLocalesOf(__jsvalue *date_time_format,
                                                    __jsvalue *arg_list,
                                                    uint32_t nargs);
__jsvalue __jsintl_DateTimeFormatFormat(__jsvalue *date_time_format, __jsvalue *date);
__jsvalue FormatDateTime(__jsvalue *date_time_format, __jsvalue *x);
__jsvalue ToLocalTime(__jsvalue *x, __jsvalue *calendar, __jsvalue *time_zone);
__jsvalue __jsintl_DateTimeFormatResolvedOptions(__jsvalue *date_time_format);
std::vector<std::string> GetAvailableCalendars();
std::string ToBCP47CalendarName(const char* name);
std::string GetDateTimeStringSkeleton(__jsvalue *locale, __jsvalue *options);
__jsvalue GetICUPattern(std::string&, std::string& pattern);
const char* ToICULocale(std::string& locale);

#endif // JSINTL_H

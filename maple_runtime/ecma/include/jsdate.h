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

#ifndef JSDATE_H
#define JSDATE_H
#include "jsvalue.h"

// ES5 15.9.1.2
const int64_t kMsPerDay = 86400000;

// ES5 15.9.1.10
const int64_t kHoursPerDay = 24;
const int64_t kMinutesPerHour = 60;
const int64_t kSecondsPerMinute = 60;
const int64_t kMsPerSecond = 1000;
const int64_t kMsPerMinute = kMsPerSecond * kSecondsPerMinute;
const int64_t kMsPerHour = kMsPerMinute * kMinutesPerHour;

__jsvalue __js_ToDate(__jsvalue *this_object, __jsvalue *arg_list, uint32_t nargs);
__jsvalue __js_new_date_obj(__jsvalue *this_object, __jsvalue *arg_list, uint32_t nargs);

// external
__jsvalue __jsdate_GetDate(__jsvalue *this_date);
__jsvalue __jsdate_GetDay(__jsvalue *this_date);
__jsvalue __jsdate_GetFullYear(__jsvalue *this_date);
__jsvalue __jsdate_GetHours(__jsvalue *this_date);
__jsvalue __jsdate_GetMilliseconds(__jsvalue *this_date);
__jsvalue __jsdate_GetMinutes(__jsvalue *this_date);
__jsvalue __jsdate_GetMonth(__jsvalue *this_date);
__jsvalue __jsdate_GetSeconds(__jsvalue *this_date);
__jsvalue __jsdate_GetTime(__jsvalue *this_date);
__jsvalue __jsdate_GetTimezoneOffset(__jsvalue *this_date);
__jsvalue __jsdate_GetUTCDate(__jsvalue *this_date);
__jsvalue __jsdate_GetUTCDay(__jsvalue *this_date);
__jsvalue __jsdate_GetUTCFullYear(__jsvalue *this_date);
__jsvalue __jsdate_GetUTCHours(__jsvalue *this_date);
__jsvalue __jsdate_GetUTCMilliseconds(__jsvalue *this_date);
__jsvalue __jsdate_GetUTCMinutes(__jsvalue *this_date);
__jsvalue __jsdate_GetUTCMonth(__jsvalue *this_date);
__jsvalue __jsdate_GetUTCSeconds(__jsvalue *this_date);
__jsvalue __jsdate_SetDate(__jsvalue *this_date, __jsvalue *value);
__jsvalue __jsdate_SetFullYear(__jsvalue *this_date, __jsvalue *args, uint32_t nargs);
__jsvalue __jsdate_SetHours(__jsvalue *this_date, __jsvalue *args, uint32_t nargs);
__jsvalue __jsdate_SetMilliseconds(__jsvalue *this_date, __jsvalue *value);
__jsvalue __jsdate_SetMinutes(__jsvalue *this_date, __jsvalue *args, uint32_t nargs);
__jsvalue __jsdate_SetMonth(__jsvalue *this_date, __jsvalue *args, uint32_t nargs);
__jsvalue __jsdate_SetSeconds(__jsvalue *this_date, __jsvalue *args, uint32_t nargs);
__jsvalue __jsdate_SetTime(__jsvalue *this_date, __jsvalue *value);
__jsvalue __jsdate_SetUTCDate(__jsvalue *this_date, __jsvalue *value);
__jsvalue __jsdate_SetUTCFullYear(__jsvalue *this_date, __jsvalue *args, uint32_t nargs);
__jsvalue __jsdate_SetUTCHours(__jsvalue *this_date, __jsvalue *args, uint32_t nargs);
__jsvalue __jsdate_SetUTCMilliseconds(__jsvalue *this_date, __jsvalue *value);
__jsvalue __jsdate_SetUTCMinutes(__jsvalue *this_date, __jsvalue *args, uint32_t nargs);
__jsvalue __jsdate_SetUTCMonth(__jsvalue *this_date, __jsvalue *args, uint32_t nargs);
__jsvalue __jsdate_SetUTCSeconds(__jsvalue *this_date, __jsvalue *args, uint32_t nargs);
__jsvalue __jsdate_ToDateString(__jsvalue *this_date);
__jsvalue __jsdate_ToLocaleDateString(__jsvalue *this_date);
__jsvalue __jsdate_ToLocaleString(__jsvalue *this_date);
__jsvalue __jsdate_ToLocaleTimeString(__jsvalue *this_date);
__jsvalue __jsdate_ToString(__jsvalue *this_date);
__jsvalue __jsdate_ToString_Obj(__jsobject *obj);
__jsvalue __jsdate_ToTimeString(__jsvalue *this_date);
__jsvalue __jsdate_ToUTCString(__jsvalue *this_date);
__jsvalue __jsdate_ValueOf(__jsvalue *this_date);
__jsvalue __jsdate_ToISOString(__jsvalue *this_date);
__jsvalue __jsdate_UTC(__jsvalue *this_date, __jsvalue *args, uint32_t nargs);
__jsvalue __jsdate_Now(__jsvalue *this_date);
__jsvalue __jsdate_ToJSON(__jsvalue *this_date, __jsvalue *value);
__jsvalue __jsdate_Parse(__jsvalue *this_date, __jsvalue *value);
__jsvalue __jsdate_GetYear(__jsvalue *this_date);
__jsvalue __jsdate_SetYear(__jsvalue *this_date,  __jsvalue *value);
__jsvalue __jsdate_ToGMTString(__jsvalue *this_date);

// internal
int64_t Day(int64_t t);
int64_t TimeWithinDday(int64_t t);
int64_t DaysInYear(int64_t y);
int64_t InLeapYear(int64_t t);
int64_t TimeFromYear(int64_t y);
int64_t YearFromTime(int64_t t);
int64_t DayFromYear(int64_t y);
int64_t DayWithinYear(int64_t t);
int64_t MonthFromTime(int64_t t);
int64_t DateFromTime(int64_t t);
int64_t HourFromTime(int64_t t);
int64_t MinFromTime(int64_t t);
int64_t SecFromTime(int64_t t);
int64_t MsFromTime(int64_t t);
int64_t DaylightSavingTA(int64_t t);
int64_t LocalTime(int64_t t);
int64_t UTC(int64_t t);
int64_t LocalTZA();
int64_t MakeDate(int64_t day, int64_t time);
int64_t MakeDay(int64_t year, int64_t month, int64_t date);
int64_t MakeTime(int64_t hour, int64_t min, int64_t sec, int64_t ms);
int64_t TimeClip(int64_t time);
int64_t WeekDay(int64_t t);

// 15.9.2 The Date Constructor called as a function
__jsvalue __js_new_dateconstructor(__jsvalue *, __jsvalue *, uint32_t);
#endif // JSDATE_H

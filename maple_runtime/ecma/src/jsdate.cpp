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

#include <sys/time.h>
#include "jsglobal.h"
#include "jsvalue.h"
#include "jsvalueinline.h"
#include "jsobject.h"
#include "jsobjectinline.h"
#include "jstycnv.h"
#include "jscontext.h"
#include "jsdate.h"
#include "jsfunction.h"
#include "vmmemory.h"

__jsvalue  __js_ToDate(__jsvalue *this_object, __jsvalue *arg_list, uint32_t nargs) {
  return __js_new_date_obj(this_object, arg_list, nargs);
}

// check arglist are all zero
static bool __js_argsZero(__jsvalue *arg_list, uint32_t nargs) {
  for (uint32_t i = 0; i < nargs; i++) {
    __jsvalue *arg = &arg_list[i];
    if ((arg->tag != JSTYPE_NUMBER) ||
        (arg->s.i32 != 0)) {
      return false;
    }
  }
  return true;
}

__jsvalue __js_new_date_obj(__jsvalue *this_object, __jsvalue *arg_list, uint32_t nargs) {
  __jsobject *obj = __create_object();
  __jsobj_set_prototype(obj, JSBUILTIN_DATEPROTOTYPE);
  obj->object_class = JSDATE;
  obj->extensible = true;

  double time;
  if (nargs == 0) {
    struct timeval tv;
    gettimeofday(&tv, 0);
    int64_t time_val = tv.tv_sec * 1000L + tv.tv_usec / 1000L; // UTC in ms
    time = (double) time_val;
  } else if (nargs == 1) {
    if (__js_argsZero(arg_list, nargs)) {
        time = 0.0;
    } else {
        __jsvalue *time_val = arg_list;

        if (__is_string(time_val)) {
            // TODO: not implemented yet
        } else {
            if (__is_nan(time_val) || __is_infinity(time_val)) {
                return __nan_value();
            } else {
                int64_t v = __js_ToNumber64(time_val);
                time = TimeClip(v);
            }
        }
    }
  } else {
    if (__is_nan(&arg_list[0])
        || __is_infinity(&arg_list[0])
        || __is_neg_infinity(&arg_list[0])) {
      time = NAN;
    } else if (__is_nan(&arg_list[1])
        || __is_infinity(&arg_list[1])
        || __is_neg_infinity(&arg_list[1])) {
      time = NAN;
    } else {
      int64_t y = (int64_t) __js_ToNumber(&arg_list[0]);
      int64_t m = (int64_t) __js_ToNumber(&arg_list[1]);

      for (int i = 2; i < nargs; i++) {
        if (__is_undefined(&arg_list[i]) || __is_nan(&arg_list[i])) {
          return __nan_value();
        }
      }
      int64_t dt = nargs >= 3 ? (int64_t) __js_ToNumber(&arg_list[2]) : 1;
      int64_t h = nargs >= 4 ? (int64_t) __js_ToNumber(&arg_list[3]) : 0;
      int64_t min = nargs >= 5 ? (int64_t) __js_ToNumber(&arg_list[4]) : 0;
      int64_t s = nargs >= 6 ? (int64_t) __js_ToNumber(&arg_list[5]) : 0;
      int64_t milli = nargs == 7 ? (int64_t) __js_ToNumber(&arg_list[6]) : 0;

      int64_t yr = (!std::isnan(y) && y >= 0 && y <= 99) ? 1900 + y : y;
      int64_t final_date = UTC(MakeDate(MakeDay(yr, m, dt), MakeTime(h, min, s, milli)));

      // 15.9.1.1 Time Range
      if (final_date <= -100000000 * kMsPerDay || final_date >= 100000000 * kMsPerDay)
        MAPLE_JS_RANGEERROR_EXCEPTION();

      time = (double) TimeClip(final_date);
    }
  }
  obj->shared.primDouble = time;

  return __object_value(obj);
}

// ES5 15.9.1.13 MakeDate(day, time)
int64_t MakeDate(int64_t day, int64_t time) {
  if (!std::isfinite(day) || !std::isfinite(time))
    return NAN;

  return day * kMsPerDay + time;
}

// ES5 15.9.1.12 MakeDay(year, month, date)
int64_t MakeDay(int64_t year, int64_t month, int64_t date) {
  if (!std::isfinite(year) || !std::isfinite(month) || !std::isfinite(date))
    return NAN;

  int64_t y = year;
  int64_t m = month;
  int64_t dt = date;
  int64_t ym = y + (int64_t) floor(m / 12);
  int64_t mm = m % 12;

  int64_t t = TimeFromYear(ym);
  if (!std::isfinite(t) || std::isnan(t))
    return NAN;

  int64_t yd = 0;
  if (mm > 0)
    yd = 31;
  if (mm > 1) {
    if (InLeapYear(t))
      yd += 29;
    else
      yd += 28;
  }
  if (mm > 2)
      yd += 31;
  if (mm > 3)
      yd += 30;
  if (mm > 4)
      yd += 31;
  if (mm > 5)
      yd += 30;
  if (mm > 6)
      yd += 31;
  if (mm > 7)
      yd += 31;
  if (mm > 8)
      yd += 30;
  if (mm > 9)
      yd += 31;
  if (mm > 10)
      yd += 30;

  t += yd * kMsPerDay;

  return Day(t) + dt - 1;
}

// ES5 15.9.1.11 MakeTime(hour, min, sec, ms)
int64_t MakeTime(int64_t hour, int64_t min, int64_t sec, int64_t ms) {
  // TODO: not implemented yet
  if (!std::isfinite(hour) || !std::isfinite(min) || !std::isfinite(sec) || !std::isfinite(ms))
    return NAN;

  int64_t h = hour;
  int64_t m = min;
  int64_t s = sec;
  int64_t milli = ms;
  int64_t t = h * kMsPerHour + m * kMsPerMinute + s * kMsPerSecond + milli;

  return t;
}

// ES5 15.9.1.14 TimeClip(time)
int64_t TimeClip(int64_t time) {
  // TODO: not implemented yet
  return time;
}

static inline __jsobject *__jsdata_value_to_obj(__jsvalue *this_date) {
  if (!__is_js_object(this_date))
    MAPLE_JS_TYPEERROR_EXCEPTION();

  __jsobject *obj = __jsval_to_object(this_date);
  if (obj->object_class != JSDATE)
    MAPLE_JS_TYPEERROR_EXCEPTION();
  return obj;
}

// ES5 15.9.5.14 Date.prototype.getDate()
__jsvalue __jsdate_GetDate(__jsvalue *this_date) {
  __jsobject *obj = __jsdata_value_to_obj(this_date);
  int64_t t = (int64_t) obj->shared.primDouble;

  if (std::isnan(t))
    return __nan_value();

  int64_t date = DateFromTime(LocalTime(t));
  return __double_value((double) date);
}

// ES5 15.9.5.16 Date.prototype.getDay()
__jsvalue __jsdate_GetDay(__jsvalue *this_date) {
  __jsobject *obj = __jsdata_value_to_obj(this_date);
  int64_t t = (int64_t) obj->shared.primDouble;

  if (std::isnan(t))
    return __nan_value();

  int64_t day = WeekDay(LocalTime(t));
  return __double_value((double) day);
}

// ES5 15.9.5.10 Date.prototype.getFullYear()
__jsvalue __jsdate_GetFullYear(__jsvalue *this_date) {
  __jsobject *obj = __jsdata_value_to_obj(this_date);
  int64_t t = (int64_t) obj->shared.primDouble;

  if (std::isnan(t))
    return __nan_value();

  int64_t year = YearFromTime(LocalTime(t));
  return __double_value((double) year);
}

// ES5 15.9.5.18 Date.prototype.getHours()
__jsvalue __jsdate_GetHours(__jsvalue *this_date) {
  __jsobject *obj = __jsdata_value_to_obj(this_date);
  int64_t t = (int64_t) obj->shared.primDouble;

  if (std::isnan(t))
    return __nan_value();

  int64_t hours = HourFromTime(LocalTime(t));
  return __double_value((double) hours);
}

// ES5 15.9.5.24 Date.prototype.getMilliseconds()
__jsvalue __jsdate_GetMilliseconds(__jsvalue *this_date) {
  __jsobject *obj = __jsdata_value_to_obj(this_date);
  int64_t t = (int64_t) obj->shared.primDouble;

  if (std::isnan(t))
    return __nan_value();

  int64_t ms = MsFromTime(LocalTime(t));
  return __double_value((double) ms);
}

// ES5 15.9.5.20 Date.prototype.getMinutes()
__jsvalue __jsdate_GetMinutes(__jsvalue *this_date) {
  __jsobject *obj = __jsdata_value_to_obj(this_date);
  int64_t t = (int64_t) obj->shared.primDouble;

  if (std::isnan(t))
    return __nan_value();

  int64_t mins = MinFromTime(LocalTime(t));
  return __double_value((double) mins);
}

// ES5 15.9.5.12 Date.prototype.getMonth()
__jsvalue __jsdate_GetMonth(__jsvalue *this_date) {
  __jsobject *obj = __jsdata_value_to_obj(this_date);
  int64_t t = (int64_t) obj->shared.primDouble;

  if (std::isnan(t))
    return __nan_value();

  int64_t month = MonthFromTime(LocalTime(t));
  return __double_value((double) month);
}

// ES5 15.9.5.22 Date.prototype.getSeconds()
__jsvalue __jsdate_GetSeconds(__jsvalue *this_date) {
  __jsobject *obj = __jsdata_value_to_obj(this_date);
  int64_t t = (int64_t) obj->shared.primDouble;

  if (std::isnan(t))
    return __nan_value();

  int64_t secs = SecFromTime(LocalTime(t));
  return __double_value((double) secs);
}

// ES5 15.9.5.9 Date.prototype.getTime()
__jsvalue __jsdate_GetTime(__jsvalue *this_date) {
  __jsobject *obj = __jsdata_value_to_obj(this_date);
  double t = obj->shared.primDouble;

  return __double_value(t);
}

// ES5 15.9.5.26 Date.prototype.getTimezoneOffset()
__jsvalue __jsdate_GetTimezoneOffset(__jsvalue *this_date) {
  __jsobject *obj = __jsdata_value_to_obj(this_date);
  int64_t t = (int64_t) obj->shared.primDouble;

  if (std::isnan(t))
    return __nan_value();

  double offset = (t - LocalTime(t)) / (double)kMsPerMinute;
  return __double_value(offset);
}

// ES5 15.9.5.15 Date.prototype.getUTCDate()
__jsvalue __jsdate_GetUTCDate(__jsvalue *this_date) {
  __jsobject *obj = __jsdata_value_to_obj(this_date);
  int64_t t = (int64_t) obj->shared.primDouble;

  if (std::isnan(t))
    return __nan_value();

  int64_t date = DateFromTime(t);
  return __double_value(date);
}

// ES5 15.9.5.17 Date.prototype.getUTCDay()
__jsvalue __jsdate_GetUTCDay(__jsvalue *this_date) {
  __jsobject *obj = __jsdata_value_to_obj(this_date);
  int64_t t = (int64_t) obj->shared.primDouble;

  if (std::isnan(t))
    return __nan_value();

  int64_t day = WeekDay(t);
  return __double_value(day);
}

// ES5 15.9.5.11 Date.prototype.getUTCFullYear()
__jsvalue __jsdate_GetUTCFullYear(__jsvalue *this_date) {
  __jsobject *obj = __jsdata_value_to_obj(this_date);
  int64_t t = (int64_t) obj->shared.primDouble;

  if (std::isnan(t))
    return __nan_value();

  int64_t year = YearFromTime(t);
  return __double_value(year);
}

// ES5 15.9.5.19 Date.prototype.getUTCHours()
__jsvalue __jsdate_GetUTCHours(__jsvalue *this_date) {
  __jsobject *obj = __jsdata_value_to_obj(this_date);
  int64_t t = (int64_t) obj->shared.primDouble;

  if (std::isnan(t))
    return __nan_value();

  int64_t hours = HourFromTime(t);
  return __double_value((double) hours);
}

// ES5 15.9.5.25 Date.prototype.getUTCMilliseconds()
__jsvalue __jsdate_GetUTCMilliseconds(__jsvalue *this_date) {
  __jsobject *obj = __jsdata_value_to_obj(this_date);
  int64_t t = (int64_t) obj->shared.primDouble;

  if (std::isnan(t))
    return __nan_value();

  int64_t ms = MsFromTime(t);
  return __double_value((double) ms);
}

// ES5 15.9.5.21 Date.prototype.getUTCMinutes()
__jsvalue __jsdate_GetUTCMinutes(__jsvalue *this_date) {
  __jsobject *obj = __jsdata_value_to_obj(this_date);
  int64_t t = (int64_t) obj->shared.primDouble;

  if (std::isnan(t))
    return __nan_value();

  int64_t mins = MinFromTime(t);
  return __double_value(mins);
}

// ES5 15.9.5.13 Date.prototype.getUTCMonth()
__jsvalue __jsdate_GetUTCMonth(__jsvalue *this_date) {
  __jsobject *obj = __jsdata_value_to_obj(this_date);
  int64_t t = (int64_t) obj->shared.primDouble;

  if (std::isnan(t))
    return __nan_value();

  int64_t month = MonthFromTime(t);
  return __double_value((double) month);
}

// ES5 15.9.5.23 Date.prototype.getUTCSeconds()
__jsvalue __jsdate_GetUTCSeconds(__jsvalue *this_date) {
  __jsobject *obj = __jsdata_value_to_obj(this_date);
  int64_t t = (int64_t) obj->shared.primDouble;

  if (std::isnan(t))
    return __nan_value();

  int64_t secs = SecFromTime(t);
  return __double_value((double) secs);
}

// ES5 15.9.5.36 Date.prototype.setDate(date)
__jsvalue __jsdate_SetDate(__jsvalue *this_date, __jsvalue *value) {
  // TODO: not implemented yet
  __jsobject *obj = __jsdata_value_to_obj(this_date);
  return __object_value(obj);
}

// ES5 15.9.5.40 Date.prototype.setFullYear(year [, month [, date ]])
__jsvalue __jsdate_SetFullYear(__jsvalue *this_date, __jsvalue *args, uint32_t nargs) {
  // TODO: not implemented yet
  __jsobject *obj = __jsdata_value_to_obj(this_date);
  return __object_value(obj);
}

// ES5 15.9.5.34 Date.prototype.setHours(hour [, min [, sec [, ms ]]])
__jsvalue __jsdate_SetHours(__jsvalue *this_date, __jsvalue *args, uint32_t nargs) {
  // TODO: not implemented yet
  __jsobject *obj = __jsdata_value_to_obj(this_date);
  return __object_value(obj);
}

// ES5 15.9.5.28 Date.prototype.setMilliseconds(ms)
__jsvalue __jsdate_SetMilliseconds(__jsvalue *this_date, __jsvalue *value) {
  // TODO: not implemented yet
  __jsobject *obj = __jsdata_value_to_obj(this_date);
  return __object_value(obj);
}

// ES5 15.9.5.32 Date.prototype.setMinutes(min [, sec [, ms ]])
__jsvalue __jsdate_SetMinutes(__jsvalue *this_date, __jsvalue *args, uint32_t nargs) {
  // TODO: not implemented yet
  __jsobject *obj = __jsdata_value_to_obj(this_date);
  return __object_value(obj);
}

// ES5 15.9.5.38 Date.prototype.setMonth(month [, date ])
__jsvalue __jsdate_SetMonth(__jsvalue *this_date, __jsvalue *args, uint32_t nargs) {
  // TODO: not implemented yet
  __jsobject *obj = __jsdata_value_to_obj(this_date);
  return __object_value(obj);
}

// ES5 15.9.5.30 Date.prototype.setSeconds(sec [, ms ])
__jsvalue __jsdate_SetSeconds(__jsvalue *this_date, __jsvalue *args, uint32_t nargs) {
  // TODO: not implemented yet
  __jsobject *obj = __jsdata_value_to_obj(this_date);
  return __object_value(obj);
}

// ES5 15.9.5.27 Dte.prototype.setTime(time)
__jsvalue __jsdate_SetTime(__jsvalue *this_date, __jsvalue *value) {
  // TODO: not implemented yet
  __jsobject *obj = __jsdata_value_to_obj(this_date);
  return __object_value(obj);
}

// ES5 15.9.5.37 Date.prototype.setUTCDate(date)
__jsvalue __jsdate_SetUTCDate(__jsvalue *this_date, __jsvalue *value) {
  // TODO: not implemented yet
  __jsobject *obj = __jsdata_value_to_obj(this_date);
  return __object_value(obj);
}

// ES5 15.9.5.41 Date.prototype.setUTCFullYear(year [, month [, date ]])
__jsvalue __jsdate_SetUTCFullYear(__jsvalue *this_date, __jsvalue *args, uint32_t nargs) {
  // TODO: not implemented yet
  __jsobject *obj = __jsdata_value_to_obj(this_date);
  return __object_value(obj);
}

// ES5 15.9.5.35 Date.prototype.setUTCHours(hour [, min [, sec [, ms ]]])
__jsvalue __jsdate_SetUTCHours(__jsvalue *this_date, __jsvalue *args, uint32_t nargs) {
  // TODO: not implemented yet
  __jsobject *obj = __jsdata_value_to_obj(this_date);
  return __object_value(obj);
}

// ES5 15.9.5.29 Date.prototype.setUTCMilliseconds(ms)
__jsvalue __jsdate_SetUTCMilliseconds(__jsvalue *this_date, __jsvalue *value) {
  // TODO: not implemented yet
  __jsobject *obj = __jsdata_value_to_obj(this_date);
  return __object_value(obj);
}

// ES5 15.9.5.33 Date.prototype.setUTCMinutes(min [, sec [, ms ]])
__jsvalue __jsdate_SetUTCMinutes(__jsvalue *this_date, __jsvalue *args, uint32_t nargs) {
  // TODO: not implemented yet
  __jsobject *obj = __jsdata_value_to_obj(this_date);
  return __object_value(obj);
}

// ES5 15.9.5.39 Date.prototype.setUTCMonth(month [, date ])
__jsvalue __jsdate_SetUTCMonth(__jsvalue *this_date, __jsvalue *args, uint32_t nargs) {
  // TODO: not implemented yet
  __jsobject *obj = __jsdata_value_to_obj(this_date);
  return __object_value(obj);
}

// ES5 15.9.5.31 Date.prototype.setUTCSeconds(sec [, ms ])
__jsvalue __jsdate_SetUTCSeconds(__jsvalue *this_date, __jsvalue *args, uint32_t nargs) {
  // TODO: not implemented yet
  __jsobject *obj = __jsdata_value_to_obj(this_date);
  return __object_value(obj);
}

// ES5 15.9.5.3 Date.prototype.toDateString()
__jsvalue __jsdate_ToDateString(__jsvalue *this_date) {
  // TODO: not implemented yet
  __jsobject *obj = __jsdata_value_to_obj(this_date);
  return __object_value(obj);
}

// ES5 15.9.5.6 Date.prototype.toLocaleDateString()
__jsvalue __jsdate_ToLocaleDateString(__jsvalue *this_date) {
  // TODO: not implemented yet
  __jsobject *obj = __jsdata_value_to_obj(this_date);
  return __object_value(obj);
}

// ES5 15.9.5.5 Date.prototype.toLocaleString()
__jsvalue __jsdate_ToLocaleString(__jsvalue *this_date) {
  // TODO: not implemented yet
  __jsobject *obj = __jsdata_value_to_obj(this_date);
  return __object_value(obj);
}

// ES5 15.9.5.7 Date.prototype.toLocaleTimeString()
__jsvalue __jsdate_ToLocaleTimeString(__jsvalue *this_date) {
  // TODO: not implemented yet
  __jsobject *obj = __jsdata_value_to_obj(this_date);
  return __object_value(obj);
}

// ES5 B.2.4 Date.prototype.getYea()
__jsvalue __jsdate_GetYear(__jsvalue *this_date) {
  // TODO: not implemented yet
  __jsobject *obj = __jsdata_value_to_obj(this_date);
  return __object_value(obj);
}

// ES5 B.2.5 Date.prototype.setYear()
__jsvalue __jsdate_SetYear(__jsvalue *this_date,  __jsvalue *value) {
  // TODO: not implemented yet
  __jsobject *obj = __jsdata_value_to_obj(this_date);
  return __object_value(obj);
}

// ES5 B.2.6 Date.prototype.toGMTString()
__jsvalue __jsdate_ToGMTString(__jsvalue *this_date) {
  // TODO: not implemented yet
  __jsobject *obj = __jsdata_value_to_obj(this_date);
  return __object_value(obj);
}

const char *WeekDays[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
const char *Months[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};


__jsvalue __jsdate_ToString_Obj(__jsobject *obj) {
  int64_t time = (int64_t) obj->shared.primDouble + LocalTime((int64_t)obj->shared.primDouble);
  if (time < -9007199254740992 || time > 9007199254740992)
    MAPLE_JS_RANGEERROR_EXCEPTION();

  if (!std::isfinite(time))
    MAPLE_JS_RANGEERROR_EXCEPTION();

  int time_zone_minutes = (int)LocalTZA() / kMsPerMinute;
  char buf[100];
  snprintf(buf, sizeof(buf), "%s %s %.2d %.4d %.2d:%.2d:%.2d GMT%+.2d%.2d",
           WeekDays[WeekDay(time)],
           Months[(int)MonthFromTime(time)],
           (int)DateFromTime(time),
           (int)YearFromTime(time),
           (int)HourFromTime(time),
           (int)MinFromTime(time),
           (int)SecFromTime(time),
           time_zone_minutes / 60,
           time_zone_minutes > 0 ? time_zone_minutes % 60 : -time_zone_minutes % 60);

  return __string_value(__jsstr_new_from_char(buf));
}
// ES5 15.9.5.2 Date.prototype.toString()
__jsvalue __jsdate_ToString(__jsvalue *this_date) {
  __jsobject *obj = __jsdata_value_to_obj(this_date);
  return __jsdate_ToString_Obj(obj);
}

// ES5 15.9.5.4 Date.prototype.toTimeString()
__jsvalue __jsdate_ToTimeString(__jsvalue *this_date) {
  // TODO: not implemented yet
  __jsobject *obj = __jsdata_value_to_obj(this_date);
  return __object_value(obj);
}

// ES5 15.9.5.42 Date.prototype.toUTCString()
__jsvalue __jsdate_ToUTCString(__jsvalue *this_date) {
  // TODO: not implemented yet
  __jsobject *obj = __jsdata_value_to_obj(this_date);
  return __object_value(obj);
}

// ES5 15.9.5.8 Date.prototype.valueOf()
__jsvalue __jsdate_ValueOf(__jsvalue *this_date) {
  __jsobject *obj = __jsdata_value_to_obj(this_date);
  int64_t t = (int64_t) obj->shared.primDouble;

  return __double_value((double) t);
}

// ES5 15.9.5.43 Date.prototype.toISOString()
__jsvalue __jsdate_ToISOString(__jsvalue *this_date) {
  __jsobject *obj = __jsdata_value_to_obj(this_date);

  int64_t time = (int64_t) obj->shared.primDouble;
  if (time < -9007199254740992 || time > 9007199254740992)
    MAPLE_JS_RANGEERROR_EXCEPTION();

  if (!std::isfinite(time))
    MAPLE_JS_RANGEERROR_EXCEPTION();

  char buf[100];
  snprintf(buf, sizeof(buf), "%.4d-%.2d-%.2dT%.2d:%.2d:%.2d.%.3dZ",
           (int)YearFromTime(time),
           (int)MonthFromTime(time) + 1,
           (int)DateFromTime(time),
           (int)HourFromTime(time),
           (int)MinFromTime(time),
           (int)SecFromTime(time),
           (int)MsFromTime(time));

  return __string_value(__jsstr_new_from_char(buf));
}

// ES5 15.9.4.3
// Date.UTC(year, month [, date [, hours [, minutes [, seconds [, ms ]]]]])
__jsvalue __jsdate_UTC(__jsvalue *this_date, __jsvalue *args, uint32_t nargs) {
  // TODO: not implemented yet
  __jsobject *obj = __jsdata_value_to_obj(this_date);
  return __double_value(0);
}

// ES5 15.9.5.44 Date.prototype.toJSON(key)
__jsvalue __jsdate_ToJSON(__jsvalue *this_date, __jsvalue *value) {
  // TODO: not implemented yet
  __jsobject *obj = __jsdata_value_to_obj(this_date);
  return __object_value(obj);
}

// ES5 15.9.4.4 Date.now()
__jsvalue __jsdate_Now(__jsvalue *this_date) {
  // TODO: not implemented yet
  return __double_value(0);
}

// ES5 15.9.4.2 Date.parse(string)
__jsvalue __jsdate_Parse(__jsvalue *this_date, __jsvalue *value) {
  // TODO: not implemented yet
  __jsstring *date_str = __js_ToString(value);

  return __double_value(0);
}

// ES5 15.9.1.2
int64_t Day(int64_t t) {
  return (int64_t) floor(t / kMsPerDay);
}

// ES5 15.9.1.2
int64_t TimeWithinDay(int64_t t) {
  return t % kMsPerDay;
}

// ES5 15.9.1.3
int64_t DaysInYear(int64_t y) {
  int64_t result;

  if (fmod(y, 4) != 0) {
    result = 365;
  } else if ((fmod(y, 4) == 0) && (fmod(y, 100) != 0)) {
    result = 366;
  } else if ((fmod(y, 100) == 0) && (fmod(y, 400) != 0)) {
    result = 365;
  } else if (fmod(y, 400) == 0) {
    result = 366;
  }

  return result;
}

// ES5 15.9.1.3
int64_t InLeapYear(int64_t t) {
  int64_t result;

  int64_t y = YearFromTime(t);
  int64_t d = DaysInYear(y);

  if (d == 365)
    result = 0;
  else if (d == 366)
    result = 1;

  return result;
}

// ES5 15.9.1.3
int64_t TimeFromYear(int64_t y) {
  return kMsPerDay * DayFromYear(y);
}

// ES5 15.9.1.3
int64_t YearFromTime(int64_t t) {
  if (!std::isfinite(t))
    return NAN;

  int64_t y = (int64_t) floor(t / (kMsPerDay * 365.2425)) + 1970;
  int64_t t2 = TimeFromYear(y);

  if (t2 > t) {
    y--;
  } else {
    if (t2 + kMsPerDay * DaysInYear(y) <= t)
      y++;
  }

  return y;
}

// ES5 15.9.1.3
int64_t DayFromYear(int64_t y) {
  int64_t day = 365 * (y - 1970)
         + (int64_t) floor((y - 1969) / 4.)
         - (int64_t) floor((y - 1901) / 100.)
         + (int64_t) floor((y - 1601) / 400.);
  return day;
}

// ES5 15.9.1.4
int64_t DayWithinYear(int64_t t) {
  return Day(t) - DayFromYear(YearFromTime(t));
}

// ES5 15.9.1.4
int64_t MonthFromTime(int64_t t) {
  if (!std::isfinite(t))
    return NAN;

  int64_t result;
  int64_t d = DayWithinYear(t);
  int64_t leap_year = InLeapYear(t);

  if (d >= 0 && d < 31)
    result = 0;
  else if ((d >= 31) && d < (59 + leap_year))
    result = 1;
  else if ((d >= 59 + leap_year) && (d < 90 + leap_year))
    result = 2;
  else if ((d >= 90 + leap_year) && (d < 120 + leap_year))
    result = 3;
  else if ((d >= 120 + leap_year) && (d < 151 + leap_year))
    result = 4;
  else if ((d >= 151 + leap_year) && (d < 181 + leap_year))
    result = 5;
  else if ((d >= 181 + leap_year) && (d < 212 + leap_year))
    result = 6;
  else if ((d >= 212 + leap_year) && (d < 243 + leap_year))
    result = 7;
  else if ((d >= 243 + leap_year) && (d < 273 + leap_year))
    result = 8;
  else if ((d >= 273 + leap_year) && (d < 304 + leap_year))
    result = 9;
  else if ((d >= 304 + leap_year) && (d < 334 + leap_year))
    result = 10;
  else if ((d >= 334 + leap_year) && (d <= 365 + leap_year))
    result = 11;
  else
    assert(false);

  return result;
}

// ES5 15.9.1.5
int64_t DateFromTime(int64_t t) {
  if (!std::isfinite(t))
    return NAN;

  if (t < 0)
    t -= (kMsPerDay - 1);

  int64_t d = DayWithinYear(t);
  int64_t leap_year = InLeapYear(t);
  int64_t m = MonthFromTime(t);
  int64_t result;

  if (m == 0)
    result = d + 1;
  else if (m == 1)
    result = d - 30;
  else if (m == 2)
    result = d - 58 - leap_year;
  else if (m == 3)
    result = d - 89 - leap_year;
  else if (m == 4)
    result = d - 119 - leap_year;
  else if (m == 5)
    result = d - 150 - leap_year;
  else if (m == 6)
    result = d - 180 - leap_year;
  else if (m == 7)
    result = d - 211 - leap_year;
  else if (m == 8)
    result = d - 242 - leap_year;
  else if (m == 9)
    result = d - 272 - leap_year;
  else if (m == 10)
    result = d - 303 - leap_year;
  else if (m == 11)
    result = d - 333 - leap_year;
  else
    MAPLE_JS_ASSERT(false);

  return result;
}

// ES5 15.9.1.10
int64_t HourFromTime(int64_t t) {
  if (t < 0)
    return (int64_t) floor((kMsPerDay + (t % kMsPerDay)) / kMsPerHour) % kHoursPerDay;
  else
    return (int64_t) floor(t / kMsPerHour) % kHoursPerDay;
}

// ES5 15.9.1.10
int64_t MinFromTime(int64_t t) {
  if (t < 0)
    return (int64_t) floor((kMsPerDay + (t % kMsPerDay)) / kMsPerMinute) % kMinutesPerHour;
  else
    return (int64_t) floor(t / kMsPerMinute) % kMinutesPerHour;
}

// ES5 15.9.1.10
int64_t SecFromTime(int64_t t) {
  if (t < 0)
    return (int64_t) floor((kMsPerDay + (t % kMsPerDay)) / kMsPerSecond) % kSecondsPerMinute;
  else
    return (int64_t) floor(t / kMsPerSecond) % kSecondsPerMinute;
}

// ES5 15.9.1.10
int64_t MsFromTime(int64_t t) {
  if (t < 0)
    return (kMsPerDay + (t % kMsPerDay)) % kMsPerSecond;
  else
    return t % kMsPerSecond;
}

// ES5 15.9.1.8
int64_t DaylightSavingTA(int64_t t) {
  // TODO: not implemented yet
  return 0;
}

// ES5 15.9.1.9
int64_t LocalTime(int64_t t) {
  return t + LocalTZA() + DaylightSavingTA(t);
}

// ES5 15.9.1.9
int64_t UTC(int64_t t) {
  int64_t localTZA = LocalTZA();
  return t - localTZA - DaylightSavingTA(t - localTZA);
}

// ES5 15.9.1.7
// returns a value localTZA measured in milliseconds
int64_t LocalTZA() {
  tzset();
  return -timezone * 1000;
}

// ES5 15.9.1.6
int64_t WeekDay(int64_t t) {
  if (t < 0) {
    t -= (kMsPerDay - 1);
    return (((Day(t) % 7) + 11) % 7);
  }
  return (Day(t) + 4) % 7;
}

// 15.9.2 The Date Constructor called as a function, return a string
__jsvalue __js_new_dateconstructor(__jsvalue *this_object, __jsvalue *arg_list, uint32_t nargs) {
  __jsvalue dateVal = __js_new_date_obj(this_object, arg_list, nargs);
  return __jsdate_ToString(&dateVal);
}

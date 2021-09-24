/*
 * Copyright (C) [2021] Futurewei Technologies, Inc. All rights reserved.
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
#include <cmath>
#include "jsmath.h"
#include "jstycnv.h"
// 15.8.2.1
__jsvalue __jsmath_pt_abs(__jsvalue *this_math, __jsvalue *value) {
  if (__is_nan(value))
    return *value;
  if (__is_infinity(value)) {
    return __number_infinity();
  }
  if (__is_negative_zero(value))
    return __positive_zero_value();
  if (__is_double(value)) {
    double v = __jsval_to_double(value);
    if (v < 0)
      return __double_value(0 - v);
    return *value;
  }
  MAPLE_JS_ASSERT(__is_number(value));
  int32_t ret = __js_ToInt32(value);
  if (ret < 0) {
    ret = 0 - ret;
  }
  return __number_value(ret);
}

// 15.8.2.6
__jsvalue __jsmath_pt_ceil(__jsvalue *this_math, __jsvalue *value) {
  if (__is_nan(value) || __is_infinity(value))
    return *value;
  double v = __jsval_to_double(value);
  if (v == 0.0)
    return *value;
  if (v < 0.0 && v > -1.0)
    return __negative_zero_value();
  double f = ceil(v);
  return __double_value(f);
}

// 15.8.2.9
__jsvalue __jsmath_pt_floor(__jsvalue *this_math, __jsvalue *value) {
  if (__is_nan(value) || __is_infinity(value))
    return *value;
  double v = __jsval_to_double(value);
  if (v == 0.0)
    return *value;
  if (v > 0.0 && v < 1.0)
    return __positive_zero_value();
  double f = floor(v);
  return __double_value(f);
}

// 15.8.2.11
__jsvalue __jsmath_pt_max(__jsvalue *this_math, __jsvalue *items, uint32_t size) {
  if (size == 0)
    return __number_neg_infinity();

  __jsvalue max_item = __number_neg_infinity();
  for (uint32_t i = 0; i < size; i++) {
    __jsvalue item = items[i];
    if (__is_js_object(&item)) {
      item = __js_ToPrimitive(&item, JSTYPE_NUMBER);
    }
    if (__is_nan(&item))
      return item;
    if (__is_infinity(&item)) {
      // -Infinity
      if (__is_neg_infinity(&item))
        continue;
      // +Infinity
      max_item = item;
    } else {
      if (__is_infinity(&max_item)) {
        if (__is_neg_infinity(&max_item))
          max_item = item;
        else
          continue; // max_item is +Infinity, just continue to check if there is any NaN
      } else {
        double v = __jsval_to_double(&item);
        if (v > __jsval_to_double(&max_item) || (v == 0.0 && __is_negative_zero(&max_item)))
          max_item = item;
      }
    }
  }
  return max_item;
}

// 15.8.2.12
__jsvalue __jsmath_pt_min(__jsvalue *this_math, __jsvalue *items, uint32_t size) {
  if (size == 0)
    return __number_infinity();

  __jsvalue min_item = __number_infinity();
  for (uint32_t i = 0; i < size; i++) {
    __jsvalue item = items[i];
    if (__is_js_object(&item)) {
      item = __js_ToPrimitive(&item, JSTYPE_NUMBER);
    }
    if (__is_nan(&item))
      return item;
    if (__is_infinity(&item)) {
      if (__is_neg_infinity(&item)) // -Infinity
        min_item = item;
      else // +Infinity
        continue;
    } else {
      if (__is_infinity(&min_item)) {
        if (__is_neg_infinity(&min_item))
          continue; // min_item is -Infinity, just continue to check if there is any NaN
        else
          min_item = item;
      } else {
        double v = __jsval_to_double(&item);
        if (v < __jsval_to_double(&min_item) || (v == 0.0 && __is_positive_zero(&min_item)))
          min_item = item;
      }
    }
  }
  return min_item;
}

// 15.8.2.13
__jsvalue __jsmath_pt_pow(__jsvalue *this_math, __jsvalue *x, __jsvalue *y) {
  if (__is_nan(y) || __is_undefined(y))
    return *y;
  bool y_is_infinity = false;
  bool y_is_neg_infinity = false;
  double py = 0.0;
  if (__is_infinity(y)) {
    if (__is_neg_infinity(y))
      y_is_neg_infinity = true;
    else
      y_is_infinity = true;
  }
  else {
    py = __jsval_to_double(y);
    if (py == 0.0)
      return __number_value(1);
  }
  if (__is_nan(x))
    return *x;
  double px = 0.0;
  if (!__is_infinity(x)) {
    px = __jsval_to_double(x);
    if (std::abs(px) > 1) {
      if (y_is_infinity)
        return __number_infinity();
      if (y_is_neg_infinity)
        return __positive_zero_value();
    }
    if (std::abs(px) == 1) {
      if (y_is_infinity || y_is_neg_infinity)
        return __nan_value();
    }
    if (std::abs(px) < 1) {
      if (y_is_infinity)
        return __positive_zero_value();
      if (y_is_neg_infinity)
        return __number_infinity();
    }
  } else {
    if (__is_neg_infinity(x)) {
      // x is -Infinity
      if (y_is_infinity)
        return __number_infinity();
      if (y_is_neg_infinity)
        return __positive_zero_value();
      if (py > 0.0) {
        if (__is_double_no_decimal(py) && ((int64_t)py) % 2 != 0)
          return __number_neg_infinity(); // y is odd, -Infinity
        return __number_infinity();
      }
      if (py < 0.0) {
        if (__is_double_no_decimal(py) && ((int64_t)py) % 2 != 0)
          return __negative_zero_value(); // y is odd, -0
        return __positive_zero_value();
      }
    }
    // x is +Infinity
    if (py > 0.0 || y_is_infinity)
      return __number_infinity();
    if (py < 0.0 || y_is_neg_infinity)
      return __positive_zero_value();
  }
  if (__is_negative_zero(x)) {
    // x is -0
    if (py > 0.0 || y_is_infinity) {
      if (__is_double_no_decimal(py) && ((int64_t)py) % 2 != 0)
        return __negative_zero_value(); // y is odd, -0
      return __positive_zero_value();
    }
    if (py < 0.0 || y_is_neg_infinity) {
      if (__is_double_no_decimal(py) && ((int64_t)py) % 2 != 0)
        return __number_neg_infinity(); // y is odd, -Infinity
      return __number_infinity();
    }
  }
  double t = pow(px, py);
  if (std::isinf(t)) {
    return __number_infinity();
  } else {
    if (__is_double_no_decimal(t) && __is_int32_range(t))
      return __number_value((int32_t)t);
    else {
      __jsvalue jsval;
      jsval = __double_value(t);
      return jsval;
    }
  }
}

// 15.8.2.15
__jsvalue __jsmath_pt_round(__jsvalue *this_math, __jsvalue *value) {
  if (__is_nan(value) || __is_infinity(value))
    return *value;
  double v = __jsval_to_double(value);
  if (v == 0.0)
    return *value;
  if (v > 0.0 && v < 0.5)
      return __positive_zero_value();
  if (v < 0.0 && v >= -0.5)
      return __negative_zero_value();
  double f = round(v);
  if (f == 0.0) {
    if (v > 0)
      return __positive_zero_value();
    return __negative_zero_value();
  }
  return __double_value(f);
}

// 15.8.2.17
__jsvalue __jsmath_pt_sqrt(__jsvalue *this_math, __jsvalue *value) {
  if (__is_nan(value))
    return *value;
  if (__is_infinity(value)) {
    if (__is_neg_infinity(value))
      return __nan_value();
    return *value;
  }
  double v = __jsval_to_double(value);
  if (v == 0.0)
    return *value;
  if (v < 0.0)
    return __nan_value();
  double f = sqrt(v);
  return __double_value(f);
}

// 15.8.2.16
__jsvalue __jsmath_pt_sin(__jsvalue *this_math, __jsvalue *value) {
  if (__is_nan(value) || __is_undefined(value))
    return *value;
  if (__is_infinity(value))
    return __nan_value();
  double v = __jsval_to_double(value);
  double f = sin(v);
  if (f == 0.0)
   return *value;
  else
   return __double_value(f);
}

// 15.8.2.3
__jsvalue __jsmath_pt_asin(__jsvalue *this_math, __jsvalue *value) {
  if (__is_nan(value) || __is_undefined(value))
    return *value;
  if (__is_infinity(value)) {
    return __nan_value();
  }
  double v = __jsval_to_double(value);
  if (v > 1.0 || v < -1.0)
    return __nan_value();
  double f = asin(v);
  if (f == 0.0)
   return *value;
  else
   return __double_value(f);
}

// 15.8.2.7
__jsvalue __jsmath_pt_cos(__jsvalue *this_math, __jsvalue *value) {
  if (__is_nan(value) || __is_undefined(value))
    return *value;
  if (__is_infinity(value))
    return __nan_value();
  double v = __jsval_to_double(value);
  double f = cos(v);
  return __double_value(f);
}

// 15.8.2.2
__jsvalue __jsmath_pt_acos(__jsvalue *this_math, __jsvalue *value) {
  if (__is_nan(value) || __is_undefined(value))
    return *value;
  if (__is_infinity(value)) {
    return __nan_value();
  }
  double v = __jsval_to_double(value);
  if (v > 1.0 || v < -1.0)
    return __nan_value();
  double f = acos(v);
  return __double_value(f);
}

// 15.8.2.18
__jsvalue __jsmath_pt_tan(__jsvalue *this_math, __jsvalue *value) {
  if (__is_nan(value) || __is_undefined(value))
    return *value;
  if (__is_infinity(value))
    return __nan_value();
  double v = __jsval_to_double(value);
  if (v == 0.0)
   return *value;
  double f = tan(v);
  return __double_value(f);
}

// 15.8.2.4
__jsvalue __jsmath_pt_atan(__jsvalue *this_math, __jsvalue *value) {
  if (__is_nan(value) || __is_undefined(value))
    return *value;
  if (__is_infinity(value)) {
    if (__is_neg_infinity(value))
      return __double_value(-MathPi/2.0);
    return __double_value(MathPi/2.0);
  }
  double v = __jsval_to_double(value);
  if (v == 0.0)
   return *value;
  double f = atan(v);
  return __double_value(f);
}

// 15.8.2.5
__jsvalue __jsmath_pt_atan2(__jsvalue *this_math, __jsvalue *y, __jsvalue *x) {
  if (__is_nan(y) || __is_undefined(y))
    return *y;
  if (__is_nan(x)|| __is_undefined(x))
    return *x;
  if (__is_infinity(y)) {
    // y = -infinity
    if (__is_neg_infinity(y)) {
      if (__is_infinity(x)) {
        // x = -infinity
        if (__is_neg_infinity(x))
          return __double_value(-3.0*MathPi/4.0);
        // x = +infinity
        return __double_value(-MathPi/4.0);
      } else {
        // x = finite
        return __double_value(-MathPi/2.0);
      }
    }
    // y = +infinity
    if (__is_infinity(x)) {
      // x = -infinity
      if (__is_neg_infinity(x))
        return __double_value(3.0*MathPi/4.0);
      // x = +infinity
      return __double_value(MathPi/4.0);
    } else {
      // x = finite
      return __double_value(MathPi/2.0);
    }
  }
  // y = finite
  double yv = __jsval_to_double(y);
  if (__is_infinity(x)) {
    // x = -infinity
    if (__is_neg_infinity(x)) {
      if (yv > 0 || __is_positive_zero(y))
        return __double_value(MathPi);
      if (yv < 0 || __is_negative_zero(y))
        return __double_value(-MathPi);
    }
    // x = +infinity
    if (yv > 0 || __is_positive_zero(y))
      return __positive_zero_value();
    if (yv < 0 || __is_negative_zero(y))
      return __negative_zero_value();
  }
  double xv = 0.0;
  if (!__is_infinity(x))
    xv = __jsval_to_double(x);
  if (__is_positive_zero(y)) {
    if (__is_neg_infinity(x) || xv < 0 || __is_negative_zero(x))
      return __double_value(MathPi);
    if ((__is_infinity(x) && !__is_neg_infinity(x)) || xv > 0 || __is_positive_zero(x))
      return __positive_zero_value();
  }
  if ((__is_negative_zero(y))) {
    if ((__is_infinity(x) && !__is_neg_infinity(x)) || xv > 0 || __is_positive_zero(x))
      return __negative_zero_value();
    if (__is_neg_infinity(x) || xv < 0 || __is_negative_zero(x))
      return __double_value(-MathPi);
  }
  if (xv == 0.0) {
    if (yv > 0)
      return __double_value(MathPi/2.0);
    if (yv < 0)
      return __double_value(-MathPi/2.0);
  }
  double f = atan2(yv, xv);
  return (f == 0) ? __positive_zero_value() : __double_value(f);
}

// 15.8.2.10
__jsvalue __jsmath_pt_log(__jsvalue *this_math, __jsvalue *value) {
  if (__is_nan(value) || __is_undefined(value))
    return *value;
  if (__is_infinity(value)) {
    if (__is_neg_infinity(value))
      return __nan_value();
    return *value;
  }
  double v = __jsval_to_double(value);
  if (v < 0.0)
   return __nan_value();
  if (v == 0.0)
   return __number_neg_infinity();
  double f = log(v);
  return __double_value(f);
}

// 15.8.2.8
__jsvalue __jsmath_pt_exp(__jsvalue *this_math, __jsvalue *value) {
  if (__is_nan(value) || __is_undefined(value))
    return *value;
  if (__is_infinity(value)) {
    if (__is_neg_infinity(value))
      return __number_value(0);
    return *value;
  }
  double v = __jsval_to_double(value);
  double f = exp(v);
  return __double_value(f);
}

// 15.8.2.14
__jsvalue __jsmath_pt_random(__jsvalue *this_math) {
  return __double_value(drand48());
}

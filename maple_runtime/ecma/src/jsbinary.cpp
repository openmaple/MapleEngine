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

#include "jsvalue.h"
#include "jsvalueinline.h"
#include "jsnum.h"
#include "jsstring.h"
#include "jstycnv.h"
#include "jsfunction.h"
#include "vmmemory.h"
#include "stdio.h"
#include "jsdate.h"


static __jsvalue __jsop_add_double(double dx, double dy) {
    if ((dx >= NumberMaxValue && dy > 0) || (dx > 0 && dy >= NumberMaxValue)) {
      return __number_infinity();
    }
    if ((dx <= -NumberMaxValue && dy < 0) || (dx < 0 && dy <= -NumberMaxValue)) {
      return __number_neg_infinity();
    }
    double res = dx + dy;
    if (fabs(res) < NumberMinValue) {
      return __number_value(0);
    }
    return __double_value(dx + dy);
}
// ecma 11.6 Additive Operators
// ecma 11.6.1 The Addition operator ( + )
__jsvalue __jsop_add(__jsvalue *x, __jsvalue *y) {
  if (__is_number(x) && __is_number(y)) {
    return __number_value(__jsval_to_number(x) + __jsval_to_number(y));
  } else if (__is_number(x) && __is_double(y)) {
    return __jsop_add_double((double)__jsval_to_number(x), __jsval_to_double(y));
  } else if (__is_double(x) && __is_double(y)) {
    return  __jsop_add_double(__jsval_to_double(x), __jsval_to_double(y));
  } else if (__is_double(x) && __is_number(y)) {
    return __jsop_add_double(__jsval_to_double(x), (double)__jsval_to_number(y));
  }

  if(__is_nan(x) || __is_nan(y))
      return __nan_value();
  if (__is_infinity(x) && __is_infinity(y)) {
      if(x->s.i32 != y->s.i32)
          return __nan_value();
      return *x;
  }
  if (__is_infinity(x)) {
    __jsvalue yPrim = __js_ToPrimitive(y, JSTYPE_UNDEFINED /* ??? */);
    bool isInfinity = __is_infinity(x);
    if (__is_string(&yPrim)) {
      __jsstring *lstr = __jsstr_get_builtin(isInfinity ?
                                    (__is_neg_infinity(x) ? JSBUILTIN_STRING_NEG_INFINITY_UL : JSBUILTIN_STRING_INFINITY_UL) :
                                    JSBUILTIN_STRING_NAN);
      __jsstring *rstr = __js_ToString(&yPrim);
      __jsvalue ret = __string_value(__jsstr_concat_2(lstr, rstr));
      memory_manager->RecallString(rstr);
      return ret;
    } else {
      return *x;
    }
  }
  if (__is_infinity(y)) {
    __jsvalue xPrim = __js_ToPrimitive(x, JSTYPE_UNDEFINED /* ??? */);
    bool isInfinity = __is_infinity(y);
    if (__is_string(&xPrim)) {
      __jsstring *lstr = __js_ToString(&xPrim);
      __jsstring *rstr = __jsstr_get_builtin(isInfinity ?
                                    (__is_neg_infinity(y) ? JSBUILTIN_STRING_NEG_INFINITY_UL : JSBUILTIN_STRING_INFINITY_UL) :
                                    JSBUILTIN_STRING_NAN);
      __jsvalue ret = __string_value(__jsstr_concat_2(lstr, rstr));
      memory_manager->RecallString(lstr);
      return ret;
    } else {
      return *y;
    }
  }
  if (((__is_null(x) || __is_number(x)) && __is_undefined(y)) ||
      ((__is_null(y) || __is_number(y)) && __is_undefined(x)) ||
      (__is_undefined(y) && __is_undefined(x))) {
    return __nan_value();
  }

  __jsvalue lprim;
  __jsvalue rprim;
  if (__is_primitive(x)) {
    lprim = *x;
  } else {
    __jsobject *objX = __jsval_to_object(x);
    lprim = (objX->object_class == __jsobj_class::JSDATE) ?
               __jsdate_ToString_Obj(objX):
               __object_internal_DefaultValue(objX, JSTYPE_UNDEFINED);
  }
  if (__is_primitive(y)) {
    rprim = *y;
  } else {
    __jsobject *objY = __jsval_to_object(y);
    rprim = (objY->object_class == __jsobj_class::JSDATE) ?
               __jsdate_ToString_Obj(objY):
               __object_internal_DefaultValue(objY, JSTYPE_UNDEFINED);
  }
  if (__is_string(&lprim) || __is_string(&rprim)) {
    bool isLstr = __is_string(&lprim);
    bool isRstr = __is_string(&rprim);
    __jsstring *lstr = __js_ToString(&lprim);
    __jsstring *rstr = __js_ToString(&rprim);
    __jsvalue ret = __string_value(__jsstr_concat_2(lstr, rstr));
    if (!isLstr)
      memory_manager->RecallString(lstr);
    if (!isRstr)
      memory_manager->RecallString(rstr);
    return ret;
  } else {
    if (__is_undefined(x) || __is_undefined(y)) {
      return __nan_value();
    }
    // ??? Overflow
    return __number_value(__js_ToNumber(&lprim) + __js_ToNumber(&rprim));
  }
}

__jsvalue __jsop_object_div(__jsvalue *x, __jsvalue *y) {
  switch (x->tag) {
    case JSTYPE_BOOLEAN: {
      if (__is_js_object(y)) {
        __jsobject *yobj = __jsval_to_object(y);
        if (yobj->object_class == JSNUMBER) {
          int32_t yval = __js_ToNumber(y);
          if (yval == 0) {
            return  __number_infinity();
          }
          int32_t res = x->s.boo / yval;
          return __number_value(res);
        } else if (yobj->object_class == JSBOOLEAN) {
          int32_t yval = yobj->shared.prim_bool;
          if (yval == 0) {
            return __number_infinity();
          }
          int32_t res = x->s.boo / yval;
          return __number_value(res);
        } else if (yobj->object_class == JSSTRING) {
          bool ok2Convert = true;
          int32_t yval = __js_str2num2(yobj->shared.prim_string, ok2Convert, true);
          if (!ok2Convert) {
            return __nan_value();
          } else {
            if (yval == 0) {
              return __number_infinity();
            }
            int32_t res = x->s.boo / yval;
            return __number_value(res);
          }
        } else if (yobj->object_class == JSOBJECT) {
          __jsvalue yjsval = __object_internal_DefaultValue(yobj, JSTYPE_NUMBER);
          if (__is_undefined(&yjsval) || __is_string(&yjsval)) {
            return __nan_value();
          } else {
            int32_t yval = yjsval.s.i32;
            if (yval == 0) {
              return  __number_infinity();
            }
            int32_t res = x->s.i32 / yval;
            return __number_value(res);
          }
        } else if (yobj->object_class == JSFUNCTION) {
          return __nan_value();
        } else {
          assert(false && "nyi");
        }
      } else {
        assert(false && "nyi");
      }
    }
    case JSTYPE_NUMBER: {
      int32_t xval = x->s.i32;
      if (__is_js_object(y)) {
        __jsobject *yobj = __jsval_to_object(y);
        if (yobj->object_class == JSNUMBER) {
          int32_t yval = __js_ToNumber(y);
          if (yval == 0) {
            return xval >= 0 ? __number_infinity() : __number_neg_infinity();
          }
          int32_t res = xval / yval;
          return __number_value(res);
        } else if (yobj->object_class == JSBOOLEAN) {
          int32_t yval = yobj->shared.prim_bool;
          if (yval == 0) {
            return xval >= 0 ? __number_infinity() : __number_neg_infinity();
          }
          int32_t res = xval / yval;
          return __number_value(res);
        } else if (yobj->object_class == JSSTRING) {
          bool ok2Convert = true;
          int32_t yval = __js_str2num2(yobj->shared.prim_string, ok2Convert, true);
          if (!ok2Convert) {
            return __nan_value();
          } else {
            if (yval == 0) {
              return xval >= 0 ? __number_infinity() : __number_neg_infinity();
            }
            int32_t res = xval / yval;
            return __number_value(res);
          }
        } else if (yobj->object_class == JSOBJECT) {
          __jsvalue yjsval = __object_internal_DefaultValue(yobj, JSTYPE_NUMBER);
          if (__is_undefined(&yjsval) || __is_string(&yjsval)) {
            return __nan_value();
          } else {
            int32_t yval = yjsval.s.i32;
            if (yval == 0) {
              return xval >= 0 ? __number_infinity() : __number_neg_infinity();
            }
            int32_t res = xval / yval;
            return __number_value(res);
          }
        } else if (yobj->object_class == JSFUNCTION) {
          return __nan_value();
        } else {
          assert(false && "nyi");
        }
      } else {
        assert(false && "nyi");
      }
    }
    case JSTYPE_OBJECT: {
      __jsobject *xobj = __jsval_to_object(x);
      int32_t xval = 0;
      if (xobj->object_class == JSNUMBER) {
        xval = __js_ToNumber(x);
      } else if (xobj->object_class == JSBOOLEAN) {
        xval = xobj->shared.prim_bool;
      } else if (xobj->object_class == JSSTRING) {
        bool ok2Convert = true;
        xval = __js_str2num2(xobj->shared.prim_string, ok2Convert, true);
        if (!ok2Convert) {
          return __nan_value();
        }
      } else if (xobj->object_class == JSOBJECT) {
        __jsvalue xjsval = __object_internal_DefaultValue(xobj, JSTYPE_NUMBER);
        if (__is_undefined(&xjsval) || __is_string(&xjsval)) {
          return __nan_value();
        } else {
          xval = xjsval.s.i32;
        }
      } else if (xobj->object_class == JSFUNCTION) {
        return __nan_value();
      } else {
        assert(false && "nyi");
      }
      if (__is_js_object(y)) {
        __jsobject *yobj = __jsval_to_object(y);
        if (yobj->object_class == JSNUMBER) {
          int32_t yval = __js_ToNumber(y);
          if (yval == 0) {
            return xval >= 0 ? __number_infinity() : __number_neg_infinity();
          }
          int32_t res = xval / yval;
          return __number_value(res);
        } else if (yobj->object_class == JSBOOLEAN) {
          int32_t yval = yobj->shared.prim_bool;
          if (yval == 0) {
            return xval >= 0 ? __number_infinity() : __number_neg_infinity();
          }
          int32_t res = xval / yval;
          return __number_value(res);
        } else if (yobj->object_class == JSSTRING) {
          bool ok2Convert = true;
          int32_t yval = __js_str2num2(yobj->shared.prim_string, ok2Convert, true);
          if (!ok2Convert) {
            return __nan_value();
          } else {
            if (yval == 0) {
              return xval >= 0 ? __number_infinity() : __number_neg_infinity();
            }
            int32_t res = xval / yval;
            return __number_value(res);
          }
        } else if (yobj->object_class == JSOBJECT) {
          __jsvalue yjsval = __object_internal_DefaultValue(yobj, JSTYPE_NUMBER);
          if (__is_undefined(&yjsval) || __is_string(&yjsval)) {
            return __nan_value();
          } else {
            int32_t yval = yjsval.s.i32;
            if (yval == 0) {
              return xval >= 0 ? __number_infinity() : __number_neg_infinity();
            }
            int32_t res = xval / yval;
            return __number_value(res);
          }
        } else if (yobj->object_class == JSFUNCTION) {
          return __nan_value();
        } else {
          assert(false && "nyi");
        }
      } else if (__is_number(y) || __is_boolean(y)) {
        if (y->s.i32 == 0) {
          return xval >= 0 ? __number_infinity() : __number_neg_infinity();
        } else {
          return __number_value(xval / y->s.i32);
        }
      } else if (__is_null(y)) {
        return xval >= 0 ? __number_infinity() : __number_neg_infinity();
      } else if (__is_undefined(y)) {
        return __nan_value();
      } else {
        assert(false && "nyi");
      }
    }
    case JSTYPE_NULL: {
      return __number_value(0);
    }
    default:
      assert(false && "nyi");
  }
}

__jsvalue __jsop_object_rem(__jsvalue *x, __jsvalue *y) {
  switch (x->tag) {
    case JSTYPE_STRING: {
      bool isNum;
      int32_t xval = __js_str2num2(__jsval_to_string(x), isNum, true);
      if (!isNum) {
        return __nan_value();
      }
      if (__is_js_object(y)) {
        __jsobject *yobj = __jsval_to_object(y);
        if (yobj->object_class == JSNUMBER) {
          int32_t yval = __js_ToNumber(y);
          int32_t res = xval % yval;
          return __number_value(res);
        } else if (yobj->object_class == JSBOOLEAN) {
          int32_t yval = yobj->shared.prim_bool;
          int32_t res = xval % yval;
          return __number_value(res);
        } else if (yobj->object_class == JSSTRING) {
          bool ok2Convert = true;
          int32_t yval = __js_str2num2(yobj->shared.prim_string, ok2Convert, true);
          if (!ok2Convert) {
            return __nan_value();
          } else {
            int32_t res = xval % yval;
            return __number_value(res);
          }
        } else if (yobj->object_class == JSOBJECT) {
          __jsvalue yjsval = __object_internal_DefaultValue(yobj, JSTYPE_NUMBER);
          if (__is_undefined(&yjsval) || __is_string(&yjsval)) {
            return __nan_value();
          } else {
            int32_t yval = yjsval.s.i32;
            int32_t res = xval % yval;
            return __number_value(res);
          }
        } else if (yobj->object_class == JSFUNCTION) {
          return __nan_value();
        } else {
          assert(false && "nyi");
        }
      } else {
        assert(false && "nyi");
      }
    }
    case JSTYPE_BOOLEAN: {
      if (__is_js_object(y)) {
        __jsobject *yobj = __jsval_to_object(y);
        if (yobj->object_class == JSNUMBER) {
          int32_t yval = __js_ToNumber(y);
          int32_t res = x->s.boo % yval;
          return __number_value(res);
        } else if (yobj->object_class == JSBOOLEAN) {
          int32_t yval = yobj->shared.prim_bool;
          int32_t res = x->s.boo % yval;
          return __number_value(res);
        } else if (yobj->object_class == JSSTRING) {
          bool ok2Convert = true;
          int32_t yval = __js_str2num2(yobj->shared.prim_string, ok2Convert, true);
          if (!ok2Convert) {
            return __nan_value();
          } else {
            int32_t res = x->s.boo % yval;
            return __number_value(res);
          }
        } else if (yobj->object_class == JSOBJECT) {
          __jsvalue yjsval = __object_internal_DefaultValue(yobj, JSTYPE_NUMBER);
          if (__is_undefined(&yjsval) || __is_string(&yjsval)) {
            return __nan_value();
          } else {
            int32_t yval = yjsval.s.i32;
            int32_t res = x->s.i32 % yval;
            return __number_value(res);
          }
        } else if (yobj->object_class == JSFUNCTION) {
          return __nan_value();
        } else {
          assert(false && "nyi");
        }
      } else {
        assert(false && "nyi");
      }
    }
    case JSTYPE_NUMBER: {
      if (__is_js_object(y)) {
        __jsobject *yobj = __jsval_to_object(y);
        if (yobj->object_class == JSNUMBER) {
          int32_t yval = __js_ToNumber(y);
          int32_t res = x->s.boo % yval;
          return __number_value(res);
        } else if (yobj->object_class == JSBOOLEAN) {
          int32_t yval = yobj->shared.prim_bool;
          int32_t res = x->s.boo % yval;
          return __number_value(res);
        } else if (yobj->object_class == JSSTRING) {
          bool ok2Convert = true;
          int32_t yval = __js_str2num2(yobj->shared.prim_string, ok2Convert, true);
          if (!ok2Convert) {
            return __nan_value();
          } else {
            int32_t res = x->s.boo % yval;
            return __number_value(res);
          }
        } else if (yobj->object_class == JSOBJECT) {
          __jsvalue yjsval = __object_internal_DefaultValue(yobj, JSTYPE_NUMBER);
          if (__is_undefined(&yjsval) || __is_string(&yjsval)) {
            return __nan_value();
          } else {
            int32_t yval = yjsval.s.i32;
            int32_t res = x->s.i32 % yval;
            return __number_value(res);
          }
        } else if (yobj->object_class == JSFUNCTION) {
          return __nan_value();
        } else {
          assert(false && "nyi");
        }
      } else {
        assert(false && "nyi");
      }
    }
    case JSTYPE_OBJECT: {
      __jsobject *xobj = __jsval_to_object(x);
      int32_t xval = 0;
      if (xobj->object_class == JSNUMBER) {
        xval = __js_ToNumber(x);
      } else if (xobj->object_class == JSBOOLEAN) {
        xval = xobj->shared.prim_bool;
      } else if (xobj->object_class == JSSTRING) {
        bool ok2Convert = true;
        xval = __js_str2num2(xobj->shared.prim_string, ok2Convert, true);
        if (!ok2Convert) {
          return __nan_value();
        }
      } else if (xobj->object_class == JSOBJECT) {
        __jsvalue xjsval = __object_internal_DefaultValue(xobj, JSTYPE_NUMBER);
        if (__is_undefined(&xjsval) || __is_string(&xjsval)) {
          return __nan_value();
        } else {
          xval = xjsval.s.i32;
        }
      } else if (xobj->object_class == JSFUNCTION) {
        return __nan_value();
      } else {
        assert(false && "nyi");
      }
      if (__is_js_object(y)) {
        __jsobject *yobj = __jsval_to_object(y);
        if (yobj->object_class == JSNUMBER) {
          int32_t yval = __js_ToNumber(y);
          int32_t res = xval % yval;
          return __number_value(res);
        } else if (yobj->object_class == JSBOOLEAN) {
          int32_t yval = yobj->shared.prim_bool;
          int32_t res = xval % yval;
          return __number_value(res);
        } else if (yobj->object_class == JSSTRING) {
          bool ok2Convert = true;
          int32_t yval = __js_str2num2(yobj->shared.prim_string, ok2Convert, true);
          if (!ok2Convert) {
            return __nan_value();
          } else {
            int32_t res = xval % yval;
            return __number_value(res);
          }
        } else if (yobj->object_class == JSOBJECT) {
          __jsvalue yjsval = __object_internal_DefaultValue(yobj, JSTYPE_NUMBER);
          if (__is_undefined(&yjsval) || __is_string(&yjsval)) {
            return __nan_value();
          } else {
            int32_t yval = yjsval.s.i32;
            int32_t res = x->s.boo % yval;
            return __number_value(res);
          }
        } else if (yobj->object_class == JSFUNCTION) {
          return __nan_value();
        } else {
          assert(false && "nyi");
        }
      } else if (__is_number(y) || __is_boolean(y)) {
        if (y->s.i32 == 0) {
          return xval >= 0 ? __number_infinity() : __number_neg_infinity();
        } else {
          return __number_value(xval % y->s.i32);
        }
      } else if (__is_null(y)) {
        return xval >= 0 ? __number_infinity() : __number_neg_infinity();
      } else if (__is_undefined(y)) {
        return __nan_value();
      } else {
        assert(false && "nyi");
      }
    }
    case JSTYPE_NULL: {
      return __number_value(0);
    }
    default:
      assert(false && "nyi");
  }
}

__jsvalue __jsop_object_sub(__jsvalue *x, __jsvalue *y) {
  switch (x->tag) {
    case JSTYPE_NUMBER:
    case JSTYPE_BOOLEAN: {
      if (__is_js_object(y)) {
        bool isConvertible = false;
        __jsvalue yval = __js_ToNumberSlow2(y, isConvertible);
        if (!isConvertible)
          return __nan_value();
        if (__is_double(&yval)) {
          double opy = (yval.s.f64);
          return __double_value((double)x->s.i32 - opy);
        } else {
          return __number_value(x->s.i32 - yval.s.i32);
        }
      } else {
        assert(false && "nyi");
      }
    }
    case JSTYPE_OBJECT: {
      bool isXConvertible = false;
      __jsvalue xval = __js_ToNumberSlow2(x, isXConvertible);
      if (!isXConvertible)
        return __nan_value();
      if (__is_js_object(y)) {
        bool isYConvertible = false;
        __jsvalue yval = __js_ToNumberSlow2(y, isYConvertible);
        if (!isYConvertible)
          return __nan_value();
        if (__is_double(&xval) && __is_double(&yval)) {
          double opx = (xval.s.f64);
          double opy = yval.s.f64;
          return __double_value(opx - opy);
        } else if (__is_double(&xval)) {
          double opx = (xval.s.f64);
          return __double_value(opx - yval.s.i32);
        } else if (__is_double(&yval)) {
          double opy = (yval.s.f64);
          return __double_value(xval.s.i32 - opy);
        } else {
          return __number_value(xval.s.i32 - yval.s.i32);
        }
      } else if (__is_number(y) || __is_boolean(y)) {
        return __is_double(&xval) ? __double_value((xval.s.f64) - y->s.i32) :
                 __number_value(xval.s.i32 - y->s.i32);
      } else if (__is_undefined(y)) {
        return __nan_value();
      } else if (__is_null(y)) {
        return xval;
      } else {
        assert(false && "nyi");
      }
    }
    case JSTYPE_NULL: {
      if (__is_js_object(y)) {
        bool isConvertible = false;
        __jsvalue yval = __js_ToNumberSlow2(y, isConvertible);
        if (!isConvertible)
          return __nan_value();
        if (__is_double(&yval)) {
          double opy = (yval.s.f64);
          return __double_value(-opy);
        } else {
          return __number_value(- yval.s.i32);
        }
      } else {
        assert(false && "should be object");
      }
    }
    default:
      assert(false && "nyi");
  }
}

__jsvalue __jsop_object_mul(__jsvalue *x, __jsvalue *y) {
  if (__is_null(x) || __is_null(y)) {
    return __number_value(0);
  }
  switch (x->tag) {
    case JSTYPE_BOOLEAN: {
      if (__is_js_object(y)) {
        __jsobject *yobj = __jsval_to_object(y);
        if (yobj->object_class == JSNUMBER) {
          int32_t yval = __js_ToNumber(y);
          int32_t res = x->s.boo * yval;
          return __number_value(res);
        } else if (yobj->object_class == JSBOOLEAN) {
          int32_t yval = yobj->shared.prim_bool;
          int32_t res = x->s.boo * yval;
          return __number_value(res);
        } else if (yobj->object_class == JSSTRING) {
          int32_t yval = __js_str2num(yobj->shared.prim_string);
          int32_t res = x->s.boo * yval;
          return __number_value(res);
        } else if (yobj->object_class == JSOBJECT) {
          __jsvalue yjsval = __object_internal_DefaultValue(yobj, JSTYPE_NUMBER);
          if (__is_undefined(&yjsval) || __is_string(&yjsval)) {
            return __nan_value();
          } else {
            int32_t yval = yjsval.s.i32;
            int32_t res = x->s.i32 * yval;
            return __number_value(res);
          }
        } else if (yobj->object_class == JSFUNCTION) {
          return __nan_value();
        } else {
          assert(false && "nyi");
        }
      } else {
        assert(false && "nyi");
      }
    }
    case JSTYPE_NUMBER: {
      if (__is_js_object(y)) {
        __jsobject *yobj = __jsval_to_object(y);
        if (yobj->object_class == JSNUMBER) {
          int32_t yval = __js_ToNumber(y);
          int32_t res = x->s.boo * yval;
          return __number_value(res);
        } else if (yobj->object_class == JSBOOLEAN) {
          int32_t yval = yobj->shared.prim_bool;
          int32_t res = x->s.boo * yval;
          return __number_value(res);
        } else if (yobj->object_class == JSSTRING) {
          int32_t yval = __js_str2num(yobj->shared.prim_string);
          int32_t res = x->s.boo * yval;
          return __number_value(res);
        } else if (yobj->object_class == JSOBJECT) {
          __jsvalue yjsval = __object_internal_DefaultValue(yobj, JSTYPE_NUMBER);
          if (__is_undefined(&yjsval) || __is_string(&yjsval)) {
            return __nan_value();
          } else {
            int32_t yval = yjsval.s.i32;
            int32_t res = x->s.i32 * yval;
            return __number_value(res);
          }
        } else if (yobj->object_class == JSFUNCTION) {
          return __nan_value();
        } else {
          assert(false && "nyi");
        }
      } else {
        assert(false && "nyi");
      }
    }
    case JSTYPE_OBJECT: {
      __jsobject *xobj = __jsval_to_object(x);
      int32_t xval = 0;
      if (xobj->object_class == JSNUMBER) {
        xval = __js_ToNumber(x);
      } else if (xobj->object_class == JSBOOLEAN) {
        xval = xobj->shared.prim_bool;
      } else if (xobj->object_class == JSSTRING) {
        xval = __js_str2num(xobj->shared.prim_string);
      } else if (xobj->object_class == JSOBJECT) {
        __jsvalue xjsval = __object_internal_DefaultValue(xobj, JSTYPE_NUMBER);
        if (__is_undefined(&xjsval) || __is_string(&xjsval)) {
          return __nan_value();
        } else {
          xval = xjsval.s.i32;
        }
      } else if (xobj->object_class == JSFUNCTION) {
        return __nan_value();
      } else {
        assert(false && "nyi");
      }
      if (__is_js_object(y)) {
        __jsobject *yobj = __jsval_to_object(y);
        if (yobj->object_class == JSNUMBER) {
          int32_t yval = __js_ToNumber(y);
          int32_t res = xval * yval;
          return __number_value(res);
        } else if (yobj->object_class == JSBOOLEAN) {
          int32_t yval = yobj->shared.prim_bool;
          int32_t res = xval * yval;
          return __number_value(res);
        } else if (yobj->object_class == JSSTRING) {
          int32_t yval = __js_str2num(yobj->shared.prim_string);
          int32_t res = xval * yval;
          return __number_value(res);
        } else if (yobj->object_class == JSOBJECT) {
          __jsvalue yjsval = __object_internal_DefaultValue(yobj, JSTYPE_NUMBER);
          if (__is_undefined(&yjsval) || __is_string(&yjsval)) {
            return __nan_value();
          } else {
            int32_t yval = yjsval.s.i32;
            int32_t res = xval * yval;
            return __number_value(res);
          }
        } else if (yobj->object_class == JSFUNCTION) {
          return __nan_value();
        } else {
          assert(false && "nyi");
        }
      } else if (__is_number(y) || __is_boolean(y)) {
        return __number_value(y->s.i32 * xval);
      } else {
        assert(false && "nyi");
      }
    }
    default:
      assert(false && "nyi");
  }
}

// ecma 11.8.5
bool __js_AbstractRelationalComparison(__jsvalue *x, __jsvalue *y, bool &isAlwaysFalse, bool leftFirst) {
  if (__is_number(x) && __is_number(y)) {
    return __jsval_to_number(x) < __jsval_to_number(y);
  }

  if (__is_double(x) && __is_double(y)) {
    return __jsval_to_double(x) < __jsval_to_double(y);
  } else if (__is_double(x) && __is_number(y)) {
    double dy = (double)__jsval_to_number(y);
    return __jsval_to_double(x) < dy;
  } else if (__is_number(x) && __is_double(y)) {
    double dx = (double)__jsval_to_number(x);
    return dx < __jsval_to_double(y);
  }
  // handle special
  if (__is_undefined(x) || __is_undefined(y)) {
    isAlwaysFalse = true;
    return false;
  }
  if (__is_infinity(x) && __is_infinity(y)) {
    return (__is_neg_infinity(x) && !__is_neg_infinity(y));
  }
  if (__is_infinity(x)) {
    return __is_neg_infinity(x);
  }
  if (__is_infinity(y)) {
    return !__is_neg_infinity(y);
  }

  // ecma 11.8.5 step 1.
  __jsvalue px;
  __jsvalue py;
  if (leftFirst) {
    px = __js_ToPrimitive(x, JSTYPE_NUMBER);
    py = __js_ToPrimitive(y, JSTYPE_NUMBER);
  } else {
    py = __js_ToPrimitive(y, JSTYPE_NUMBER);
    px = __js_ToPrimitive(x, JSTYPE_NUMBER);
  }
  // ecma 11.8.5 step 3.
  if (!(__is_string(&px) && __is_string(&py))) {
    bool okToCvt1 = false;
    bool okToCvt2 = false;
    __jsvalue jsv1 = __js_ToNumber2(&px, okToCvt1);
    __jsvalue jsv2 = __js_ToNumber2(&py, okToCvt2);
    if (!okToCvt1 || !okToCvt2) {
      isAlwaysFalse = true;
      return false;
    }
    double v1 = __is_double(&jsv1) ? __jsval_to_double(&jsv1) : __jsval_to_number(&jsv1);
    double v2 = __is_double(&jsv2) ? __jsval_to_double(&jsv2) : __jsval_to_number(&jsv2);
    return v1 < v2;
  } else { /* ecma 11.8.5 step 4. */
    __jsstring *str1 = __jsval_to_string(&px);
    __jsstring *str2 = __jsval_to_string(&py);
    int32_t value = __jsstr_compare(str1, str2);
    return value < 0;
  }
}

// ecma 11.8.6
bool __jsop_instanceof(__jsvalue *x, __jsvalue *y) {
  if (!__js_Impl_HasInstance(y)) {
    MAPLE_JS_TYPEERROR_EXCEPTION();
  }
  __jsobject *rval = __jsval_to_object(y);
  __jsfunction *fun = rval->shared.fun;
  if (fun) {
    while (fun->attrs & 0xff & JSFUNCPROP_BOUND) {
      rval = (__jsobject *)fun->fp;
      fun = rval->shared.fun;
      if (!fun) {
        break;
      }
    }
  }
  return __jsfun_internal_HasInstance(rval, x);
}

// ecma 11.8.7
bool __jsop_in(__jsvalue *x, __jsvalue *y) {
  if (__jsval_typeof(y) != JSTYPE_OBJECT) {
    MAPLE_JS_TYPEERROR_EXCEPTION();
  }
  __jsobject *rval = __jsval_to_object(y);
  __jsstring *p = __js_ToString(x);
  bool res = __jsobj_internal_HasProperty(rval, p);
  if (!__is_string(x)) {
    memory_manager->RecallString(p);
  }
  return res;
}

bool __js_StrictEquality(__jsvalue *x, __jsvalue *y);

// ecma 11.9.3
bool __js_AbstractEquality(__jsvalue *x, __jsvalue *y, bool &isAlwaysFalse) {
  // handle special value first
  if (__is_infinity(x) || __is_infinity(y)) {
    if (__is_neg_infinity(x) && __is_neg_infinity(y))
      return true;
    if (__is_infinity(x) && __is_infinity(y))
      return true;
    if (__is_infinity(x) && __is_js_object(y)) {
      __jsvalue prim_value = __js_ToPrimitive2(y);
      GCCheckAndIncRf(prim_value.s.asbits, IsNeedRc(prim_value.tag));
      bool res = __js_AbstractEquality(x, &prim_value, isAlwaysFalse);
      GCCheckAndDecRf(prim_value.s.asbits, IsNeedRc(prim_value.tag));
      return res;
    }
    if (__is_infinity(y) && __is_js_object(x)) {
      return __js_AbstractEquality(y, x, isAlwaysFalse);
    }
    return false;
  }
  // ecma 11.9.3 step 1.
  if (__jsval_typeof(x) == __jsval_typeof(y)) {
    return __js_SameValue(x, y);
  }
  // ecma 11.9.3 step 2~3.
  if ((__is_null(x) && __is_undefined(y)) || (__is_undefined(x) && __is_null(y))) {
    return true;
  }
  if ((__is_number(x) && __is_double(y)) || (__is_number(y) && __is_double(x))) {
    return __js_StrictEquality(x, y);
  }
  // ecma 11.9.3 step 4.
  if ((__is_number(x) || __is_double(x)) && __is_string(y)) {
    bool isConvertable = false;
    __jsvalue num_y = __js_ToNumber2(y, isConvertable);
    return __js_AbstractEquality(x, &num_y, isAlwaysFalse);
  }
  // ecma 11.9.3 step 5.
  if (__is_string(x) && (__is_number(y) || __is_double(y))) {
    bool isConvertable = false;
    __jsvalue num_x = __js_ToNumber2(x, isConvertable);
    return __js_AbstractEquality(&num_x, y, isAlwaysFalse);
  }
  // ecma 11.9.3 step 6.
  if (__is_boolean(x)) {
    __jsvalue num_x = __number_value(__js_ToNumber(x));
    return __js_AbstractEquality(&num_x, y, isAlwaysFalse);
  }
  // ecma 11.9.3 step 7.
  if (__is_boolean(y)) {
    __jsvalue num_y = __number_value(__js_ToNumber(y));
    return __js_AbstractEquality(x, &num_y, isAlwaysFalse);
  }
  // ecma 11.9.3 step 8.
  if ((__is_number(x) || __is_string(x) || __is_double(x)) && __is_js_object(y)) {
    __jsvalue prim_value = __js_ToPrimitive2(y);
    GCCheckAndIncRf(prim_value.s.asbits, IsNeedRc((prim_value.tag)));
    bool res = __js_AbstractEquality(x, &prim_value, isAlwaysFalse);
    GCCheckAndDecRf(prim_value.s.asbits, IsNeedRc(prim_value.tag));
    return res;
  }
  // ecma 11.9.3 step 9.
  if ((__is_number(y) || __is_string(y) || __is_double(y)) && __is_js_object(x)) {
    __jsvalue prim_value = __js_ToPrimitive2(x);
    GCCheckAndIncRf(prim_value.s.asbits, IsNeedRc(prim_value.tag));
    bool res = __js_AbstractEquality(&prim_value, y, isAlwaysFalse);
    GCCheckAndDecRf(prim_value.s.asbits, IsNeedRc(prim_value.tag));
    return res;
  }
  // ecma 11.9.3 step 10.
  return false;
}

// ecma 11.9.6
// This algorithm differs from the SameValue Algorithm (9.12) in its
// treatment of signed zeroes and NaNs, So if the target does not support
// float just return __js_SameValue(x, y);
bool __js_StrictEquality(__jsvalue *x, __jsvalue *y) {
  if (__is_nan(x) || __is_nan(y))
    return false;
  if (__is_negative_zero(x)) {
    __jsvalue zeroval = __positive_zero_value();
    return __js_StrictEquality(&zeroval, y);
  } else if (__is_negative_zero(y)) {
    __jsvalue zeroval = __positive_zero_value();
    return __js_StrictEquality(x, &zeroval);
  } else if (__is_double(x) && __is_number(y)) {
    double db1 = __jsval_to_double(x);
    double db2 = (double) y->s.i32;
    return (fabs(db1 - db2) < NumberMinValue);
  }
  else if(__is_double(y) && __is_number(x)) {
    double db1 = __jsval_to_double(y);
    double db2 = (double) x->s.i32;
    return (fabs(db1 - db2) < NumberMinValue);
  } else if (__is_double(x) && __is_double(y)) {
    return (fabs(__jsval_to_double(x) - __jsval_to_double(y)) < NumberMinValue);
  } else if ((__is_boolean(x) && !__is_boolean(y)) || (!__is_boolean(x) && __is_boolean(y)))
      return false;
  else if ((__is_none(x) && __is_undefined(y)) || (__is_none(y) && __is_undefined(x)))
     return true;
  return __js_SameValue(x, y);
}

bool __jsop_stricteq(__jsvalue *x, __jsvalue *y) {
  return __js_StrictEquality(x, y);
}

bool __jsop_strictne(__jsvalue *x, __jsvalue *y) {
  return !__js_StrictEquality(x, y);
}

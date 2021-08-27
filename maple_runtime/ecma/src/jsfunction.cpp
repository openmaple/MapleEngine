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

#include "jsfunction.h"
#include "jsvalue.h"
#include "jsvalueinline.h"
#include "jsobject.h"
#include "jsobjectinline.h"
#include "jscontext.h"
#include "jsbinary.h"
#include "vmmemory.h"
#include "cmpl.h"
#include "mfunction.h"
#include "mshimdyn.h"
#include "jsdate.h"
#include "jsglobal.h"

__jsobject *__create_function(void *fp, void *env, uint32_t attrs, int32_t fileIdx) {
  __jsobject *f = __create_object();
  f->object_class = JSFUNCTION;
  f->extensible = true;
  __jsobj_set_prototype(f, JSBUILTIN_FUNCTIONPROTOTYPE);
  // Implementation depends.
  __jsfunction *fun = (__jsfunction *)VMMallocGC(sizeof(__jsfunction), MemHeadJSFunc);
  fun->fp = fp;
  if (attrs & 0xff & JSFUNCPROP_BOUND) {
    GCIncRf(fp);
  }
  fun->env = env;
  // TODO: no object or string here?
  if (env && memory_manager->GetMemHeader(env).memheadtag == MemHeadEnv) {
    // %enviroment needs GC record
    GCIncRf(env);
  }
  fun->attrs = attrs;
  fun->fileIndex = fileIdx;
  f->shared.fun = fun;
  return f;
}

// ecma 13.2: Creating Function Objects
// If varg_p, pass nargs as -nargs-1.
__jsvalue __js_new_function(void *fp, void *env, uint32_t attrs, int32_t fileIdx, bool needpt) {
  // ecma 13.2 step 1/3/4/13、15
  __jsobject *f = __create_function(fp, env, attrs, fileIdx);
  // ecma 13.2 step 16.
  __jsobject *proto = __js_new_obj_obj_0();
  // ecma 13.2 step 17.
  /* Avoid dead reference count between f and proto in GC.  */
  // ecma 13.2 step 18.
  __jsvalue length_value = __number_value((attrs >> 8) & 0xff);
  __jsobj_helper_init_value_property(f, JSBUILTIN_STRING_LENGTH, &length_value, JSPROP_DESC_HAS_VUWUEC);

  if (needpt) {
    __jsvalue proto_value = __object_value(proto);
    __jsobj_helper_init_value_property(f, JSBUILTIN_STRING_PROTOTYPE, &proto_value, JSPROP_DESC_HAS_VWUEUC);
    __jsvalue func_value = __object_value(f);
    __jsobj_helper_init_value_property(proto, JSBUILTIN_STRING_CONSTRUCTOR, &func_value, JSPROP_DESC_HAS_VWUEUC);
  }
  return __object_value(f);
}

// ecma 15.3.2.1 Function (p1, p2, … , pn, body)
__jsvalue __js_new_functionN(void *fp, __jsvalue *arg_list, uint32_t argnum) {
  MIR_ASSERT(0 && "__js_new_functionN NIY");
  return __undefined_value();
}

__jsvalue __js_entry_function(__jsvalue *this_arg, bool strict_p) {
  __jsvalue old_this = __js_ThisBinding;

  if (strict_p) {
    __js_ThisBinding = *this_arg;
  } else if (__is_null_or_undefined(this_arg)) {
    __js_ThisBinding = __object_value(__jsobj_get_or_create_builtin(JSBUILTIN_GLOBALOBJECT));
  } else if (!__is_js_object(this_arg)) {
    __js_ThisBinding = __object_value(__js_ToObject(this_arg));
#ifdef MACHINE64
    // __jsobject *obj = (__jsobject *)memory_manager->GetRealAddr(__js_ThisBinding.x.payload.obj);
    __jsobject *obj = __js_ThisBinding.x.obj;
#else
    __jsobject *obj = __js_ThisBinding.x.payload.obj;
#endif
    GCIncRf(obj);
  } else {
    __js_ThisBinding = *this_arg;
  }

  return old_this;
}

void __js_exit_function(__jsvalue *this_arg, __jsvalue old_this, bool strict_p) {
  if (!__is_js_object(this_arg) && !__is_null_or_undefined(this_arg) && !strict_p) {
#ifdef MACHINE64
    // __jsobject *obj = (__jsobject *)memory_manager->GetRealAddr(__js_ThisBinding.x.payload.obj);
    __jsobject *obj = (__js_ThisBinding.x.obj);
#else
    __jsobject *obj = __js_ThisBinding.x.payload.obj;
#endif
    GCDecRf(obj);
  }
  __js_ThisBinding = old_this;
}

// ecma 13.2.1
// ??? To be finished, see 13.2.1
__jsvalue __jsfun_internal_call(__jsobject *f, __jsvalue *this_arg, __jsvalue *arg_list, uint32_t arg_count, __jsvalue *origArg) {
  MIR_ASSERT(f != NULL);
  __jsfunction *fun = f->shared.fun;
  MIR_ASSERT(fun != NULL);
  uint32_t attrs = fun->attrs;
  bool varg_p = attrs >> 24;
  uint8_t flag = attrs & 0xff;
  uint8_t nargs = attrs >> 16 & 0xff;
  __jsvalue return_value;

  // MIR_ASSERT(nargs >= 0);
  bool isCalleeStrict = flag & JSFUNCPROP_STRICT;
  __jsvalue old_this = __js_entry_function((isCalleeStrict && origArg) ? origArg : this_arg, isCalleeStrict);
  // user function calls need to be interpreted
  if (flag & JSFUNCPROP_USERFUNC) {
     MValue ret = gInterSource->FuncCall_JS(f, this_arg, fun->env, arg_list, (int32_t)arg_count);
     __js_exit_function(this_arg, old_this, flag & JSFUNCPROP_STRICT);
     if (ret.x.u64 == (uint64_t)Exec_handle_exc) {
       throw "callee exception";
     }
     ret = gInterSource->retVal0;
     mDecode(ret);
     return ret;
  }

  // varg_p
  if (varg_p) {
    if(fun->fp != __js_new_function) {
      __jsvalue (*fp)(__jsvalue *, __jsvalue *, uint32_t) = (__jsvalue(*)(__jsvalue *, __jsvalue *, uint32_t))fun->fp;
      return_value = (*fp)(this_arg, arg_list, arg_count);
    } else
      return_value = __js_new_function(this_arg, arg_list, arg_count);
    __js_exit_function(this_arg, old_this, flag & JSFUNCPROP_STRICT);
    return return_value;
  }

  MAPLE_JS_ASSERT(nargs <= 3 && "NYI");
  __jsvalue arguments[3];
  if (arg_count < (uint32_t)nargs) {
    for (uint32_t i = 0; i < arg_count; i++) {
      arguments[i] = arg_list[i];
      // check argument is defined or not
      if (__is_none(&arguments[i])) {
        MAPLE_JS_REFERENCEERROR_EXCEPTION();
      }
    }
    for (uint32_t i = arg_count; i < (uint32_t)nargs; i++) {
      arguments[i] = __undefined_value();
    }
  } else {
    for (uint32_t i = 0; i < (uint32_t)nargs; i++) {
      arguments[i] = arg_list[i];
    }
  }

  switch (nargs) {
    case 0: {
      __jsvalue (*fp)(__jsvalue *) = (__jsvalue(*)(__jsvalue *))fun->fp;
      return_value = (*fp)(this_arg);
      break;
    }
    case 1: {
      __jsvalue (*fp)(__jsvalue *, __jsvalue *) = (__jsvalue(*)(__jsvalue *, __jsvalue *))fun->fp;
      return_value = (*fp)(this_arg, &arguments[0]);
      break;
    }
    case 2: {
      __jsvalue (*fp)(__jsvalue *, __jsvalue *, __jsvalue *) =
        (__jsvalue(*)(__jsvalue *, __jsvalue *, __jsvalue *))fun->fp;
      return_value = (*fp)(this_arg, &arguments[0], &arguments[1]);
      break;
    }
    case 3: {
      __jsvalue (*fp)(__jsvalue *, __jsvalue *, __jsvalue *, __jsvalue *) =
        (__jsvalue(*)(__jsvalue *, __jsvalue *, __jsvalue *, __jsvalue *))fun->fp;
      return_value = (*fp)(this_arg, &arguments[0], &arguments[1], &arguments[2]);
      break;
    }
    default:
      MAPLE_JS_EXCEPTION(false && "NYI");
  }

  __js_exit_function(this_arg, old_this, flag & JSFUNCPROP_STRICT);
  return return_value;
}

// Helper function for internal use.
__jsvalue __jsfun_val_call(__jsvalue *function, __jsvalue *this_arg, __jsvalue *arg_list, uint32_t arg_count) {
  if (!__js_IsCallable(function)) {
    MAPLE_JS_TYPEERROR_EXCEPTION();
  }
  __jsobject *f = __jsval_to_object(function);
  __jsvalue result = __jsfun_internal_call(f, this_arg, arg_list, arg_count);
  return result;
}

// ecma 13.2.2
__jsvalue __jsfun_intr_Construct(__jsobject *f, __jsvalue *this_arg, __jsvalue *arg_list, uint32_t arg_count) {
  __jsfunction *fun = f->shared.fun;
  uint32_t attrs = fun->attrs;
  uint8_t flag = attrs & 0xff;
  __jsvalue reverse_args[MAXCALLARGNUM];
  __jsvalue args[MAXCALLARGNUM];
  uint32_t n = 0;
  uint32_t sum_nargs = 0;
  if (flag & JSFUNCPROP_NATIVE && !(flag & JSFUNCPROP_CONSTRUCTOR)) {
    // is a native function but not a constructor, throw exception
    MAPLE_JS_TYPEERROR_EXCEPTION();
  }
  if (flag & JSFUNCPROP_BOUND) {
    while (flag & JSFUNCPROP_BOUND) {
      int8_t nargs = (fun->attrs >> 16 & 0xff) - 1;
      if (nargs >= 0) {
        sum_nargs += nargs;
      }
      __jsvalue *bound_args = &((__jsvalue *)fun->env)[1];
      MIR_ASSERT(arg_count + sum_nargs <= MAXCALLARGNUM);
      for (int32_t i = 0; i < nargs; i++) {
        reverse_args[n++] = bound_args[i];
      }
      f = (__jsobject *)fun->fp;
      fun = f->shared.fun;
      attrs = fun->attrs;
      flag = attrs & 0xff;
    }
    for (uint32_t i = 0; i < n; i++) {
      args[i] = reverse_args[n - i - 1];
    }
  }
  // reset date constructor function which returns string, return date object in constructor
  if ((flag & JSFUNCPROP_CONSTRUCTOR) && fun && (fun->fp == __js_new_dateconstructor)) {
    fun->fp = (void *)__js_new_date_obj;
  }
  for (uint32_t i = 0; i < arg_count; i++) {
    args[n++] = arg_list[i];
  }
  arg_count = n;

  // ecma 13.2 step 1~7.
  __jsobject *obj = __js_new_obj_obj_0();
  __jsvalue proto = __jsobj_internal_Get(f, JSBUILTIN_STRING_PROTOTYPE);
  if (__is_js_object(&proto)) {
    __jsobj_set_prototype(obj, __jsval_to_object(&proto));
  } else {
    __jsobj_set_prototype(obj, JSBUILTIN_OBJECTPROTOTYPE);
  }
  // ecma 13.2 step 8.
  __jsvalue o = __object_value(obj);
  GCIncRf(obj);

  __jsvalue result = __jsfun_internal_call(f, &o, args, arg_count);

  // ecma 13.2 step 9.
  if (__is_js_object(&result)) {
    GCDecRf(obj);
    return result;
  } else if (((flag & JSFUNCPROP_USERFUNC) == 0) && __is_primitive(&result)) {
    // wrap primitive result with object
    __jsobject *resobj = __js_ToObject(&result);
    GCDecRf(obj);
    return __object_value(resobj);
  } else { /*ecma 13.2 step 10. */
    GCDecRfNoRecall(obj);
    return o;
  }
}

// ecma 15.3.4.3
__jsvalue __jsfun_pt_apply(__jsvalue *function, __jsvalue *this_arg, __jsvalue *arg_array) {
  // ecma 15.3.4.3 step 1.
  if (!__js_IsCallable(function)) {
    MAPLE_JS_TYPEERROR_EXCEPTION();
  }
  // ecma 15.3.4.3 step 2.
  __jsobject *func = __jsval_to_object(function);
  if (__is_null(arg_array) || __is_undefined(arg_array)) {
    return __jsfun_internal_call(func, this_arg, NULL, 0);
  }
  // ecma 15.3.4.3 step 3.
  if (__jsval_typeof(arg_array) != JSTYPE_OBJECT) {
    MAPLE_JS_TYPEERROR_EXCEPTION();
  }
  // ecma 15.3.4.3 step 4.
  __jsobject *arr_obj = __jsval_to_object(arg_array);
  // ecma 15.3.4.3 step 5.
  uint32_t n = __jsobj_helper_get_length(arr_obj);
  // ecma 15.3.4.3 step 6.
  __jsvalue *arg_list = (__jsvalue *)VMMallocNOGC(n * sizeof(__jsvalue));
  // ecma 15.3.4.3 step 7.
  uint32_t index = 0;
  // ecma 15.3.4.3 step 8.
  while (index < n) {
    __jsvalue next_arg = __jsobj_internal_Get(arr_obj, index);
    arg_list[index] = next_arg;
    index++;
  }
  // ecma 15.3.4.3 step 9.
  __jsvalue result = __jsfun_internal_call(func, this_arg, arg_list, n);
  VMFreeNOGC(arg_list, n * sizeof(__jsvalue));
  return result;
}

// ecma 15.3.4.4
__jsvalue __jsfun_pt_call(__jsvalue *function, __jsvalue *args, uint32_t arg_count) {
  // ecma 15.3.4.4 step 1.
  if (!__js_IsCallable(function)) {
    MAPLE_JS_TYPEERROR_EXCEPTION();
  }
  // ecma 15.3.4.4 step 2~4.
  __jsobject *f = __jsval_to_object(function);
  __jsvalue this_arg;
  __jsvalue result;
  if (arg_count == 0) {
    this_arg = __undefined_value();
    result = __jsfun_internal_call(f, &this_arg, NULL, 0);
  } else {
    result = __jsfun_internal_call(f, &args[0], &args[1], arg_count - 1);
  }
  return result;
}

// ecma 15.3.4.5
__jsvalue __jsfun_pt_bind(__jsvalue *function, __jsvalue *args, uint32_t arg_count) {
  // ecma 15.3.4.5 step 1.
  __jsvalue target = __js_ThisBinding;
  // ecma 15.3.4.5 step 2.
  if (!__js_IsCallable(&target)) {
    MAPLE_JS_TYPEERROR_EXCEPTION();
  }
  // ecma 15.3.4.5 step 3.
  // ecma 15.3.4.5 step 4....
  __jsvalue *bound_args = NULL;
  if (arg_count > 0) {
    bound_args = (__jsvalue *)VMMallocGC(arg_count * sizeof(__jsvalue));
  }
  for (uint32_t i = 0; i < arg_count; i++) {
#ifdef RC_NO_MMAP
    GCCheckAndUpdateRf(bound_args[i].x.asbits, IsNeedRc(bound_args[i].ptyp), args[i].x.asbits, IsNeedRc(args[i].ptyp));
#endif
    bound_args[i] = args[i];
  }

  uint32_t attr = arg_count << 16 | (uint32_t)JSFUNCPROP_BOUND;

  // ecma 15.3.4.5 step 15~16.
  uint32_t len = 0;
  if (__is_js_function(&__js_ThisBinding)) {
    arg_count = arg_count > 0 ? (arg_count - 1) : 0;
    if (__jsop_length(&__js_ThisBinding) > arg_count) {
      len = (int32_t)__jsop_length(&__js_ThisBinding) - arg_count;
    }
  }
  __jsvalue len_val = __number_value(len);
  attr = attr | (len << 8);
  __jsobject *f = __create_function((void *)__jsval_to_object(&target), (void *)bound_args, attr, -1);

  // ecma 15.3.4.5 step 17.
  __jsobj_helper_init_value_property(f, JSBUILTIN_STRING_LENGTH, &len_val, JSPROP_DESC_HAS_VUWUEC);
  // ecma 15.3.4.5 step 22....
  return __object_value(f);
}

// ecma 15.3.5.3
bool __jsfun_internal_HasInstance(__jsobject *f, __jsvalue *v) {
  // ecma 15.3.5.3 step 1.
  if (!__is_js_object(v)) {
    return false;
  }
  // ecma 15.3.5.3 step 2.
  __jsvalue o = __jsobj_internal_Get(f, JSBUILTIN_STRING_PROTOTYPE);
  // ecma 15.3.5.3 step 3.
  if (__jsval_typeof(&o) != JSTYPE_OBJECT) {
    MAPLE_JS_TYPEERROR_EXCEPTION();
  }
  // ecma 15.3.5.3 step 4.
  __jsobject *obj = __jsval_to_object(v);
  __jsobject *pt = __jsobj_get_prototype(obj);
  __jsobject *objO = __jsval_to_object(&o);
  while (true) {
    if (!pt) {
      return false;
    }
    if (objO == pt) {
      return true;
    }
    pt = __jsobj_get_prototype(pt);
  }
}

bool __js_Impl_HasInstance(__jsvalue *v) {
  // Object
  if(!__is_js_object(v)) {
      MAPLE_JS_TYPEERROR_EXCEPTION();
  }
  // Only Function objects implement [[HasInstance]].
  // If the argument object is a Function object,
  // then return true, otherwise return false.
  return __is_js_function(v);
}

// ecma 15.3.4.2 Function.prototype.toString
__jsvalue __jsfun_pt_tostring(__jsvalue *function) {
  __jsstring *str;
  if (!__js_IsCallable(function)) {
    MAPLE_JS_TYPEERROR_EXCEPTION();
  }
  str = __jsstr_new_from_char("[object Undefined]");
  return __string_value(str);
}

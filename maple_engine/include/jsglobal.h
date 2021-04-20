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

#ifndef JSGLOBAL_H
#define JSGLOBAL_H
#include "jsvalue.h"
// ecma 15.1.4.1 Object ( . . . ), See 15.2.1 and 15.2.2.
// ecma 15.2.1
// ecma 15.2.2
__jsobject *__js_new_obj_obj_0();
__jsobject *__js_new_obj_obj_1(__jsvalue *);
__jsvalue __js_new_obj(__jsvalue *, __jsvalue *, uint32_t);
__jsvalue __js_new_arr(__jsvalue *, __jsvalue *, uint32_t);
// ecma 15.5.2
__jsvalue __js_new_str_obj(__jsvalue *, __jsvalue *, uint32_t);
// 15.1.4.5 Boolean ( . . . ), See 15.6.1 and 15.6.2.
// For 15.6.1 Use __js_ToBoolean instead.
// ecma 15.6.2
__jsvalue __js_new_boo_obj(__jsvalue *, __jsvalue *, uint32_t );
// 15.1.4.6 Number ( . . . ), See 15.7.1 and 15.7.2.
// ecma 15.7.1  Use __js_ToNumer instead.
// ecma 15.7.2.
__jsvalue __js_new_num_obj(__jsvalue *, __jsvalue *, uint32_t);
// 15.3.4 Properties of the Function Prototype Object.
__jsvalue __js_empty(__jsvalue *, __jsvalue *, uint32_t);
__jsvalue __js_isnan(__jsvalue *, __jsvalue *, uint32_t);
__jsvalue __js_new_reference_error_obj(__jsvalue *, __jsvalue *, uint32_t);
__jsvalue __js_new_error_obj(__jsvalue *, __jsvalue *, uint32_t);
__jsvalue __js_new_evalerror_obj(__jsvalue *, __jsvalue *, uint32_t);
__jsvalue __js_new_rangeerror_obj(__jsvalue *, __jsvalue *, uint32_t);
__jsvalue __js_new_syntaxerror_obj(__jsvalue *, __jsvalue *, uint32_t);
__jsvalue __js_new_urierror_obj(__jsvalue *, __jsvalue *, uint32_t);
__jsvalue __js_new_type_error_obj(__jsvalue *, __jsvalue *, uint32_t);

__jsvalue __js_parseint(__jsvalue *, __jsvalue *, uint32_t);
__jsvalue __js_decodeuri(__jsvalue *, __jsvalue *, uint32_t);
__jsvalue __js_decodeuricomponent(__jsvalue *, __jsvalue *, uint32_t);
__jsvalue __js_parsefloat(__jsvalue *, __jsvalue *, uint32_t);
__jsvalue __js_isfinite(__jsvalue *, __jsvalue *, uint32_t);
__jsvalue __js_encodeuri(__jsvalue *, __jsvalue *, uint32_t);
__jsvalue __js_encodeuricomponent(__jsvalue *, __jsvalue *, uint32_t);
__jsvalue __js_eval();
__jsvalue __js_escape(__jsvalue *, __jsvalue *);

// 15.7.1 The Number Constructor Called as a Function
__jsvalue __js_new_numberconstructor(__jsvalue *, __jsvalue *, uint32_t);
// 15.6.1 The Boolean Constructor Called as a Function
__jsvalue __js_new_booleanconstructor(__jsvalue *, __jsvalue *, uint32_t);
// 15.5.1 The String Constructor Called as a Function
__jsvalue __js_new_stringconstructor(__jsvalue *, __jsvalue *, uint32_t);
__jsvalue __js_new_math_obj(__jsvalue *);
__jsvalue __js_new_json_obj(__jsvalue *);
#endif

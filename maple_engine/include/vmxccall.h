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

#include "types_def.h"
using namespace maple;

enum UserCType { Cty_undef, Cty_void, Cty_int, Cty_ptr };

struct CfuncNode {
  void *pfunc;
  uint32 num_args;

  UserCType retty;
  UserCType arg0ty;
  UserCType arg1ty;
  UserCType arg2ty;
  UserCType arg3ty;
};

extern CfuncNode cfuncmap[10];

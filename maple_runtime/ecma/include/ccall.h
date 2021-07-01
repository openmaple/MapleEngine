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

#ifndef MAPLEVM_INCLUDE_CCALL_H
#define MAPLEVM_INCLUDE_CCALL_H

#include "mir_config.h"
#include "vmconfig.h"
#include <cstring>
#include <cstdio>

extern "C" {

typedef int (*funcCallback)(int *);

struct function_map {
  const char *name;
  funcCallback funcptr;
};

int add(int *input);
int sub(int *input);
int add4(int *input);

funcCallback getCCallback(const char *name);
}
#endif  // MAPLEVM_INCLUDE_CCALL_H

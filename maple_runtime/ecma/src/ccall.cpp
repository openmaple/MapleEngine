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

#include "ccall.h"
#include <cstdio>
#include <ctime>

extern "C" {

// CCALL map
struct function_map ccall_map[] = {
  { "add", &add },
  { "sub", &sub },
  { "add4", &add4 },
};

funcCallback getCCallback(const char *name) {
  int i, mapsize;
  funcCallback funcptr = NULL;
  mapsize = sizeof(ccall_map) / sizeof(ccall_map[0]);
  // This lookup is too slow... do a hack in JS temporarily
  for (i = 0; i < mapsize; i++) {
    if (!strcmp(ccall_map[i].name, name)) {
      funcptr = ccall_map[i].funcptr;
    }
  }
  return funcptr;
}

int add(int *input) {
  int a = *input;
  int b = *(input + 1);
  return a + b;
}

int sub(int *input) {
  int a = *input;
  int b = *(input + 1);
  return a - b;
}

int add4(int *input) {
  return *input + *(input + 1) + *(input + 2) + *(input + 3);
}
}

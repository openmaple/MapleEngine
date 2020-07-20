/*
 * Copyright (C) [2020] Futurewei Technologies, Inc. All rights reverved.
 *
 * Licensed under the Mulan Permissive Software License v2.
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

#ifndef MAPLERE_MASSERT_H_
#define MAPLERE_MASSERT_H_

#include <cstdio>
#include <cstdlib>

#define MASSERT(cond, fmt, ...) \
    do { \
        if (!(cond)) { \
            fprintf(stderr, __FILE__ ":%d: Assert failed: " fmt "\n", __LINE__, ##__VA_ARGS__); \
            abort(); \
        } \
    } while (0)

#endif // MAPLERE_MASSERT_H_

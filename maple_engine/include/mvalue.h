/*
 * Copyright (C) [2020-2021] Futurewei Technologies, Inc. All rights reserved.
 *
 * OpenArkCompiler is licensed underthe Mulan Permissive Software License v2.
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

#ifndef MAPLERE_MVALUE_H_
#define MAPLERE_MVALUE_H_

#include <cstdint>
#include "prim_types.h"

struct __jsobject;

namespace maple {
    struct nan_encode {
        uint64_t payload:48;
        uint16_t type:15;
        uint8_t  sign:1;
    };

    struct MValue {
        union {
            // All integer primitive types are signed in Java
            uint8_t    u1;      // Java boolean
            int8_t     i8;      // Java byte
            int16_t    i16;     // Java short
            uint16_t   u16;     // Java char, defined as Unicode character
            int32_t    i32;     // Java int
            int64_t    i64;     // Java long
            float      f32;     // Java float
            double     f64;     // Java double
            uint8_t*   a64;     // Java object ref (use uint8_t* instead of void* for reference)
            uint8_t    u8;      // For operand type in OP_lshr
            uint32_t   u32;     // For operand type in OP_ge, OP_eq...
            uint64_t   u64;     // For zero-extension
            struct nan_encode c;
            uint32_t boo;
            void *ptr;
            void *str;
            __jsobject *obj;
            uint64_t asbits;
        } x;
#ifdef MACHINE64
        uint32_t ptyp;
#else
        PrimType ptyp:8;
#endif
    };

    struct TValue {
        union {
            // All integer primitive types are signed in Java
            uint8_t    u1;      // Java boolean
            int8_t     i8;      // Java byte
            int16_t    i16;     // Java short
            uint16_t   u16;     // Java char, defined as Unicode character
            int32_t    i32;     // Java int
            int64_t    i64;     // Java long
            float      f32;     // Java float
            double     f64;     // Java double
            uint8_t*   a64;     // Java object ref (use uint8_t* instead of void* for reference)
            uint8_t    u8;      // For operand type in OP_lshr
            uint32_t   u32;     // For operand type in OP_ge, OP_eq...
            uint64_t   u64;     // For zero-extension
            struct nan_encode c;
        } x;
    };
}

#endif // MAPLERE_MVALUE_H_

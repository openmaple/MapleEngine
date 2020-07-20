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

#include <cstdarg>

#include "mfunction.h"
#include "massert.h" // for MASSERT

namespace maple {

#define MPUSH(x) (shim_caller.operand_stack.at(++shim_caller.sp) = x)

// For instance method, the first argument is the "this" ref
// For static method, the first argument is its class object ref

// The returned value can be float/double also
extern "C" int64_t __engine_shim(int64_t first_arg, ...) {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wuninitialized"
#if defined(__aarch64__)
    register method_header_t *mir_start_addr __asm__ ("%x9");
    // Copy the value in register x9 into a local variable
    const method_header_t* const header = mir_start_addr;
#elif defined(__x86_64__)
    MASSERT(*(uint32_t*)first_arg == 0x494c504d, "Not a Maple mir method");
    const method_header_t* const header = (const method_header_t*)(first_arg + 4);
#else
    #error Unsuported arch.
#endif
#pragma GCC diagnostic pop

    DEBUGSYMBOL(header, "[Enter engine shim:");

    // Get the number arugements
    const uint16_t arg_num = header->formals_num;

    // Create a local MFunction object for shim
    MFunction shim_caller(header, nullptr, true);

    MValue val;
    if(arg_num > 0) {
        va_list args;
        va_start(args, first_arg);

        uint16_t arg_idx = 0;
        while(arg_idx < arg_num) {
            // Process all other argements
            val.ptyp = (PrimType)(header->primtype_table[arg_idx]);
            switch(val.ptyp) {
                case PTY_i8:
                    val.x.i8 = va_arg(args, int);
                    break;
                case PTY_i16:
                    val.x.i16 = va_arg(args, int);
                    break;
                case PTY_i32:
                    val.x.i32 = va_arg(args, int);
                    break;
                case PTY_i64:
                    val.x.i64 = va_arg(args, long long);
                    break;
                case PTY_u16:
                    val.x.u16 = va_arg(args, int);
                    break;
                case PTY_u1:
                    val.x.u1 = va_arg(args, int);
                    break;
                case PTY_a64:
                    val.x.a64 = va_arg(args, uint8_t*);
                    break;
                case PTY_f32:
                    // Variadic function expects that float arg is promoted to double
                case PTY_f64:
                    val.x.f64 = va_arg(args, double);
                    break;
                default:
                    MIR_FATAL("Unsupported PrimType %d", val.ptyp);
            }
            MPUSH(val);
            ++arg_idx;
        }
        va_end(args);
    }

    static thread_local int shim_cnt = 0;
    ++shim_cnt;
    try {
        val = maple_invoke_method(header, &shim_caller);
    } catch(const MException e) {
        if(shim_cnt == 1) {
            DEBUGSYMBOL(e, "Uncaught exception from engine shim]");
        }
        else {
            DEBUGSYMBOL(e, "Exception from engine shim]");
        }
        --shim_cnt;
        throw;
    }
    --shim_cnt;

    // See calling convention specs for more details about returning a float or double
    if(val.ptyp == PTY_f32 || val.ptyp == PTY_f64) {
#if defined(__aarch64__)
       __asm__ __volatile__("fmov d0, %0" :: "r" (val.x.f64));
#elif defined(__x86_64__)
       __asm__ __volatile__("movq %0, %%xmm0" :: "r" (val.x.f64));
#else
    #error Unsuported arch.
#endif
    }
    DEBUGSYMBOL(header, "Returned from engine shim]");
    return val.x.i64;
}

} // namespace maple

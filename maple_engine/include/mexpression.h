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

#ifndef MAPLERE_MEXPRESSION_H_
#define MAPLERE_MEXPRESSION_H_

#include <cstdint>
#include "massert.h" // for MASSERT

#define EXPRUNROP(exprop) \
    do { \
        MValue &op0 = func.operand_stack.at(func.sp); \
        MASSERT(op0.ptyp == expr.primType, "Type mismatch: 0x%02x and 0x%02x", op0.ptyp, expr.primType); \
        switch(expr.primType) { \
            case PTY_i8:  op0.x.i8  = exprop op0.x.i8;  break; \
            case PTY_i16: op0.x.i16 = exprop op0.x.i16; break; \
            case PTY_i32: op0.x.i32 = exprop op0.x.i32; break; \
            case PTY_i64: op0.x.i64 = exprop op0.x.i64; break; \
            case PTY_u16: op0.x.u16 = exprop op0.x.u16; break; \
            case PTY_u1:  op0.x.u1  = exprop op0.x.u1;  break; \
            case PTY_f32: op0.x.f32 = exprop op0.x.f32; break; \
            case PTY_f64: op0.x.f64 = exprop op0.x.f64; break; \
            default: MIR_FATAL("Unsupported PrimType %d for unary oprator %s", expr.primType, #exprop); \
        } \
    } while(0)

#define EXPRUNRINTOP(exprop) \
    do { \
        MValue &op0 = func.operand_stack.at(func.sp); \
        op0.ptyp = expr.primType; /* MASSERT(op0.ptyp == expr.ptyp, "Type mismatch: 0x%02x and 0x%02x", op0.ptyp, expr.ptyp); Workaround */ \
        switch(expr.primType) { \
            case PTY_i8:  op0.x.i8  = exprop op0.x.i8;  break; \
            case PTY_i16: op0.x.i16 = exprop op0.x.i16; break; \
            case PTY_i32: op0.x.i32 = exprop op0.x.i32; break; \
            case PTY_i64: op0.x.i64 = exprop op0.x.i64; break; \
            case PTY_u16: op0.x.u16 = exprop op0.x.u16; break; \
            case PTY_u1:  op0.x.u1  = exprop op0.x.u1;  break; \
            default: MIR_FATAL("Unsupported PrimType %d for integer unary oprator %s", expr.primType, #exprop); \
        } \
    } while(0)

#define EXPRBINOP(exprop) \
    do { \
        MValue &op1 = func.operand_stack.at(func.sp--); \
        MValue &op0 = func.operand_stack.at(func.sp); \
        MASSERT(op0.ptyp == expr.primType, "Type mismatch: 0x%02x and 0x%02x", op0.ptyp, expr.primType); \
        switch(expr.primType) { \
            case PTY_i8:  op0.x.i8  = op0.x.i8  exprop op1.x.i8;  break; \
            case PTY_i16: op0.x.i16 = op0.x.i16 exprop op1.x.i16; break; \
            case PTY_i32: op0.x.i32 = op0.x.i32 exprop op1.x.i32; break; \
            case PTY_a64: op0.x.u64 = op0.x.u64 exprop op1.x.u64; break; \
            case PTY_i64: op0.x.i64 = op0.x.i64 exprop op1.x.i64; break; \
            case PTY_u16: op0.x.u16 = op0.x.u16 exprop op1.x.u16; break; \
            case PTY_u1:  op0.x.u1 = op0.x.u1  exprop op1.x.u1;  break; \
            case PTY_f32: op0.x.f32 = op0.x.f32 exprop op1.x.f32; break; \
            case PTY_f64: op0.x.f64 = op0.x.f64 exprop op1.x.f64; break; \
            default: MIR_FATAL("Unsupported PrimType %d for binary oprator %s", expr.primType, #exprop); \
        } \
    } while(0)

#define EXPRPTRBINOP(exprop) \
    do { \
        MValue &op1 = func.operand_stack.at(func.sp--); \
        MValue &op0 = func.operand_stack.at(func.sp); \
        MASSERT(op0.ptyp == expr.primType, "Type mismatch: 0x%02x and 0x%02x", op0.ptyp, expr.primType); \
        switch(expr.primType) { \
            case PTY_i8:  op0.x.i8  = op0.x.i8  exprop op1.x.i8;  break; \
            case PTY_i16: op0.x.i16 = op0.x.i16 exprop op1.x.i16; break; \
            case PTY_i32: op0.x.i32 = op0.x.i32 exprop op1.x.i32; break; \
            case PTY_i64: op0.x.i64 = op0.x.i64 exprop op1.x.i64; break; \
            case PTY_u16: op0.x.u16 = op0.x.u16 exprop op1.x.u16; break; \
            case PTY_u1:  op0.x.u1  = op0.x.u1  exprop op1.x.u1;  break; \
            case PTY_a64: op0.x.a64 = op0.x.a64 exprop op1.x.i64; break; \
            case PTY_f32: op0.x.f32 = op0.x.f32 exprop op1.x.f32; break; \
            case PTY_f64: op0.x.f64 = op0.x.f64 exprop op1.x.f64; break; \
            default: MIR_FATAL("Unsupported PrimType %d for binary oprator %s", expr.primType, #exprop); \
        } \
    } while(0)

#define EXPRBININTOP(exprop) \
    do { \
        MValue &op1 = func.operand_stack.at(func.sp--); \
        MValue &op0 = func.operand_stack.at(func.sp); \
        MASSERT(op0.ptyp == expr.primType, "Type mismatch: 0x%02x and 0x%02x", op0.ptyp, expr.primType); \
        switch(expr.primType) { \
            case PTY_i8:  op0.x.i8  = op0.x.i8  exprop op1.x.i8;  break; \
            case PTY_i16: op0.x.i16 = op0.x.i16 exprop op1.x.i16; break; \
            case PTY_i32: op0.x.i32 = op0.x.i32 exprop op1.x.i32; break; \
            case PTY_a64: op0.x.u64 = op0.x.u64 exprop op1.x.u64; break; \
            case PTY_i64: op0.x.i64 = op0.x.i64 exprop op1.x.i64; break; \
            case PTY_u16: op0.x.u16 = op0.x.u16 exprop op1.x.u16; break; \
            case PTY_u1:  op0.x.u1  = op0.x.u1 exprop op1.x.u1; break; \
            default: MIR_FATAL("Unsupported PrimType %d for integer binary oprator %s", expr.primType, #exprop); \
        } \
    } while(0)

#define EXPRBININTOPUNSIGNED(exprop) \
    do { \
        MValue &op1 = func.operand_stack.at(func.sp--); \
        MValue &op0 = func.operand_stack.at(func.sp); \
        MASSERT(op0.ptyp == expr.primType, "Type mismatch: 0x%02x and 0x%02x", op0.ptyp, expr.primType); \
        switch(expr.primType) { \
            case PTY_i8:  op0.x.u8  = op0.x.u8  exprop op1.x.u8;  break; \
            case PTY_i16: op0.x.u16 = op0.x.u16 exprop op1.x.u16; break; \
            case PTY_i32: op0.x.u32 = op0.x.u32 exprop op1.x.u32; break; \
            case PTY_a64: op0.x.u64 = op0.x.u64 exprop op1.x.u64; break; \
            case PTY_i64: op0.x.u64 = op0.x.u64 exprop op1.x.u64; break; \
            case PTY_u16: op0.x.u16 = op0.x.u16 exprop op1.x.u16; break; \
            case PTY_u1:  op0.x.u1 = op0.x.u1 exprop op1.x.u1; break; \
            default: MIR_FATAL("Unsupported PrimType %d for integer binary oprator %s", expr.primType, #exprop); \
        } \
    } while(0)

#define EXPRREMOP(exprop) \
    do { \
        MValue &op1 = func.operand_stack.at(func.sp--); \
        MValue &op0 = func.operand_stack.at(func.sp); \
        MASSERT(op0.ptyp == op1.ptyp, "Type mismatch: 0x%02x and 0x%02x", op0.ptyp, op1.ptyp); \
        MASSERT(op0.ptyp == expr.primType, "Type mismatch: 0x%02x and 0x%02x", op0.ptyp, expr.primType); \
        switch(expr.primType) { \
            case PTY_i8:  if(op1.x.i8 == 0) THROWJAVAEXCEPTION(ArithmeticException); \
                              else if(op1.x.i8 == -1 && op0.x.i8 == INT8_MIN) op0.x.i8 = 0; \
                              else op0.x.i8  = op0.x.i8  exprop op1.x.i8;  break; \
            case PTY_i16: if(op1.x.i16 == 0) THROWJAVAEXCEPTION(ArithmeticException); \
                              else if(op1.x.i16 == -1 && op0.x.i16 == INT16_MIN) op0.x.i16 = 0; \
                              else op0.x.i16 = op0.x.i16 exprop op1.x.i16; break; \
            case PTY_i32: if(op1.x.i32 == 0) THROWJAVAEXCEPTION(ArithmeticException); \
                              else if(op1.x.i32 == -1 && op0.x.i32 == INT32_MIN) op0.x.i32 = 0; \
                              else op0.x.i32 = op0.x.i32 exprop op1.x.i32; break; \
            case PTY_i64: if(op1.x.i64 == 0) THROWJAVAEXCEPTION(ArithmeticException); \
                              else if(op1.x.i64 == -1 && op0.x.i64 == INT64_MIN) op0.x.i64 = 0; \
                              else op0.x.i64 = op0.x.i64 exprop op1.x.i64; break; \
            default: MIR_FATAL("Unsupported PrimType %d for binary oprator %s", expr.primType, #exprop); \
        } \
    } while(0)

#define EXPRDIVOP(exprop) \
    do { \
        MValue &op1 = func.operand_stack.at(func.sp--); \
        MValue &op0 = func.operand_stack.at(func.sp); \
        MASSERT(op0.ptyp == op1.ptyp, "Type mismatch: 0x%02x and 0x%02x", op0.ptyp, op1.ptyp); \
        MASSERT(op0.ptyp == expr.primType, "Type mismatch: 0x%02x and 0x%02x", op0.ptyp, expr.primType); \
        switch(expr.primType) { \
            case PTY_i8:  if(op1.x.i8 == 0) THROWJAVAEXCEPTION(ArithmeticException); \
                              else if(op1.x.i8 == -1 && op0.x.i8 == INT8_MIN) op0.x.i8 = INT8_MIN; \
                              else op0.x.i8  = op0.x.i8  exprop op1.x.i8;  break; \
            case PTY_i16: if(op1.x.i16 == 0) THROWJAVAEXCEPTION(ArithmeticException); \
                              else if(op1.x.i16 == -1 && op0.x.i16 == INT16_MIN) op0.x.i16 = INT16_MIN; \
                              else op0.x.i16 = op0.x.i16 exprop op1.x.i16; break; \
            case PTY_i32: if(op1.x.i32 == 0) THROWJAVAEXCEPTION(ArithmeticException); \
                              else if(op1.x.i32 == -1 && op0.x.i32 == INT32_MIN) op0.x.i32 = INT32_MIN; \
                              else op0.x.i32 = op0.x.i32 exprop op1.x.i32; break; \
            case PTY_i64: if(op1.x.i64 == 0) THROWJAVAEXCEPTION(ArithmeticException); \
                              else if(op1.x.i64 == -1 && op0.x.i64 == INT64_MIN) op0.x.i64 = INT64_MIN; \
                              else op0.x.i64 = op0.x.i64 exprop op1.x.i64; break; \
            case PTY_f32: op0.x.f32 = op0.x.f32 exprop op1.x.f32; break; \
            case PTY_f64: op0.x.f64 = op0.x.f64 exprop op1.x.f64; break; \
            default: MIR_FATAL("Unsupported PrimType %d for binary oprator %s", expr.primType, #exprop); \
        } \
    } while(0)

#define EXPRMAXMINOP(exprop) \
    do { \
        MValue &op1 = func.operand_stack.at(func.sp--); \
        MValue &op0 = func.operand_stack.at(func.sp); \
        MASSERT(op0.ptyp == op1.ptyp, "Type mismatch: 0x%02x and 0x%02x", op0.ptyp, op1.ptyp); \
        MASSERT(op0.ptyp == expr.primType, "Type mismatch: 0x%02x and 0x%02x", op0.ptyp, expr.primType); \
        switch(expr.primType) { \
            case PTY_i8:  op0.x.i8  = op0.x.i8  exprop op1.x.i8?  op0.x.i8  : op1.x.i8;  break; \
            case PTY_i16: op0.x.i16 = op0.x.i16 exprop op1.x.i16? op0.x.i16 : op1.x.i16; break; \
            case PTY_i32: op0.x.i32 = op0.x.i32 exprop op1.x.i32? op0.x.i32 : op1.x.i32; break; \
            case PTY_i64: op0.x.i64 = op0.x.i64 exprop op1.x.i64? op0.x.i64 : op1.x.i64; break; \
            case PTY_u16: op0.x.u16 = op0.x.u16 exprop op1.x.u16? op0.x.u16 : op1.x.u16; break; \
            case PTY_a64: op0.x.a64 = op0.x.a64 exprop op1.x.a64? op0.x.a64 : op1.x.a64; break; \
            case PTY_f32: op0.x.f32 = op0.x.f32 exprop op1.x.f32? op0.x.f32 : op1.x.f32; break; \
            case PTY_f64: op0.x.f64 = op0.x.f64 exprop op1.x.f64? op0.x.f64 : op1.x.f64; break; \
            default: MIR_FATAL("Unsupported PrimType %d for binary max/min oprator %s", expr.primType, #exprop); \
        } \
    } while(0)

#define EXPRCOMPOP(exprop, resptyp, optype) \
    do { \
        MValue &op1 = func.operand_stack.at(func.sp--); \
        MValue &op0 = func.operand_stack.at(func.sp); \
        /*MASSERT(op0.ptyp == op1.ptyp, "Type mismatch: 0x%02x and 0x%02x", op0.ptyp, op1.ptyp);*/ \
        switch(optype) { \
            case PTY_i8:  op0.x.i64 = op0.x.i8  exprop op1.x.i8;  break; \
            case PTY_i16: op0.x.i64 = op0.x.i16 exprop op1.x.i16; break; \
            case PTY_i32: op0.x.i64 = op0.x.i32 exprop op1.x.i32; break; \
            case PTY_i64: op0.x.i64 = op0.x.i64 exprop op1.x.i64; break; \
            case PTY_u64: op0.x.i64 = op0.x.u64 exprop op1.x.u64; break; \
            case PTY_u32: op0.x.i64 = op0.x.u32 exprop op1.x.u32; break; \
            case PTY_u16: op0.x.i64 = op0.x.u16 exprop op1.x.u16; break; \
            case PTY_u1:  op0.x.i64 = op0.x.u1  exprop op1.x.u1;  break; \
            case PTY_a64: op0.x.i64 = op0.x.a64 exprop op1.x.a64; break; \
            case PTY_f32: op0.x.i64 = op0.x.f32 exprop op1.x.f32; break; \
            case PTY_f64: op0.x.i64 = op0.x.f64 exprop op1.x.f64; break; \
            default: MIR_FATAL("Unsupported operand PrimType %d for comparison oprator %s", op0.ptyp, #exprop); \
        } \
        op0.ptyp = resptyp; \
    } while(0)

#define EXPRCMPLGOP(exprop, nanres, resptyp, optype) \
    do { \
        MValue &op1 = func.operand_stack.at(func.sp--); \
        MValue &op0 = func.operand_stack.at(func.sp); \
        MASSERT(op0.ptyp == op1.ptyp, "Type mismatch: 0x%02x and 0x%02x", op0.ptyp, op1.ptyp); \
        switch(optype) { \
            case PTY_i8:  op0.x.i64 = op0.x.i8  == op1.x.i8 ? 0 : (op0.x.i8  < op1.x.i8 ? -1 : 1);  break; \
            case PTY_i16: op0.x.i64 = op0.x.i16 == op1.x.i16? 0 : (op0.x.i16 < op1.x.i16? -1 : 1);  break; \
            case PTY_i32: op0.x.i64 = op0.x.i32 == op1.x.i32? 0 : (op0.x.i32 < op1.x.i32? -1 : 1);  break; \
            case PTY_i64: op0.x.i64 = op0.x.i64 == op1.x.i64? 0 : (op0.x.i64 < op1.x.i64? -1 : 1);  break; \
            case PTY_u64: op0.x.i64 = op0.x.u64 == op1.x.u64? 0 : (op0.x.u64 < op1.x.u64? -1 : 1);  break; \
            case PTY_u32: op0.x.i64 = op0.x.u32 == op1.x.u32? 0 : (op0.x.u32 < op1.x.u32? -1 : 1);  break; \
            case PTY_u16: op0.x.i64 = op0.x.u64 == op1.x.u16? 0 : (op0.x.u16 < op1.x.u16? -1 : 1);  break; \
            case PTY_u1 : op0.x.i64 = op0.x.u1  == op1.x.u1 ? 0 : (op0.x.u1  < op1.x.u1 ? -1 : 1);  break; \
            case PTY_a64: op0.x.i64 = op0.x.a64 == op1.x.a64? 0 : (op0.x.a64 < op1.x.a64? -1 : 1);  break; \
            case PTY_f32: op0.x.i64 = isnan(op0.x.f32) || isnan(op1.x.f32)? \
                          nanres : (op0.x.f32 == op1.x.f32? 0 : (op0.x.f32 < op1.x.f32? -1 : 1));  break; \
            case PTY_f64: op0.x.i64 = isnan(op0.x.f64) || isnan(op1.x.f64)? \
                          nanres : (op0.x.f64 == op1.x.f64? 0 : (op0.x.f64 < op1.x.f64? -1 : 1));  break; \
            default: MIR_FATAL("Unsupported PrimType %d for comparison cmp/cmpl/cmpg oprator %s", op0.ptyp, #exprop); \
        } \
        op0.ptyp = resptyp; \
    } while(0)

#define EXPRSELECTOP() \
    do { \
        MValue &op2 = func.operand_stack.at(func.sp--); \
        MValue &op1 = func.operand_stack.at(func.sp--); \
        MValue &op0 = func.operand_stack.at(func.sp); \
        MASSERT(op1.ptyp == op2.ptyp, "Type mismatch: 0x%02x and 0x%02x", op1.ptyp, op2.ptyp); \
        MASSERT(op2.ptyp == expr.primType, "Type mismatch: 0x%02x and result type 0x%02x", op2.ptyp, expr.primType); \
        op0.ptyp = expr.primType; \
        switch(expr.primType) { \
            case PTY_i8:  op0.x.i8  = op0.x.i64? op1.x.i8  : op2.x.i8;  break; \
            case PTY_i16: op0.x.i16 = op0.x.i64? op1.x.i16 : op2.x.i16; break; \
            case PTY_i32: op0.x.i32 = op0.x.i64? op1.x.i32 : op2.x.i32; break; \
            case PTY_i64: op0.x.i64 = op0.x.i64? op1.x.i64 : op2.x.i64; break; \
            case PTY_u16: op0.x.u16 = op0.x.i64? op1.x.u16 : op2.x.u16; break; \
            case PTY_u1:  op0.x.u1  = op0.x.i64? op1.x.u1  : op2.x.u1;  break; \
            case PTY_a64: op0.x.a64 = op0.x.i64? op1.x.a64 : op2.x.a64; break; \
            case PTY_f32: op0.x.f32 = op0.x.i64? op1.x.f32 : op2.x.f32; break; \
            case PTY_f64: op0.x.f64 = op0.x.i64? op1.x.f64 : op2.x.f64; break; \
            default: MIR_FATAL("Unsupported PrimType %d for select oprator", expr.primType); \
        } \
    } while(0)

#endif // MAPLERE_MEXPRESSION_H_

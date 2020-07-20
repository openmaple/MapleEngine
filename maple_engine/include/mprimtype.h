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

#ifndef MAPLERE_MPRIMTYPE_H_
#define MAPLERE_MPRIMTYPE_H_

// Define size info of primtypes, some of them are undefined for now(set to 0)
// 1 - 1 byte
// 2 - 2 bytes
// 3 - 4 bytes
// 4 - 8 bytes
// 5 - 16 bytes
// 6 - 32 bytes
#define PTYSIZE_Invalid 0
#define PTYSIZE_void 0
#define PTYSIZE_i8 1
#define PTYSIZE_i16 2
#define PTYSIZE_i32 3
#define PTYSIZE_i64 4
#define PTYSIZE_u8 1
#define PTYSIZE_u16 2
#define PTYSIZE_u32 3
#define PTYSIZE_u64 4
#define PTYSIZE_u1 1
#define PTYSIZE_ptr 3
#define PTYSIZE_ref 3
#define PTYSIZE_a32 3
#define PTYSIZE_a64 4
#define PTYSIZE_f32 3
#define PTYSIZE_f64 4
#define PTYSIZE_f128 5
#define PTYSIZE_c64 5
#define PTYSIZE_c128 6
#define PTYSIZE_simplestr 3
#define PTYSIZE_simpleobj 3
#define PTYSIZE_dynany 4
#define PTYSIZE_dynundef 4
#define PTYSIZE_dynnull 4
#define PTYSIZE_dynbool 4
#define PTYSIZE_dyni32 4
#define PTYSIZE_dynstr 4
#define PTYSIZE_dynobj 4
#define PTYSIZE_dynf64 4
#define PTYSIZE_dynf32 4
#define PTYSIZE_dynnone 4
#define PTYSIZE_constStr 0
#define PTYSIZE_gen 0
#define PTYSIZE_agg 0
#define PTYSIZE_v2i64 0
#define PTYSIZE_v4i32 0
#define PTYSIZE_v8i16 0
#define PTYSIZE_v16i8 0
#define PTYSIZE_v2f64 0
#define PTYSIZE_v4f32 0
#define PTYSIZE_unknown 0
#define PTYSIZE_Derived 0

// Define ffi types for each primtype, some of them are unsupported(set to ffi_type_void)
#define FFITYPE_Invalid ffi_type_void
#define FFITYPE_void ffi_type_void
#define FFITYPE_i8 ffi_type_sint8
#define FFITYPE_i16 ffi_type_sint16
#define FFITYPE_i32 ffi_type_sint32
#define FFITYPE_i64 ffi_type_sint64
#define FFITYPE_u8 ffi_type_uint8
#define FFITYPE_u16 ffi_type_uint16
#define FFITYPE_u32 ffi_type_uint32
#define FFITYPE_u64 ffi_type_uint64
#define FFITYPE_u1 ffi_type_uint8
#define FFITYPE_ptr ffi_type_pointer
#define FFITYPE_ref ffi_type_pointer
#define FFITYPE_a32 ffi_type_pointer
#define FFITYPE_a64 ffi_type_pointer
#define FFITYPE_f32 ffi_type_float
#define FFITYPE_f64 ffi_type_double
#define FFITYPE_f128 ffi_type_void
#define FFITYPE_c64 ffi_type_void
#define FFITYPE_c128 ffi_type_void
#define FFITYPE_simplestr ffi_type_void
#define FFITYPE_simpleobj ffi_type_void
#define FFITYPE_dynany ffi_type_void
#define FFITYPE_dynundef ffi_type_void
#define FFITYPE_dynnull ffi_type_void
#define FFITYPE_dynbool ffi_type_void
#define FFITYPE_dyni32 ffi_type_void
#define FFITYPE_dynstr ffi_type_void
#define FFITYPE_dynobj ffi_type_void
#define FFITYPE_dynf64 ffi_type_void
#define FFITYPE_dynf32 ffi_type_void
#define FFITYPE_dynnone ffi_type_void
#define FFITYPE_constStr ffi_type_void
#define FFITYPE_gen ffi_type_void
#define FFITYPE_agg ffi_type_void
#define FFITYPE_v2i64 ffi_type_void
#define FFITYPE_v4i32 ffi_type_void
#define FFITYPE_v8i16 ffi_type_void
#define FFITYPE_v16i8 ffi_type_void
#define FFITYPE_v2f64 ffi_type_void
#define FFITYPE_v4f32 ffi_type_void
#define FFITYPE_unknown ffi_type_void
#define FFITYPE_Derived ffi_type_void

#endif // MAPLERE_MPRIMTYPE_H_

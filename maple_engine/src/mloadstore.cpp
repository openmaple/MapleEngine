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

#include "mir_config.h"
#include "mloadstore.h"
#include "mprimtype.h"
#include "massert.h" // for MASSERT

#define CONCATEXT(x,y) x##y,
#define CONCAT(x,y) CONCATEXT(x,y)

namespace maple {

#define MMASK0 0x0ull
#define MMASK1 0xffull
#define MMASK2 0xffffull
#define MMASK3 0xffffffffull
#define MMASK4 0xffffffffffffffffull
#define MMASK5 0xffffffffffffffffull
#define MMASK6 0xffffffffffffffffull
    const uint64_t primtype_masks[] = {
        0, // kPtyInvalid
#define PRIMTYPE(P) CONCAT(MMASK,PTYSIZE_##P)
#define LOAD_ALGO_PRIMARY_TYPE
#include "prim_types.def"
#undef PRIMTYPE
        0 // kPtyDerived
    };

    void mload(uint8_t *addr, PrimType pty, MValue &res) {
        static void* const load_labels[] = { // Use GNU extentions
            &&load_label_0, // kPtyInvalid
#define PRIMTYPE(P) CONCAT(&&load_label_,PTYSIZE_##P)
#define LOAD_ALGO_PRIMARY_TYPE
#include "prim_types.def"
#undef PRIMTYPE
            &&load_label_0 // kPtyDerived
        };
        res.ptyp = pty;
        goto *(load_labels[pty]);

load_label_1:
        res.x.i8 = *(int8_t *)addr;
        res.x.u64 &= MMASK1;
        return;
load_label_2:
        res.x.i16 = *(int16_t *)addr;
        res.x.u64 &= MMASK2;
        return;
load_label_3:
        res.x.i32 = *(int32_t *)addr;
        res.x.u64 &= MMASK3;
        return;
load_label_4:
        res.x.i64 = *(int64_t *)addr;
        return;
load_label_0:
load_label_5:
load_label_6:
        MIR_FATAL("Load unsupported PrimType");
    }

    void mstore(uint8_t *addr, PrimType pty, const MValue &val) {
        static const int type_size[] = {
            0, // kPtyInvalid
#define PRIMTYPE(P) PTYSIZE_##P,
#define LOAD_ALGO_PRIMARY_TYPE
#include "prim_types.def"
#undef PRIMTYPE
            0 // kPtyDerived
        };
        static void* const store_labels[] = { // Use GNU extentions
            &&store_label_0, // kPtyInvalid
#define PRIMTYPE(P) CONCAT(&&store_label_,PTYSIZE_##P)
#define LOAD_ALGO_PRIMARY_TYPE
#include "prim_types.def"
#undef PRIMTYPE
            &&store_label_0 // kPtyDerived
        };
        // to allow pty == maple::PTY_u16 && val.ptyp == maple::PTY_u32 is because
        // there are some code
        // iassign <* [16]> (addr, dread u32 val)
        // since it's zero extended, so get the value from val by trunc should be fine
        MASSERT((pty == val.ptyp ||
                    type_size[pty] <= type_size[val.ptyp]), "Type mismatch: 0x%02x and 0x%02x", pty, val.ptyp);
        goto *(store_labels[pty]);

store_label_1:
        *(int8_t *)addr = val.x.i8;
        return;
store_label_2:
        *(int16_t *)addr = val.x.i16;
        return;
store_label_3:
        *(int32_t *)addr = val.x.i32;
        return;
store_label_4:
        *(int64_t *)addr = val.x.i64;
        return;
store_label_0:
store_label_5:
store_label_6:
        MIR_FATAL("Store unsupported PrimType");
    }

} // namespace maple

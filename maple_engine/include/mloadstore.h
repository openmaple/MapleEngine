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

#ifndef MAPLERE_MLOADSTORE_H_
#define MAPLERE_MLOADSTORE_H_

#include "prim_types.h"
#include "mvalue.h"

#define MVALUEBITMASK(val) (val).x.u64 &= maple::primtype_masks[(val).ptyp]

namespace maple {
    extern const uint64_t primtype_masks[];
    void mload(uint8_t *addr, PrimType pty, MValue &res);
    void mstore(uint8_t *addr, maple::PrimType pty, const MValue &val);
}

#endif // MAPLERE_MLOADSTORE_H_

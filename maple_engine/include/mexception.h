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

#ifndef MAPLERE_MEXCEPTION_H_
#define MAPLERE_MEXCEPTION_H_

extern "C" void MRT_ThrowNewException(const char *classname, const char *msg);
extern "C" void* MRT_PendingException();
extern "C" void MRT_ClearPendingException();

namespace maple {

    typedef void* MException;

}

#endif // MAPLERE_MEXCEPTION_H_

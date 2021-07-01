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

#ifndef MAPLEBE_INCLUDE_MACROS_H_
#define MAPLEBE_INCLUDE_MACROS_H_

#define VMDEBUGBEFORECALL 0x1
#define VMDEBUGBEFORERETURN 0x2

#define VMDEBUG1 0x1
#define VMDEBUGMEM 0x2
// for reusing EHstack
#define VMEH_MAXREUSESTACKSIZE 2

#if MIR_FEATURE_FULL && DEBUGSTATE
#define DUMPSTATE(pc)      \
  if (interpreter->currEH) \
    interpreter->currEH->DumpState(pc);
#define _LOC __func__ << "() at " << __FILE__ << ":" << __LINE__
#define DEBUGPRINT(v) \
  { cout << _LOC << " " << #v << " = " << (v) << endl; }
#else
#define DUMPSTATE(pc)
#define DEBUGPRINT(v)
#endif

#endif  // MAPLEBE_INCLUDE_MACROS_H_

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

#include "mfunction.h"
#include "mshimdyn.h"
#include "jseh.h"

void JsEh::Init(void *s, void *catchN, void *finallyN) {
  gosubStack = StackT<void *>();  // initialize gosubStack
  tryNode = s;
  catchNode = catchN;
  finallyNode = finallyN;
  stage = EHS_try;
  thrownStage = EHT_none;
}

void *JsEh::GetEHpc(DynMFunction *callerFunc) {
  void *target = NULL;
  switch (stage) {
    case EHS_try:
      target = catchNode ? catchNode : finallyNode;
      break;
    case EHS_catch:
      if (finallyNode) {
        target = finallyNode;
      }
      break;
    default:
      break;
  }

  if (target) {
    JsEh *eh = gInterSource->currEH;
    if (eh->dynFunc == callerFunc) {
      return target;
    } else {
      return NULL;
    }
  } else {
    // free currEH
    MValue v = thrownVal;
    FreeEH();

    if (gInterSource->EHstack.IsEmpty()) {
      return NULL;
    }

    // get upper level VMEH
    JsEh *eh = gInterSource->EHstack.Top();
    // update it with exception raised and new thrownval
    eh->SetThrownStage(EHT_raised);
    eh->SetThrownval(v);

    // recursively call GetEHpc()
    target = eh->GetEHpc(callerFunc);
    return target;
  }
}

void JsEh::FreeEH() {
  if (gInterSource->currEH == NULL)
    return;

  while (!gosubStack.IsEmpty()) {
    PopGosub();
  }

  JsEh *eh = gInterSource->EHstack.Pop();

  // update currEH
  if (!gInterSource->EHstack.IsEmpty()) {
    gInterSource->currEH = gInterSource->EHstack.Top();
  } else {
    gInterSource->currEH = NULL;
  }

  // park the VMEH at EHReuse to avoid too much malloc/free
  if (gInterSource->EHstackReuseSize < 2) { // VMEH_MAXREUSESTACKSIZE
    gInterSource->EHstackReuse.Push(eh);
    gInterSource->EHstackReuseSize++;
  } else {
    VMFreeNOGC(eh, sizeof(JsEh));
  }
}

void JsEh::UpdateState(Opcode op) {
  MIR_ASSERT((op == OP_jstry) || (op == OP_throw) || (op == OP_jscatch) || (op == OP_finally));
  switch (op) {
    case OP_jstry:
      stage = EHS_try;
      break;
    case OP_throw:
      thrownStage = EHT_raised;
      break;
    case OP_jscatch:
      stage = EHS_catch;
      thrownStage = EHT_handled;
      break;
    case OP_finally:
      stage = EHS_finally;
      break;
    default:
      break;
  }

  return;
}

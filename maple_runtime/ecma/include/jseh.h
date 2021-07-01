//
// Copyright (C) [2020-2021] Futurewei Technologies, Inc. All rights reserved.
//
// OpenArkCompiler is licensed underthe Mulan Permissive Software License v2.
// You can use this software according to the terms and conditions of the MulanPSL - 2.0.
// You may obtain a copy of MulanPSL - 2.0 at:
//
//   https://opensource.org/licenses/MulanPSL-2.0
//
// THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR
// FIT FOR A PARTICULAR PURPOSE.
// See the MulanPSL - 2.0 for more details.
//

#ifndef MAPLEJS_EH
#define MAPLEJS_EH
#include "mvalue.h"
#include "vmmmap.h"
#include "vmutils.h"

namespace maple{
class DynMFunction;

enum EHStage {
  EHS_unknown,
  EHS_try,
  EHS_catch,
  EHS_finally,
};
enum EHThrownStage { EHT_none, EHT_raised, EHT_handled };

class JsEh {
  private:
    void *tryNode;
    void *catchNode;
    void *finallyNode;
    EHStage stage;
    EHThrownStage thrownStage;
    StackT<void *> gosubStack;
    MValue thrownVal;

  public:
    DynMFunction *dynFunc;

  void Init (void *, void *, void *);
  void *GetEHpc(DynMFunction *callerFunc);
  inline bool IsCatchnode(void *s) {
    return s == catchNode;
  }
  inline bool IsFinallynode(void *s) {
    return s == finallyNode;
  }
  void UpdateState(Opcode);
  inline void PushGosub(void *s) {
    gosubStack.Push(s);
  }

  inline void *PopGosub() {
    return gosubStack.Pop();
  }

  inline void SetThrownStage(EHThrownStage s) {
    thrownStage = s;
  }

  inline MValue GetThrownval() {
    return thrownVal;
  }

  inline void SetThrownval(MValue v) {
    thrownVal = v;
  }

  inline bool IsRaised() {
    return thrownStage == EHT_raised;
  }
  void FreeEH();
};
}; // namespace maple
#endif //MAPLEJS_EH

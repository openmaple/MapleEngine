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

#include "cfg_primitive_types.h"
#include "vmutils.h"
#include "vmmemory.h"
#include "mval.h"
#include "mvalue.h"
#include "jsplugin.h"


namespace maple {

#define CHECK_REFERENCE  0x1
#define JSEHOP_jstry   0x1
#define JSEHOP_throw   0x2
#define JSEHOP_jscatch 0x3
#define JSEHOP_finally 0x4


struct JavaScriptGlobal {
  uint8_t flavor;
  uint8_t srcLang;
  uint16_t id;
  uint16_t globalmemsize;
  uint8_t globalwordstypetagged;
  uint8_t globalwordsrefcounted;
  uint8_t numfuncs;
  uint64_t entryfunc;
};

class JsEh;
// imported from class Interpreter
class InterSource {
public:
  void *memory;  // memory block for APP and VM
  uint32_t total_memory_size_;
  uint32_t heap_size_;
  uint32_t stack;
  uint32_t heap;
  uint32_t sp;  // stack pointer
  uint32_t fp;  // frame pointer
  uint8_t *gp;  // global data pointer
  uint8_t *topGp;
  // AddrMap *heapRefList;     // ref used in heap
  // AddrMap *globalRefList;   // ref used in global memory
  MValue retVal0;
  JsEh *currEH;
  JsPlugin *jsPlugin;
  uint32_t EHstackReuseSize;
  StackT<JsEh *> EHstackReuse;
  StackT<JsEh *> EHstack;
  static uint8_t ptypesizetable[kPtyDerived];
  DynMFunction *curDynFunction;


public:
  explicit InterSource();
  void SetRetval0(MValue, bool);
  void SetRetval0Object(void *, bool);
  void SetRetval0NoInc (uint64_t);
  void EmulateStore(uint8_t *, uint8_t,  MValue, PrimType);
  void EmulateStoreGC(uint8 *, uint8_t, MValue, PrimType);
  MValue EmulateLoad(uint8 *, uint8, PrimType);
  int32_t PassArguments(MValue , void *, MValue *, int32_t, int32_t);
  inline void *GetSPAddr() {return (void *) (sp + (uint8 *)memory);}
  inline void *GetFPAddr() {return (void *) (fp + (uint8 *)memory);}
  inline void *GetGPAddr() {return (void *) gp;}
  MValue VmJSopAdd(MValue, MValue);
  MValue PrimAdd(MValue, MValue, PrimType);
  MValue JSopArith(MValue &, MValue &, PrimType, Opcode);
  MValue JSopMul(MValue &, MValue &, PrimType, Opcode);
  MValue JSopSub(MValue &, MValue &, PrimType, Opcode);
  MValue JSopDiv(MValue &, MValue &, PrimType, Opcode);
  MValue JSopRem(MValue &, MValue &, PrimType, Opcode);
  MValue JSopBitOp(MValue &, MValue &, PrimType, Opcode);
  MValue JSopCmp(MValue, MValue, Opcode, PrimType);
  bool JSopSwitchCmp(MValue &, MValue &, MValue &);
  MValue JSopCVT(MValue, PrimType, PrimType);
  MValue JSopNewArrLength(MValue &);
  void JSopSetProp(MValue &, MValue &, MValue &);
  void JSopInitProp(MValue &, MValue &, MValue &);
  MValue  JSopNew(MValue &size);
  MValue  JSopNewIterator(MValue &, MValue &);
  MValue JSopNextIterator(MValue &);
  MValue JSopMoreIterator(MValue &);
  MValue JSopBinary(MIRIntrinsicID, MValue &, MValue &);
  MValue JSBoolean(MValue &);
  MValue JSNumber(MValue &);
  MValue JSopConcat(MValue &, MValue &);
  MValue JSopNewObj0();
  MValue JSopNewObj1(MValue &);
  void JSopSetPropByName (MValue &, MValue &, MValue &, bool isStrict = false);
  MValue JSopGetProp (MValue &, MValue &);
  MValue JSopGetPropByName(MValue &, MValue &);
  MValue JSopDelProp(MValue &, MValue &, bool throw_p = false);
  void JSopInitPropByName(MValue &, MValue &, MValue &);
  void JSopInitPropGetter(MValue &, MValue &, MValue &);
  void JSopInitPropSetter(MValue &, MValue &, MValue &);
  MValue JSopNewArrElems(MValue &, MValue &);
  MValue JSopGetBuiltinString(MValue &);
  MValue JSopGetBuiltinObject(MValue &);
  MValue JSString(MValue &);
  MValue JSopLength(MValue &);
  MValue JSopThis();
  MValue JSUnary(MIRIntrinsicID, MValue &);
  MValue JSopUnary(MValue &, Opcode, PrimType);
  MValue JSopUnaryNeg(MValue &);
  MValue JSopUnaryLnot(MValue &);
  MValue JSopUnaryBnot(MValue &);
  MValue JSopRequire(MValue);
  void CreateJsPlugin(char *);
  void SwitchPluginContext(JsFileInforNode *);
  void RestorePluginContext(JsFileInforNode *);
  MValue IntrinCall(MIRIntrinsicID, MValue *, int);
  MValue NativeFuncCall(MIRIntrinsicID, MValue *, int);
  MValue BoundFuncCall(MValue *, int);
  MValue FuncCall(void *, bool, void *, MValue *, int, int, int, bool);
  MValue IntrinCCall(MValue *, int);
  MValue FuncCall_JS(__jsobject*, __jsvalue *, void *, __jsvalue *, int32_t);
  void JsTry(void *, void *, void *, DynMFunction *);
  void JSPrint(MValue);
  void IntrnError(MValue *, int);
  void InsertProlog(uint16);
  void InsertEplog();
  MValue JSopGetArgumentsObject(void *);
  void* CreateArgumentsObject(MValue *, uint32_t, MValue *);
  MValue GetOrCreateBuiltinObj(__jsbuiltin_object_id);
  void JSdoubleConst(uint64_t, MValue &);
  MValue JSIsNan(MValue &);
  MValue JSDate(MValue &);
  uint64_t GetIntFromJsstring(__jsstring* );
  MValue JSRegExp(MValue &);
  void JSopSetThisPropByName (MValue &, MValue &);
  void JSopInitThisPropByName(MValue &);
  MValue JSopGetThisPropByName(MValue &);
  void UpdateArguments(int32_t, MValue &);
  void SetCurFunc(DynMFunction *func) {
    curDynFunction = func;
  }
  DynMFunction* GetCurFunc() {
    return curDynFunction;
  }

private:
  bool JSopPrimCmp(Opcode, MValue &, MValue &, PrimType);
};

inline uint8_t GetTagFromPtyp (PrimType ptyp) {
  switch(ptyp) {
        case PTY_dynnone:     return JSTYPE_NONE;
        case PTY_dynnull:  return JSTYPE_NULL;
        case PTY_dynbool:
        case PTY_u1:       return JSTYPE_BOOLEAN;
        case PTY_i8:
        case PTY_i16:
        case PTY_i32:
        case PTY_i64:
        case PTY_u16:
        case PTY_u8:
        case PTY_u32:
        case PTY_a64:
        case PTY_u64:       return  JSTYPE_NUMBER;
        case PTY_f32:
        case PTY_f64:       return  JSTYPE_DOUBLE;
        case PTY_simplestr: return  JSTYPE_STRING;
        case PTY_simpleobj: return  JSTYPE_OBJECT;
        case kPtyInvalid:
        case PTY_dynany:
        case PTY_dynundef:
        case PTY_dynstr:
        case PTY_dynobj:
        case PTY_void:
        default:            assert(false&&"unexpected");
    };
}

inline bool IsPrimitiveDyn(PrimType ptyp) {
  // return ptyp >= PTY_simplestr && ptyp <= PTY_dynnone;
  return ptyp >= PTY_dynany && ptyp <= PTY_dynnone;
}

inline uint32_t GetMvalueValue(MValue &mv) {
  return mv.x.u32;
}
inline uint8_t GetMValueTag(MValue &mv) {
  return (mv.ptyp);
}

inline void SetMValueValue (MValue &mv, uint64_t val) {
  mv.x.u64 = val;
}
inline void SetMValueTag (MValue &mv, uint32_t tag) {
  mv.ptyp = tag;
}
extern JavaScriptGlobal *jsGlobal;
extern uint32_t *jsGlobalMemmap;
extern InterSource *gInterSource;

}

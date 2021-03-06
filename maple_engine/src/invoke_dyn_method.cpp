/*
 * Copyright (C) [2021] Futurewei Technologies, Inc. All rights reserved.
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

#include <cstdio>
#include <cmath>
#include <climits>

#include "ark_mir_emit.h"

#include "mvalue.h"
#include "mprimtype.h"
#include "mfunction.h"
#include "mloadstore.h"
#include "mexpression.h"
#include "mexception.h"

#include "opcodes.h"
#include "massert.h" // for MASSERT
#include "mdebug.h"
#include "jsstring.h"
#include "jscontext.h"
#include "mval.h"
#include "mshimdyn.h"

#include "jsfunction.h"
#include "vmconfig.h"
#include "jsiter.h"
#include "jsvalueinline.h"
#include "jsobject.h"
#include "jseh.h"
#include "jstycnv.h"

namespace maple {

static const char* typestr(PrimType t) {
    switch(t) {
        case PTY_i8:        return "    i8";
        case PTY_i16:       return "   i16";
        case PTY_i32:       return "   i32";
        case PTY_i64:       return "   i64";
        case PTY_u16:       return "   u16";
        case PTY_u1:        return "    u1";
        case PTY_a64:       return "   a64";
        case PTY_f32:       return "   f32";
        case PTY_f64:       return "   f64";
        case PTY_u64:       return "   u64";
        case PTY_void:      return "   ---";
        case kPtyInvalid:   return "   INV";
        case PTY_u8:        return "    u8";
        case PTY_u32:       return "   u32";
        case PTY_simplestr: return "splstr";
        case PTY_simpleobj: return "splobj";
        case PTY_dynany:    return "dynany";
        case PTY_dynundef:  return "dynund";
        case PTY_dynstr:    return "dynstr";
        case PTY_dynobj:    return "dynobj";
        case PTY_dyni32:    return "dyni32";
        default:            return "   UNK";
    };
}

static const char* flagStr(uint32_t flag) {
  switch ((__jstype)flag) {
    case JSTYPE_NONE: return " none";
    case JSTYPE_NULL: return " null";
    case JSTYPE_BOOLEAN: return "boolean";
    case JSTYPE_STRING: return "string";
    case JSTYPE_NUMBER: return "number";
    case JSTYPE_OBJECT: return "object";
    case JSTYPE_ENV: return "  env";
    case JSTYPE_UNKNOWN: return "unknown";
    case JSTYPE_UNDEFINED: return "undefined";
    case JSTYPE_DOUBLE: return "double";
    case JSTYPE_NAN: return "nan";
    case JSTYPE_INFINITY: return "infinity";
    case JSTYPE_SPBASE: return "spbase";
    case JSTYPE_FPBASE: return "fpbase";
    case JSTYPE_GPBASE: return "gpbase";
    case JSTYPE_FUNCTION: return "function";

    default: return "unexpected";
  }
}
static void PrintReferenceErrorVND() {
  fprintf(stderr, "ReferenceError: variable is not defined\n");
  exit(3);
}

static void PrintUncaughtException(__jsstring *msg) {
  fprintf(stderr, "uncaught exception: ");
  __jsstr_print(msg, stderr);
  fprintf(stderr, "\n");
  exit(3);
}

#define ABS(v) ((((v) < 0) ? -(v) : (v)))

#define SetRetval0(v) {\
  if (IsNeedRc((v).ptyp))\
    GCIncRf((void *)(v).x.u64);\
  if (IS_NEEDRC(gInterSource->retVal0.x.u64))\
    GCDecRf((void *)(gInterSource->retVal0.x.u64 & PAYLOAD_MASK));\
  mEncode((v));\
  gInterSource->retVal0.x.u64 = (v).x.u64;\
}

#define SetRetval0NoEncode(v) {\
  if (IS_NEEDRC((v).x.u64))\
    GCIncRf((void *)(v).x.u64);\
  if (IS_NEEDRC(gInterSource->retVal0.x.u64))\
    GCDecRf((void *)(gInterSource->retVal0.x.u64 & PAYLOAD_MASK));\
  gInterSource->retVal0.x.u64 = (v).x.u64;\
}

#define SetRetval0NoInc(v, t) {\
  if (IS_NEEDRC(gInterSource->retVal0.x.u64))\
    GCDecRf((void *)(gInterSource->retVal0.x.u64 & PAYLOAD_MASK));\
  gInterSource->retVal0.x.u64 = (v) | t;\
}

#define ENCODE_MPUSH(v)   {\
  mEncode((v));\
  func.operand_stack[++func.sp].x.u64 = (v).x.u64;\
}

#define MPUSH_SELF(v)   {\
  ++func.sp;\
}

//#define TValue2MValue(t)  {.ptyp = NOT_DOUBLE(t.x.u64) ? t.x.c.type & ~NAN_BASE : JSTYPE_DOUBLE, .x.u64 = NOT_DOUBLE(t.x.u64) ? t.x.u64 &= PAYLOAD_MASK : t.x.u64}
inline MValue TValue2MValue(TValue t) {
  MValue r;
  if (NOT_DOUBLE(t.x.u64)) {
    r.ptyp = t.x.c.type & ~NAN_BASE;
    r.x.u64 = (t.x.u64 == POS_ZERO) ? POS_ZERO : (t.x.u64 & PAYLOAD_MASK);
  } else {
    r.ptyp = JSTYPE_DOUBLE;
    r.x.u64 = t.x.u64;
  }
  return r;
}

#define MPUSHV(v)   (func.operand_stack[++func.sp].x.u64 = (v))
#define MPUSH(v)   (func.operand_stack[++func.sp].x.u64 = (v).x.u64)
#define MPOP()     (func.operand_stack[func.sp--])
#define MTOP()     (func.operand_stack[func.sp])

#define MARGS(x)   (caller->operand_stack[caller_args + x])
#define RETURNVAL  (func.operand_stack[0])
#define THROWVAL   (func.operand_stack[1])
#define MLOCALS(x) (func.operand_stack[x])

#define FAST_COMPARE(op) {\
  if (IS_NUMBER_OR_BOOL(mVal0.x.u64) && IS_NUMBER_OR_BOOL(mVal1.x.u64)) {\
      mVal0.x.u64 = (mVal0.x.i32 op mVal1.x.i32) | NAN_BOOLEAN ;\
      MPUSH_SELF(mVal0);\
      func.pc += sizeof(mre_instr_t);\
      goto *(labels[*func.pc]);\
  } else if (IS_DOUBLE(mVal0.x.u64)) {\
    if (IS_DOUBLE(mVal1.x.u64)) {\
      mVal0.x.u64 = (mVal0.x.f64 op mVal1.x.f64) | NAN_BOOLEAN;\
      MPUSH_SELF(mVal0);\
      func.pc += sizeof(mre_instr_t);\
      goto *(labels[*func.pc]);\
    } else if (IS_NUMBER_OR_BOOL(mVal1.x.u64)) {\
      mVal0.x.u64 = (mVal0.x.f64 op (double)mVal1.x.i32) | NAN_BOOLEAN;\
      MPUSH_SELF(mVal0);\
      func.pc += sizeof(mre_instr_t);\
      goto *(labels[*func.pc]);\
    }\
  } else if (IS_DOUBLE(mVal1.x.u64)) {\
    if (IS_NUMBER_OR_BOOL(mVal0.x.u64)) {\
      mVal0.x.u64 = ((double)mVal0.x.i32 op mVal1.x.f64) | NAN_BOOLEAN;\
      MPUSH_SELF(mVal0);\
      func.pc += sizeof(mre_instr_t);\
      goto *(labels[*func.pc]);\
    }\
  }\
}

#define FAST_MATH(op) {\
  if (IS_NUMBER(op1.x.u64)) {\
    if (IS_NUMBER(op0.x.u64)) {\
      int64_t r = (int64_t)op0.x.i32 op (int64_t)op1.x.i32;\
      if (ABS(r) > INT_MAX) {\
        op0.x.f64 = (double)r;\
      } else\
        op0.x.i32 = r;\
      MPUSH_SELF(op0);\
      func.pc += sizeof(binary_node_t);\
      goto *(labels[*func.pc]);\
    } else if (IS_DOUBLE(op0.x.u64)) {\
      double r;\
      r = op0.x.f64 op (double)op1.x.i32;\
      if (r == 0) { \
        op0.x.u64 = POS_ZERO;\
        MPUSH_SELF(op0);\
        func.pc += sizeof(binary_node_t);\
        goto *(labels[*func.pc]);\
      } else if (ABS(r) <= NumberMaxValue) {\
        op0.x.f64 = r;\
        MPUSH_SELF(op0);\
        func.pc += sizeof(binary_node_t);\
        goto *(labels[*func.pc]);\
      }\
    }\
  } else if (IS_DOUBLE(op1.x.u64)) {\
    double r;\
    if (IS_DOUBLE(op0.x.u64)) {\
      r = op0.x.f64 op op1.x.f64;\
      if (r == 0) { \
        op0.x.u64 = POS_ZERO;\
        MPUSH_SELF(op0);\
        func.pc += sizeof(binary_node_t);\
        goto *(labels[*func.pc]);\
     } else if (ABS(r) <= NumberMaxValue) {\
        op0.x.f64 = r;\
        MPUSH_SELF(op0);\
        func.pc += sizeof(binary_node_t);\
        goto *(labels[*func.pc]);\
      }\
    } else if (IS_NUMBER(op0.x.u64)) {\
      r = (double)op0.x.i32 op op1.x.f64;\
      if (r == 0) { \
        op0.x.u64 = POS_ZERO;\
        MPUSH_SELF(op0);\
        func.pc += sizeof(binary_node_t);\
        goto *(labels[*func.pc]);\
      } else if (ABS(r) <= NumberMaxValue) {\
        op0.x.f64 = r;\
        MPUSH_SELF(op0);\
        func.pc += sizeof(binary_node_t);\
        goto *(labels[*func.pc]);\
      }\
    }\
  }\
}

#define FAST_MUL(op) {\
  if (IS_NUMBER(op1.x.u64)) {\
    if (IS_NUMBER(op0.x.u64)) {\
      int64_t r = (int64_t)op0.x.i32 op (int64_t)op1.x.i32;\
      if (ABS(r) > INT_MAX) {\
        op0.x.f64 = (double)r;\
      } else\
        op0.x.i32 = r;\
      MPUSH_SELF(op0);\
      func.pc += sizeof(binary_node_t);\
      goto *(labels[*func.pc]);\
    } else if (IS_DOUBLE(op0.x.u64)) {\
      double r;\
      r = op0.x.f64 op (double)op1.x.i32;\
      if (r == 0) { \
        op0.x.u64 = ((op0.x.f64 > 0 && op1.x.i32 >= 0) || (op0.x.f64 <= 0 && op1.x.i32 < 0)) ?  POS_ZERO : NEG_ZERO;\
        MPUSH_SELF(op0);\
        func.pc += sizeof(binary_node_t);\
        goto *(labels[*func.pc]);\
      } else if (ABS(r) <= NumberMaxValue) {\
        op0.x.f64 = r;\
        MPUSH_SELF(op0);\
        func.pc += sizeof(binary_node_t);\
        goto *(labels[*func.pc]);\
      }\
    }\
  } else if (IS_DOUBLE(op1.x.u64)) {\
    double r;\
    if (IS_DOUBLE(op0.x.u64)) {\
      r = op0.x.f64 op op1.x.f64;\
      if (r == 0) { \
        op0.x.u64 = ((op0.x.f64 > 0 && op1.x.f64 > 0) || (op0.x.f64 <= 0 && op1.x.f64 <= 0)) ?  POS_ZERO : NEG_ZERO;\
        MPUSH_SELF(op0);\
        func.pc += sizeof(binary_node_t);\
        goto *(labels[*func.pc]);\
     } else if (ABS(r) <= NumberMaxValue) {\
        op0.x.f64 = r;\
        MPUSH_SELF(op0);\
        func.pc += sizeof(binary_node_t);\
        goto *(labels[*func.pc]);\
      }\
    } else if (IS_NUMBER(op0.x.u64)) {\
      r = (double)op0.x.i32 op op1.x.f64;\
      if (r == 0) { \
        op0.x.u64 = ((op0.x.i32 >= 0 && op1.x.f64 > 0) || (op0.x.i32 < 0 && op1.x.f64 < 0)) ?  POS_ZERO : NEG_ZERO;\
        MPUSH_SELF(op0);\
        func.pc += sizeof(binary_node_t);\
        goto *(labels[*func.pc]);\
      } else if (ABS(r) <= NumberMaxValue) {\
        op0.x.f64 = r;\
        MPUSH_SELF(op0);\
        func.pc += sizeof(binary_node_t);\
        goto *(labels[*func.pc]);\
      }\
    }\
  }\
}

#define FAST_DIVISION() {\
  if (IS_NUMBER(op1.x.u64) && op1.x.i32 != 0) {\
    if (IS_NUMBER(op0.x.u64)) {\
      if ((op0.x.i32 % op1.x.i32 == 0))\
        op0.x.i32 /= op1.x.i32;\
      else \
        op0.x.f64 = (double)op0.x.i32 / (double)op1.x.i32;\
      MPUSH_SELF(op0);\
      func.pc += sizeof(binary_node_t);\
      goto *(labels[*func.pc]);\
    } else if (IS_DOUBLE(op0.x.u64)) {\
      double r;\
      r = op0.x.f64 / (double)op1.x.i32;\
      if (r == 0) { \
        op0.x.u64 = ((op0.x.f64 > 0 && op1.x.i32 >= 0) || (op0.x.f64 <= 0 && op1.x.i32 < 0)) ?  POS_ZERO : NEG_ZERO;\
        MPUSH_SELF(op0);\
        func.pc += sizeof(binary_node_t);\
        goto *(labels[*func.pc]);\
      } else if (ABS(r) <= NumberMaxValue) {\
        op0.x.f64 = r;\
        MPUSH_SELF(op0);\
        func.pc += sizeof(binary_node_t);\
        goto *(labels[*func.pc]);\
      }\
    }\
  } else if (IS_DOUBLE(op1.x.u64) && op1.x.f64 != 0) {\
    double r;\
    if (IS_DOUBLE(op0.x.u64)) {\
      r = op0.x.f64 / op1.x.f64;\
      if (r == 0) { \
        op0.x.u64 = ((op0.x.f64 > 0 && op1.x.f64 > 0) || (op0.x.f64 <= 0 && op1.x.f64 <= 0)) ?  POS_ZERO : NEG_ZERO;\
        MPUSH_SELF(op0);\
        func.pc += sizeof(binary_node_t);\
        goto *(labels[*func.pc]);\
      } else if (ABS(r) <= NumberMaxValue) {\
        op0.x.f64 = r;\
        MPUSH_SELF(op0);\
        func.pc += sizeof(binary_node_t);\
        goto *(labels[*func.pc]);\
      }\
    } else if IS_NUMBER(op0.x.u64) {\
      r = (double)op0.x.i32 / op1.x.f64;\
      if (r == 0) { \
        op0.x.u64 = ((op0.x.i32 >= 0 && op1.x.f64 > 0) || (op0.x.i32 < 0 && op1.x.f64 < 0)) ?  POS_ZERO : NEG_ZERO;\
        MPUSH_SELF(op0);\
        func.pc += sizeof(binary_node_t);\
        goto *(labels[*func.pc]);\
      } else if (ABS(r) <= NumberMaxValue) {\
        op0.x.f64 = r;\
        MPUSH_SELF(op0);\
        func.pc += sizeof(binary_node_t);\
        goto *(labels[*func.pc]);\
      }\
    }\
  }\
}

#define THROWANDHANDLEREFERENCE() \
       if (!gInterSource->currEH) {\
         PrintReferenceErrorVND(); \
       }\
       gInterSource->currEH->SetThrownval(gInterSource->GetOrCreateBuiltinObj(JSBUILTIN_REFERENCEERRORCONSTRUCTOR));\
       gInterSource->currEH->UpdateState(OP_throw);\
       void *newPc = gInterSource->currEH->GetEHpc(&func);\
       if (newPc) {\
         func.pc = (uint8_t *)newPc;\
         goto *(labels[*(uint8_t *)newPc]);\
       } else {\
         gInterSource->InsertEplog();\
         MValue ret;\
         ret.x.u64 = (uint64_t) Exec_handle_exc;\
         ret.ptyp = JSTYPE_NONE;\
         return ret;\
       }\


#define CHECKREFERENCEMVALUE(mv) \
    if (IS_NONE(mv.x.u64)) {\
      THROWANDHANDLEREFERENCE() \
     }\

#define JSARITH() {\
    TValue &op1 = MPOP(); \
    TValue &op0 = MPOP(); \
    CHECKREFERENCEMVALUE(op0); \
    CHECKREFERENCEMVALUE(op1); \
    MValue op1_ = TValue2MValue(op1); \
    MValue op0_ = TValue2MValue(op0); \
    op0_ = gInterSource->JSopArith(op0_, op1_, expr.primType, (Opcode)expr.op); \
    mEncode(op0_);\
    op0.x.u64 = op0_.x.u64;\
    MPUSH_SELF(op0);}



#define OPCATCHANDGOON(instrt) \
    catch (const char *estr) { \
      isEhHappend = true; \
      if (!strcmp(estr, "callee exception")) { \
        if (!gInterSource->currEH) { \
          fprintf(stderr, "eh thown but never catched"); \
        } else { \
          isEhHappend = true; \
          newPc = gInterSource->currEH->GetEHpc(&func); \
        } \
      } else { \
        MValue m; \
        if (!strcmp(estr, "TypeError")) { \
          m = gInterSource->GetOrCreateBuiltinObj(JSBUILTIN_TYPEERROR_CONSTRUCTOR); \
        } else if (!strcmp(estr, "RangeError")) { \
          m = gInterSource->GetOrCreateBuiltinObj(JSBUILTIN_RANGEERROR_CONSTRUCTOR); \
        } else { \
          m = (__string_value(__jsstr_new_from_char(estr))); \
        } \
        gInterSource->currEH->SetThrownval(m); \
        gInterSource->currEH->UpdateState(OP_throw); \
        newPc = gInterSource->currEH->GetEHpc(&func); \
      } \
    } \
    if (!isEhHappend) { \
      ENCODE_MPUSH(res); \
      func.pc += sizeof(instrt); \
      goto *(labels[*func.pc]); \
    } else { \
      if (newPc) { \
        func.pc = (uint8_t *)newPc; \
        goto *(labels[*(uint8_t *)newPc]); \
      } else { \
        gInterSource->InsertEplog(); \
        MValue ret; \
        ret.x.u64 = (uint64_t) Exec_handle_exc; \
        ret.ptyp = JSTYPE_NONE;\
        return ret; \
      } \
    } \

#define CATCHINTRINSICOP() \
    catch(const char *estr) { \
      isEhHappend = true; \
      if (!gInterSource->currEH) { \
        PrintUncaughtException(__jsstr_new_from_char("intrinsic thrown val nerver caught")); \
      } \
      if (!strcmp(estr, "callee exception")) { \
        newPc = gInterSource->currEH->GetEHpc(&func); \
      } else { \
        MValue m; \
        if (!strcmp(estr, "TypeError")) { \
          m = gInterSource->GetOrCreateBuiltinObj(JSBUILTIN_TYPEERROR_CONSTRUCTOR); \
        } else if (!strcmp(estr, "RangeError")) { \
          m = gInterSource->GetOrCreateBuiltinObj(JSBUILTIN_RANGEERROR_CONSTRUCTOR); \
        } else if (!strcmp(estr, "ReferenceError")) { \
          m = gInterSource->GetOrCreateBuiltinObj(JSBUILTIN_REFERENCEERRORCONSTRUCTOR); \
        } else { \
          m = (__string_value(__jsstr_new_from_char(estr))); \
        } \
        gInterSource->currEH->SetThrownval(m); \
        gInterSource->currEH->UpdateState(OP_throw); \
        newPc = gInterSource->currEH->GetEHpc(&func); \
      } \
    } \

#define OPCATCHBINARY() \
    catch (const char *estr) { \
      isEhHappend = true; \
      if (!strcmp(estr, "callee exception")) { \
        if (!gInterSource->currEH) { \
          fprintf(stderr, "eh thown but never catched"); \
        } else { \
          isEhHappend = true; \
          newPc = gInterSource->currEH->GetEHpc(&func); \
        } \
      } else { \
        MASSERT(false, "NYI"); \
      } \
    } \
    if (!isEhHappend) { \
      ENCODE_MPUSH(res); \
      func.pc += sizeof(binary_node_t); \
      goto *(labels[*func.pc]); \
    } else { \
      if (newPc) { \
        func.pc = (uint8_t *)newPc; \
        goto *(labels[*(uint8_t *)newPc]); \
      } else { \
        gInterSource->InsertEplog(); \
        MValue ret; \
        ret.x.u64 = (uint64_t) Exec_handle_exc; \
        ret.ptyp = JSTYPE_NONE;\
        return ret; \
      } \
    } \

#define JSUNARY() \
    TValue &mv0 = MPOP(); \
    CHECKREFERENCEMVALUE(mv0); \
    MValue mv0_ = TValue2MValue(mv0); \
    mv0_ = gInterSource->JSopUnary(mv0_, (Opcode)expr.op, expr.primType); \
    mEncode(mv0_);\
    mv0.x.u64 = mv0_.x.u64;\
    MPUSH_SELF(mv0); \

uint32_t __opcode_cnt_dyn = 0;
extern "C" uint32_t __inc_opcode_cnt_dyn() {
    return ++__opcode_cnt_dyn;
}

#define DEBUGOPCODE(opc,msg) \
  if(debug_engine && (debug_engine & (kEngineDebugInstruction | kEngineDebuggerOn))) {\
    __inc_opcode_cnt_dyn(); \
    if(debug_engine & kEngineDebugInstruction) {\
      TValue v_ = func.operand_stack[func.sp]; \
      MValue v = TValue2MValue(v_); \
      fprintf(stderr, "Debug [%ld] 0x%lx:%04lx: 0x%016lx, %s, sp=%-2ld: op=0x%02x, ptyp=0x%02x, op#=%3d,      OP_" \
        #opc ", " #msg ", %d\n", gettid(), (uint8_t*)func.header - func.lib_addr, func.pc - (uint8_t*)func.header - func.header->header_size, \
        v.x.u64, flagStr(v.ptyp), func.sp - func.header->frameSize/8, *func.pc, *(func.pc+1), *(func.pc+3), __opcode_cnt_dyn);\
    }\
  }

#define DEBUGCOPCODE(opc,msg) \
  if(debug_engine && (debug_engine & (kEngineDebugInstruction | kEngineDebuggerOn))) {\
    __inc_opcode_cnt_dyn(); \
    if(debug_engine & kEngineDebugInstruction) {\
      TValue v_ = func.operand_stack[func.sp];\
      MValue v = TValue2MValue(v_);\
      fprintf(stderr, "Debug [%ld] 0x%lx:%04lx: 0x%016lx, %s, sp=%-2ld: op=0x%02x, ptyp=0x%02x, param=0x%04x, OP_" \
        #opc ", " #msg ", %d\n", gettid(), (uint8_t*)func.header - func.lib_addr, func.pc - (uint8_t*)func.header - func.header->header_size, \
        v.x.u64, flagStr(v.ptyp), func.sp - func.header->frameSize/8, *func.pc, *(func.pc+1), *((uint16_t*)(func.pc+2)), __opcode_cnt_dyn); \
    }\
  }

#define DEBUGSOPCODE(opc,msg,idx) \
  if(debug_engine && (debug_engine & (kEngineDebugInstruction | kEngineDebuggerOn))) {\
    __inc_opcode_cnt_dyn(); \
    if(debug_engine & kEngineDebugInstruction) {\
      TValue v_ = func.operand_stack[func.sp];\
      MValue v = TValue2MValue(v_);\
      fprintf(stderr, "Debug [%ld] 0x%lx:%04lx: 0x%016lx, %s, sp=%-2ld: op=0x%02x, ptyp=0x%02x, param=0x%04x, OP_" \
        #opc ", " #msg ", %d\n", gettid(), (uint8_t*)func.header - func.lib_addr, func.pc - (uint8_t*)func.header - func.header->header_size, \
        v.x.u64, flagStr(v.ptyp), func.sp - func.header->frameSize/8, *func.pc, *(func.pc+1), *((uint16_t*)(func.pc+2)), \
        __opcode_cnt_dyn); \
    }\
  }

struct prop_cache {
  void *o;
  void *p;
  __jsvalue ret;
} named_prop_cache[2] = {{.o = 0}};

MValue InvokeInterpretMethod(DynMFunction &func) {
    uint8_t *frame_pointer = (uint8_t *)gInterSource->GetFPAddr();
    uint8_t *global_pointer = (uint8_t *)gInterSource->GetGPAddr();
    // Array of labels for threaded interpretion
    static void* const labels[] = { // Use GNU extentions
        &&label_OP_Undef,
#define OPCODE(base_node,dummy1,dummy2,dummy3) &&label_OP_##base_node,
#include "mre_opcodes.def"
#undef OPCODE
        &&label_OP_Undef };
    bool is_strict = func.is_strict();
    DEBUGMETHODSYMBOL(func.header, "Running JavaScript method:", func.header->evalStackDepth);
    gInterSource->SetCurFunc(&func);

    // Get the first mir instruction of this method
    goto *(labels[((base_node_t *)func.pc)->op]);

// handle each mir instruction
label_OP_Undef:
    DEBUGOPCODE(Undef, Undef);
    MIR_FATAL("Error: hit OP_Undef");

label_OP_assertnonnull:
  {
    // Handle statement node: assertnonnull
    DEBUGOPCODE(assertnonnull, Stmt);

    TValue &addr = MPOP();

    func.pc += sizeof(base_node_t);
    goto *(labels[*func.pc]);
  }

label_OP_dread:
  {
#if 0
    // Handle expression node: dread
    mre_instr_t &expr = *(reinterpret_cast<mre_instr_t *>(func.pc));
    int32_t idx = (int32_t)expr.param.frameIdx;
    DEBUGSOPCODE(dread, Expr, idx);
    // Always allocates an object in heap for Java
    // To support other languages, such as C/C++, we need to calculate
    // the address of the field, and store the value based on its type
    if(idx > 0){
        // MPUSH(MARGS(idx));
    } else {
        // DEBUGUNINITIALIZED(-idx);
        MValue &local = MLOCALS(-idx);
        local.ptyp = expr.primType;
        MPUSH(local);
    }
#endif
    func.pc += sizeof(mre_instr_t);
    goto *(labels[*func.pc]);
  }

label_OP_iread:
  {
#if 0
    // Handle expression node: iread
    base_node_t &expr = *(reinterpret_cast<base_node_t *>(func.pc));
    DEBUGOPCODE(iread, Expr);

    MValue &addr = MPOP(); TValue2MValue(addr);
    // //(addr.x.a64);
    MValue res;
    mload(addr.x.a64, expr.primType, res);
    ENCODE_MPUSH(res);

#endif
    func.pc += sizeof(mre_instr_t);
    goto *(labels[*func.pc]);
  }

label_OP_addrof:
  {
#if 0
    // For address of local var/parameter
    mre_instr_t &expr = *(reinterpret_cast<mre_instr_t *>(func.pc));
    int32_t idx = (int32_t)expr.param.frameIdx;
    DEBUGSOPCODE(addrof, Expr, idx);

    MValue res;
    res.ptyp = expr.primType;
    if(idx > 0) {
        //MValue &arg = MARGS(idx);
        //res.x.a64 = (uint8_t*)&arg.x;
    } else {
        MValue &local = MLOCALS(-idx);
        TValue2MValue(local);
        // local.ptyp = (PrimType)func.header->primtype_table[(func.header->formals_num - idx)*2]; // both formals and locals are 2B each
        res.x.a64 = (uint8_t*)&local.x;
        //mEncode(local);
    }
    ENCODE_MPUSH(res);

#endif
    func.pc += sizeof(mre_instr_t);
    goto *(labels[*func.pc]);
  }

label_OP_addrof32:
  {
#if 0
    // Handle expression node: addrof
    addrof_node_t &expr = *(reinterpret_cast<addrof_node_t *>(func.pc));
    DEBUGOPCODE(addrof32, Expr);

    //MASSERT(expr.primType == PTY_a64, "Type mismatch: 0x%02x (should be PTY_a64)", expr.primType);
    MValue target;
    // Uses the offset to the GOT entry for the symbol name (symbolname@GOTPCREL)
    uint8_t* pc = (uint8_t*)&expr.stIdx;
    target.x.a64 = *(uint8_t**)(pc + *(int32_t *)pc);
    target.ptyp = PTY_a64;
    ENCODE_MPUSH(target);

#endif
    func.pc += sizeof(addrof_node_t) - 4; // Using 4 bytes for symbolname@GOTPCREL
    goto *(labels[*func.pc]);
  }

label_OP_ireadoff:
  {
      // Handle expression node: ireadoff
      mre_instr_t &expr = *(reinterpret_cast<mre_instr_t *>(func.pc));
      DEBUGCOPCODE(ireadoff, Expr);
      TValue &mv = MPOP();
      //(base.x.a64);
      uint8_t *addr = (uint8_t *)(mv.x.u64 & PAYLOAD_MASK) + expr.param.offset;

      mv.x.u64 =  *((uint64_t *)addr);
      MPUSH_SELF(mv);
      func.pc += sizeof(mre_instr_t);
      goto *(labels[*func.pc]);
  }

label_OP_ireadoff32:
  {
#if 0
      ireadoff_node_t &expr = *(reinterpret_cast<ireadoff_node_t *>(func.pc));
      DEBUGOPCODE(ireadoff32, Expr);

      TValue &base = MTOP();
      //(base.x.a64);
      auto addr = (uint8_t *)(base.x.u64 & PAYLOAD_MASK) + expr.offset;
      mload(addr, expr.primType, base);
#endif
      func.pc += sizeof(ireadoff_node_t);
      goto *(labels[*func.pc]);
  }

label_OP_regread:
  {
    // Handle expression node: regread
    mre_instr_t &expr = *(reinterpret_cast<mre_instr_t *>(func.pc));
    int32_t idx = (int32_t)expr.param.frameIdx;

    DEBUGSOPCODE(regread, Expr, idx);
    switch (idx) {
      case -kSregSp:{
        MPUSHV((uint64_t)gInterSource->GetSPAddr() | NAN_SPBASE);
        break;
      }
      case -kSregFp: {
        MPUSHV((uint64_t)frame_pointer | NAN_FPBASE);
        break;
      }
      case -kSregGp: {
        MPUSHV((uint64_t)global_pointer | NAN_GPBASE);
        break;
      }
      case -kSregRetval0: {
        MPUSH(gInterSource->retVal0);
        break;
      }
      case -kSregThrownval: {
        MValue mvl = gInterSource->currEH->GetThrownval();
        ENCODE_MPUSH(mvl);
        break;
      }
    }

    func.pc += sizeof(mre_instr_t);
    goto *(labels[*func.pc]);
  }

label_OP_addroffunc:
  {
    // Handle expression node: addroffunc
    //addroffunc_node_t &expr = *(reinterpret_cast<addroffunc_node_t *>(func.pc));
    constval_node_t &expr = *(reinterpret_cast<constval_node_t *>(func.pc));
    DEBUGOPCODE(addroffunc, Expr);

    TValue res;
    // expr.puidx contains the offset after lowering
    //res.x.a64 = (uint8_t*)&expr.puidx + expr.puidx;
    // res.x.a64 = *(uint8_t **)&expr.puIdx;
    res.x.u64 = ((uint64_t )expr.constVal.value) | NAN_FUNCTION;
    MPUSH(res);

    func.pc += sizeof(constval_node_t);
    goto *(labels[*func.pc]);
  }

label_OP_constval:
  {
    // Handle expression node: constval
    mre_instr_t &expr = *(reinterpret_cast<mre_instr_t *>(func.pc));
    DEBUGCOPCODE(constval, Expr);
    TValue res = {.x.u64 = 0};
    switch(expr.primType) {
        case PTY_i8:  res.x.i32 = expr.param.constval.i8;  break;
        case PTY_i16:
        case PTY_i32: res.x.i32 = expr.param.constval.i16; break;
        case PTY_i64: res.x.i64 = expr.param.constval.i16; break;
        case PTY_u1:
        case PTY_u8:  res.x.u64 = expr.param.constval.u8;  break;
        case PTY_u16:
        case PTY_u32:
        case PTY_u64:
        case PTY_a32: // running on 64-bits machine
        case PTY_a64: res.x.u64 = expr.param.constval.u16; break;
        default: MASSERT(false, "Unexpected type for OP_constval: 0x%02x", res.x.c.type);
    }
    //res.ptyp = JSTYPE_NUMBER;
    res.x.u64 = (res.x.u64 == 0) ? POS_ZERO : (res.x.u64 | NAN_NUMBER);
    MPUSH(res);
    func.pc += sizeof(mre_instr_t);
    goto *(labels[*func.pc]);
  }

label_OP_constval64:
  {
    constval_node_t &expr = *(reinterpret_cast<constval_node_t *>(func.pc));
    DEBUGOPCODE(constval64, Expr);

    MValue res = {0, JSTYPE_NONE};
    uint64_t u64Val = ((uint64_t)expr.constVal.value);
    PrimType exprPtyp = expr.primType;
    if (exprPtyp == PTY_dynf64) {
      union {
        uint64_t u64;
        double f64;
      }xx;
      xx.u64 = u64Val;
      if (xx.f64 == -0.0f) {
       xx.u64 = NEG_ZERO;
      }
      res.x.f64 = xx.f64;
      MPUSH(res);
      func.pc += sizeof(constval_node_t);
      goto *(labels[*func.pc]);
    } else {
      switch (exprPtyp) {
        case PTY_i32: {
          res.x.i32 = (int32_t)u64Val;
          res.x.u64 |= NAN_NUMBER;
          MPUSH(res);
          func.pc += sizeof(constval_node_t);
          goto *(labels[*func.pc]);
          break;
        }
        case PTY_dynnull: {
          //assert(u64Val == 0x100000000);
          res.ptyp = JSTYPE_NULL;
          res.x.u64 = 0;
          break;
        }
        case PTY_dynundef: {
          //assert(u64Val == 0x800000000);
          res.ptyp = JSTYPE_UNDEFINED;
          res.x.u64 = 0;
          break;
        }
        case PTY_dynbool: {
          res.ptyp = JSTYPE_BOOLEAN;
          if (u64Val == 0x200000001) {
            res.x.u64= 1;
          } else {
            //assert(u64Val == 0x200000000);
            res.x.u64= 0;
          }
          break;
        }
        case PTY_dynany: {
          res.x.u64 = 0;
          switch ((uint8_t)(u64Val >> 32)) {
//          convert JSTYPE_* for now until sync with js2mpl
//            case JSTYPE_NONE: {
            case 0: {
              res.ptyp = JSTYPE_NONE;
              break;
            }
//            case JSTYPE_UNDEFINED: {
            case 8: {
              res.ptyp = JSTYPE_NONE;
              break;
            }
//            case JSTYPE_NULL: {
            case 1: {
              res.ptyp = JSTYPE_NONE;
              break;
            }
//            case JSTYPE_NAN: {
            case 0xa: {
              res.ptyp = JSTYPE_NAN;
              break;
            }
//            case JSTYPE_BOOLEAN: {
            case 2: {
              res.ptyp = JSTYPE_BOOLEAN;
              break;
            }
//            case JSTYPE_NUMBER: {
            case 4: {
              res.ptyp = JSTYPE_NUMBER;
              break;
            }
//            case JSTYPE_INFINITY:{
            case 0xb:{
              res.x.u64 = u64Val & 0xffffffff;
              res.ptyp = JSTYPE_INFINITY;
              break;
            }
//            case JSTYPE_DOUBLE:
            case 9:
            default:
              MAPLE_JS_ASSERT(false);
          }
          break;
        }
        case PTY_dyni32: {
	  res.x.u64 = (int32_t)((u64Val << 32) >> 32);
	  res.ptyp = JSTYPE_NUMBER;
	  break;
        }
        default: {
          res.x.u64 = u64Val;
          res.ptyp = GetTagFromPtyp(expr.primType);
          break;
        }
      }
    }
    ENCODE_MPUSH(res);
    func.pc += sizeof(constval_node_t);
    goto *(labels[*func.pc]);
  }

label_OP_conststr:
  {
    // Handle expression node: conststr
    conststr_node_t &expr = *(reinterpret_cast<conststr_node_t *>(func.pc));
    DEBUGOPCODE(conststr, Expr);

    MValue res;
    res.ptyp = expr.primType;
    res.x.a64 = *(uint8_t **)&expr.stridx;
    ENCODE_MPUSH(res);

    func.pc += sizeof(conststr_node_t) + 4; // Needs Ed to fix the type for conststr
    goto *(labels[*func.pc]);
  }

label_OP_cvt:
  {
    // Handle expression node: cvt
    mre_instr_t &expr = *(reinterpret_cast<mre_instr_t *>(func.pc));
    PrimType from_ptyp = (PrimType)expr.param.constval.u8;
    PrimType destPtyp = expr.primType;
    DEBUGOPCODE(cvt, Expr);

    TValue &op_ = MTOP();
    if (IsPrimitiveDyn(from_ptyp)) {
      CHECKREFERENCEMVALUE(op_);
    }
    MValue op = TValue2MValue(op_);
    func.pc += sizeof(mre_instr_t);
    auto target = labels[*func.pc];

    int64_t from_int;
    float   from_float;
    double  from_double;
    if (IsPrimitiveDyn(destPtyp) || IsPrimitiveDyn(from_ptyp)) {
      bool isEhHappend = false;
      void *newPc = nullptr;
      try {
        op = gInterSource->JSopCVT(op, destPtyp, from_ptyp);
        mEncode(op);
        op_.x.u64 = op.x.u64;
      }
      CATCHINTRINSICOP();
      if (isEhHappend) {
        if (newPc) {
          func.pc = (uint8_t *)newPc;
          goto *(labels[*(uint8_t *)newPc]);
        } else {
          gInterSource->InsertEplog();
            // gInterSource->FinishFunc();
          MValue ret;
          ret.x.u64 = (uint64_t) Exec_handle_exc;
          ret.ptyp = JSTYPE_NONE;
          return ret;
        }
      } else {
        mEncode(op);
        op_.x.u64 = op.x.u64;
        goto *target;
      }
    }
    switch(from_ptyp) {
        case PTY_i8:  from_int = op.x.i8;     goto label_cvt_int;
        case PTY_i16: from_int = op.x.i16;    goto label_cvt_int;
        case PTY_u16: from_int = op.x.u16;    goto label_cvt_int;
        case PTY_u32: from_int = op.x.u32;    goto label_cvt_int;
        case PTY_i32: from_int = op.x.i32;    goto label_cvt_int;
        case PTY_i64: from_int = op.x.i64;    goto label_cvt_int;
        case PTY_simplestr: {
          from_int = gInterSource->GetIntFromJsstring((__jsstring*)op.x.u64);
          goto label_cvt_int;
        }
        case PTY_f32: {
          from_float = op.x.f32;
#if defined(__x86_64__)
          PrimType toPtyp = destPtyp;
          switch (toPtyp) {
            case PTY_i64: {
              int64_t int64Val = (int64_t) from_float;
              if (from_float > 0.0f && int64Val == LONG_MIN) {
                int64Val = int64Val -1;
              }
              op.x.i64 = isnan(from_float) ? 0 :  int64Val;
              mEncode(op);
              op_.x.u64 = op.x.u64;
              goto *target;
            }
            case PTY_u64: {
              op.x.i64 = (uint64_t) from_float;
              mEncode(op);
              op_.x.u64 = op.x.u64;
              goto *target;
            }
            case PTY_i32:
            case PTY_u32: {
              int32_t fromFloatInt = (int32_t)from_float;
              if( from_float > 0.0f &&
                ABS(from_float - (float)fromFloatInt) >= 1.0f) {
                if (ABS((float)(fromFloatInt -1) - from_float) < 1.0f) {
                  op.x.i64 = toPtyp == PTY_i32 ? fromFloatInt - 1 : (uint32_t)from_float - 1;
                } else {
                  if (fromFloatInt == INT_MIN) {
                    // some cornar cases
                    op.x.i64 = INT_MAX;
                  } else {
                    MASSERT(false, "NYI %f", from_float);
                  }
                }
                mEncode(op);
                op_.x.u64 = op.x.u64;
                goto *target;
              } else {
                if (isnan(from_float)) {
                  op.x.i64 = 0;
                  mEncode(op);
                  op_.x.u64 = op.x.u64;
                  goto *target;
                } else {
                  goto label_cvt_float;
                }
              }
            }
            default:
              goto label_cvt_float;
          }
#else
          goto label_cvt_float;
#endif
        }
        case PTY_f64: {
          from_double = op.x.f64;
#if defined(__x86_64__)
          PrimType toPtyp = destPtyp;
          switch (toPtyp) {
            case PTY_i32: {
              int32_t int32Val = (int32_t) from_double;
              if (from_double > 0.0f && int32Val == INT_MIN) {
                int32Val = int32Val -1;
              }
              op.x.i64 = isnan(from_double) ? 0 :  int32Val;
              mEncode(op);
              op_.x.u64 = op.x.u64;
              goto *target;
            }
            case PTY_u32: {
              op.x.i64 = (uint32_t) from_double;
              mEncode(op);
              op_.x.u64 = op.x.u64;
              goto *target;
            }
            case PTY_i64:
            case PTY_u64: {
              int64_t fromDoubleInt = (int64_t)from_double;
              if( from_double > 0.0f &&
                ABS(from_double - (double)fromDoubleInt) >= 1.0f) {
                if (ABS((double)(fromDoubleInt -1) - from_double) < 1.0f) {
                  op.x.i64 = toPtyp == PTY_i64 ? fromDoubleInt - 1 : (uint64_t)from_double - 1;
                } else {
                  if (fromDoubleInt == LONG_MIN) {
                    // some cornar cases
                    op.x.i64 = LONG_MAX;
                  } else {
                    MASSERT(false, "NYI %f", from_double);
                  }
                }
                mEncode(op);
                op_.x.u64 = op.x.u64;
                goto *target;
              } else {
                if (isnan(from_double)) {
                  op.x.i64 = 0;
                  mEncode(op);
                  op_.x.u64 = op.x.u64;
                  goto *target;
                } else {
                  goto label_cvt_double;
                }
              }
            }
            default:
              goto label_cvt_double;
          }
#else
          goto label_cvt_double;
#endif
        }
        default: mEncode(op); op_.x.u64 = op.x.u64; goto *target;
    }
#define CVTIMPL(typ) label_cvt_##typ: \
    switch(op.ptyp) { \
        case PTY_i8:  op.x.i64 = (int8_t)from_##typ;   mEncode(op); op_.x.u64 = op.x.u64; goto *target; \
        case PTY_i16: op.x.i64 = (int16_t)from_##typ;  mEncode(op); op_.x.u64 = op.x.u64; goto *target; \
        case PTY_u1:  op.x.i64 = from_##typ != 0;      mEncode(op); op_.x.u64 = op.x.u64; goto *target; \
        case PTY_u16: op.x.i64 = (uint16_t)from_##typ; mEncode(op); op_.x.u64 = op.x.u64; goto *target; \
        case PTY_u32: op.x.i64 = (uint32_t)from_##typ; mEncode(op); op_.x.u64 = op.x.u64; goto *target; \
        case PTY_i32: op.x.i64 = (int32_t)from_##typ;  mEncode(op); op_.x.u64 = op.x.u64; goto *target; \
        case PTY_i64: op.x.i64 = (int64_t)from_##typ;  mEncode(op); op_.x.u64 = op.x.u64; goto *target; \
        case PTY_f32: op.x.f32 = (float)from_##typ;    mEncode(op); op_.x.u64 = op.x.u64; goto *target; \
        case PTY_f64: op.x.f64 = (double)from_##typ;   mEncode(op); op_.x.u64 = op.x.u64; goto *target; \
        case PTY_a64: op.x.u64 = (uint64_t)from_##typ; mEncode(op); op_.x.u64 = op.x.u64; goto *target; \
        default: MASSERT(false, "Unexpected type for OP_cvt: 0x%02x to 0x%02x", from_ptyp, op.ptyp); \
                 mEncode(op); op_.x.u64 = op.x.u64; goto *target; \
    }
    CVTIMPL(int);
    CVTIMPL(float);
    CVTIMPL(double);
  }

label_OP_retype:
  {
#if 0
    // Handle expression node: retype
    mre_instr_t &expr = *(reinterpret_cast<mre_instr_t *>(func.pc));
    DEBUGOPCODE(retype, Expr);

    // retype <prim-type> <type> (<opnd0>)
    // Converted to <prim-type> which has derived type <type> without changing any bits.
    // The size of <opnd0> and <prim-type> must be the same.
    MValue &res = MTOP();
    //MASSERT(expr.GetOpPtyp() == res.ptyp, "Type mismatch: 0x%02x and 0x%02x", expr.GetOpPtyp(), res.ptyp);
    res.ptyp = expr.primType;
#endif
    func.pc += sizeof(mre_instr_t);
    goto *(labels[*func.pc]);
  }


label_OP_sext:
  {
    // Handle expression node: sext
    mre_instr_t &expr = *(reinterpret_cast<mre_instr_t *>(func.pc));
    DEBUGOPCODE(sext, Expr);

    //MASSERT(expr.param.extractbits.boffset == 0, "Unexpected offset");
    uint32_t mask = expr.param.extractbits.bsize < 32 ? (1 << expr.param.extractbits.bsize) - 1 : ~0;
    TValue &op = MTOP();
    op.x.i32 = (op.x.i32 >> (expr.param.extractbits.bsize - 1) & 1) ? op.x.i32 | ~mask : op.x.i32 & mask;
    // op.ptyp = expr.primType;
    func.pc += sizeof(mre_instr_t);
    goto *(labels[*func.pc]);
  }

label_OP_zext:
  {
    // Handle expression node: zext
    mre_instr_t &expr = *(reinterpret_cast<mre_instr_t *>(func.pc));
    DEBUGOPCODE(zext, Expr);

    //MASSERT(expr.param.extractbits.boffset == 0, "Unexpected offset");
    uint32_t mask = expr.param.extractbits.bsize < 32 ? (1 << expr.param.extractbits.bsize) - 1 : ~0;
    TValue &op = MTOP();
    op.x.i32 &= mask;
    // op.ptyp = expr.primType;

    func.pc += sizeof(mre_instr_t);
    goto *(labels[*func.pc]);
  }

label_OP_add:
  {
    // Handle expression node: add
    binary_node_t &expr = *(reinterpret_cast<binary_node_t *>(func.pc));
    DEBUGOPCODE(add, Expr);
    TValue &op1 = MPOP();
    TValue &op0 = MPOP();
    FAST_MATH(+);
    if (IS_NUMBER(op1.x.u64) && IS_ADDRESS(op0.x.u64)) {
      op0.x.u64 = op0.x.u64 + op1.x.i32;
      MPUSH_SELF(op0);
      func.pc += sizeof(binary_node_t);
      goto *(labels[*func.pc]);
    }
    CHECKREFERENCEMVALUE(op0);
    CHECKREFERENCEMVALUE(op1);
    MValue op1_ = TValue2MValue(op1);
    MValue op0_ = TValue2MValue(op0);
    MValue resVal = gInterSource->PrimAdd(op0_, op1_, expr.primType);
    ENCODE_MPUSH(resVal);
    func.pc += sizeof(binary_node_t);
    goto *(labels[*func.pc]);
  }

label_OP_sub:
  {
    // Handle expression node: sub
    binary_node_t &expr = *(reinterpret_cast<binary_node_t *>(func.pc));
    DEBUGOPCODE(sub, Expr);
    if (!IsPrimitiveDyn(expr.primType)) {
      JSARITH();
      func.pc += sizeof(binary_node_t);
      goto *(labels[*func.pc]);
    } else {
      TValue &op1 = MPOP();
      TValue &op0 = MPOP();
      FAST_MATH(-);
      CHECKREFERENCEMVALUE(op0);
      CHECKREFERENCEMVALUE(op1);
      MValue op1_ = TValue2MValue(op1);
      MValue op0_ = TValue2MValue(op0);
      MValue res;
      bool isEhHappend = false;
      void *newPc = nullptr;
      try {
        res = gInterSource->JSopSub(op0_, op1_, expr.primType, (Opcode)expr.op);
      }
      OPCATCHANDGOON(binary_node_t);
    }
  }

label_OP_mul:
  {
    // Handle expression node: mul
    binary_node_t &expr = *(reinterpret_cast<binary_node_t *>(func.pc));
    DEBUGOPCODE(mul, Expr);
    if (!IsPrimitiveDyn(expr.primType)) {
      JSARITH();
      func.pc += sizeof(binary_node_t);
      goto *(labels[*func.pc]);
    } else {
      TValue &op1 = MPOP();
      TValue &op0 = MPOP();
      FAST_MUL(*);
      CHECKREFERENCEMVALUE(op0);
      CHECKREFERENCEMVALUE(op1);
      MValue op1_ = TValue2MValue(op1);
      MValue op0_ = TValue2MValue(op0);
      MValue res;
      bool isEhHappend = false;
      void *newPc = nullptr;
      try {
        res = gInterSource->JSopMul(op0_, op1_, expr.primType, (Opcode)expr.op);
      }
      OPCATCHANDGOON(binary_node_t);
    }
  }

label_OP_div:
  {
    // Handle expression node: div
    binary_node_t &expr = *(reinterpret_cast<binary_node_t *>(func.pc));
    DEBUGOPCODE(div, Expr);
    if (!IsPrimitiveDyn(expr.primType)) {
      JSARITH();
      func.pc += sizeof(binary_node_t);
      goto *(labels[*func.pc]);
    } else {
      TValue &op1 = MPOP();
      TValue &op0 = MPOP();
      FAST_DIVISION();
      CHECKREFERENCEMVALUE(op0);
      CHECKREFERENCEMVALUE(op1);
      MValue op1_ = TValue2MValue(op1);
      MValue op0_ = TValue2MValue(op0);
      MValue res;
      bool isEhHappend = false;
      void *newPc = nullptr;
      try {

        res = gInterSource->JSopDiv(op0_, op1_, expr.primType, (Opcode)expr.op);
      }
      OPCATCHANDGOON(binary_node_t);
    }
  }

label_OP_rem:
  {
    // Handle expression node: rem
    binary_node_t &expr = *(reinterpret_cast<binary_node_t *>(func.pc));
    DEBUGOPCODE(rem, Expr);
    if (!IsPrimitiveDyn(expr.primType)) {
      JSARITH();
      func.pc += sizeof(binary_node_t);
      goto *(labels[*func.pc]);
    } else {
      TValue &op1 = MPOP();
      TValue &op0 = MPOP();
      if (IS_NUMBER(op0.x.u64) && IS_NUMBER(op1.x.u64) && op1.x.i32 > 0) {
        op0.x.i32 = op0.x.i32 % op1.x.i32;
        MPUSH_SELF(op0);
        func.pc += sizeof(binary_node_t);
        goto *(labels[*func.pc]);
      }
      CHECKREFERENCEMVALUE(op0);
      CHECKREFERENCEMVALUE(op1);
      MValue op1_ = TValue2MValue(op1);
      MValue op0_ = TValue2MValue(op0);
      MValue res;
      bool isEhHappend = false;
      void *newPc = nullptr;
      try {
        res = gInterSource->JSopRem(op0_, op1_, expr.primType, (Opcode)expr.op);
      }
      OPCATCHANDGOON(binary_node_t);
    }
  }

label_OP_shl:
label_OP_band:
label_OP_bior:
label_OP_bxor:
label_OP_land:
label_OP_lior:
label_OP_lshr:
label_OP_ashr:
  {
    // Handle expression node: shl
    binary_node_t &expr = *(reinterpret_cast<binary_node_t *>(func.pc));
    DEBUGOPCODE(shl, Expr);
    if (!IsPrimitiveDyn(expr.primType)) {
      JSARITH();
      func.pc += sizeof(binary_node_t);
      goto *(labels[*func.pc]);
    } else {
      TValue &op1 = MPOP();
      TValue &op0 = MPOP();
      CHECKREFERENCEMVALUE(op0);
      CHECKREFERENCEMVALUE(op1);
      MValue op1_ = TValue2MValue(op1);
      MValue op0_ = TValue2MValue(op0);
      MValue res;
      bool isEhHappend = false;
      void *newPc = nullptr;
      try {
        res = gInterSource->JSopBitOp(op0_, op1_, expr.primType, (Opcode)expr.op);
      }
      OPCATCHANDGOON(binary_node_t);
    }
  }

label_OP_max:
  {
    // Handle expression node: max
    binary_node_t &expr = *(reinterpret_cast<binary_node_t *>(func.pc));
    DEBUGOPCODE(max, Expr);
    JSARITH();
    func.pc += sizeof(binary_node_t);
    goto *(labels[*func.pc]);
  }

label_OP_min:
  {
    // Handle expression node: min
    binary_node_t &expr = *(reinterpret_cast<binary_node_t *>(func.pc));
    DEBUGOPCODE(min, Expr);
    JSARITH();
    func.pc += sizeof(binary_node_t);
    goto *(labels[*func.pc]);
  }

label_OP_CG_array_elem_add:
  {
    // Handle expression node: CG_array_elem_add
    DEBUGOPCODE(CG_array_elem_add, Expr);

    TValue &offset = MPOP();
    TValue &base = MTOP();
    base.x.c.payload += offset.x.i32;

    func.pc += sizeof(binary_node_t);
    goto *(labels[*func.pc]);
  }

label_OP_eq:
  {
    // Handle expression node: eq
    mre_instr_t &expr = *(reinterpret_cast<mre_instr_t *>(func.pc));
    DEBUGOPCODE(eq, Expr);
    TValue  &mVal1 = MPOP();
    TValue  &mVal0 = MPOP();
    FAST_COMPARE(==);
    CHECKREFERENCEMVALUE(mVal1);
    CHECKREFERENCEMVALUE(mVal0);
    MValue mVal1_ = TValue2MValue(mVal1);
    MValue mVal0_ = TValue2MValue(mVal0);
    bool isEhHappend = false;
    void *newPc = nullptr;
    MValue res;
    try {
      res = gInterSource->JSopCmp(mVal0_, mVal1_, OP_eq, expr.primType);
    }
    OPCATCHANDGOON(mre_instr_t);
  }
label_OP_ge:
  {
    // Handle expression node: ge
    mre_instr_t &expr = *(reinterpret_cast<mre_instr_t *>(func.pc));
    DEBUGOPCODE(ge, Expr);
    TValue  &mVal1 = MPOP();
    TValue  &mVal0 = MPOP();
    FAST_COMPARE(>=);
    CHECKREFERENCEMVALUE(mVal1);
    CHECKREFERENCEMVALUE(mVal0);
    MValue mVal1_ = TValue2MValue(mVal1);
    MValue mVal0_ = TValue2MValue(mVal0);
    bool isEhHappend = false;
    void *newPc = nullptr;
    MValue res;
    try {
      res = gInterSource->JSopCmp(mVal0_, mVal1_, OP_ge, expr.primType);
    }
    OPCATCHANDGOON(mre_instr_t);
  }

label_OP_gt:
  {
    // Handle expression node: gt
    mre_instr_t &expr = *(reinterpret_cast<mre_instr_t *>(func.pc));
    DEBUGOPCODE(gt, Expr);
    TValue  &mVal1 = MPOP();
    TValue  &mVal0 = MPOP();
    FAST_COMPARE(>);
    CHECKREFERENCEMVALUE(mVal1);
    CHECKREFERENCEMVALUE(mVal0);
    MValue mVal1_ = TValue2MValue(mVal1);
    MValue mVal0_ = TValue2MValue(mVal0);
    bool isEhHappend = false;
    void *newPc = nullptr;
    MValue res;
    try {
      res = gInterSource->JSopCmp(mVal0_, mVal1_, OP_gt, expr.primType);
    }
    OPCATCHANDGOON(mre_instr_t);
  }

label_OP_le:
  {
    // Handle expression node: le
    mre_instr_t &expr = *(reinterpret_cast<mre_instr_t *>(func.pc));
    DEBUGOPCODE(le, Expr);
    TValue  &mVal1 = MPOP();
    TValue  &mVal0 = MPOP();
    FAST_COMPARE(<=);
    CHECKREFERENCEMVALUE(mVal1);
    CHECKREFERENCEMVALUE(mVal0);
    MValue mVal1_ = TValue2MValue(mVal1);
    MValue mVal0_ = TValue2MValue(mVal0);
    bool isEhHappend = false;
    void *newPc = nullptr;
    MValue res;
    try {
      res = gInterSource->JSopCmp(mVal0_, mVal1_, OP_le, expr.primType);
    }
    OPCATCHANDGOON(mre_instr_t);
  }

label_OP_lt:
  {
    // Handle expression node: lt
    mre_instr_t &expr = *(reinterpret_cast<mre_instr_t *>(func.pc));
    DEBUGOPCODE(lt, Expr);
    TValue  &mVal1 = MPOP();
    TValue  &mVal0 = MPOP();
    FAST_COMPARE(<);
    CHECKREFERENCEMVALUE(mVal1);
    CHECKREFERENCEMVALUE(mVal0);
    MValue mVal1_ = TValue2MValue(mVal1);
    MValue mVal0_ = TValue2MValue(mVal0);
    bool isEhHappend = false;
    void *newPc = nullptr;
    MValue res;
    try {
      res = gInterSource->JSopCmp(mVal0_, mVal1_, OP_lt, expr.primType);
    }
    OPCATCHANDGOON(mre_instr_t);
  }

label_OP_ne:
  {
    // Handle expression node: ne
    mre_instr_t &expr = *(reinterpret_cast<mre_instr_t *>(func.pc));
    DEBUGOPCODE(ne, Expr);
    TValue  &mVal1 = MPOP();
    TValue  &mVal0 = MPOP();
    FAST_COMPARE(!=);
    CHECKREFERENCEMVALUE(mVal1);
    CHECKREFERENCEMVALUE(mVal0);
    MValue mVal1_ = TValue2MValue(mVal1);
    MValue mVal0_ = TValue2MValue(mVal0);
    bool isEhHappend = false;
    void *newPc = nullptr;
    MValue res;
    try {
      res = gInterSource->JSopCmp(mVal0_, mVal1_, OP_ne, expr.primType);
    }
    OPCATCHANDGOON(mre_instr_t);
  }

label_OP_cmp:
  {
    // Handle expression node: cmp
    mre_instr_t &expr = *(reinterpret_cast<mre_instr_t *>(func.pc));
    DEBUGOPCODE(cmp, Expr);
    EXPRCMPLGOP_T(cmp, 1, expr.primType, expr.GetOpPtyp()); // if any operand is NaN, the result is definitely not 0.
    func.pc += sizeof(mre_instr_t);
    goto *(labels[*func.pc]);
  }

label_OP_cmpl:
  {
    // Handle expression node: cmpl
    mre_instr_t &expr = *(reinterpret_cast<mre_instr_t *>(func.pc));
    DEBUGOPCODE(cmpl, Expr);
    EXPRCMPLGOP_T(cmpl, -1, expr.primType, expr.GetOpPtyp());
    func.pc += sizeof(mre_instr_t);
    goto *(labels[*func.pc]);
  }

label_OP_cmpg:
  {
    // Handle expression node: cmpg
    mre_instr_t &expr = *(reinterpret_cast<mre_instr_t *>(func.pc));
    DEBUGOPCODE(cmpg, Expr);
    EXPRCMPLGOP_T(cmpg, 1, expr.primType, expr.GetOpPtyp());
    func.pc += sizeof(mre_instr_t);
    goto *(labels[*func.pc]);
  }

label_OP_select:
  {
    // Handle expression node: select
    ternary_node_t &expr = *(reinterpret_cast<ternary_node_t *>(func.pc));
    DEBUGOPCODE(select, Expr);
    EXPRSELECTOP_T();
    func.pc += sizeof(ternary_node_t);
    goto *(labels[*func.pc]);
  }

label_OP_extractbits:
  {
    // Handle expression node: extractbits
    mre_instr_t &expr = *(reinterpret_cast<mre_instr_t *>(func.pc));
    DEBUGOPCODE(extractbits, Expr);

    uint64 mask = ((1ull << expr.param.extractbits.bsize) - 1) << expr.param.extractbits.boffset;
    TValue &op = MTOP();
    op.x.i64 = (uint64)(op.x.i64 & mask) >> expr.param.extractbits.boffset;
    //op.ptyp = expr.primType;

    func.pc += sizeof(mre_instr_t);
    goto *(labels[*func.pc]);
  }

label_OP_ireadpcoff:
  {
#if 0
    // Handle expression node: ireadpcoff
    ireadpcoff_node_t &expr = *(reinterpret_cast<ireadpcoff_node_t *>(func.pc));
    DEBUGOPCODE(ireadpcoff, Expr);

    // Generated from addrof for symbols defined in other module
    MValue res;
    auto addr = (uint8_t*)&expr.offset + expr.offset;
    //(addr);
    mload(addr, expr.primType, res);
    ENCODE_MPUSH(res);
#endif
    func.pc += sizeof(ireadpcoff_node_t);
    goto *(labels[*func.pc]);
  }

label_OP_addroffpc:
  {
    // Handle expression node: addroffpc
    addroffpc_node_t &expr = *(reinterpret_cast<addroffpc_node_t *>(func.pc));
    DEBUGOPCODE(addroffpc, Expr);

    MValue target;
    target.ptyp = expr.primType == kPtyInvalid ? PTY_a64 : expr.primType; // Workaround for kPtyInvalid type
    target.x.a64 = (uint8_t*)&expr.offset + expr.offset;
    ENCODE_MPUSH(target);

    func.pc += sizeof(addroffpc_node_t);
    goto *(labels[*func.pc]);
  }

label_OP_dassign:
  {
    // Handle statement node: dassign
    mre_instr_t &stmt = *(reinterpret_cast<mre_instr_t *>(func.pc));
    int32_t idx = (int32_t)stmt.param.frameIdx;
    DEBUGSOPCODE(dassign, Stmt, idx);

    TValue &res = MPOP();
    //MASSERT(res.ptyp == stmt.primType, "Type mismatch: 0x%02x and 0x%02x", res.ptyp, stmt.primType);
    CHECKREFERENCEMVALUE(res);
    if(idx > 0) {
        // MARGS(idx) = res;
        }
    else {
        //mEncode(res);
        MLOCALS(-idx).x.u64 = res.x.u64;
    }
    func.pc += sizeof(mre_instr_t);
    goto *(labels[*func.pc]);
  }

label_OP_iassign:
  {
    // Handle statement node: iassign
    iassignoff_stmt_t &stmt = *(reinterpret_cast<iassignoff_stmt_t *>(func.pc));
    DEBUGOPCODE(iassign, Stmt);

    // Lower iassign to iassignoff
    TValue &res = MPOP();

    CHECKREFERENCEMVALUE(res);
    MValue res_ = TValue2MValue(res);
    auto addr = (uint8_t*)&stmt.offset + stmt.offset;
    //(addr);
    mstore(addr, stmt.primType, res_);

    func.pc += sizeof(iassignoff_stmt_t);
    goto *(labels[*func.pc]);
  }

label_OP_iassignoff:
  {
    // Handle statement node: iassignoff
    mre_instr_t &stmt = *(reinterpret_cast<mre_instr_t *>(func.pc));
    DEBUGCOPCODE(iassignoff, Stmt);
    TValue &res = MPOP();
    TValue &base = MPOP();
    if (IS_NEEDRC(res.x.u64)) {
      GCIncRf((void *)(res.x.u64));
    }
    uint8_t* addr = (uint8_t*)(base.x.u64 & PAYLOAD_MASK) + (int32_t)stmt.param.offset;
    uint64_t v = (uint64_t)*addr;
    if (IS_NEEDRC(v)) {
      GCDecRf((void *)(v & PAYLOAD_MASK));
    }
    *(uint64_t *)addr = res.x.u64;

    func.pc += sizeof(mre_instr_t);
    goto *(labels[*func.pc]);
  }

label_OP_iassignoff32:
  {
    iassignoff_stmt_t &stmt = *(reinterpret_cast<iassignoff_stmt_t *>(func.pc));
    DEBUGOPCODE(iassignoff32, Stmt);

    TValue &res = MPOP();
    TValue &base = MPOP();
    //(base.x.a64);
    MValue res_ = TValue2MValue(res);
    MValue base_ = TValue2MValue(base);
    auto addr = base_.x.a64 + stmt.offset;
    mstore(addr, stmt.primType, res_);

    func.pc += sizeof(iassignoff_stmt_t);
    goto *(labels[*func.pc]);
  }

label_OP_regassign:
  {
    // Handle statement node: regassign
    mre_instr_t &stmt = *(reinterpret_cast<mre_instr_t *>(func.pc));
    int32_t idx = (int32_t)stmt.param.frameIdx;
    TValue &res = MPOP();
    CHECKREFERENCEMVALUE(res);
    DEBUGSOPCODE(regassign, Stmt, idx);
    switch (idx) {
     case -kSregRetval0: {
      SetRetval0NoEncode(res);
      break;
    }
     case -kSregThrownval:
      // return interpreter->currEH->GetThrownval();
      assert(false && "NYI");
      break;
    default:
      assert(false && "NYI");
      break;
    }

    func.pc += sizeof(mre_instr_t);
    goto *(labels[*func.pc]);
  }

label_OP_igoto:
  assert(false); // To support GCC's indirect goto
label_OP_goto:
  assert(false); // Will have compact encoding version
label_OP_goto32:
  {
    // Handle statement node: goto
    goto_stmt_t &stmt = *(reinterpret_cast<goto_stmt_t *>(func.pc));
    // mre_instr_t &stmt = *(reinterpret_cast<mre_instr_t *>(func.pc));
    // gInterSource->currEH->FreeEH();
    DEBUGOPCODE(goto32, Stmt);

   // if(*(func.pc + sizeof(goto_stmt_t)) == OP_endtry)
   //     func.try_catch_pc = nullptr;

    // func.pc += sizeof(mre_instr_t);
    func.pc = (uint8_t*)&stmt.offset + stmt.offset;
    goto *(labels[*func.pc]);
  }

label_OP_brfalse:
  assert(false); // Will have compact encoding version
label_OP_brfalse32:
  {
    // Handle statement node: brfalse
    condgoto_stmt_t &stmt = *(reinterpret_cast<condgoto_stmt_t *>(func.pc));
    DEBUGOPCODE(brfalse32, Stmt);

    TValue &cond = MPOP();
    if(cond.x.u1)
        func.pc += sizeof(condgoto_stmt_t);
    else
        func.pc = (uint8_t*)&stmt.offset + stmt.offset;
    goto *(labels[*func.pc]);
  }

label_OP_brtrue:
  assert(false); // Will have compact encoding version
label_OP_brtrue32:
  {
    // Handle statement node: brtrue
    condgoto_stmt_t &stmt = *(reinterpret_cast<condgoto_stmt_t *>(func.pc));
    DEBUGOPCODE(brtrue32, Stmt);

    TValue &cond = MPOP();
    if(cond.x.u1)
        func.pc = (uint8_t*)&stmt.offset + stmt.offset;
    else
        func.pc += sizeof(condgoto_stmt_t);
    goto *(labels[*func.pc]);
  }

label_OP_return:
  {
    // Handle statement node: return
    DEBUGOPCODE(return, Stmt);

    MValue ret;
    ret.x.u64 = 0; // return 0 means ok
    ret.ptyp = JSTYPE_NONE;
    gInterSource->InsertEplog();
    MVALUEBITMASK(ret); // If returning void, it is set to {0x0, PTY_void}
    return ret;
  }

label_OP_rangegoto:
  {
    // Handle statement node: rangegoto
    mre_instr_t &stmt = *(reinterpret_cast<mre_instr_t *>(func.pc));
    DEBUGOPCODE(rangegoto, Stmt);
    int32_t adjusted = *(int32_t*)(func.pc + sizeof(mre_instr_t));
    TValue &val = MPOP();
    int32_t idx = val.x.i32 - adjusted;
    MASSERT(idx < stmt.param.numCases, "Out of range: index = %d, numCases = %d", idx, stmt.param.numCases);
    func.pc += sizeof(mre_instr_t) + sizeof(int32_t) + idx * 4;
    func.pc += *(int32_t*)func.pc;
    goto *(labels[*func.pc]);
  }

label_OP_call:
  {
    // Handle statement node: call
    // call_stmt_t &stmt = *(reinterpret_cast<call_stmt_t *>(func.pc));
    mre_instr_t &stmt = *(reinterpret_cast<mre_instr_t *>(func.pc));
    DEBUGOPCODE(call, Stmt);
    // get the parameters
    MValue args[MAXCALLARGNUM];
    int numArgs = stmt.param.intrinsic.numOpnds;
    int startArg = 1;
    int i = 0;
    // assert(numArgs < MAXCALLARGNUM && "too many args");
    for (i = 0; i < numArgs - startArg; i ++) {
      TValue &v = MPOP();
      MValue v_ = TValue2MValue(v);
      args[numArgs - startArg - i - 1] = v_;
    }
    MValue thisVal,env;
    thisVal = env = NullPointValue();
    int32_t offset = gInterSource->PassArguments(thisVal, (void *)env.x.u64, args,
                    numArgs - startArg, -1);
    gInterSource->sp += offset;
    // __jsvalue this_arg = MvalToJsval(thisVal);
    // __jsvalue old_this = __js_entry_function(&this_arg, false);
    constval_node_t &expr = *(reinterpret_cast<constval_node_t *>(func.pc));
    //(call, Stmt);
    MValue res;
    res.ptyp = expr.primType;
    res.x.u64 = ((uint64_t)expr.constVal.value);
    MValue val;
    DynamicMethodHeaderT* calleeHeader = (DynamicMethodHeaderT*)(res.x.a64 + 4);
    val = maple_invoke_dynamic_method (calleeHeader, nullptr);
    // __js_exit_function(&this_arg, old_this, false);
    gInterSource->sp -= offset;
    gInterSource->SetCurFunc(&func);
    // func.direct_call(stmt.primType, stmt.numOpnds, func.pc);

    // Skip the function name
    func.pc += sizeof(constval_node_t);
    goto *(labels[*func.pc]);
  }

label_OP_icall:
  {
    mre_instr_t &stmt = *(reinterpret_cast<mre_instr_t *>(func.pc));
    DEBUGOPCODE(icall, Stmt);
    // get the parameters
    MValue args[MAXCALLARGNUM];
    int numArgs = stmt.param.intrinsic.numOpnds;
    //MASSERT(numArgs >= 2, "num of args of icall should be gt than 2");
    int i = 0;
    for (i = 0; i < numArgs - 1; i ++) {
      TValue &v = MPOP();
      MValue v_ = TValue2MValue(v);
      args[numArgs - i - 1] = v_;
    }
    TValue &args0 = MPOP();
    args0.x.u64 &= PAYLOAD_MASK;
    args[0] = (__function_value((void *)args0.x.u64));
    MValue retCall = gInterSource->FuncCall((void *)args0.x.u64, false, nullptr, args, numArgs, 2, -1, false);

    if (retCall.x.u64 == (uint64_t) Exec_handle_exc && retCall.ptyp == JSTYPE_NONE) {
      void *newPc = gInterSource->currEH->GetEHpc(&func);
      if (newPc) {
        func.pc = (uint8_t *)newPc;
        goto *(labels[*func.pc]);
      }
      gInterSource->InsertEplog();
      // gInterSource->FinishFunc();
      return retCall; // continue to unwind
    } else {
      // the first is the addr of callee, the second is to be ignore
      func.pc += sizeof(mre_instr_t);
      goto *(labels[*func.pc]);
    }
  }

label_OP_intrinsiccall:
  {
    // Handle statement node: intrinsiccall
    mre_instr_t &stmt = *(reinterpret_cast<mre_instr_t *>(func.pc));
    uint32_t numOpnds = stmt.param.intrinsic.numOpnds;
    DEBUGCOPCODE(intrinsiccall, Stmt);
    MIRIntrinsicID intrnid = (MIRIntrinsicID)stmt.param.intrinsic.intrinsicId;
    uint32_t argnums = numOpnds;
    bool isEhHappend = false;
    void *newPc = nullptr;
    switch (intrnid) {
      case INTRN_JS_NEW: {
        TValue &conVal = MPOP();
        MValue conVal_ = TValue2MValue(conVal);
        MValue retMv = gInterSource->JSopNew(conVal_);
        SetRetval0(retMv);
        break;
      }
      case INTRN_JS_INIT_CONTEXT: {
        TValue &v0 = MPOP();
        __js_init_context(v0.x.u1);
      }
      break;
    case INTRN_JS_STRING:
      MIR_ASSERT(argnums == 1);
      try {
        TValue &v0 = MPOP(); MValue v0_ = TValue2MValue(v0);
        MValue retMv = gInterSource->JSString(v0_);
        SetRetval0(retMv);
      }
      CATCHINTRINSICOP();
      break;
    case INTRN_JS_BOOLEAN: {
      MIR_ASSERT(argnums == 1);
      TValue &v0 = MPOP();
      CHECKREFERENCEMVALUE(v0);
      MValue v0_ = TValue2MValue(v0);
      MValue retMv = gInterSource->JSBoolean(v0_);
      SetRetval0(retMv);
      break;
    }
    case INTRN_JS_NUMBER: {
      MIR_ASSERT(argnums == 1);

      try {
        TValue &v0 = MPOP();
        if (IS_NUMBER(v0.x.u64) || IS_DOUBLE(v0.x.u64)) {
          SetRetval0NoEncode(v0);
        } else {
          MValue v0_ = TValue2MValue(v0);
          MValue retMv = gInterSource->JSNumber(v0_);
          SetRetval0(retMv);
        }
      }
      CATCHINTRINSICOP();
      break;
    }
    case INTRN_JSOP_CONCAT: {
      MIR_ASSERT(argnums == 2);
      TValue &arg1 = MPOP();
      MValue arg1_ = TValue2MValue(arg1);
      TValue &arg0 = MPOP(); MValue arg0_ = TValue2MValue(arg0);
      MValue retMv = gInterSource->JSopConcat(arg0_, arg1_);
      SetRetval0(retMv);
      break;
    }  // Objects
    case INTRN_JS_NEW_OBJECT_0: {
      MIR_ASSERT(argnums == 0);
      MValue retMv; // = gInterSource->JSopNewObj0();
      retMv.x.a64 = (uint8_t *) __js_new_obj_obj_0();
      retMv.ptyp = JSTYPE_OBJECT;
      SetRetval0(retMv);
      break;
    }
    case INTRN_JS_NEW_OBJECT_1: {
      MIR_ASSERT(argnums >= 1);
      for(int tmp = argnums; tmp > 1; --tmp) {
          MPOP();
      }
      TValue &v0 = MPOP();
      MValue v0_ = TValue2MValue(v0);
      v0_.x.a64 = (uint8_t *) __js_new_obj_obj_1(&v0_);
      v0_.ptyp = JSTYPE_OBJECT;
      SetRetval0(v0_);
      break;
    }
    case INTRN_JSOP_INIT_THIS_PROP_BY_NAME: {
      MIR_ASSERT(argnums == 1);
      TValue &v0 = MPOP();
      __jsstring *v1 = (__jsstring *)(v0.x.u64 & PAYLOAD_MASK);
      __jsvalue &v = __js_Global_ThisBinding;
      __jsop_init_this_prop_by_name(&v, v1);
      break;
    }
    case INTRN_JSOP_SET_THIS_PROP_BY_NAME:{
      MIR_ASSERT(argnums == 2);
      TValue &arg1 = MPOP();
      TValue &arg0 = MPOP();
      named_prop_cache[1].o = 0; // invalidate cache
      CHECKREFERENCEMVALUE(arg1);
      MValue arg1_ = TValue2MValue(arg1);
      try {
        __jsvalue &v0 = __js_Global_ThisBinding;
        __jsstring *v1 = (__jsstring *)(arg0.x.u64 & PAYLOAD_MASK);
        if(is_strict && (__is_undefined(&__js_ThisBinding) ||
              __js_SameValue(&__js_Global_ThisBinding, &__js_ThisBinding)) &&
          __jsstr_throw_typeerror(v1)) {
          MAPLE_JS_TYPEERROR_EXCEPTION();
        }
        __jsop_set_this_prop_by_name(&v0, v1, &arg1_, true);
      }
      CATCHINTRINSICOP();
      break;
    }
    case INTRN_JSOP_SETPROP_BY_NAME: {
      MIR_ASSERT(argnums == 3);
      TValue &v2 = MPOP();
      TValue &v1 = MPOP();
      TValue &v0 = MPOP();
      named_prop_cache[0].o = 0; // invalidate cache
      CHECKREFERENCEMVALUE(v0);
      MValue v2_ = TValue2MValue(v2);
      MValue v0_ = TValue2MValue(v0);
      try {
        __jsstring *s1 = (__jsstring *)(v1.x.u64 & PAYLOAD_MASK);
        if (v0_.x.asbits == __js_Global_ThisBinding.x.asbits &&
          __is_global_strict && __jsstr_throw_typeerror(s1)) {
          MAPLE_JS_TYPEERROR_EXCEPTION();
        }
        __jsop_setprop_by_name(&v0_, s1, &v2_, is_strict);
      }
      CATCHINTRINSICOP();
      break;
    }
    case INTRN_JSOP_GETPROP: {
      MIR_ASSERT(argnums == 2);
      TValue &v1 = MPOP();
      TValue &v0 = MPOP();
      CHECKREFERENCEMVALUE(v0);
      MValue v1_ = TValue2MValue(v1);
      MValue v0_ = TValue2MValue(v0);
      try {
        MValue retMv = __jsop_getprop(&v0_, &v1_);
        SetRetval0(retMv);
      }
      CATCHINTRINSICOP();
      break;
    }
    case INTRN_JSOP_GETPROP_BY_NAME: {
      MIR_ASSERT(argnums == 2);
      TValue &v1 = MPOP();
      TValue &v0 = MPOP();
      if (named_prop_cache[0].o == v0.x.a64 && named_prop_cache[0].p == v1.x.a64) {
        SetRetval0(named_prop_cache[0].ret);;
        break;
      }
      CHECKREFERENCEMVALUE(v0);
      MValue v1_ = TValue2MValue(v1);
      MValue v0_ = TValue2MValue(v0);
      try {
        MValue retMv = gInterSource->JSopGetPropByName(v0_, v1_);
        SetRetval0(retMv);

        named_prop_cache[0].o = v0.x.a64;
        named_prop_cache[0].p = v1.x.a64;
        named_prop_cache[0].ret = retMv;
      }
      CATCHINTRINSICOP();
      break;
    }
    case INTRN_JS_DELNAME: {
      MIR_ASSERT(argnums == 2);
      TValue &v1 = MPOP();
      TValue &v0 = MPOP();
      CHECKREFERENCEMVALUE(v0);
      MValue v1_ = TValue2MValue(v1);
      MValue v0_ = TValue2MValue(v0);
      try {
        MValue retMv = gInterSource->JSopDelProp(v0_, v1_, is_strict);
        SetRetval0(retMv);
        named_prop_cache[0].o = 0; // invalidate cache
        named_prop_cache[1].o = 0; // invalidate cache
      }
      CATCHINTRINSICOP();
      break;
    }
    case INTRN_JSOP_DELPROP: {
      MIR_ASSERT(argnums == 2);
      TValue &v1 = MPOP();
      TValue &v0 = MPOP();
      CHECKREFERENCEMVALUE(v0);
      MValue v1_ = TValue2MValue(v1);
      MValue v0_ = TValue2MValue(v0);
      try {
        MValue retMv = gInterSource->JSopDelProp(v0_, v1_, is_strict);
        SetRetval0(retMv);
        named_prop_cache[0].o = 0; // invalidate cache
        named_prop_cache[1].o = 0; // invalidate cache
      }
      CATCHINTRINSICOP();
      break;
    }
    case INTRN_JSOP_INITPROP: {
      TValue &v2 = MPOP();
      TValue &v1 = MPOP();
      TValue &v0 = MPOP();
      MValue v2_ = TValue2MValue(v2);
      MValue v1_ = TValue2MValue(v1);
      MValue v0_ = TValue2MValue(v0);
      __jsop_initprop(&v0_, &v1_, &v2_);
      break;
    }
    case INTRN_JSOP_INITPROP_BY_NAME: {
      //MIR_ASSERT(argnums == 3);
      TValue &v2 = MPOP();
      TValue &v1 = MPOP();
      TValue &v0 = MPOP();
      MValue v2_ = TValue2MValue(v2);
      MValue v1_ = TValue2MValue(v1);
      MValue v0_ = TValue2MValue(v0);
      gInterSource->JSopInitPropByName(v0_, v1_, v2_);
      break;
    }
    case INTRN_JSOP_INITPROP_GETTER: {
      //MIR_ASSERT(argnums == 3);
      TValue &v2 = MPOP();
      TValue &v1 = MPOP();
      TValue &v0 = MPOP();
      MValue v2_ = TValue2MValue(v2);
      MValue v1_ = TValue2MValue(v1);
      MValue v0_ = TValue2MValue(v0);
      gInterSource->JSopInitPropGetter(v0_, v1_, v2_);
      break;
    }
    case INTRN_JSOP_INITPROP_SETTER: {
      //MIR_ASSERT(argnums == 3);
      TValue &v2 = MPOP();
      TValue &v1 = MPOP();
      TValue &v0 = MPOP();
      MValue v2_ = TValue2MValue(v2);
      MValue v1_ = TValue2MValue(v1);
      MValue v0_ = TValue2MValue(v0);
      gInterSource->JSopInitPropSetter(v0_, v1_, v2_);
      break;
    }
    // Array
    case INTRN_JS_NEW_ARR_ELEMS: {
      //MIR_ASSERT(argnums == 2);
      TValue &v1 = MPOP();
      TValue &v0 = MPOP();
      MValue v1_ = TValue2MValue(v1);
      MValue v0_ = TValue2MValue(v0);
      MValue retMv = gInterSource->JSopNewArrElems(v0_, v1_);
      SetRetval0(retMv);
      break;
    }
    // Statements
    case INTRN_JS_PRINT: {
        static uint8_t sp[] = {0,0,1,0,' '};
        static uint8_t nl[] = {0,0,1,0,'\n'};
        static uint64_t sp_u64, nl_u64;
        MValue m = {.ptyp=JSTYPE_STRING};
        if (sp_u64 == 0) {
          m.x.a64 = reinterpret_cast<uint8_t *>(&sp);
          sp_u64  = m.x.u64;
          m.x.a64 = reinterpret_cast<uint8_t *>(&nl);
          nl_u64  = m.x.u64;
        }
        for (uint32_t i = 0; i < numOpnds; i++) {
          TValue mval = func.operand_stack[func.sp - numOpnds + i + 1];
          MValue mval_ = TValue2MValue(mval);
          gInterSource->JSPrint(mval_);
          if (i != numOpnds-1) {
            // insert space between printed arguments
            m.x.u64 = sp_u64;
            gInterSource->JSPrint(m);
          }
          //mEncode(mval);
        }
        // pop arguments of JS_print
        func.sp -= numOpnds;
        // insert CR/LF at EOL
        m.x.u64 = nl_u64;
        gInterSource->JSPrint(m);
        // gInterSource->retVal0.x.u64 = 0;
        SetRetval0NoInc(0, NAN_NUMBER);
    }
    break;
    case INTRN_JS_ISNAN: {
      TValue &v0 = MPOP();
      SetRetval0NoInc(IS_NAN(v0.x.u64), NAN_BOOLEAN);
      break;
    }
      case INTRN_JS_DATE: {
        TValue &v0 = MPOP();
        MValue v0_ = TValue2MValue(v0);
        MValue retMv = gInterSource->JSDate(v0_);
        SetRetval0(retMv);
        break;
      }
      case INTRN_JS_NEW_FUNCTION: {
        TValue &attrsVal = MPOP();
        TValue &envVal = MPOP();
        TValue &fpVal = MPOP();
        MValue ret = (__js_new_function((void *)(fpVal.x.u64 & PAYLOAD_MASK), (void *)(envVal.x.u64 & PAYLOAD_MASK),
                                   attrsVal.x.u32, gInterSource->jsPlugin->fileIndex, true/*needpt*/));
        SetRetval0(ret);
      }
      break;
      case INTRN_JSOP_ADD: {
        //assert(numOpnds == 2 && "false");
        TValue &op1 = MPOP();
        TValue &op0 = MPOP();
        if (IS_NUMBER(op1.x.u64)) {
          if (IS_NUMBER(op0.x.u64)) {
            int64_t r = (int64_t)op0.x.i32 + (int64_t)op1.x.i32;
            if (ABS(r) > INT_MAX) {
              op0.x.f64 = (double)r;
            } else {
              op0.x.i32 = r;
            }
            SetRetval0NoEncode(op0);
            break;
          } else if (IS_DOUBLE(op0.x.u64)) {
            double r;
            r = op0.x.f64 + op1.x.i32;
            if (r == 0) {
              op0.x.u64 = POS_ZERO;
              SetRetval0NoEncode(op0);
              break;
            }
            if (ABS(r) <= NumberMaxValue) {
              op0.x.f64 = r;
              SetRetval0NoEncode(op0);
              break;
            }
          }
        } else if (IS_DOUBLE(op1.x.u64)) {
          double r;
          if (IS_DOUBLE(op0.x.u64)) {
            r = op1.x.f64 + op0.x.f64;
            if (r == 0) {
              op0.x.u64 = POS_ZERO;
              SetRetval0NoEncode(op0);
              break;
            }
            if (ABS(r) <= NumberMaxValue && ABS(r) >= NumberMinValue) {
              op1.x.f64 = r;
              SetRetval0NoEncode(op1);
              break;
            }
          } else if (IS_NUMBER(op0.x.u64)) {
            r = op1.x.f64 + op0.x.i32;
            if (r == 0) {
              op0.x.u64 = POS_ZERO;
              SetRetval0NoEncode(op0);
              break;
            }
            if (ABS(r) <= NumberMaxValue && ABS(r) >= NumberMinValue) {
              op1.x.f64 = r;
              SetRetval0NoEncode(op1);
              break;
            }
          }
        }
        CHECKREFERENCEMVALUE(op0);
        CHECKREFERENCEMVALUE(op1);
        MValue op1_ = TValue2MValue(op1);
        MValue op0_ = TValue2MValue(op0);
        try {
          MValue retMv = gInterSource->VmJSopAdd(op0_, op1_);
          SetRetval0(retMv);
        }
        CATCHINTRINSICOP();
      }
      break;
      case INTRN_JS_NEW_ARR_LENGTH: {
        TValue &conVal = MPOP();
        MValue conVal_ = TValue2MValue(conVal);
        MValue retMv;
        retMv.x.a64 = (uint8_t *) __js_new_arr_length(&conVal_);
        retMv.ptyp = JSTYPE_OBJECT;
        //MValue retMv = gInterSource->JSopNewArrLength(conVal);
        SetRetval0(retMv);
        break;
      }
      case INTRN_JSOP_SETPROP: {
        TValue &v2 = MPOP();
        TValue &v1 = MPOP();
        TValue &v0 = MPOP();
        MValue v2_ = TValue2MValue(v2);
        MValue v1_ = TValue2MValue(v1);
        MValue v0_ = TValue2MValue(v0);
        __jsop_setprop(&v0_, &v1_, &v2_);
        break;
      }
      case INTRN_JSOP_NEW_ITERATOR: {
        TValue &v1 = MPOP();
        TValue &v0 = MPOP();
        MValue v1_ = TValue2MValue(v1);
        MValue v0_ = TValue2MValue(v0);
        MValue retMv = gInterSource->JSopNewIterator(v0_, v1_);
        SetRetval0(retMv);
        break;
      }
      case INTRN_JSOP_NEXT_ITERATOR: {
        TValue &arg = MPOP();
        MValue arg_ = TValue2MValue(arg);
        MValue retMv = gInterSource->JSopNextIterator(arg_);
        SetRetval0(retMv);
        break;
      }
      case INTRN_JSOP_MORE_ITERATOR: {
        TValue &arg = MPOP();
        MValue arg_ = TValue2MValue(arg);
        SetRetval0NoInc(gInterSource->JSopMoreIterator(arg_).x.u32, NAN_BOOLEAN);
        break;
      }
      case INTRN_JSOP_CALL:
      case INTRN_JSOP_NEW: {
        MValue args[MAXCALLARGNUM];
        int numArgs = stmt.param.intrinsic.numOpnds;
        int i = 0;
        //assert(numArgs < MAXCALLARGNUM && numArgs >= 2 && "num of args of jsop call is wrong");
        for (i = 0; i < numArgs; i ++) {
          TValue &v0 = MPOP();
          if ((numArgs - i - 1) == 0)
            CHECKREFERENCEMVALUE(v0);
          MValue v0_ = TValue2MValue(v0);
          args[numArgs - i - 1] = v0_;
        }
        try {
          MValue retCall = gInterSource->IntrinCall(intrnid, args, numArgs);
          if (retCall.x.u64 == (uint64_t) Exec_handle_exc && retCall.ptyp == JSTYPE_NONE) {
            isEhHappend = true;
            newPc = gInterSource->currEH->GetEHpc(&func);
            // gInterSource->InsertEplog();
            // gInterSource->FinishFunc();
            // return retCall; // continue to unwind
          }
        } catch(const char *estr) {
          MValue m;
          isEhHappend = true;
          bool isTypeError = false;
          bool isRangeError = false;
          bool isUriError = false;
          bool isSyntaxError = false;
          bool isRefError = false;
          if (!strcmp(estr, "TypeError")) {
            m = gInterSource->GetOrCreateBuiltinObj(JSBUILTIN_TYPEERROR_CONSTRUCTOR);
            isTypeError = true;
          } else if (!strcmp(estr, "RangeError")) {
            m = gInterSource->GetOrCreateBuiltinObj(JSBUILTIN_RANGEERROR_CONSTRUCTOR);
            isRangeError = true;
          } else if (!strcmp(estr, "SyntaxError")) {
            m = gInterSource->GetOrCreateBuiltinObj(JSBUILTIN_SYNTAXERROR_CONSTRUCTOR);
            isSyntaxError = true;
          } else if (!strcmp(estr, "UriError")) {
            m = gInterSource->GetOrCreateBuiltinObj(JSBUILTIN_URIERROR_CONSTRUCTOR);
            isUriError = true;
          } else if (!strcmp(estr, "ReferenceError")) {
            m = gInterSource->GetOrCreateBuiltinObj(JSBUILTIN_REFERENCEERRORCONSTRUCTOR);
            isRefError = true;
          } else {
            m = (__string_value(__jsstr_new_from_char(estr)));
          }
          if (!gInterSource->currEH) {
            if (isTypeError) {
              fprintf(stderr, "TypeError:  not a function");
            } else if (isRangeError) {
              fprintf(stderr, "RangeError:  not a function");
            } else if (isSyntaxError) {
              fprintf(stderr, "SyntaxError:  not a function");
            } else if (isUriError) {
              fprintf(stderr, "UriError:  not a catched");
            } else if (isRefError) {
              fprintf(stderr, "ReferenceError: not defined");
            } else {
              fprintf(stderr, "eh thown but never catched");
            }
            exit(3);
          }

          if (isTypeError || isRangeError || isSyntaxError || isUriError || (gInterSource->currEH->GetThrownval()).x.u64 == 0) {
            gInterSource->currEH->SetThrownval(m);
          }
          gInterSource->currEH->UpdateState(OP_throw);
          newPc = gInterSource->currEH->GetEHpc(&func);
        }
        break;
      }
      case INTRN_JS_ERROR: {
        MValue args[MAXCALLARGNUM];
        int numArgs = stmt.param.intrinsic.numOpnds;
        for (int i = 0; i < numArgs; i ++) {
          TValue &v0 = MPOP();
          MValue v0_ = TValue2MValue(v0);
          args[numArgs - i - 1] = v0_;
        }
        gInterSource->IntrnError(args, numArgs);
        break;
      }
      case INTRN_JSOP_CCALL: {
        MValue args[MAXCALLARGNUM];
        int numArgs = stmt.param.intrinsic.numOpnds;
        for (int i = 0; i < numArgs; i ++) {
          TValue &v0 = MPOP();
          MValue v0_ = TValue2MValue(v0);
          args[numArgs - i - 1] = v0_;
        }
        gInterSource->IntrinCCall(args, numArgs);
        break;
      }
      case INTRN_JS_REQUIRE: {
        TValue &v0 = MPOP();
        MValue v0_ = TValue2MValue(v0);
        gInterSource->JSopRequire(v0_);
        break;
      }
      case INTRN_JSOP_ASSERTVALUE: {
        // no need to do anything
        TValue &mv = MPOP();// TValue2MValue(mv);
        CHECKREFERENCEMVALUE(mv);
        // gInterSource->retVal0.x.u64 = mv.x.u64;
        break;
      }
      default:
        MASSERT(false, "Hit OP_intrinsiccall with id: 0x%02x", (int)intrnid);
        break;
    }
    if (isEhHappend) {
      if (newPc) {
         func.pc = (uint8_t *)newPc;
         goto *(labels[*(uint8_t *)newPc]);
      } else {
         gInterSource->InsertEplog();
            // gInterSource->FinishFunc();
         MValue ret;
         ret.x.u64 = (uint64_t) Exec_handle_exc;
         ret.ptyp = JSTYPE_NONE;
        return ret;
      }
    } else {
      func.pc += sizeof(mre_instr_t);
      goto *(labels[*func.pc]);
    }
  }

label_OP_javatry:
  {
    // Handle statement node: javatry
    MIR_FATAL("Error: hit OP_javacatch unexpectedly");
    mre_instr_t &stmt = *(reinterpret_cast<mre_instr_t *>(func.pc));
    DEBUGOPCODE(javatry, Stmt);

    // func.try_catch_pc = func.pc;
    // Skips the try-catch table
    func.pc += stmt.param.numCases * 4 + sizeof(mre_instr_t);
    goto *(labels[*func.pc]);
  }

label_OP_throw:
  {
    mre_instr_t &stmt = *(reinterpret_cast<mre_instr_t *>(func.pc));
    DEBUGOPCODE(throw, Stmt);
    //MIR_ASSERT(gInterSource->currEH);
    TValue &m = MPOP();
    MValue m_ = TValue2MValue(m);
    if (!gInterSource->currEH) {
      __jsvalue jval = m_;
      __jsstring *msg = __jsstr_get_builtin(JSBUILTIN_STRING_EMPTY);
      if (__is_string(&jval)) {
        msg = __jsval_to_string(&jval);
      }
      PrintUncaughtException(msg);
    }
    gInterSource->currEH->SetThrownval(m_);
    gInterSource->currEH->UpdateState(OP_throw);
    void *newPc = gInterSource->currEH->GetEHpc(&func);
    if (newPc) {
      func.pc = (uint8_t *)newPc;
      goto *(labels[*(uint8_t *)newPc]);
    } else {
      gInterSource->InsertEplog();
      // gInterSource->FinishFunc();
      MValue ret;
      ret.x.u64 = (uint64_t) Exec_handle_exc;
      ret.ptyp = JSTYPE_NONE;
      return ret;
    }
  }

label_OP_javacatch:
  {
    // Handle statement node: javacatch
    DEBUGOPCODE(javacatch, Stmt);
    // OP_javacatch is handled at label_exception_handler
    MASSERT(true, "Hit OP_javacatch unexpectedly");
    MIR_FATAL("Error: hit OP_javacatch unexpectedly");
    //func.pc += stmt.param.numCases * 4 + sizeof(mre_instr_t);
    //goto *(labels[*func.pc]);
  }

label_OP_cleanuptry:
{
    DEBUGOPCODE(cleanuptry, Stmt);
    goto_stmt_t &stmt = *(reinterpret_cast<goto_stmt_t *>(func.pc));
    gInterSource->currEH->FreeEH();
    func.pc += sizeof(base_node_t);
    goto *(labels[*func.pc]);
}
label_OP_endtry:
  {
    // Handle statement node: endtry
    DEBUGOPCODE(endtry, Stmt);
    // func.try_catch_pc = nullptr;
    // mre_instr_t &stmt = *(reinterpret_cast<mre_instr_t *>(func.pc));
    goto_stmt_t &stmt = *(reinterpret_cast<goto_stmt_t *>(func.pc));
    gInterSource->currEH->FreeEH();
    func.pc += sizeof(base_node_t);
    goto *(labels[*func.pc]);
  }

label_OP_membaracquire:
  {
    // Handle statement node: membaracquire
    //(membaracquire, Stmt);
    // Every load on X86_64 implies load acquire semantics
    DEBUGOPCODE(membaracquire, Stmt);
    func.pc += sizeof(base_node_t);
    goto *(labels[*func.pc]);
  }

label_OP_membarrelease:
  {
    // Handle statement node: membarrelease
    DEBUGOPCODE(membarrelease, Stmt);
    // Every store on X86_64 implies store release semantics
    func.pc += sizeof(base_node_t);
    goto *(labels[*func.pc]);
  }

label_OP_membarstoreload:
  {
    // Handle statement node: membarstoreload
    DEBUGOPCODE(membarstoreload, Stmt);
    // X86_64 has strong memory model
    func.pc += sizeof(base_node_t);
    goto *(labels[*func.pc]);
  }

label_OP_membarstorestore:
  {
    // Handle statement node: membarstorestore
    DEBUGOPCODE(membarstorestore, Stmt);
    // X86_64 has strong memory model
    func.pc += sizeof(base_node_t);
    goto *(labels[*func.pc]);
  }

label_OP_iassignpcoff:
  {
    // Handle statement node: iassignpcoff
    DEBUGOPCODE(iassignpcoff, Stmt);
    MASSERT(false, "Not supported yet");
    func.pc += sizeof(base_node_t);
    goto *(labels[*func.pc]);
  }

label_OP_checkpoint:
  {
    // Handle statement node: checkpoint
    DEBUGOPCODE(checkpoint, Stmt);
    // MRT_YieldpointHandler_x86_64();
    func.pc += sizeof(base_node_t);
    goto *(labels[*func.pc]);
  }

label_OP_iaddrof:
  {
    // Handle expression node: iaddrof
    DEBUGOPCODE(iaddrof, Expr);
    MASSERT(false, "Not supported yet");
    func.pc += sizeof(iread_node_t);
    goto *(labels[*func.pc]);
  }

label_OP_array:
  {
    // Handle expression node: array
    DEBUGOPCODE(array, Expr);
    MASSERT(false, "Not supported yet");
    func.pc += sizeof(array_node_t);
    goto *(labels[*func.pc]);
  }

label_OP_ireadfpoff: // offset from stack frame
  {
    // Handle expression node: ireadfpoff
    mre_instr_t &expr = *(reinterpret_cast<mre_instr_t *>(func.pc));
    DEBUGOPCODE(ireadfpoff, Expr);
    int32_t offset = (int32_t)expr.param.offset;
    uint8 *addr = frame_pointer + offset;
    //MValue  mv;
    //mv.x.u64 =  *((uint64_t *)addr);
    uint64_t lValue = *((uint64_t *)addr);
    if (lValue == 0) {
      lValue = NONE_VALUE; // NONE value
    }
    MPUSHV(lValue);

    func.pc += sizeof(mre_instr_t);
    goto *(labels[*func.pc]);
  }

label_OP_addroflabel:
  {
    // Handle expression node: addroflabel
    DEBUGOPCODE(addroflabel, Expr);
    MASSERT(false, "Not supported yet");
    func.pc += sizeof(addroflabel_node_t);
    goto *(labels[*func.pc]);
  }

label_OP_ceil:
  {
    // Handle expression node: ceil
    DEBUGOPCODE(ceil, Expr);
    MASSERT(false, "Not supported yet");
    func.pc += sizeof(mre_instr_t);
    goto *(labels[*func.pc]);
  }

label_OP_floor:
  {
    // Handle expression node: floor
    DEBUGOPCODE(floor, Expr);
    MASSERT(false, "Not supported yet");
    func.pc += sizeof(mre_instr_t);
    goto *(labels[*func.pc]);
  }

label_OP_round:
  {
    // Handle expression node: round
    DEBUGOPCODE(round, Expr);
    MASSERT(false, "Not supported yet");
    func.pc += sizeof(mre_instr_t);
    goto *(labels[*func.pc]);
  }

label_OP_trunc:
  {
    // Handle expression node: trunc
    DEBUGOPCODE(trunc, Expr);
    MASSERT(false, "Not supported yet");
    func.pc += sizeof(mre_instr_t);
    goto *(labels[*func.pc]);
  }

label_OP_abs:
  {
    // Handle expression node: abs
    DEBUGOPCODE(abs, Expr);
    mre_instr_t &expr = *(reinterpret_cast<mre_instr_t *>(func.pc));
    JSUNARY();
    func.pc += sizeof(mre_instr_t);
    goto *(labels[*func.pc]);
  }

label_OP_recip:
  {
    // Handle expression node: recip
    DEBUGOPCODE(recip, Expr);
    mre_instr_t &expr = *(reinterpret_cast<mre_instr_t *>(func.pc));
    JSUNARY();
    func.pc += sizeof(mre_instr_t);
    goto *(labels[*func.pc]);
  }

label_OP_lnot:
  {
    // Handle expression node: neg
    DEBUGOPCODE(lnot, Expr);
    mre_instr_t &expr = *(reinterpret_cast<mre_instr_t *>(func.pc));
    TValue &mv0 = MPOP();
    CHECKREFERENCEMVALUE(mv0);
    MValue mv0_ = TValue2MValue(mv0);
    bool isEhHappend = false;
    void *newPc = nullptr;
    MValue res;
    try {
      res = gInterSource->JSopUnaryLnot(mv0_);
    }
    OPCATCHANDGOON(mre_instr_t);
  }
label_OP_bnot:
  {
    // Handle expression node: neg
    DEBUGOPCODE(bnot, Expr);
    mre_instr_t &expr = *(reinterpret_cast<mre_instr_t *>(func.pc));
    TValue &mv0 = MPOP();
    CHECKREFERENCEMVALUE(mv0);
    MValue mv0_ = TValue2MValue(mv0);
    bool isEhHappend = false;
    void *newPc = nullptr;
    MValue res;
    try {
      res = gInterSource->JSopUnaryBnot(mv0_);
    }
    OPCATCHANDGOON(mre_instr_t);
  }
label_OP_neg:
  {
    // Handle expression node: neg
    DEBUGOPCODE(neg, Expr);
    mre_instr_t &expr = *(reinterpret_cast<mre_instr_t *>(func.pc));
    TValue &mv0 = MPOP();
    CHECKREFERENCEMVALUE(mv0);
    MValue mv0_ = TValue2MValue(mv0);
    bool isEhHappend = false;
    void *newPc = nullptr;
    MValue res;
    try {
      res = gInterSource->JSopUnaryNeg(mv0_);
    }
    OPCATCHANDGOON(mre_instr_t);
  }

label_OP_sqrt:
  {
    // Handle expression node: sqrt
    DEBUGOPCODE(sqrt, Expr);
    mre_instr_t &expr = *(reinterpret_cast<mre_instr_t *>(func.pc));
    JSUNARY();
    func.pc += sizeof(mre_instr_t);
    goto *(labels[*func.pc]);
  }

label_OP_alloca:
  {
    // Handle expression node: alloca
    DEBUGOPCODE(alloca, Expr);
    MASSERT(false, "Not supported yet");
    func.pc += sizeof(unary_node_t);
    goto *(labels[*func.pc]);
  }

label_OP_malloc:
  {
    // Handle expression node: malloc
    DEBUGOPCODE(malloc, Expr);
    MASSERT(false, "Not supported yet");
    func.pc += sizeof(unary_node_t);
    goto *(labels[*func.pc]);
  }

label_OP_gcmalloc:
  {
    // Handle expression node: gcmalloc
    DEBUGOPCODE(gcmalloc, Expr);
    MASSERT(false, "Not supported yet");
    func.pc += sizeof(unary_node_t);
    goto *(labels[*func.pc]);
  }

label_OP_gcpermalloc:
  {
    // Handle expression node: gcpermalloc
    DEBUGOPCODE(gcpermalloc, Expr);
    MASSERT(false, "Not supported yet");
    func.pc += sizeof(unary_node_t);
    goto *(labels[*func.pc]);
  }

label_OP_stackmalloc:
  {
    // Handle expression node: stackmalloc
    DEBUGOPCODE(stackmalloc, Expr);
    MASSERT(false, "Not supported yet");
    func.pc += sizeof(unary_node_t);
    goto *(labels[*func.pc]);
  }

label_OP_gcmallocjarray:
  {
    // Handle expression node: gcmallocjarray
    DEBUGOPCODE(gcmallocjarray, Expr);
    MASSERT(false, "Not supported yet");
    func.pc += sizeof(jarraymalloc_node_t);
    goto *(labels[*func.pc]);
  }

label_OP_intrinsicop:
  {
    // Handle expression node: intrinsicop
    //(intrinsicop, Expr);
    mre_instr_t &expr = *(reinterpret_cast<mre_instr_t *>(func.pc));
    uint32_t numOpnds = expr.param.intrinsic.numOpnds;
    MValue retMv;
    DEBUGOPCODE(intrinsicop, Stmt);
    bool isEhHappend = false;
    void *newPc = nullptr;
    MIRIntrinsicID intrnid = (MIRIntrinsicID)expr.param.intrinsic.intrinsicId;
    if (intrnid == INTRN_JSOP_TYPEOF) {
      MASSERT(numOpnds == 1, "should be 1 operand");
      TValue &v0 = MPOP();
      MValue v0_ = TValue2MValue(v0);
      retMv = __jsop_typeof(&v0_);
      //retMv = gInterSource->JSUnary(intrnid, v0);
    }
    else if (intrnid >= INTRN_JSOP_STRICTEQ && intrnid <= INTRN_JSOP_IN) {
      MASSERT(numOpnds == 2, "should be 2 operands");
      TValue &v1 = MPOP();
      TValue &v0 = MPOP();
      CHECKREFERENCEMVALUE(v0);
      CHECKREFERENCEMVALUE(v1);
      MValue v1_ = TValue2MValue(v1);
      MValue v0_ = TValue2MValue(v0);
      try {
        retMv = gInterSource->JSopBinary(intrnid, v0_, v1_);
      }
      CATCHINTRINSICOP();
    } else {
       uint32_t argnums = numOpnds;
       switch (intrnid) {
         case INTRN_JS_GET_BISTRING: {
           MIR_ASSERT(argnums == 1);
           TValue &v0 = MPOP();// TValue2MValue(v0);
           //retMv = gInterSource->JSopGetBuiltinString(v0);
           retMv.x.a64 = (uint8_t *) __jsstr_get_builtin((__jsbuiltin_string_id)v0.x.u32);
           retMv.ptyp = JSTYPE_STRING;
           break;
         }
         case INTRN_JS_GET_BIOBJECT: {
           MIR_ASSERT(argnums == 1);
           TValue &v0 = MPOP();// TValue2MValue(v0);
           //retMv = gInterSource->JSopGetBuiltinObject(v0);
           retMv.x.a64 = (uint8_t *) __jsobj_get_or_create_builtin((__jsbuiltin_object_id)v0.x.u32);
           retMv.ptyp = JSTYPE_OBJECT;
           break;
         }
         case INTRN_JS_BOOLEAN: {
           MIR_ASSERT(argnums == 1);
           TValue &op0 = MTOP();
           if (IS_BOOLEAN(op0.x.u64)) {
             func.pc += sizeof(mre_instr_t);
             goto *(labels[*func.pc]);
           } else {
             TValue &v0 = MPOP();
             CHECKREFERENCEMVALUE(v0);
             MValue v0_ = TValue2MValue(v0);
             retMv = gInterSource->JSBoolean(v0_);
             break;
           }
         }
         case INTRN_JS_NUMBER: {
           //MIR_ASSERT(argnums == 1);
           TValue &op0 = MTOP();
           if (IS_NUMBER(op0.x.u64) || IS_DOUBLE(op0.x.u64)) {
             func.pc += sizeof(mre_instr_t);
             goto *(labels[*func.pc]);
           } else {
             TValue &v0 = MPOP();
             CHECKREFERENCEMVALUE(v0);
             MValue v0_ = TValue2MValue(v0);
             try {
               retMv = gInterSource->JSNumber(v0_);
             }
             CATCHINTRINSICOP();
           }
           break;
         }
         case INTRN_JS_STRING: {
           MIR_ASSERT(argnums == 1);
           TValue &v0 = MPOP();
           MValue v0_ = TValue2MValue(v0);
           retMv = gInterSource->JSString(v0_);
           break;
         }
         case INTRN_JSOP_LENGTH: {
           MIR_ASSERT(argnums == 1);
           TValue &v0 = MPOP();
           MValue v0_ = TValue2MValue(v0);
           retMv = gInterSource->JSopLength(v0_);
           break;
         }
         case INTRN_JSOP_THIS: {
           MIR_ASSERT(argnums == 0);
           retMv = gInterSource->JSopThis();
           break;
         }
         case INTRN_JSOP_GET_THIS_PROP_BY_NAME: {
           MIR_ASSERT(argnums == 1);
           TValue &v0 = MPOP();

           if (named_prop_cache[1].o == __js_Global_ThisBinding.x.a64 && named_prop_cache[1].p == v0.x.a64) {
             retMv = named_prop_cache[1].ret;
             break;
           }

           MValue v0_ = TValue2MValue(v0);
           retMv = gInterSource->JSopGetThisPropByName(v0_);

           named_prop_cache[1].o = __js_Global_ThisBinding.x.a64;
           named_prop_cache[1].p = v0.x.a64;
           named_prop_cache[1].ret = retMv;

           break;
         }
         case INTRN_JSOP_GETPROP: {
           MIR_ASSERT(argnums == 2);
           TValue &v1 = MPOP();
           TValue &v0 = MPOP();
           MValue v1_ = TValue2MValue(v1);
           MValue v0_ = TValue2MValue(v0);
           retMv = gInterSource->JSopGetProp(v0_, v1_);
           break;
         }
         case INTRN_JS_GET_ARGUMENTOBJECT: {
           MIR_ASSERT(argnums == 0);
           //retMv = gInterSource->JSopGetArgumentsObject(func.argumentsObj);
           retMv.x.a64 = (uint8_t*)func.argumentsObj;
           retMv.ptyp = JSTYPE_OBJECT;
           break;
         }
         case INTRN_JS_GET_REFERENCEERROR_OBJECT: {
           MIR_ASSERT(argnums == 0);
           retMv.x.a64 = (uint8_t *)__jsobj_get_or_create_builtin(JSBUILTIN_REFERENCEERRORCONSTRUCTOR);
           retMv.ptyp = JSTYPE_OBJECT;
           break;
         }
         case INTRN_JS_GET_TYPEERROR_OBJECT: {
           MIR_ASSERT(argnums == 0);
           retMv.x.a64 = (uint8_t *)__jsobj_get_or_create_builtin(JSBUILTIN_TYPEERROR_CONSTRUCTOR);
           retMv.ptyp = JSTYPE_OBJECT;
           break;
         }
         case INTRN_JS_REGEXP: {
           MIR_ASSERT(argnums == 1);
           TValue &v0 = MPOP();
           MValue v0_ = TValue2MValue(v0);
           retMv = gInterSource->JSRegExp(v0_);
           break;
         }
         case INTRN_JSOP_SWITCH_CMP: {
           MIR_ASSERT(argnums == 3);
           TValue &v2 = MPOP();
           TValue &v1 = MPOP();
           TValue &v0 = MPOP();
           MValue v2_ = TValue2MValue(v2);
           MValue v1_ = TValue2MValue(v1);
           MValue v0_ = TValue2MValue(v0);
           retMv.x.u64 = gInterSource->JSopSwitchCmp(v0_, v1_, v2_);
           retMv.ptyp = JSTYPE_BOOLEAN;
           break;
         }
         default:
           MIR_FATAL("unknown intrinsic JS ops");
      }
    }
    if (isEhHappend) {
      if (newPc) {
         func.pc = (uint8_t *)newPc;
         goto *(labels[*(uint8_t *)newPc]);
      } else {
         gInterSource->InsertEplog();
            // gInterSource->FinishFunc();
         MValue ret;
         ret.x.u64 = (uint64_t) Exec_handle_exc;
         ret.ptyp = JSTYPE_NONE;
        return ret;
      }
    } else {
      ENCODE_MPUSH(retMv);
      func.pc += sizeof(mre_instr_t);
      goto *(labels[*func.pc]);
    }
  }

label_OP_depositbits:
  {
    // Handle expression node: depositbits
    DEBUGOPCODE(depositbits, Expr);
    MASSERT(false, "Not supported yet");
    func.pc += sizeof(binary_node_t);
    goto *(labels[*func.pc]);
  }

label_OP_free:
  {
    // Handle statement node: free
    DEBUGOPCODE(free, Stmt);
    MASSERT(false, "Not supported yet");
    func.pc += sizeof(base_node_t);
    goto *(labels[*func.pc]);
  }

label_OP_iassignfpoff:
  {
    // Handle statement node: iassignfpoff
    DEBUGOPCODE(iassignfpoff, Stmt);
    mre_instr_t &stmt = *(reinterpret_cast<mre_instr_t *>(func.pc));
    TValue &rVal = MPOP();
    CHECKREFERENCEMVALUE(rVal);
    int32_t offset = (int32_t)stmt.param.offset;
    uint8 *addr = frame_pointer + offset;

    uint64_t oldV = *((uint64_t*)addr);
    if (IS_NEEDRC(rVal.x.u64)) {
      GCIncRf((void *)(rVal.x.u64));
    }
    if (IS_NEEDRC(oldV)) {
      GCDecRf((void *)(oldV & PAYLOAD_MASK));
    }
    *(uint64_t *)addr = rVal.x.u64;

    if (!is_strict && offset > 0 && DynMFunction::is_jsargument(func.header)) {
      MValue rVal_ = TValue2MValue(rVal);
      gInterSource->UpdateArguments(offset / sizeof(void *) - 1, rVal_);
    }
    func.pc += sizeof(base_node_t);
    goto *(labels[*func.pc]);
  }

label_OP_xintrinsiccall:
  {
    // Handle statement node: xintrinsiccall
    DEBUGOPCODE(xintrinsiccall, Stmt);
    MASSERT(false, "Not supported yet");
    func.pc += sizeof(intrinsiccall_stmt_t);
    goto *(labels[*func.pc]);
  }

label_OP_callassigned:
  {
    // Handle statement node: callassigned
    DEBUGOPCODE(callassigned, Stmt);
    MASSERT(false, "Not supported yet");
    func.pc += sizeof(callassigned_stmt_t);
    goto *(labels[*func.pc]);
  }

label_OP_icallassigned:
  {
    // Handle statement node: icallassigned
    DEBUGOPCODE(icallassigned, Stmt);
    MASSERT(false, "Not supported yet");
    func.pc += sizeof(icallassigned_stmt_t);
    goto *(labels[*func.pc]);
  }

label_OP_intrinsiccallassigned:
  {
    // Handle statement node: intrinsiccallassigned
    DEBUGOPCODE(intrinsiccallassigned, Stmt);
    MASSERT(false, "Not supported yet");
    func.pc += sizeof(intrinsiccallassigned_stmt_t);
    goto *(labels[*func.pc]);
  }

label_OP_gosub:
  {
    // Handle statement node: gosub
    DEBUGOPCODE(gosub, Stmt);
    goto_stmt_t &stmt = *(reinterpret_cast<goto_stmt_t *>(func.pc));
    func.pc += sizeof(goto_stmt_t);
    gInterSource->currEH->PushGosub((void *)func.pc);
    func.pc = (uint8_t*)&stmt.offset + stmt.offset;
    goto *(labels[*func.pc]);
  }

label_OP_retsub:
  {
    // Handle statement node: retsub
    DEBUGOPCODE(retsub, Stmt);
    base_node_t &expr = *(reinterpret_cast<base_node_t *>(func.pc));
    if (gInterSource->currEH->IsRaised()) {
      func.pc = (uint8_t *)gInterSource->currEH->GetEHpc(&func);
      if (!func.pc) {
        gInterSource->InsertEplog();
        // gInterSource->FinishFunc();
        MValue ret;
        ret.x.u64 = (uint64_t) Exec_handle_exc;
        ret.ptyp = JSTYPE_NONE;
        return ret;
      }
    } else {
      func.pc = (uint8_t *)gInterSource->currEH->PopGosub();
    }
    goto *(labels[*func.pc]);
  }

label_OP_syncenter:
  {
    // Handle statement node: syncenter
    DEBUGOPCODE(syncenter, Stmt);
    MASSERT(false, "Not supported yet");
    func.pc += sizeof(base_node_t);
    goto *(labels[*func.pc]);
  }

label_OP_syncexit:
  {
    // Handle statement node: syncexit
    DEBUGOPCODE(syncexit, Stmt);
    MASSERT(false, "Not supported yet");
    func.pc += sizeof(base_node_t);
    goto *(labels[*func.pc]);
  }

label_OP_comment:
    // Not supported yet: label
    DEBUGOPCODE(maydassign, Unused);
    MASSERT(false, "Not supported yet");

label_OP_label:
    // Not supported yet: label
    DEBUGOPCODE(maydassign, Unused);
    MASSERT(false, "Not supported yet");

label_OP_maydassign:
    // Not supported yet: maydassign
    DEBUGOPCODE(maydassign, Unused);
    MASSERT(false, "Not supported yet");

label_OP_block:
    // Not supported yet: block
    DEBUGOPCODE(block, Unused);
    MASSERT(false, "Not supported yet");

label_OP_doloop:
    // Not supported yet: doloop
    DEBUGOPCODE(doloop, Unused);
    MASSERT(false, "Not supported yet");

label_OP_dowhile:
    // Not supported yet: dowhile
    DEBUGOPCODE(dowhile, Unused);
    MASSERT(false, "Not supported yet");

label_OP_if:
    // Not supported yet: if
    DEBUGOPCODE(if, Unused);
    MASSERT(false, "Not supported yet");

label_OP_while:
    // Not supported yet: while
    DEBUGOPCODE(while, Unused);
    MASSERT(false, "Not supported yet");

label_OP_switch:
    // Not supported yet: switch
    DEBUGOPCODE(switch, Unused);
    MASSERT(false, "Not supported yet");

label_OP_multiway:
    // Not supported yet: multiway
    DEBUGOPCODE(multiway, Unused);
    MASSERT(false, "Not supported yet");

label_OP_foreachelem:
    // Not supported yet: foreachelem
    DEBUGOPCODE(foreachelem, Unused);
    MASSERT(false, "Not supported yet");

label_OP_eval:
    // Not supported yet: eval
    DEBUGOPCODE(eval, Unused);
    MASSERT(false, "Not supported yet");

label_OP_assertge:
    // Not supported yet: assertge
    DEBUGOPCODE(assertge, Unused);
    MASSERT(false, "Not supported yet");

label_OP_assertlt:
    // Not supported yet: assertlt
    DEBUGOPCODE(assertlt, Unused);
    MASSERT(false, "Not supported yet");

label_OP_sizeoftype:
    // Not supported yet: sizeoftype
    DEBUGOPCODE(sizeoftype, Unused);
    MASSERT(false, "Not supported yet");

label_OP_virtualcall:
    // Not supported yet: virtualcall
    DEBUGOPCODE(virtualcall, Unused);
    MASSERT(false, "Not supported yet");

label_OP_superclasscall:
    // Not supported yet: superclasscall
    DEBUGOPCODE(superclasscall, Unused);
    MASSERT(false, "Not supported yet");

label_OP_interfacecall:
    // Not supported yet: interfacecall
    DEBUGOPCODE(interfacecall, Unused);
    MASSERT(false, "Not supported yet");

label_OP_customcall:
    // Not supported yet: customcall
    DEBUGOPCODE(customcall, Unused);
    MASSERT(false, "Not supported yet");

label_OP_polymorphiccall:
    // Not supported yet: polymorphiccall
    DEBUGOPCODE(polymorphiccall, Unused);
    MASSERT(false, "Not supported yet");

label_OP_interfaceicall:
    // Not supported yet: interfaceicall
    DEBUGOPCODE(interfaceicall, Unused);
    MASSERT(false, "Not supported yet");

label_OP_virtualicall:
    // Not supported yet: virtualicall
    DEBUGOPCODE(virtualicall, Unused);
    MASSERT(false, "Not supported yet");

label_OP_intrinsiccallwithtype:
    // Not supported yet: intrinsiccallwithtype
    DEBUGOPCODE(intrinsiccallwithtype, Unused);
    MASSERT(false, "Not supported yet");

label_OP_virtualcallassigned:
    // Not supported yet: virtualcallassigned
    DEBUGOPCODE(virtualcallassigned, Unused);
    MASSERT(false, "Not supported yet");

label_OP_superclasscallassigned:
    // Not supported yet: superclasscallassigned
    DEBUGOPCODE(superclasscallassigned, Unused);
    MASSERT(false, "Not supported yet");

label_OP_interfacecallassigned:
    // Not supported yet: interfacecallassigned
    DEBUGOPCODE(interfacecallassigned, Unused);
    MASSERT(false, "Not supported yet");

label_OP_customcallassigned:
    // Not supported yet: customcallassigned
    DEBUGOPCODE(customcallassigned, Unused);
    MASSERT(false, "Not supported yet");

label_OP_polymorphiccallassigned:
    // Not supported yet: polymorphiccallassigned
    DEBUGOPCODE(polymorphiccallassigned, Unused);
    MASSERT(false, "Not supported yet");

label_OP_interfaceicallassigned:
    // Not supported yet: interfaceicallassigned
    DEBUGOPCODE(interfaceicallassigned, Unused);
    MASSERT(false, "Not supported yet");

label_OP_virtualicallassigned:
    // Not supported yet: virtualicallassigned
    DEBUGOPCODE(virtualicallassigned, Unused);
    MASSERT(false, "Not supported yet");

label_OP_intrinsiccallwithtypeassigned:
    // Not supported yet: intrinsiccallwithtypeassigned
    DEBUGOPCODE(intrinsiccallwithtypeassigned, Unused);
    MASSERT(false, "Not supported yet");

label_OP_xintrinsiccallassigned:
    // Not supported yet: xintrinsiccallassigned
    DEBUGOPCODE(xintrinsiccallassigned, Unused);
    MASSERT(false, "Not supported yet");

label_OP_callinstant:
    // Not supported yet: callinstant
    DEBUGOPCODE(callinstant, Unused);
    MASSERT(false, "Not supported yet");

label_OP_callinstantassigned:
    // Not supported yet: callinstantassigned
    DEBUGOPCODE(callinstantassigned, Unused);
    MASSERT(false, "Not supported yet");

label_OP_virtualcallinstant:
    // Not supported yet: virtualcallinstant
    DEBUGOPCODE(virtualcallinstant, Unused);
    MASSERT(false, "Not supported yet");

label_OP_virtualcallinstantassigned:
    // Not supported yet: virtualcallinstantassigned
    DEBUGOPCODE(virtualcallinstantassigned, Unused);
    MASSERT(false, "Not supported yet");

label_OP_superclasscallinstant:
    // Not supported yet: superclasscallinstant
    DEBUGOPCODE(superclasscallinstant, Unused);
    MASSERT(false, "Not supported yet");

label_OP_superclasscallinstantassigned:
    // Not supported yet: superclasscallinstantassigned
    DEBUGOPCODE(superclasscallinstantassigned, Unused);
    MASSERT(false, "Not supported yet");

label_OP_interfacecallinstant:
    // Not supported yet: interfacecallinstant
    DEBUGOPCODE(interfacecallinstant, Unused);
    MASSERT(false, "Not supported yet");

label_OP_interfacecallinstantassigned:
    // Not supported yet: interfacecallinstantassigned
    DEBUGOPCODE(interfacecallinstantassigned, Unused);
    MASSERT(false, "Not supported yet");

label_OP_try:
    // Not supported yet: try
    DEBUGOPCODE(try, Unused);
    MASSERT(false, "Not supported yet");

label_OP_jstry: {
    uint8_t *curPc = func.pc;
    mre_instr_t &stmt = *(reinterpret_cast<mre_instr_t *>(curPc));
    DEBUGOPCODE(jstry, Stmt);
    // constval_node_t &catchCon = *(reinterpret_cast<constval_node_t *>(curPc + sizeof(mre_instr_t)));
    uint8_t* catchV;
    uint8_t* finaV;
    uint32_t catchVoff = *(uint32_t *)(curPc + sizeof(mre_instr_t));
    uint32_t finaVoff = *(uint32_t *)(curPc + sizeof(mre_instr_t) + 4);
    catchV = (catchVoff == 0 ? nullptr : (curPc + sizeof(mre_instr_t) + catchVoff));
    finaV = (finaVoff == 0 ? nullptr : (curPc + sizeof(mre_instr_t) + 4 + finaVoff));
    gInterSource->JsTry(curPc, catchV, finaV, &func);
    func.pc += sizeof(mre_instr_t) + 8;
    goto *(labels[*func.pc]);
}

label_OP_catch:
    // Not supported yet: catch
    DEBUGOPCODE(catch, Unused);
    MASSERT(false, "Not supported yet");

label_OP_jscatch: {
    mre_instr_t &stmt = *(reinterpret_cast<mre_instr_t *>(func.pc));
    DEBUGOPCODE(jscatch, Stmt);
    gInterSource->currEH->UpdateState(OP_jscatch);
    func.pc += sizeof(mre_instr_t);
    goto *(labels[*func.pc]);
}

label_OP_finally: {
    mre_instr_t &stmt = *(reinterpret_cast<mre_instr_t *>(func.pc));
    DEBUGOPCODE(finally, Stmt);
    gInterSource->currEH->UpdateState(OP_finally);
    func.pc += sizeof(mre_instr_t);
    goto *(labels[*func.pc]);
}
label_OP_decref:
    // Not supported yet: decref
    DEBUGOPCODE(decref, Unused);
    MASSERT(false, "Not supported yet");

label_OP_incref:
    // Not supported yet: incref
    DEBUGOPCODE(incref, Unused);
    MASSERT(false, "Not supported yet");

label_OP_decrefreset:
    // Not supported yet: decrefreset
    DEBUGOPCODE(decrefreset, Unused);
    MASSERT(false, "Not supported yet");

label_OP_conststr16:
    // Not supported yet: conststr16
    DEBUGOPCODE(conststr16, Unused);
    MASSERT(false, "Not supported yet");

label_OP_gcpermallocjarray:
    // Not supported yet: gcpermallocjarray
    DEBUGOPCODE(gcpermallocjarray, Unused);
    MASSERT(false, "Not supported yet");

label_OP_stackmallocjarray:
    // Not supported yet: stackmallocjarray
    DEBUGOPCODE(stackmallocjarray, Unused);
    MASSERT(false, "Not supported yet");

label_OP_resolveinterfacefunc:
    // Not supported yet: resolveinterfacefunc
    DEBUGOPCODE(resolveinterfacefunc, Unused);
    MASSERT(false, "Not supported yet");

label_OP_resolvevirtualfunc:
    // Not supported yet: resolvevirtualfunc
    DEBUGOPCODE(resolvevirtualfunc, Unused);
    MASSERT(false, "Not supported yet");

label_OP_cand:
    // Not supported yet: cand
    DEBUGOPCODE(cand, Unused);
    MASSERT(false, "Not supported yet");

label_OP_cior:
    // Not supported yet: cior
    DEBUGOPCODE(cior, Unused);
    MASSERT(false, "Not supported yet");

label_OP_intrinsicopwithtype:
    // Not supported yet: intrinsicopwithtype
    DEBUGOPCODE(intrinsicopwithtype, Unused);
    MASSERT(false, "Not supported yet");
    for(;;);

label_OP_fieldsdist:
    // Not supported yet: intrinsicopwithtype
    DEBUGOPCODE(fieldsdist, Unused);
    MASSERT(false, "Not supported yet");
    for(;;);

label_OP_cppcatch:
    // Not supported yet: cppcatch
    DEBUGOPCODE(cppcatch, Unused);
    MASSERT(false, "Not supported yet");
    for(;;);

label_OP_cpptry:
    // Not supported yet: cpptry
    DEBUGOPCODE(cpptry, Unused);
    MASSERT(false, "Not supported yet");
    for(;;);
label_OP_ireadfpoff32:
  {
    // Not supported yet: ireadfpoff32
    DEBUGOPCODE(ireadfpoff32, Unused);
    MASSERT(false, "Not supported yet");
  }
label_OP_iassignfpoff32:
  {
    // Not supported yet: iassignfpoff32
    DEBUGOPCODE(iassignfpoff32, Unused);
    MASSERT(false, "Not supported yet");
  }

}

MValue maple_invoke_dynamic_method(DynamicMethodHeaderT *header, void *obj) {
    TValue stack[header->frameSize/sizeof(void *) + header->evalStackDepth];
    DynMFunction func(header, obj, stack);
    gInterSource->InsertProlog(header->frameSize);
    return InvokeInterpretMethod(func);
}

MValue maple_invoke_dynamic_method_main(uint8_t *mPC, DynamicMethodHeaderT* cheader) {
    TValue stack[cheader->frameSize/sizeof(void *) + cheader->evalStackDepth];
    DynMFunction func(mPC, cheader, stack);
    gInterSource->InsertProlog(cheader->frameSize);
#ifdef MEMORY_LEAK_CHECK
    memory_manager->mainSP = gInterSource->GetSPAddr();
    memory_manager->mainFP = gInterSource->GetFPAddr();
    memory_manager->mainGP = gInterSource->gp;
    memory_manager->mainTopGP = gInterSource->topGp;
#endif
    return InvokeInterpretMethod(func);
}

DynMFunction::DynMFunction(DynamicMethodHeaderT * cheader, void *obj, TValue *stack):
  header(cheader) {
    argumentsDeleted = 0;
    argumentsObj = obj;
    pc = (uint8_t *)header + *(int32_t*)header;
    sp = 0;
    operand_stack = stack;
    operand_stack[sp] = {.x.a64 = (uint8_t*)0x7ff9f00ddeadbeef};
}
DynMFunction::DynMFunction(uint8_t *argPC, DynamicMethodHeaderT *cheader, TValue *stack):header(cheader) {
    pc = argPC;
    argumentsDeleted = 0;
    argumentsObj = nullptr;
    sp = 0;
    operand_stack = stack;
    operand_stack[sp] = {.x.a64 = (uint8_t*)0x7ff9f00ddeadbeef};
}


} // namespace maple

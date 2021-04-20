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

#define MPUSH(x)   (func.operand_stack.at(++func.sp) = x)
#define MPOP()     (func.operand_stack.at(func.sp--))
#define MTOP()     (func.operand_stack.at(func.sp))

#define MARGS(x)   (caller->operand_stack.at(caller_args + x))
#define RETURNVAL  (func.operand_stack.at(0))
#define THROWVAL   (func.operand_stack.at(1))
#define MLOCALS(x) (func.operand_stack.at(x))


#define CHECKREFERENCEMVALUE(mv) \
    if (CHECK_REFERENCE && mv.x.u64 == 0) {\
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
         return ret;\
       }\
     }\

#define JSARITH() \
    MValue &op1 = MPOP(); \
    MValue &op0 = MPOP(); \
    if(IsPrimitiveDyn(op0.ptyp)) { \
      CHECKREFERENCEMVALUE(op0); \
    } \
    if(IsPrimitiveDyn(op1.ptyp)) { \
      CHECKREFERENCEMVALUE(op1); \
    } \
    MValue resVal = gInterSource->JSopArith(op0, op1, expr.primType, (Opcode)expr.op); \
    MPUSH(resVal);



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
          m = JsvalToMValue(__string_value(__jsstr_new_from_char(estr))); \
        } \
        gInterSource->currEH->SetThrownval(m); \
        gInterSource->currEH->UpdateState(OP_throw); \
        newPc = gInterSource->currEH->GetEHpc(&func); \
      } \
    } \
    if (!isEhHappend) { \
      res.ptyp = expr.primType; \
      MPUSH(res); \
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
        return ret; \
      } \
    } \

#define CATCHINTRINSICOP() \
    catch(const char *estr) { \
      isEhHappend = true; \
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
          m = JsvalToMValue(__string_value(__jsstr_new_from_char(estr))); \
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
      res.ptyp = expr.primType; \
      MPUSH(res); \
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
        return ret; \
      } \
    } \

#define JSUNARY() \
    MValue mv0 = MPOP(); \
    if(IsPrimitiveDyn(mv0.ptyp)) { \
      CHECKREFERENCEMVALUE(mv0); \
    } \
    MPUSH(gInterSource->JSopUnary(mv0, (Opcode)expr.op, expr.GetPtyp())); \

uint32_t __opcode_cnt_dyn = 0;
extern "C" uint32_t __inc_opcode_cnt_dyn() {
    return ++__opcode_cnt_dyn;
}

#define DEBUGOPCODE(opc,msg) \
  __inc_opcode_cnt_dyn(); \
  if(debug_engine & kEngineDebugInstruction) \
    fprintf(stderr, "Debug [%ld] 0x%lx:%04lx: 0x%016llx, %s, sp=%-2ld: op=0x%02x, ptyp=0x%02x, op#=%3d,      OP_" \
        #opc ", " #msg ", %d\n", gettid(), (uint8_t*)func.header - func.lib_addr, func.pc - (uint8_t*)func.header - func.header->header_size, \
        (unsigned long long)(func.operand_stack.at(func.sp)).x.i64, \
        typestr(func.operand_stack.at(func.sp).ptyp), \
        func.sp - func.header->frameSize/8, *func.pc, *(func.pc+1), *(func.pc+3), __opcode_cnt_dyn)
#define DEBUGCOPCODE(opc,msg) \
  __inc_opcode_cnt_dyn(); \
  if(debug_engine & kEngineDebugInstruction) \
    fprintf(stderr, "Debug [%ld] 0x%lx:%04lx: 0x%016llx, %s, sp=%-2ld: op=0x%02x, ptyp=0x%02x, param=0x%04x, OP_" \
        #opc ", " #msg ", %d\n", gettid(), (uint8_t*)func.header - func.lib_addr, func.pc - (uint8_t*)func.header - func.header->header_size, \
        (unsigned long long)(func.operand_stack.at(func.sp)).x.i64, \
        typestr(func.operand_stack.at(func.sp).ptyp), \
        func.sp - func.header->frameSize/8, *func.pc, *(func.pc+1), *((uint16_t*)(func.pc+2)), __opcode_cnt_dyn)
/*
#define DEBUGSOPCODE(opc,msg,idx) \
  __inc_opcode_cnt_dyn(); \
  if(debug_engine & kEngineDebugInstruction) \
    fprintf(stderr, "Debug [%ld] 0x%lx:%04lx: 0x%016llx, %s, sp=%-2ld: op=0x%02x, ptyp=0x%02x, param=0x%04x, OP_" \
        #opc " (%s), " #msg ", %d\n", gettid(), (uint8_t*)func.header - func.lib_addr, func.pc - (uint8_t*)func.header - func.header->header_size, \
        (unsigned long long)(func.operand_stack.at(func.sp)).x.i64, \
        typestr(func.operand_stack.at(func.sp).ptyp), \
        func.sp - func.header->locals_num, *func.pc, *(func.pc+1), *((uint16_t*)(func.pc+2)), \
        func.var_names == nullptr ? "" : func.var_names + (idx > 0 ? idx - 1 : mir_header->formals_num - idx) * VARNAMELENGTH, \
        __opcode_cnt_dyn)
*/
#define DEBUGSOPCODE(opc,msg,idx) \
  __inc_opcode_cnt_dyn(); \
  if(debug_engine & kEngineDebugInstruction) \
    fprintf(stderr, "Debug [%ld] 0x%lx:%04lx: 0x%016llx, %s, sp=%-2ld: op=0x%02x, ptyp=0x%02x, param=0x%04x, OP_" \
        #opc ", " #msg ", %d\n", gettid(), (uint8_t*)func.header - func.lib_addr, func.pc - (uint8_t*)func.header - func.header->header_size, \
        (unsigned long long)(func.operand_stack.at(func.sp)).x.i64, \
        typestr(func.operand_stack.at(func.sp).ptyp), \
        func.sp - func.header->frameSize/8, *func.pc, *(func.pc+1), *((uint16_t*)(func.pc+2)), \
        __opcode_cnt_dyn)

MValue InvokeInterpretMethod(DynMFunction &func) {
    // Array of labels for threaded interpretion
    static void* const labels[] = { // Use GNU extentions
        &&label_OP_Undef,
#define OPCODE(base_node,dummy1,dummy2,dummy3) &&label_OP_##base_node,
#include "mre_opcodes.def"
#undef OPCODE
        &&label_OP_Undef };

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

    MValue &addr = MPOP();

    func.pc += sizeof(base_node_t);
    goto *(labels[*func.pc]);
  }

label_OP_dread:
  {
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
        MValue local = MLOCALS(-idx);
        local.ptyp = expr.GetPtyp();
        MPUSH(local);
    }
    func.pc += sizeof(mre_instr_t);
    goto *(labels[*func.pc]);
  }

label_OP_iread:
  {
    // Handle expression node: iread
    base_node_t &expr = *(reinterpret_cast<base_node_t *>(func.pc));
    DEBUGOPCODE(iread, Expr);

    MValue &addr = MPOP();
    // //(addr.x.a64);
    MValue res;
    mload(addr.x.a64, expr.primType, res);
    MPUSH(res);

    func.pc += sizeof(mre_instr_t);
    goto *(labels[*func.pc]);
  }

label_OP_addrof:
  {
    // For address of local var/parameter
    mre_instr_t &expr = *(reinterpret_cast<mre_instr_t *>(func.pc));
    int32_t idx = (int32_t)expr.param.frameIdx;
    DEBUGSOPCODE(addrof, Expr, idx);

    MValue res;
    res.ptyp = expr.GetPtyp();
    if(idx > 0) {
        //MValue &arg = MARGS(idx);
        //res.x.a64 = (uint8_t*)&arg.x;
    } else {
        MValue &local = MLOCALS(-idx);
        // local.ptyp = (PrimType)func.header->primtype_table[(func.header->formals_num - idx)*2]; // both formals and locals are 2B each
        res.x.a64 = (uint8_t*)&local.x;
    }
    MPUSH(res);

    func.pc += sizeof(mre_instr_t);
    goto *(labels[*func.pc]);
  }

label_OP_addrof32:
  {
    // Handle expression node: addrof
    addrof_node_t &expr = *(reinterpret_cast<addrof_node_t *>(func.pc));
    DEBUGOPCODE(addrof32, Expr);

    MASSERT(expr.primType == PTY_a64, "Type mismatch: 0x%02x (should be PTY_a64)", expr.primType);
    MValue target;
    // Uses the offset to the GOT entry for the symbol name (symbolname@GOTPCREL)
    uint8_t* pc = (uint8_t*)&expr.stIdx;
    target.x.a64 = *(uint8_t**)(pc + *(int32_t *)pc);
    target.ptyp = PTY_a64;
    MPUSH(target);

    func.pc += sizeof(addrof_node_t) - 4; // Using 4 bytes for symbolname@GOTPCREL
    goto *(labels[*func.pc]);
  }

label_OP_ireadoff:
  {
      // Handle expression node: ireadoff
      mre_instr_t &expr = *(reinterpret_cast<mre_instr_t *>(func.pc));
      DEBUGCOPCODE(ireadoff, Expr);
      MValue &base = MPOP();
      //(base.x.a64);
      uint8_t *addr = base.x.a64 + expr.param.offset;
      MValue mv;
      mv.x.u64 = gInterSource->EmulateLoad(addr, expr.GetPtyp());
      mv.ptyp = expr.GetPtyp();
      MPUSH(mv);
      func.pc += sizeof(mre_instr_t);
      goto *(labels[*func.pc]);
  }

label_OP_ireadoff32:
  {
      ireadoff_node_t &expr = *(reinterpret_cast<ireadoff_node_t *>(func.pc));
      DEBUGOPCODE(ireadoff32, Expr);

      MValue &base = MTOP();
      //(base.x.a64);
      auto addr = base.x.a64 + expr.offset;
      mload(addr, expr.primType, base);

      func.pc += sizeof(ireadoff_node_t);
      goto *(labels[*func.pc]);
  }

label_OP_regread:
  {
    // Handle expression node: regread
    mre_instr_t &expr = *(reinterpret_cast<mre_instr_t *>(func.pc));
    int32_t idx = (int32_t)expr.param.frameIdx;
    MValue mval;
    mval.ptyp = expr.GetPtyp();

    DEBUGSOPCODE(regread, Expr, idx);
    switch (idx) {
     case -kSregSp:
      mval.x.a64 = (uint8_t *)gInterSource->GetSPAddr();
      break;
     case -kSregFp:
      mval.x.a64 = (uint8_t *)gInterSource->GetFPAddr();
      break;
     case -kSregGp:
      mval.x.a64 = (uint8_t *)gInterSource->GetGPAddr();
      break;
     case -kSregRetval0:
      mval = gInterSource->retVal0;
      break;
     case -kSregThrownval:
      mval = gInterSource->currEH->GetThrownval();
      break;
    }
    MPUSH(mval);

    func.pc += sizeof(mre_instr_t);
    goto *(labels[*func.pc]);
  }

label_OP_addroffunc:
  {
    // Handle expression node: addroffunc
    //addroffunc_node_t &expr = *(reinterpret_cast<addroffunc_node_t *>(func.pc));
    constval_node_t &expr = *(reinterpret_cast<constval_node_t *>(func.pc));
    DEBUGOPCODE(addroffunc, Expr);

    MValue res;
    res.ptyp = expr.primType; // FIXME
    // expr.puidx contains the offset after lowering
    //res.x.a64 = (uint8_t*)&expr.puidx + expr.puidx;
    // res.x.a64 = *(uint8_t **)&expr.puIdx;
    res.x.u64 = *(uint64_t *)GetConstval(&expr);
    MPUSH(res);

    func.pc += sizeof(constval_node_t);
    goto *(labels[*func.pc]);
  }

label_OP_constval:
  {
    // Handle expression node: constval
    mre_instr_t &expr = *(reinterpret_cast<mre_instr_t *>(func.pc));
    DEBUGCOPCODE(constval, Expr);
    MValue res;
    res.ptyp = expr.GetPtyp();
    switch(res.ptyp) {
        case PTY_i8:  res.x.i64 = expr.param.constval.i8;  break;
        case PTY_i16:
        case PTY_i32:
        case PTY_i64: res.x.i64 = expr.param.constval.i16; break;
        case PTY_u1:
        case PTY_u8:  res.x.u64 = expr.param.constval.u8;  break;
        case PTY_u16:
        case PTY_u32:
        case PTY_u64:
        case PTY_a32: // running on 64-bits machine
        case PTY_a64: res.x.u64 = expr.param.constval.u16; break;
        default: MASSERT(false, "Unexpected type for OP_constval: 0x%02x", res.ptyp);
    }
    MPUSH(res);
    func.pc += sizeof(mre_instr_t);
    goto *(labels[*func.pc]);
  }

label_OP_constval64:
  {
    constval_node_t &expr = *(reinterpret_cast<constval_node_t *>(func.pc));
    DEBUGOPCODE(constval64, Expr);

    MValue res;
    res.ptyp = expr.primType;
    uint64_t u64Val = *(uint64_t *)GetConstval(&expr);
    if (res.ptyp == PTY_dynf64) {
      gInterSource->JSdoubleConst(u64Val, res);
    }else {
      res.x.u64 = u64Val;
    }
    MPUSH(res);
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
    MPUSH(res);

    func.pc += sizeof(conststr_node_t) + 4; // Needs Ed to fix the type for conststr
    goto *(labels[*func.pc]);
  }

label_OP_cvt:
  {
    // Handle expression node: cvt
    mre_instr_t &expr = *(reinterpret_cast<mre_instr_t *>(func.pc));
    PrimType from_ptyp = (PrimType)expr.param.constval.u8;
    PrimType destPtyp = expr.GetPtyp();
    DEBUGOPCODE(cvt, Expr);

    MValue &op = MTOP();
    if (IsPrimitiveDyn(from_ptyp)) {
      CHECKREFERENCEMVALUE(op);
    }

    func.pc += sizeof(mre_instr_t);
    auto target = labels[*func.pc];

    int64_t from_int;
    float   from_float;
    double  from_double;
    op.ptyp = destPtyp;
    if (IsPrimitiveDyn(destPtyp) || IsPrimitiveDyn(from_ptyp)) {
      bool isEhHappend = false;
      void *newPc = nullptr;
      try {
        op.x.u64 = gInterSource->JSopCVT(op, destPtyp, from_ptyp).x.u64;
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
          return ret;
        }
      } else {
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
          PrimType toPtyp = op.ptyp;
          switch (toPtyp) {
            case PTY_i64: {
              int64_t int64Val = (int64_t) from_float;
              if (from_float > 0.0f && int64Val == LONG_MIN) {
                int64Val = int64Val -1;
              }
              op.x.i64 = isnan(from_float) ? 0 :  int64Val;
              goto *target;
            }
            case PTY_u64: {
              op.x.i64 = (uint64_t) from_float;
              goto *target;
            }
            case PTY_i32:
            case PTY_u32: {
              int32_t fromFloatInt = (int32_t)from_float;
              if( from_float > 0.0f &&
                fabs(from_float - (float)fromFloatInt) >= 1.0f) {
                if (fabs((float)(fromFloatInt -1) - from_float) < 1.0f) {
                  op.x.i64 = toPtyp == PTY_i32 ? fromFloatInt - 1 : (uint32_t)from_float - 1;
                } else {
                  if (fromFloatInt == INT_MIN) {
                    // some cornar cases
                    op.x.i64 = INT_MAX;
                  } else {
                    MASSERT(false, "NYI %f", from_float);
                  }
                }
                goto *target;
              } else {
                if (isnan(from_float)) {
                  op.x.i64 = 0;
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
          PrimType toPtyp = op.ptyp;
          switch (toPtyp) {
            case PTY_i32: {
              int32_t int32Val = (int32_t) from_double;
              if (from_double > 0.0f && int32Val == INT_MIN) {
                int32Val = int32Val -1;
              }
              op.x.i64 = isnan(from_double) ? 0 :  int32Val;
              goto *target;
            }
            case PTY_u32: {
              op.x.i64 = (uint32_t) from_double;
              goto *target;
            }
            case PTY_i64:
            case PTY_u64: {
              int64_t fromDoubleInt = (int64_t)from_double;
              if( from_double > 0.0f &&
                fabs(from_double - (double)fromDoubleInt) >= 1.0f) {
                if (fabs((double)(fromDoubleInt -1) - from_double) < 1.0f) {
                  op.x.i64 = toPtyp == PTY_i64 ? fromDoubleInt - 1 : (uint64_t)from_double - 1;
                } else {
                  if (fromDoubleInt == LONG_MIN) {
                    // some cornar cases
                    op.x.i64 = LONG_MAX;
                  } else {
                    MASSERT(false, "NYI %f", from_double);
                  }
                }
                goto *target;
              } else {
                if (isnan(from_double)) {
                  op.x.i64 = 0;
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
        default: goto *target;
    }
#define CVTIMPL(typ) label_cvt_##typ: \
    switch(op.ptyp) { \
        case PTY_i8:  op.x.i64 = (int8_t)from_##typ;   goto *target; \
        case PTY_i16: op.x.i64 = (int16_t)from_##typ;  goto *target; \
        case PTY_u1:  op.x.i64 = from_##typ != 0;      goto *target; \
        case PTY_u16: op.x.i64 = (uint16_t)from_##typ; goto *target; \
        case PTY_u32: op.x.i64 = (uint32_t)from_##typ; goto *target; \
        case PTY_i32: op.x.i64 = (int32_t)from_##typ;  goto *target; \
        case PTY_i64: op.x.i64 = (int64_t)from_##typ;  goto *target; \
        case PTY_f32: op.x.f32 = (float)from_##typ;    goto *target; \
        case PTY_f64: op.x.f64 = (double)from_##typ;   goto *target; \
        case PTY_a64: op.x.u64 = (uint64_t)from_##typ; goto *target; \
        default: MASSERT(false, "Unexpected type for OP_cvt: 0x%02x to 0x%02x", from_ptyp, op.ptyp); \
                 goto *target; \
    }
    CVTIMPL(int);
    CVTIMPL(float);
    CVTIMPL(double);
  }

label_OP_retype:
  {
    // Handle expression node: retype
    mre_instr_t &expr = *(reinterpret_cast<mre_instr_t *>(func.pc));
    DEBUGOPCODE(retype, Expr);

    // retype <prim-type> <type> (<opnd0>)
    // Converted to <prim-type> which has derived type <type> without changing any bits.
    // The size of <opnd0> and <prim-type> must be the same.
    MValue &res = MTOP();
    MASSERT(expr.GetOpPtyp() == res.ptyp, "Type mismatch: 0x%02x and 0x%02x", expr.GetOpPtyp(), res.ptyp);
    res.ptyp = expr.primType;

    func.pc += sizeof(mre_instr_t);
    goto *(labels[*func.pc]);
  }


label_OP_sext:
  {
    // Handle expression node: sext
    mre_instr_t &expr = *(reinterpret_cast<mre_instr_t *>(func.pc));
    DEBUGOPCODE(sext, Expr);

    MASSERT(expr.param.extractbits.boffset == 0, "Unexpected offset");
    uint64 mask = expr.param.extractbits.bsize < 64 ? (1ull << expr.param.extractbits.bsize) - 1 : ~0ull;
    MValue &op = MTOP();
    op.x.i64 = ((uint64)op.x.i64 >> (expr.param.extractbits.bsize - 1) & 1ull) ? op.x.i64 | ~mask : op.x.i64 & mask;
    op.ptyp = expr.primType;

    func.pc += sizeof(mre_instr_t);
    goto *(labels[*func.pc]);
  }

label_OP_zext:
  {
    // Handle expression node: zext
    mre_instr_t &expr = *(reinterpret_cast<mre_instr_t *>(func.pc));
    DEBUGOPCODE(zext, Expr);

    MASSERT(expr.param.extractbits.boffset == 0, "Unexpected offset");
    uint64 mask = expr.param.extractbits.bsize < 64 ? (1ull << expr.param.extractbits.bsize) - 1 : ~0ull;
    MValue &op = MTOP();
    op.x.i64 &= mask;
    op.ptyp = expr.primType;

    func.pc += sizeof(mre_instr_t);
    goto *(labels[*func.pc]);
  }

label_OP_add:
  {
    // Handle expression node: add
    binary_node_t &expr = *(reinterpret_cast<binary_node_t *>(func.pc));
    DEBUGOPCODE(add, Expr);
    MValue &op1 = MPOP();
    MValue &op0 = MPOP();
    if (IsPrimitiveDyn(op0.ptyp)) {
      CHECKREFERENCEMVALUE(op0);
    }
    if (IsPrimitiveDyn(op1.ptyp)) {
      CHECKREFERENCEMVALUE(op1);
    }
    MValue resVal = gInterSource->PrimAdd(op0, op1, expr.primType);
    MPUSH(resVal);
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
      MValue &op1 = MPOP();
      MValue &op0 = MPOP();
      if (IsPrimitiveDyn(op0.ptyp)) {
        CHECKREFERENCEMVALUE(op0);
      }
      if (IsPrimitiveDyn(op1.ptyp)) {
        CHECKREFERENCEMVALUE(op1);
      }
      MValue res;
      bool isEhHappend = false;
      void *newPc = nullptr;
      try {
        res = gInterSource->JSopSub(op0, op1, expr.primType, (Opcode)expr.op);
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
      MValue &op1 = MPOP();
      MValue &op0 = MPOP();
      if (IsPrimitiveDyn(op0.ptyp)) {
        CHECKREFERENCEMVALUE(op0);
      }
      if (IsPrimitiveDyn(op1.ptyp)) {
        CHECKREFERENCEMVALUE(op1);
      }
      MValue res;
      bool isEhHappend = false;
      void *newPc = nullptr;
      try {
        res = gInterSource->JSopMul(op0, op1, expr.primType, (Opcode)expr.op);
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
      MValue &op1 = MPOP();
      MValue &op0 = MPOP();
      if (IsPrimitiveDyn(op0.ptyp)) {
        CHECKREFERENCEMVALUE(op0);
      }
      if (IsPrimitiveDyn(op1.ptyp)) {
        CHECKREFERENCEMVALUE(op1);
      }
      MValue res;
      bool isEhHappend = false;
      void *newPc = nullptr;
      try {

        res = gInterSource->JSopDiv(op0, op1, expr.primType, (Opcode)expr.op);
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
      MValue &op1 = MPOP();
      MValue &op0 = MPOP();
      if (IsPrimitiveDyn(op0.ptyp)) {
        CHECKREFERENCEMVALUE(op0);
      }
      if (IsPrimitiveDyn(op1.ptyp)) {
        CHECKREFERENCEMVALUE(op1);
      }
      MValue res;
      bool isEhHappend = false;
      void *newPc = nullptr;
      try {
        res = gInterSource->JSopRem(op0, op1, expr.primType, (Opcode)expr.op);
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
      MValue &op1 = MPOP();
      MValue &op0 = MPOP();
      if (IsPrimitiveDyn(op0.ptyp)) {
        CHECKREFERENCEMVALUE(op0);
      }
      if (IsPrimitiveDyn(op1.ptyp)) {
        CHECKREFERENCEMVALUE(op1);
      }
      MValue res;
      bool isEhHappend = false;
      void *newPc = nullptr;
      try {
        res = gInterSource->JSopBitOp(op0, op1, expr.primType, (Opcode)expr.op);
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

    MValue  offset = MPOP();
    MValue &base = MTOP();
    base.x.a64 += offset.x.i64;

    func.pc += sizeof(binary_node_t);
    goto *(labels[*func.pc]);
  }

label_OP_eq:
  {
    // Handle expression node: eq
    mre_instr_t &expr = *(reinterpret_cast<mre_instr_t *>(func.pc));
    DEBUGOPCODE(eq, Expr);
    MValue  mVal1 = MPOP();
    MValue  mVal0 = MPOP();
    if (IsPrimitiveDyn(mVal1.ptyp)) {
      CHECKREFERENCEMVALUE(mVal1);
    }
    if (IsPrimitiveDyn(mVal0.ptyp)) {
      CHECKREFERENCEMVALUE(mVal0);
    }
    bool isEhHappend = false;
    void *newPc = nullptr;
    MValue res;
    try {
      res = gInterSource->JSopCmp(mVal0, mVal1, OP_eq);
    }
    OPCATCHANDGOON(mre_instr_t);
  }
label_OP_ge:
  {
    // Handle expression node: ge
    mre_instr_t &expr = *(reinterpret_cast<mre_instr_t *>(func.pc));
    DEBUGOPCODE(ge, Expr);
    MValue  mVal1 = MPOP();
    MValue  mVal0 = MPOP();
    if (IsPrimitiveDyn(mVal1.ptyp)) {
      CHECKREFERENCEMVALUE(mVal1);
    }
    if (IsPrimitiveDyn(mVal0.ptyp)) {
      CHECKREFERENCEMVALUE(mVal0);
    }
    bool isEhHappend = false;
    void *newPc = nullptr;
    MValue res;
    try {
      res = gInterSource->JSopCmp(mVal0, mVal1, OP_ge);
    }
    OPCATCHANDGOON(mre_instr_t);
  }

label_OP_gt:
  {
    // Handle expression node: gt
    mre_instr_t &expr = *(reinterpret_cast<mre_instr_t *>(func.pc));
    DEBUGOPCODE(gt, Expr);
    MValue  mVal1 = MPOP();
    MValue  mVal0 = MPOP();
    if (IsPrimitiveDyn(mVal1.ptyp)) {
      CHECKREFERENCEMVALUE(mVal1);
    }
    if (IsPrimitiveDyn(mVal0.ptyp)) {
      CHECKREFERENCEMVALUE(mVal0);
    }
    bool isEhHappend = false;
    void *newPc = nullptr;
    MValue res;
    try {
      res = gInterSource->JSopCmp(mVal0, mVal1, OP_gt);
    }
    OPCATCHANDGOON(mre_instr_t);
  }

label_OP_le:
  {
    // Handle expression node: le
    mre_instr_t &expr = *(reinterpret_cast<mre_instr_t *>(func.pc));
    DEBUGOPCODE(le, Expr);
    MValue  mVal1 = MPOP();
    MValue  mVal0 = MPOP();
    if (IsPrimitiveDyn(mVal1.ptyp)) {
      CHECKREFERENCEMVALUE(mVal1);
    }
    if (IsPrimitiveDyn(mVal0.ptyp)) {
      CHECKREFERENCEMVALUE(mVal0);
    }
    bool isEhHappend = false;
    void *newPc = nullptr;
    MValue res;
    try {
      res = gInterSource->JSopCmp(mVal0, mVal1, OP_le);
    }
    OPCATCHANDGOON(mre_instr_t);
  }

label_OP_lt:
  {
    // Handle expression node: lt
    mre_instr_t &expr = *(reinterpret_cast<mre_instr_t *>(func.pc));
    DEBUGOPCODE(lt, Expr);
    MValue  mVal1 = MPOP();
    MValue  mVal0 = MPOP();
    if (IsPrimitiveDyn(mVal1.ptyp)) {
      CHECKREFERENCEMVALUE(mVal1);
    }
    if (IsPrimitiveDyn(mVal0.ptyp)) {
      CHECKREFERENCEMVALUE(mVal0);
    }
    bool isEhHappend = false;
    void *newPc = nullptr;
    MValue res;
    try {
      res = gInterSource->JSopCmp(mVal0, mVal1, OP_lt);
    }
    OPCATCHANDGOON(mre_instr_t);
  }

label_OP_ne:
  {
    // Handle expression node: ne
    mre_instr_t &expr = *(reinterpret_cast<mre_instr_t *>(func.pc));
    DEBUGOPCODE(ne, Expr);
    MValue  mVal1 = MPOP();
    MValue  mVal0 = MPOP();
    if (IsPrimitiveDyn(mVal1.ptyp)) {
      CHECKREFERENCEMVALUE(mVal1);
    }
    if (IsPrimitiveDyn(mVal0.ptyp)) {
      CHECKREFERENCEMVALUE(mVal0);
    }
    bool isEhHappend = false;
    void *newPc = nullptr;
    MValue res;
    try {
      res = gInterSource->JSopCmp(mVal0, mVal1, OP_ne);
    }
    OPCATCHANDGOON(mre_instr_t);
  }

label_OP_cmp:
  {
    // Handle expression node: cmp
    mre_instr_t &expr = *(reinterpret_cast<mre_instr_t *>(func.pc));
    DEBUGOPCODE(cmp, Expr);
    EXPRCMPLGOP(cmp, 1, expr.GetPtyp(), expr.GetOpPtyp()); // if any operand is NaN, the result is definitely not 0.
    func.pc += sizeof(mre_instr_t);
    goto *(labels[*func.pc]);
  }

label_OP_cmpl:
  {
    // Handle expression node: cmpl
    mre_instr_t &expr = *(reinterpret_cast<mre_instr_t *>(func.pc));
    DEBUGOPCODE(cmpl, Expr);
    EXPRCMPLGOP(cmpl, -1, expr.GetPtyp(), expr.GetOpPtyp());
    func.pc += sizeof(mre_instr_t);
    goto *(labels[*func.pc]);
  }

label_OP_cmpg:
  {
    // Handle expression node: cmpg
    mre_instr_t &expr = *(reinterpret_cast<mre_instr_t *>(func.pc));
    DEBUGOPCODE(cmpg, Expr);
    EXPRCMPLGOP(cmpg, 1, expr.GetPtyp(), expr.GetOpPtyp());
    func.pc += sizeof(mre_instr_t);
    goto *(labels[*func.pc]);
  }

label_OP_select:
  {
    // Handle expression node: select
    ternary_node_t &expr = *(reinterpret_cast<ternary_node_t *>(func.pc));
    DEBUGOPCODE(select, Expr);
    EXPRSELECTOP();
    func.pc += sizeof(ternary_node_t);
    goto *(labels[*func.pc]);
  }

label_OP_extractbits:
  {
    // Handle expression node: extractbits
    mre_instr_t &expr = *(reinterpret_cast<mre_instr_t *>(func.pc));
    DEBUGOPCODE(extractbits, Expr);

    uint64 mask = ((1ull << expr.param.extractbits.bsize) - 1) << expr.param.extractbits.boffset;
    MValue &op = MTOP();
    op.x.i64 = (uint64)(op.x.i64 & mask) >> expr.param.extractbits.boffset;
    op.ptyp = expr.primType;

    func.pc += sizeof(mre_instr_t);
    goto *(labels[*func.pc]);
  }

label_OP_ireadpcoff:
  {
    // Handle expression node: ireadpcoff
    ireadpcoff_node_t &expr = *(reinterpret_cast<ireadpcoff_node_t *>(func.pc));
    DEBUGOPCODE(ireadpcoff, Expr);

    // Generated from addrof for symbols defined in other module
    MValue res;
    auto addr = (uint8_t*)&expr.offset + expr.offset;
    //(addr);
    mload(addr, expr.primType, res);
    MPUSH(res);

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
    MPUSH(target);

    func.pc += sizeof(addroffpc_node_t);
    goto *(labels[*func.pc]);
  }

label_OP_dassign:
  {
    // Handle statement node: dassign
    mre_instr_t &stmt = *(reinterpret_cast<mre_instr_t *>(func.pc));
    int32_t idx = (int32_t)stmt.param.frameIdx;
    DEBUGSOPCODE(dassign, Stmt, idx);

    MValue &res = MPOP();
    MASSERT(res.ptyp == stmt.GetPtyp(), "Type mismatch: 0x%02x and 0x%02x", res.ptyp, stmt.GetPtyp());
    if (IsPrimitiveDyn(res.ptyp)) {
      CHECKREFERENCEMVALUE(res);
    }
    if(idx > 0) {
        // MARGS(idx) = res;
        }
    else
        MLOCALS(-idx) = res;

    func.pc += sizeof(mre_instr_t);
    goto *(labels[*func.pc]);
  }

label_OP_iassign:
  {
    // Handle statement node: iassign
    iassignoff_stmt_t &stmt = *(reinterpret_cast<iassignoff_stmt_t *>(func.pc));
    DEBUGOPCODE(iassign, Stmt);

    // Lower iassign to iassignoff
    MValue &res = MPOP();
    if (IsPrimitiveDyn(res.ptyp)) {
      CHECKREFERENCEMVALUE(res);
    }

    auto addr = (uint8_t*)&stmt.offset + stmt.offset;
    //(addr);
    mstore(addr, stmt.primType, res);

    func.pc += sizeof(iassignoff_stmt_t);
    goto *(labels[*func.pc]);
  }

label_OP_iassignoff:
  {
      // Handle statement node: iassignoff
      mre_instr_t &stmt = *(reinterpret_cast<mre_instr_t *>(func.pc));
      DEBUGCOPCODE(iassignoff, Stmt);
      MValue &res = MPOP();
      MValue &base = MPOP();
      //(base.x.a64);

      int32_t offset = (int32_t)stmt.param.offset;
      uint8_t* addr = base.x.a64 + offset;
      gInterSource->EmulateStoreGC(addr, res, stmt.primType);
      func.pc += sizeof(mre_instr_t);
      goto *(labels[*func.pc]);
  }

label_OP_iassignoff32:
  {
    iassignoff_stmt_t &stmt = *(reinterpret_cast<iassignoff_stmt_t *>(func.pc));
    DEBUGOPCODE(iassignoff32, Stmt);

    MValue &res = MPOP();
    MValue &base = MPOP();
    //(base.x.a64);
    auto addr = base.x.a64 + stmt.offset;
    mstore(addr, stmt.primType, res);

    func.pc += sizeof(iassignoff_stmt_t);
    goto *(labels[*func.pc]);
  }

label_OP_regassign:
  {
    // Handle statement node: regassign
    mre_instr_t &stmt = *(reinterpret_cast<mre_instr_t *>(func.pc));
    int32_t idx = (int32_t)stmt.param.frameIdx;
    MValue &res = MPOP();
    if (IsPrimitiveDyn(res.ptyp)) {
      CHECKREFERENCEMVALUE(res);
    }
    DEBUGSOPCODE(regassign, Stmt, idx);
    switch (idx) {
     case -kSregRetval0: {
      gInterSource->SetRetval0(res, true);
      gInterSource->retVal0.ptyp = stmt.primType;
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

    MValue &cond = MPOP();
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

    MValue &cond = MPOP();
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
    MValue &val = MPOP();
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
    assert(numArgs < MAXCALLARGNUM && "too many args");
    for (i = 0; i < numArgs - startArg; i ++) {
      args[numArgs - startArg - i - 1] = MPOP();
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
    res.x.u64 = *(uint64_t *)GetConstval(&expr);
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
    MASSERT(numArgs >= 2, "num of args of icall should be gt than 2");
    int i = 0;
    for (i = 0; i < numArgs; i ++) {
      args[numArgs - i - 1] = MPOP();
    }
    MValue retCall = gInterSource->FuncCall((void *)args[0].x.u64, false, nullptr, args, numArgs, 2, -1, false);
    if (retCall.x.u64 == (uint64_t) Exec_handle_exc) {
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
        MValue conVal = MPOP();
        MValue retMv;
        gInterSource->SetRetval0(gInterSource->JSopNew(conVal), true);
        break;
      }
      case INTRN_JS_INIT_CONTEXT: {
        MValue &v0 = MPOP();
        __js_init_context(v0.x.u1);
      }
      break;
    case INTRN_JS_STRING:
      MIR_ASSERT(argnums == 1);
      try {
        gInterSource->SetRetval0(gInterSource->JSString(MPOP()), true);
      }
      CATCHINTRINSICOP();
      break;
    case INTRN_JS_BOOLEAN: {
      MIR_ASSERT(argnums == 1);
      MValue &v0 = MPOP();
      CHECKREFERENCEMVALUE(v0);
      gInterSource->SetRetval0(gInterSource->JSBoolean(v0), true);
      break;
    }
    case INTRN_JS_NUMBER: {
      MIR_ASSERT(argnums == 1);

      try {
        gInterSource->SetRetval0(gInterSource->JSNumber(MPOP()), true);
      }
      CATCHINTRINSICOP();
      break;
    }
    case INTRN_JSOP_CONCAT: {
      MIR_ASSERT(argnums == 2);
      MValue &arg1 = MPOP();
      MValue &arg0 = MPOP();
      gInterSource->SetRetval0(gInterSource->JSopConcat(arg0, arg1), true);
      break;
    }  // Objects
    case INTRN_JS_NEW_OBJECT_0:
      MIR_ASSERT(argnums == 0);
      gInterSource->SetRetval0(gInterSource->JSopNewObj0(), true);
      break;
    case INTRN_JS_NEW_OBJECT_1:
      MIR_ASSERT(argnums >= 1);
      for(int tmp = argnums; tmp > 1; --tmp)
          MPOP();
      gInterSource->SetRetval0(gInterSource->JSopNewObj1(MPOP()), true);
      break;
    case INTRN_JSOP_INIT_THIS_PROP_BY_NAME: {
      MIR_ASSERT(argnums == 1);
      gInterSource->JSopInitThisPropByName(MPOP());
      break;
    }
    case INTRN_JSOP_SET_THIS_PROP_BY_NAME:{
      MIR_ASSERT(argnums == 2);
      MValue &arg1 = MPOP();
      MValue &arg0 = MPOP();
      CHECKREFERENCEMVALUE(arg1);
      try {
        gInterSource->JSopSetThisPropByName(arg0, arg1);
      }
      CATCHINTRINSICOP();
      break;
    }
    case INTRN_JSOP_SETPROP_BY_NAME: {
      MIR_ASSERT(argnums == 3);
      MValue &v2 = MPOP();
      MValue &v1 = MPOP();
      MValue &v0 = MPOP();
      CHECKREFERENCEMVALUE(v0);
      try {
        gInterSource->JSopSetPropByName(v0, v1, v2, func.is_strict());
      }
      CATCHINTRINSICOP();
      break;
    }
    case INTRN_JSOP_GETPROP: {
      MIR_ASSERT(argnums == 2);
      MValue &v1 = MPOP();
      MValue &v0 = MPOP();
      CHECKREFERENCEMVALUE(v0);
      try {
        gInterSource->SetRetval0(gInterSource->JSopGetProp(v0, v1), true);
      }
      CATCHINTRINSICOP();
      break;
    }
    case INTRN_JSOP_GETPROP_BY_NAME: {
      MIR_ASSERT(argnums == 2);
      MValue &v1 = MPOP();
      MValue &v0 = MPOP();
      CHECKREFERENCEMVALUE(v0);
      try {
        gInterSource->SetRetval0(gInterSource->JSopGetPropByName(v0, v1), true);
      }
      CATCHINTRINSICOP();
      break;
    }
    case INTRN_JS_DELNAME: {
      MIR_ASSERT(argnums == 2);
      MValue &v1 = MPOP();
      MValue &v0 = MPOP();
      CHECKREFERENCEMVALUE(v0);
      try {
        gInterSource->SetRetval0(gInterSource->JSopDelProp(v0, v1, func.is_strict()), true);
      }
      CATCHINTRINSICOP();
      break;
    }
    case INTRN_JSOP_DELPROP: {
      MIR_ASSERT(argnums == 2);
      MValue &v1 = MPOP();
      MValue &v0 = MPOP();
      CHECKREFERENCEMVALUE(v0);
      try {
        gInterSource->SetRetval0(gInterSource->JSopDelProp(v0, v1, func.is_strict()), true);
      }
      CATCHINTRINSICOP();
      break;
    }
    case INTRN_JSOP_INITPROP: {
      MValue v2 = MPOP();
      MValue v1 = MPOP();
      MValue v0 = MPOP();
      gInterSource->JSopInitProp(v0, v1, v2);
      break;
    }
    case INTRN_JSOP_INITPROP_BY_NAME: {
      MIR_ASSERT(argnums == 3);
      MValue &v2 = MPOP();
      MValue &v1 = MPOP();
      MValue &v0 = MPOP();
      gInterSource->JSopInitPropByName(v0, v1, v2);
      break;
    }
    case INTRN_JSOP_INITPROP_GETTER: {
      MIR_ASSERT(argnums == 3);
      MValue &v2 = MPOP();
      MValue &v1 = MPOP();
      MValue &v0 = MPOP();
      gInterSource->JSopInitPropGetter(v0, v1, v2);
      break;
    }
    case INTRN_JSOP_INITPROP_SETTER: {
      MIR_ASSERT(argnums == 3);
      MValue &v2 = MPOP();
      MValue &v1 = MPOP();
      MValue &v0 = MPOP();
      gInterSource->JSopInitPropSetter(v0, v1, v2);
      break;
    }
    // Array
    case INTRN_JS_NEW_ARR_ELEMS: {
      MIR_ASSERT(argnums == 2);
      MValue &v1 = MPOP();
      MValue &v0 = MPOP();
      gInterSource->SetRetval0(gInterSource->JSopNewArrElems(v0, v1), true);
      break;
    }
    // Statements
    case INTRN_JS_PRINT: {
        static uint8_t sp[] = {0,0,1,0,' '};
        static uint8_t nl[] = {0,0,1,0,'\n'};
        static uint64_t sp_u64, nl_u64;
        MValue m = {.ptyp=PTY_simplestr};
        if (sp_u64 == 0) {
          m.x.a64 = reinterpret_cast<uint8_t *>(&sp);
          sp_u64  = gInterSource->JSopCVT(m, PTY_dynstr, PTY_simplestr).x.u64;
          m.x.a64 = reinterpret_cast<uint8_t *>(&nl);
          nl_u64  = gInterSource->JSopCVT(m, PTY_dynstr, PTY_simplestr).x.u64;
        }
        for (uint32_t i = 0; i < numOpnds; i++) {
          MValue mval = func.operand_stack[func.sp - numOpnds + i + 1];
          gInterSource->JSPrint(mval);
          if (i != numOpnds-1) {
            // insert space between printed arguments
            m.x.u64 = sp_u64;
            gInterSource->JSPrint(m);
          }
        }
        // pop arguments of JS_print
        func.sp -= numOpnds;
        // insert CR/LF at EOL
        m.x.u64 = nl_u64;
        gInterSource->JSPrint(m);
        gInterSource->SetRetval0({.ptyp = PTY_i32}, false);
    }
    break;
    case INTRN_JS_ISNAN: {
      MValue &v0 = MPOP();
      gInterSource->SetRetval0NoInc(gInterSource->JSIsNan(v0).x.u64);
      gInterSource->retVal0.ptyp = PTY_u1;
      break;
    }
      case INTRN_JS_DATE: {
        MValue &v0 = MPOP();
        gInterSource->SetRetval0(gInterSource->JSDate(v0), true);
        break;
      }
      case INTRN_JS_NEW_FUNCTION: {
        MValue &attrsVal = MPOP();
        MValue &envVal = MPOP();
        MValue &fpVal = MPOP();
        // MValue ret = gInterSource->JSNewFunction(&fpVal, &envVal, &attrsVal, jsPlugin->fileIndex);
        MValue ret = JsvalToMValue(__js_new_function((void *)fpVal.x.u64, (void *)envVal.x.u64,
                                   attrsVal.x.u32, gInterSource->jsPlugin->fileIndex, true/*needpt*/));
        gInterSource->SetRetval0(ret, true);
      }
      break;
      case INTRN_JSOP_ADD: {
        assert(numOpnds == 2 && "false");
        MValue arg1 = MPOP();
        MValue arg0 = MPOP();
        if (IsPrimitiveDyn(arg0.ptyp)) {
          CHECKREFERENCEMVALUE(arg0);
        }
        if (IsPrimitiveDyn(arg1.ptyp)) {
          CHECKREFERENCEMVALUE(arg1);
        }
        try {
          gInterSource->SetRetval0(gInterSource->VmJSopAdd(arg0, arg1), true);
        }
        CATCHINTRINSICOP();
      }
      break;
      case INTRN_JS_NEW_ARR_LENGTH: {
        MValue conVal = MPOP();
        gInterSource->SetRetval0Object(gInterSource->JSopNewArrLength(conVal), true);
        gInterSource->retVal0.ptyp = conVal.ptyp;
        break;
      }
      case INTRN_JSOP_SETPROP: {
        MValue v2 = MPOP();
        MValue v1 = MPOP();
        MValue v0 = MPOP();
        gInterSource->JSopSetProp(v0, v1, v2);
        break;
      }
      case INTRN_JSOP_NEW_ITERATOR: {
        MValue v1 = MPOP();
        MValue v0 = MPOP();
        gInterSource->SetRetval0NoInc(gInterSource->JSopNewIterator(v0, v1));
        gInterSource->retVal0.ptyp = PTY_a32;
        break;
      }
      case INTRN_JSOP_NEXT_ITERATOR: {
        MValue arg = MPOP();
        gInterSource->SetRetval0(gInterSource->JSopNextIterator(arg), true);
        gInterSource->retVal0.ptyp = PTY_a32;
        break;
      }
      case INTRN_JSOP_MORE_ITERATOR: {
        MValue arg = MPOP();
        gInterSource->SetRetval0NoInc(gInterSource->JSopMoreIterator(arg));
        gInterSource->retVal0.ptyp = PTY_u32;
        break;
      }
      case INTRN_JSOP_CALL:
      case INTRN_JSOP_NEW: {
        MValue args[MAXCALLARGNUM];
        int numArgs = stmt.param.intrinsic.numOpnds;
        int i = 0;
        assert(numArgs < MAXCALLARGNUM && numArgs >= 2 && "num of args of jsop call is wrong");
        for (i = 0; i < numArgs; i ++) {
          args[numArgs - i - 1] = MPOP();
        }
        CHECKREFERENCEMVALUE(args[0]);
        try {
          MValue retCall = gInterSource->IntrinCall(intrnid, args, numArgs);
          if (retCall.x.u64 == (uint64_t) Exec_handle_exc) {
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
            m = JsvalToMValue(__string_value(__jsstr_new_from_char(estr)));
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
          args[numArgs - i - 1] = MPOP();
        }
        gInterSource->IntrnError(args, numArgs);
        break;
      }
      case INTRN_JSOP_CCALL: {
        MValue args[MAXCALLARGNUM];
        int numArgs = stmt.param.intrinsic.numOpnds;
        for (int i = 0; i < numArgs; i ++) {
          args[numArgs - i - 1] = MPOP();
        }
        gInterSource->IntrinCCall(args, numArgs);
        break;
      }
      case INTRN_JS_REQUIRE: {
        gInterSource->JSopRequire(MPOP());
        break;
      }
      case INTRN_JSOP_ASSERTVALUE: {
        // no need to do anything
        MValue mv = MPOP();
        if (IsPrimitiveDyn(mv.ptyp)) {
          CHECKREFERENCEMVALUE(mv);
        }
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
    MValue m = MPOP();
    if (!gInterSource->currEH) {
      __jsvalue jval = MValueToJsval(m);
      __jsstring *msg = __jsstr_get_builtin(JSBUILTIN_STRING_EMPTY);
      if (__is_string(&jval)) {
        msg = __jsval_to_string(&jval);
      }
      PrintUncaughtException(msg);
    }
    gInterSource->currEH->SetThrownval(m);
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
    DEBUGOPCODE(ireadoff, Expr);
    int32_t offset = (int32_t)expr.param.offset;
    uint8 *addr = (uint8 *)(gInterSource->GetFPAddr()) + offset;
    MValue mv;
    mv.x.u64 = gInterSource->EmulateLoad(addr, expr.GetPtyp());
    mv.ptyp = expr.GetPtyp();
    MPUSH(mv);

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
    MValue mv0 = MPOP();
    if(IsPrimitiveDyn(mv0.ptyp)) {
      CHECKREFERENCEMVALUE(mv0);
    }
    bool isEhHappend = false;
    void *newPc = nullptr;
    MValue res;
    try {
      res = gInterSource->JSopUnaryLnot(mv0);
    }
    OPCATCHANDGOON(mre_instr_t);
  }
label_OP_bnot:
  {
    // Handle expression node: neg
    DEBUGOPCODE(bnot, Expr);
    mre_instr_t &expr = *(reinterpret_cast<mre_instr_t *>(func.pc));
    MValue mv0 = MPOP();
    if(IsPrimitiveDyn(mv0.ptyp)) {
      CHECKREFERENCEMVALUE(mv0);
    }
    bool isEhHappend = false;
    void *newPc = nullptr;
    MValue res;
    try {
      res = gInterSource->JSopUnaryBnot(mv0);
    }
    OPCATCHANDGOON(mre_instr_t);
  }
label_OP_neg:
  {
    // Handle expression node: neg
    DEBUGOPCODE(neg, Expr);
    mre_instr_t &expr = *(reinterpret_cast<mre_instr_t *>(func.pc));
    MValue mv0 = MPOP();
    if(IsPrimitiveDyn(mv0.ptyp)) {
      CHECKREFERENCEMVALUE(mv0);
    }
    bool isEhHappend = false;
    void *newPc = nullptr;
    MValue res;
    try {
      res = gInterSource->JSopUnaryNeg(mv0);
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
    DEBUGOPCODE(intrinsiccall, Stmt);
    bool isEhHappend = false;
    void *newPc = nullptr;
    MIRIntrinsicID intrnid = (MIRIntrinsicID)expr.param.intrinsic.intrinsicId;
    if (intrnid == INTRN_JSOP_TYPEOF) {
      MASSERT(numOpnds == 1, "should be 1 operand");
      retMv = gInterSource->JSUnary(intrnid, MPOP());
    }
    else if (intrnid >= INTRN_JSOP_STRICTEQ && intrnid <= INTRN_JSOP_IN) {
      MASSERT(numOpnds == 2, "should be 2 operands");
      MValue v1 = MPOP();
      MValue v0 = MPOP();
      if(IsPrimitiveDyn(v0.ptyp)) {
        CHECKREFERENCEMVALUE(v0);
      }
      if(IsPrimitiveDyn(v1.ptyp)) {
        CHECKREFERENCEMVALUE(v1);
      }
      try {
        retMv.x.u64 = gInterSource->JSopBinary(intrnid, v0, v1);
      }
      CATCHINTRINSICOP();
    } else {
       uint32_t argnums = numOpnds;
       switch (intrnid) {
         case INTRN_JS_GET_BISTRING: {
           MIR_ASSERT(argnums == 1);
           retMv = gInterSource->JSopGetBuiltinString(MPOP());
           break;
         }
         case INTRN_JS_GET_BIOBJECT: {
           MIR_ASSERT(argnums == 1);
           retMv = gInterSource->JSopGetBuiltinObject(MPOP());
           break;
         }
         case INTRN_JS_BOOLEAN: {
           MIR_ASSERT(argnums == 1);
           MValue &v0 = MPOP();
           CHECKREFERENCEMVALUE(v0);
           retMv = gInterSource->JSBoolean(v0);
           break;
         }
         case INTRN_JS_NUMBER: {
           MIR_ASSERT(argnums == 1);
           MValue op0 = MPOP();
           if (IsPrimitiveDyn(op0.ptyp)) {
             CHECKREFERENCEMVALUE(op0);
           }
           try {
             retMv = gInterSource->JSNumber(op0);
           }
           CATCHINTRINSICOP();
           break;
         }
         case INTRN_JS_STRING: {
           MIR_ASSERT(argnums == 1);
           retMv = gInterSource->JSString(MPOP());
           break;
         }
         case INTRN_JSOP_LENGTH: {
           MIR_ASSERT(argnums == 1);
           retMv = gInterSource->JSopLength(MPOP());
           break;
         }
         case INTRN_JSOP_THIS: {
           MIR_ASSERT(argnums == 0);
           retMv = gInterSource->JSopThis();
           break;
         }
         case INTRN_JSOP_GET_THIS_PROP_BY_NAME: {
           MIR_ASSERT(argnums == 1);
           MValue &v0 = MPOP();
           retMv = gInterSource->JSopGetThisPropByName(v0);
           break;
         }
         case INTRN_JSOP_GETPROP: {
           MIR_ASSERT(argnums == 2);
           MValue &v1 = MPOP();
           MValue &v0 = MPOP();
           // CHECKREFERENCEMVALUE(v0);
           retMv = gInterSource->JSopGetProp(v0, v1);
           break;
         }
         case INTRN_JS_GET_ARGUMENTOBJECT: {
           MIR_ASSERT(argnums == 0);
           //retMv = gInterSource->JSopGetArgumentsObject(func.argumentsObj);
           retMv.x.a64 = (uint8_t*)func.argumentsObj;
           break;
         }
         case INTRN_JS_GET_REFERENCEERROR_OBJECT: {
           MIR_ASSERT(argnums == 0);
           retMv.x.a64 = (uint8_t *)__jsobj_get_or_create_builtin(JSBUILTIN_REFERENCEERRORCONSTRUCTOR);
           break;
         }
         case INTRN_JS_GET_TYPEERROR_OBJECT: {
           MIR_ASSERT(argnums == 0);
           retMv.x.a64 = (uint8_t *)__jsobj_get_or_create_builtin(JSBUILTIN_TYPEERROR_CONSTRUCTOR);
           break;
         }
         case INTRN_JS_REGEXP: {
           MIR_ASSERT(argnums == 1);
           retMv = gInterSource->JSRegExp(MPOP());
           break;
         }
         case INTRN_JSOP_SWITCH_CMP: {
           MIR_ASSERT(argnums == 3);
           MValue &v2 = MPOP();
           MValue &v1 = MPOP();
           MValue &v0 = MPOP();
           retMv.x.u64 = gInterSource->JSopSwitchCmp(v0, v1, v2);
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
        return ret;
      }
    } else {
      retMv.ptyp = expr.GetPtyp();
      MPUSH(retMv);
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
    MValue &rVal = MPOP();
    if (IsPrimitiveDyn(rVal.ptyp)) {
      CHECKREFERENCEMVALUE(rVal);
    }
    int32_t offset = (int32_t)stmt.param.offset;
    uint8 *addr = (uint8 *)(gInterSource->GetFPAddr()) + offset;
    PrimType ptyp = stmt.primType;
    // memory_manager->UpdateGCReference(addr, mVal);
    gInterSource->EmulateStoreGC(addr, rVal, ptyp);
    if (!func.is_strict() && offset > 0) {
      gInterSource->UpdateArguments(offset / sizeof(void *) - 1, rVal);
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
    DynMFunction func(header, obj);
    gInterSource->InsertProlog(header->frameSize);
    return InvokeInterpretMethod(func);
}

MValue maple_invoke_dynamic_method_main(uint8_t *mPC, DynamicMethodHeaderT* cheader) {
    DynMFunction func(mPC, cheader);
    gInterSource->InsertProlog(cheader->frameSize);
    return InvokeInterpretMethod(func);
}

DynMFunction::DynMFunction(DynamicMethodHeaderT * cheader, void *obj):
  header(cheader) {
    uint8_t *addr = (uint8_t *)header;
    // init operandStack size with localVariableCount + maxEvalStackSize
    uint32_t operStackSize = (header->frameSize/sizeof(void *)) + header->evalStackDepth;
    argumentsDeleted = 0;
    argumentsObj = obj;
    pc = addr + *(int32_t*)addr;
    sp = 0;
    operand_stack.resize(operStackSize);
    operand_stack[sp] = {.x.a64 = (uint8_t*)0xcafef00ddeadbeef, PTY_void};
}
DynMFunction::DynMFunction(uint8_t *argPC, DynamicMethodHeaderT *cheader):header(cheader) {
    uint32_t operStackSize = (header->frameSize/sizeof(void *)) + header->evalStackDepth;
    pc = argPC;
    argumentsDeleted = 0;
    argumentsObj = nullptr;
    sp = 0;
    operand_stack.resize(operStackSize);
    operand_stack[sp] = {.x.a64 = (uint8_t*)0xcafef00ddeadbeef, PTY_void};
}


} // namespace maple

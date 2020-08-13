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


namespace maple {

static const char* typestr(PrimType t) {
    switch(t) {
        case PTY_i8:      return " i8";
        case PTY_i16:     return "i16";
        case PTY_i32:     return "i32";
        case PTY_i64:     return "i64";
        case PTY_u16:     return "u16";
        case PTY_u1:      return " u1";
        case PTY_a64:     return "a64";
        case PTY_f32:     return "f32";
        case PTY_f64:     return "f64";
        case PTY_u64:     return "u64";
        case PTY_void:    return "---";
        case kPtyInvalid: return "INV";
        case PTY_u8:      return " u8";
        case PTY_u32:     return "u32";
        default:          return "UNK";
    };
}


thread_local uint32_t __opcode_cnt = 0;
extern "C" uint32_t __inc_opcode_cnt() {
    return ++__opcode_cnt;
}

#define DEBUGOPCODE(opc,msg)  if(debug_engine & kEngineDebugInstruction) \
    fprintf(stderr, "Debug [%ld] 0x%lx:%04lx: 0x%016llx, %s, sp=%-2ld: op=0x%02x, ptyp=0x%02x, op#=%2d,       OP_" \
        #opc ", " #msg ", %d\n", gettid(), (uint8_t*)func.header - func.lib_addr, func.pc - (uint8_t*)func.header - func.header->header_size, \
        (unsigned long long)(func.operand_stack.at(func.sp)).x.i64, \
        typestr(func.operand_stack.at(func.sp).ptyp), \
        func.sp - func.header->locals_num, *func.pc, *(func.pc+1), *(func.pc+3), __inc_opcode_cnt())
#define DEBUGCOPCODE(opc,msg) if(debug_engine & kEngineDebugInstruction) \
    fprintf(stderr, "Debug [%ld] 0x%lx:%04lx: 0x%016llx, %s, sp=%-2ld: op=0x%02x, ptyp=0x%02x, param=0x%04x, OP_" \
        #opc ", " #msg ", %d\n", gettid(), (uint8_t*)func.header - func.lib_addr, func.pc - (uint8_t*)func.header - func.header->header_size, \
        (unsigned long long)(func.operand_stack.at(func.sp)).x.i64, \
        typestr(func.operand_stack.at(func.sp).ptyp), \
        func.sp - func.header->locals_num, *func.pc, *(func.pc+1), *((uint16_t*)(func.pc+2)), __inc_opcode_cnt())
#define DEBUGSOPCODE(opc,msg,idx) if(debug_engine & kEngineDebugInstruction) \
    fprintf(stderr, "Debug [%ld] 0x%lx:%04lx: 0x%016llx, %s, sp=%-2ld: op=0x%02x, ptyp=0x%02x, param=0x%04x, OP_" \
        #opc " (%s), " #msg ", %d\n", gettid(), (uint8_t*)func.header - func.lib_addr, func.pc - (uint8_t*)func.header - func.header->header_size, \
        (unsigned long long)(func.operand_stack.at(func.sp)).x.i64, \
        typestr(func.operand_stack.at(func.sp).ptyp), \
        func.sp - func.header->locals_num, *func.pc, *(func.pc+1), *((uint16_t*)(func.pc+2)), \
        func.var_names == nullptr ? "" : func.var_names + (idx > 0 ? idx - 1 : mir_header->formals_num - idx) * VARNAMELENGTH, \
        __inc_opcode_cnt())
#define DEBUGARGS() if(debug_engine & kEngineDebugInstruction) \
    do {   char buffer[1024]; \
            int argc = mir_header->formals_num; \
            int loc = snprintf(buffer, 1023, "Debug [%ld] %d Args:", gettid(), argc); \
            for(int i = 0; i < argc; ++i) { \
                MValue &arg = caller->operand_stack.at(caller_args + i + 1); \
                loc += snprintf(buffer + loc, 1023 - loc, " %d: %s, %s, 0x%llx ", i + 1, \
                        func.var_names == nullptr ? "" : func.var_names + i * VARNAMELENGTH, \
                        typestr(arg.ptyp), (unsigned long long)arg.x.i64); \
            } \
            fprintf(stderr, "%s\n", buffer); \
       } while(0)
#define DEBUGUNINITIALIZED(x) if(debug_engine & kEngineDebugInstruction) \
    do { \
           if(func.operand_stack.at(x).ptyp == PTY_void) \
               fprintf(stderr, "Debug [%ld] === USE OF UNINITIALIZED LOCAL %d\n", gettid(), x); \
       } while(0)

#define MPUSH(x)   (func.operand_stack.at(++func.sp) = x)
#define MPOP()     (func.operand_stack.at(func.sp--))
#define MTOP()     (func.operand_stack.at(func.sp))

#define MARGS(x)   (caller->operand_stack.at(caller_args + x))
#define RETURNVAL  (func.operand_stack.at(0))
#define THROWVAL   (func.operand_stack.at(1))
#define MLOCALS(x) (func.operand_stack.at(x))

#if defined(MPLRE_C)
#define THROWJAVAEXCEPTION_WITH_MSG
#define THROWJAVAEXCEPTION(ex) THROWJAVAEXCEPTION_WITH_MSG
#else
#define THROWJAVAEXCEPTION_WITH_MSG(ex, msg) \
    do { \
        MRT_ThrowNewException("java/lang/" #ex, msg); \
        maple::MException mex = MRT_PendingException(); \
        MRT_ClearPendingException(); \
        THROWVAL = {.x.a64 = (uint8_t*)mex, .ptyp = PTY_a64}; \
        goto label_exception_handler; \
    } while(0)

#define THROWJAVAEXCEPTION(ex) THROWJAVAEXCEPTION_WITH_MSG(ex, nullptr)
#endif // MPLRE_C

#define NULLPTRCHECK(ptr) \
    if((uintptr_t)ptr < (uintptr_t)0x1000ul) /* first 4 KiB page */ { \
        THROWVAL = {.x.a64 = (uint8_t*)nullptr, .ptyp = PTY_a64}; \
        goto label_exception_handler; \
    }

#if !(defined(MPLRE_C))
extern "C" std::ptrdiff_t __maple_java_PC_offset;
extern "C" void *__maple_method_address;
extern "C" bool MCC_JavaInstanceOf(void* obj, void* java_class);
extern "C" void MRT_YieldpointHandler_x86_64();
extern "C" void MRT_set_collect_stack_refs_cb(void (*cb)(void*, std::set<void*>&));
void collect_stack_refs(void* bp, std::set<void*>& refs);
#endif // !MPLPRE_C

extern "C" void MCC_DecRef_NaiveRCFast(void* obj);
extern "C" void MCC_IncRef_NaiveRCFast(void* obj);

MValue maple_invoke_method(const method_header_t* const mir_header, const MFunction *caller) {
    // Array of labels for threaded interpretion
    static void* const labels[] = { // Use GNU extentions
        &&label_OP_Undef,
#define OPCODE(base_node,dummy1,dummy2,dummy3) &&label_OP_##base_node,
#include "mre_opcodes.def"
#undef OPCODE
        &&label_OP_Undef };

    MFunction func(mir_header, caller);

#if !(defined(MPLRE_C))
#if defined(__x86_64__)
    if(__maple_java_PC_offset == 0) {
        void *frame_pointer;
        asm ("mov %%rbp, %0\n" : "=r" (frame_pointer));
        __maple_java_PC_offset = (uint8_t *)&func.pc - (uint8_t *)frame_pointer;
        __maple_method_address = (void *)&maple_invoke_method;
        MRT_set_collect_stack_refs_cb(collect_stack_refs);
    }
#endif
#endif // !MPLRE_C

    DEBUGMETHODSYMBOL(mir_header, "Running Java method:", mir_header->eval_depth);

    MFunction::MStack::size_type const caller_args = caller->sp - mir_header->formals_num;
    DEBUGARGS();

#if !(defined(MPLRE_C))
    MRT_YieldpointHandler_x86_64();
#endif

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
    if(addr.x.a64 == nullptr)
        THROWJAVAEXCEPTION(NullPointerException);

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
    if(idx > 0)
        MPUSH(MARGS(idx));
    else {
        DEBUGUNINITIALIZED(-idx);
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
    NULLPTRCHECK(addr.x.a64);
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
        MValue &arg = MARGS(idx);
        res.x.a64 = (uint8_t*)&arg.x;
    } else {
        MValue &local = MLOCALS(-idx);
        local.ptyp = (PrimType)func.header->primtype_table[(func.header->formals_num - idx)*2]; // both formals and locals are 2B each
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
      MValue &base = MTOP();
      NULLPTRCHECK(base.x.a64);
      auto *addr = base.x.a64 + expr.param.offset;
      mload(addr, expr.GetPtyp(), base);
      func.pc += sizeof(mre_instr_t);
      goto *(labels[*func.pc]);
  }

label_OP_ireadoff32:
  {
      ireadoff_node_t &expr = *(reinterpret_cast<ireadoff_node_t *>(func.pc));
      DEBUGOPCODE(ireadoff32, Expr);

      MValue &base = MTOP();
      NULLPTRCHECK(base.x.a64);
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
    DEBUGSOPCODE(regread, Expr, idx);

    if(idx > 0) {
        MValue arg = MARGS(idx);
        arg.ptyp = expr.GetPtyp();
        MVALUEBITMASK(arg);
        MPUSH(arg);
    } else {
        DEBUGUNINITIALIZED(-idx);
        MValue local = MLOCALS(-idx);
        local.ptyp = expr.GetPtyp();
        MPUSH(local);
    }

    func.pc += sizeof(mre_instr_t);
    goto *(labels[*func.pc]);
  }

label_OP_addroffunc:
  {
    // Handle expression node: addroffunc
    addroffunc_node_t &expr = *(reinterpret_cast<addroffunc_node_t *>(func.pc));
    DEBUGOPCODE(addroffunc, Expr);

    MValue res;
    res.ptyp = expr.primType;
    // expr.puidx contains the offset after lowering
    //res.x.a64 = (uint8_t*)&expr.puidx + expr.puidx;
    res.x.a64 = *(uint8_t **)&expr.puIdx;
    MPUSH(res);

    func.pc += sizeof(addroffunc_node_t) + 4; // Needs Ed to fix the type for addroffunc
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
    res.x.i64 = *(int64_t *)GetConstval(&expr);
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
    DEBUGCOPCODE(cvt, Expr);

    MValue &op = MTOP();
    //MASSERT(expr.GetOpPtyp() == op.primType, "Type mismatch: 0x%02x and 0x%02x", expr.GetOpPtyp(), op.primType); // Workaround

    func.pc += sizeof(mre_instr_t);
    auto target = labels[*func.pc];

    int64_t from_int;
    float   from_float;
    double  from_double;
    auto    from_ptyp = op.ptyp;
    op.ptyp = expr.GetPtyp();
    switch(from_ptyp) {
        case PTY_i8:  from_int = op.x.i8;     goto label_cvt_int;
        case PTY_i16: from_int = op.x.i16;    goto label_cvt_int;
        case PTY_u16: from_int = op.x.u16;    goto label_cvt_int;
        case PTY_u32: from_int = op.x.u32;    goto label_cvt_int;
        case PTY_i32: from_int = op.x.i32;    goto label_cvt_int;
        case PTY_i64: from_int = op.x.i64;    goto label_cvt_int;
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

label_OP_bnot:
  {
    // Handle expression node: bnot
    unary_node_t &expr = *(reinterpret_cast<unary_node_t *>(func.pc));
    DEBUGOPCODE(bnot, Expr);
    EXPRUNRINTOP(~);
    func.pc += sizeof(unary_node_t);
    goto *(labels[*func.pc]);
  }

label_OP_lnot:
  {
    // Handle expression node: lnot
    unary_node_t &expr = *(reinterpret_cast<unary_node_t *>(func.pc));
    DEBUGOPCODE(lnot, Expr);
    EXPRUNRINTOP(!);
    func.pc += sizeof(unary_node_t);
    goto *(labels[*func.pc]);
  }

label_OP_neg:
  {
    // Handle expression node: neg
    unary_node_t &expr = *(reinterpret_cast<unary_node_t *>(func.pc));
    DEBUGOPCODE(neg, Expr);
    EXPRUNROP(-);
    func.pc += sizeof(unary_node_t);
    goto *(labels[*func.pc]);
  }

label_OP_sext:
  {
    // Handle expression node: sext
    mre_instr_t &expr = *(reinterpret_cast<mre_instr_t *>(func.pc));
    DEBUGOPCODE(sext, Expr);

    MASSERT(expr.param.extractbits.boffset == 0, "Unexpected offset");
    uint64 mask = (1ull << expr.param.extractbits.bsize) - 1;
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
    uint64 mask = (1ull << expr.param.extractbits.bsize) - 1;
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
    EXPRPTRBINOP(+);
    func.pc += sizeof(binary_node_t);
    goto *(labels[*func.pc]);
  }

label_OP_sub:
  {
    // Handle expression node: sub
    binary_node_t &expr = *(reinterpret_cast<binary_node_t *>(func.pc));
    DEBUGOPCODE(sub, Expr);
    EXPRPTRBINOP(-);
    func.pc += sizeof(binary_node_t);
    goto *(labels[*func.pc]);
  }

label_OP_mul:
  {
    // Handle expression node: mul
    binary_node_t &expr = *(reinterpret_cast<binary_node_t *>(func.pc));
    DEBUGOPCODE(mul, Expr);
    EXPRBINOP(*);
    func.pc += sizeof(binary_node_t);
    goto *(labels[*func.pc]);
  }

label_OP_div:
  {
    // Handle expression node: div
    binary_node_t &expr = *(reinterpret_cast<binary_node_t *>(func.pc));
    DEBUGOPCODE(div, Expr);
    // check div-by-0 exception
    EXPRDIVOP(/);
    func.pc += sizeof(binary_node_t);
    goto *(labels[*func.pc]);
  }

label_OP_rem:
  {
    // Handle expression node: rem
    binary_node_t &expr = *(reinterpret_cast<binary_node_t *>(func.pc));
    DEBUGOPCODE(rem, Expr);
    EXPRREMOP(%);
    func.pc += sizeof(binary_node_t);
    goto *(labels[*func.pc]);
  }

label_OP_ashr:
  {
    // Handle expression node: ashr
    binary_node_t &expr = *(reinterpret_cast<binary_node_t *>(func.pc));
    DEBUGOPCODE(ashr, Expr);
    EXPRBININTOP(>>); // Implementation-dependent in C/C++. Most compilers implement it as arithmetic right shift
    func.pc += sizeof(binary_node_t);
    goto *(labels[*func.pc]);
  }

label_OP_lshr:
  {
    // Handle expression node: lshr
    binary_node_t &expr = *(reinterpret_cast<binary_node_t *>(func.pc));
    DEBUGOPCODE(lshr, Expr);
    EXPRBININTOPUNSIGNED(>>);
    func.pc += sizeof(binary_node_t);
    goto *(labels[*func.pc]);
  }

label_OP_shl:
  {
    // Handle expression node: shl
    binary_node_t &expr = *(reinterpret_cast<binary_node_t *>(func.pc));
    DEBUGOPCODE(shl, Expr);
    EXPRBININTOP(<<);
    func.pc += sizeof(binary_node_t);
    goto *(labels[*func.pc]);
  }

label_OP_max:
  {
    // Handle expression node: max
    binary_node_t &expr = *(reinterpret_cast<binary_node_t *>(func.pc));
    DEBUGOPCODE(max, Expr);
    EXPRMAXMINOP(>);
    func.pc += sizeof(binary_node_t);
    goto *(labels[*func.pc]);
  }

label_OP_min:
  {
    // Handle expression node: min
    binary_node_t &expr = *(reinterpret_cast<binary_node_t *>(func.pc));
    DEBUGOPCODE(min, Expr);
    EXPRMAXMINOP(<);
    func.pc += sizeof(binary_node_t);
    goto *(labels[*func.pc]);
  }

label_OP_band:
  {
    // Handle expression node: band
    binary_node_t &expr = *(reinterpret_cast<binary_node_t *>(func.pc));
    DEBUGOPCODE(band, Expr);
    EXPRBININTOP(&);
    func.pc += sizeof(binary_node_t);
    goto *(labels[*func.pc]);
  }

label_OP_bior:
  {
    // Handle expression node: bior
    binary_node_t &expr = *(reinterpret_cast<binary_node_t *>(func.pc));
    DEBUGOPCODE(bior, Expr);
    EXPRBININTOP(|);
    func.pc += sizeof(binary_node_t);
    goto *(labels[*func.pc]);
  }

label_OP_bxor:
  {
    // Handle expression node: bxor
    binary_node_t &expr = *(reinterpret_cast<binary_node_t *>(func.pc));
    DEBUGOPCODE(bxor, Expr);
    EXPRBININTOP(^);
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
    EXPRCOMPOP(==, expr.GetPtyp(), expr.GetOpPtyp());
    func.pc += sizeof(mre_instr_t);
    goto *(labels[*func.pc]);
  }

label_OP_ge:
  {
    // Handle expression node: ge
    mre_instr_t &expr = *(reinterpret_cast<mre_instr_t *>(func.pc));
    DEBUGOPCODE(ge, Expr);
    EXPRCOMPOP(>=, expr.GetPtyp(), expr.GetOpPtyp());
    func.pc += sizeof(mre_instr_t);
    goto *(labels[*func.pc]);
  }

label_OP_gt:
  {
    // Handle expression node: gt
    mre_instr_t &expr = *(reinterpret_cast<mre_instr_t *>(func.pc));
    DEBUGOPCODE(gt, Expr);
    EXPRCOMPOP(>, expr.GetPtyp(), expr.GetOpPtyp());
    func.pc += sizeof(mre_instr_t);
    goto *(labels[*func.pc]);
  }

label_OP_le:
  {
    // Handle expression node: le
    mre_instr_t &expr = *(reinterpret_cast<mre_instr_t *>(func.pc));
    DEBUGOPCODE(le, Expr);
    EXPRCOMPOP(<=, expr.GetPtyp(), expr.GetOpPtyp());
    func.pc += sizeof(mre_instr_t);
    goto *(labels[*func.pc]);
  }

label_OP_lt:
  {
    // Handle expression node: lt
    mre_instr_t &expr = *(reinterpret_cast<mre_instr_t *>(func.pc));
    DEBUGOPCODE(lt, Expr);
    EXPRCOMPOP(<, expr.GetPtyp(), expr.GetOpPtyp());
    func.pc += sizeof(mre_instr_t);
    goto *(labels[*func.pc]);
  }

label_OP_ne:
  {
    // Handle expression node: ne
    mre_instr_t &expr = *(reinterpret_cast<mre_instr_t *>(func.pc));
    DEBUGOPCODE(ne, Expr);
    EXPRCOMPOP(!=, expr.GetPtyp(), expr.GetOpPtyp());
    func.pc += sizeof(mre_instr_t);
    goto *(labels[*func.pc]);
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

label_OP_land:
  {
    // Handle expression node: land
    binary_node_t &expr = *(reinterpret_cast<binary_node_t *>(func.pc));
    DEBUGOPCODE(land, Expr);
    EXPRBININTOP(&);
    func.pc += sizeof(binary_node_t);
    goto *(labels[*func.pc]);
  }

label_OP_lior:
  {
    // Handle expression node: lior
    binary_node_t &expr = *(reinterpret_cast<binary_node_t *>(func.pc));
    DEBUGOPCODE(lior, Expr);
    EXPRBININTOP(||);
    func.pc += sizeof(binary_node_t);
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
    NULLPTRCHECK(addr);
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
    if(idx > 0)
        MARGS(idx) = res;
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

    auto addr = (uint8_t*)&stmt.offset + stmt.offset;
    NULLPTRCHECK(addr);
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
      NULLPTRCHECK(base.x.a64);
      auto addr = base.x.a64 + stmt.param.offset;
      mstore(addr, stmt.GetPtyp(), res);
      func.pc += sizeof(mre_instr_t);
      goto *(labels[*func.pc]);
  }

label_OP_iassignoff32:
  {
    iassignoff_stmt_t &stmt = *(reinterpret_cast<iassignoff_stmt_t *>(func.pc));
    DEBUGOPCODE(iassignoff32, Stmt);

    MValue &res = MPOP();
    MValue &base = MPOP();
    NULLPTRCHECK(base.x.a64);
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
    DEBUGSOPCODE(regassign, Stmt, idx);

    MValue &res = MPOP();
    if(idx > 0)
        MARGS(idx) = res;
    else
        MLOCALS(-idx) = res;

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
    DEBUGOPCODE(goto32, Stmt);

    if(*(func.pc + sizeof(goto_stmt_t)) == OP_endtry)
        func.try_catch_pc = nullptr;

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

    MValue &ret = MTOP();
    MVALUEBITMASK(ret); // If returning void, it is set to {0x0, PTY_void}
    return ret;
  }

label_OP_rangegoto:
  {
    // Handle statement node: rangegoto
    mre_instr_t &stmt = *(reinterpret_cast<mre_instr_t *>(func.pc));
    DEBUGCOPCODE(rangegoto, Stmt);
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
    call_stmt_t &stmt = *(reinterpret_cast<call_stmt_t *>(func.pc));
    DEBUGOPCODE(call, Stmt);

    func.pc += sizeof(call_stmt_t);
    func.direct_call(stmt.primType, stmt.numOpnds, func.pc);

    // Skip the function name
    func.pc = (uint8_t *)func.pc + ((*(uint16_t *)func.pc + 2 + 3) & ~3U);
    goto *(labels[*func.pc]);
  }

label_OP_icall:
  {
    // Handle statement node: icall
    icall_stmt_t &stmt = *(reinterpret_cast<icall_stmt_t *>(func.pc));
    DEBUGOPCODE(icall, Stmt);

    try {
        func.indirect_call(stmt.primType, stmt.numOpnds);
    }
    catch(maple::MException e) { // Catch Java exception thrown from callee
        THROWVAL = {.x.a64 = (uint8_t*)e, PTY_a64};
        goto label_exception_handler;
    }

    func.pc += sizeof(icall_stmt_t);
    goto *(labels[*func.pc]);
  }

label_OP_intrinsiccall:
  {
    // Handle statement node: intrinsiccall
    mre_instr_t &stmt = *(reinterpret_cast<mre_instr_t *>(func.pc));
    DEBUGCOPCODE(intrinsiccall, Stmt);

    try {
        func.invoke_intrinsic(stmt.primType,
                              stmt.param.intrinsic.numOpnds,
                              (MIRIntrinsicID)stmt.param.intrinsic.intrinsicId);
    }
    catch(maple::MException e) { // Catch Java exception thrown from runtime
        THROWVAL = {.x.a64 = (uint8_t*)e, PTY_a64};
        goto label_exception_handler;
    }

    func.pc += sizeof(mre_instr_t);
    goto *(labels[*func.pc]);
  }

label_OP_javatry:
  {
    // Handle statement node: javatry
    mre_instr_t &stmt = *(reinterpret_cast<mre_instr_t *>(func.pc));
    DEBUGOPCODE(javatry, Stmt);

    func.try_catch_pc = func.pc;
    // Skips the try-catch table
    func.pc += stmt.param.numCases * 4 + sizeof(mre_instr_t);
    goto *(labels[*func.pc]);
  }

label_OP_throw:
  {
    // Handle statement node: throw
    DEBUGOPCODE(throw, Stmt);
    THROWVAL = MPOP();
    func.throw_exception();

    {
      // throw_exception() calls MCC_ThrowException(), which increments RC by 1, so we
      // decrease RC first.
      MCC_DecRef_NaiveRCFast(THROWVAL.x.a64);
    }

    goto label_exception_handler;
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
    // Handle statement node: cleanuptry
    DEBUGOPCODE(cleanuptry, Stmt);
    func.try_catch_pc = nullptr;
    func.pc += sizeof(base_node_t);
    goto *(labels[*func.pc]);
  }

label_OP_endtry:
  {
    // Handle statement node: endtry
    DEBUGOPCODE(endtry, Stmt);
    func.try_catch_pc = nullptr;
    func.pc += sizeof(base_node_t);
    goto *(labels[*func.pc]);
  }

label_exception_handler:
  {
#if defined(MPLRE_C)
    MASSERT(false, "maple_invoke_method: exception handling not execpted in C environment");
#else
    uint8_t* &thrownval = THROWVAL.x.a64;
    if(thrownval == nullptr)
        THROWJAVAEXCEPTION_WITH_MSG(NullPointerException, "unknown reason");
    if(func.try_catch_pc) {
        mre_instr_t &try_stmt = *(reinterpret_cast<mre_instr_t *>(func.try_catch_pc));
        MASSERT(maple::OP_javatry == (maple::Opcode)try_stmt.op, "Not a try block");
        // Check each catch block
        int16_t num_catch = try_stmt.param.numCases;
        int32_t *catch_offset = (int32_t *)(func.try_catch_pc + sizeof(mre_instr_t));
        while(num_catch) {
            func.pc = (uint8_t *)catch_offset + *catch_offset;
            mre_instr_t &catch_stmt = *(reinterpret_cast<mre_instr_t *>(func.pc));
            DEBUGOPCODE(javacatch, Stmt);
            MASSERT(maple::OP_javacatch == (maple::Opcode)catch_stmt.op, "Not a catch block");
            // Check each exception type of a catch block
            int16_t num_catch_type = catch_stmt.param.numCases;
            int32_t *type_offset = (int32_t *)((uint8_t *)&catch_stmt + sizeof(mre_instr_t));
            while(num_catch_type) {
                if(*type_offset == 0 // Special case for clean-up code generated by compiler
                    || MCC_JavaInstanceOf(thrownval, *(uint8_t **)((uint8_t *)type_offset + *type_offset))) {
                    // Found matched exception type
                    // Set func.pc to the first instruction of catch block
                    func.pc = (uint8_t *)(type_offset + num_catch_type);
                    // Clean up and goto catch block
                    func.try_catch_pc = nullptr;
                    func.ResetSP();
                    goto *(labels[*func.pc]);
                }
                ++type_offset;
                --num_catch_type;
            }
            ++catch_offset;
            --num_catch;
        }
    }

    { // Exception not handled; do reference counting cleanup for formals and localrefvars
      uint8_t* formals_table = (uint8_t*)&func.header->primtype_table;
      for(int i=0; i<func.header->formals_num; i++) {
        if(formals_table[2*i+1] == 1) { // 2B for each formal; clean up if the 2nd byte is 1.
          MValue &arg = MARGS(i+1); // with MARGS(), formals index starts with 1
          void* ref = (void*)arg.x.a64;
          MCC_DecRef_NaiveRCFast(ref);
        }
      }

      uint8_t* locals_table = (uint8_t*)&func.header->primtype_table + func.header->formals_num*2; // find start of table for locals; skip formals, which are 2B each
      for(int i=0; i<func.header->locals_num; i++) {
        if(locals_table[2*i+1] == 2 || locals_table[2*i+1] == 3) { // 2B for each local var; clean up if the 2nd byte is 2 or 3.
          if(func.operand_stack[i].ptyp != PTY_a64)
            continue;
          void* ref = (void*)func.operand_stack[i].x.a64;
          if(ref && ref != thrownval) {
            MCC_DecRef_NaiveRCFast(ref);
          }
        } else if(locals_table[2*i+1] == 4 || locals_table[2*i+1] == 5) {
          if(func.operand_stack[i].ptyp != PTY_a64)
            continue;
          void* ref = (void*)func.operand_stack[i].x.a64;
          if(ref) {
            MCC_IncRef_NaiveRCFast(ref);
          }
        }
      }
    }

    func.sp = 1;
    DEBUGOPCODE(: THROW EXCEPTION, Throw);
    // No matched exception type
    throw thrownval;
#endif // MPLRE_C
  }

label_OP_membaracquire:
  {
    // Handle statement node: membaracquire
    DEBUGOPCODE(membaracquire, Stmt);
#if defined(__aarch64__)
    asm("dmb ishld");
#endif
    // Every load on X86_64 implies load acquire semantics
    func.pc += sizeof(base_node_t);
    goto *(labels[*func.pc]);
  }

label_OP_membarrelease:
  {
    // Handle statement node: membarrelease
    DEBUGOPCODE(membarrelease, Stmt);
#if defined(__aarch64__)
    asm("dmb ishst");
#endif
    // Every store on X86_64 implies store release semantics
    func.pc += sizeof(base_node_t);
    goto *(labels[*func.pc]);
  }

label_OP_membarstoreload:
  {
    // Handle statement node: membarstoreload
    DEBUGOPCODE(membarstoreload, Stmt);
#if defined(__aarch64__)
    asm("dmb ish");
#endif
    // X86_64 has strong memory model
    func.pc += sizeof(base_node_t);
    goto *(labels[*func.pc]);
  }

label_OP_membarstorestore:
  {
    // Handle statement node: membarstorestore
    DEBUGOPCODE(membarstorestore, Stmt);
#if defined(__aarch64__)
    asm("dmb ishst");
#endif
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
#if !(defined(MPLRE_C))
    MRT_YieldpointHandler_x86_64();
#endif
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
    DEBUGOPCODE(ireadfpoff, Expr);

    MIR_FATAL("Unsupported opcode");

    func.pc += sizeof(ireadoff_node_t);
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
    MASSERT(false, "Not supported yet");
    func.pc += sizeof(unary_node_t);
    goto *(labels[*func.pc]);
  }

label_OP_recip:
  {
    // Handle expression node: recip
    DEBUGOPCODE(recip, Expr);
    MASSERT(false, "Not supported yet");
    func.pc += sizeof(unary_node_t);
    goto *(labels[*func.pc]);
  }

label_OP_sqrt:
  {
    // Handle expression node: sqrt
    DEBUGOPCODE(sqrt, Expr);
    MASSERT(false, "Not supported yet");
    func.pc += sizeof(unary_node_t);
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
    DEBUGOPCODE(intrinsicop, Expr);
    MASSERT(false, "Not supported yet");
    func.pc += sizeof(intrinsicop_node_t);
    goto *(labels[*func.pc]);
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
    MASSERT(false, "Not supported yet");
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
    MASSERT(false, "Not supported yet");
    func.pc += sizeof(base_node_t);
    goto *(labels[*func.pc]);
  }

label_OP_retsub:
  {
    // Handle statement node: retsub
    DEBUGOPCODE(retsub, Stmt);
    MASSERT(false, "Not supported yet");
    func.pc += sizeof(base_node_t);
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

label_OP_jstry:
    // Not supported yet: jstry
    DEBUGOPCODE(jstry, Unused);
    MASSERT(false, "Not supported yet");

label_OP_catch:
    // Not supported yet: catch
    DEBUGOPCODE(catch, Unused);
    MASSERT(false, "Not supported yet");

label_OP_jscatch:
    // Not supported yet: jscatch
    DEBUGOPCODE(jscatch, Unused);
    MASSERT(false, "Not supported yet");

label_OP_finally:
    // Not supported yet: finally
    DEBUGOPCODE(finally, Unused);
    MASSERT(false, "Not supported yet");

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
}

#if !(defined(MPLRE_C))
// Collect object references on the internal stack for the purpose of collecting
// root set for GC
void collect_stack_refs(void* bp, std::set<void*>& refs)
{
  const MFunction* func = (MFunction*)((char*)bp + __maple_java_PC_offset);
  while(func) {
    for(size_t i = 0; i < func->sp; ++i) {
      if(func->operand_stack[i].ptyp == PTY_a64) {
        refs.insert((void*)func->operand_stack[i].x.a64);
      }
    }
    func = func->caller;
  }
}
#endif


} // namespace maple

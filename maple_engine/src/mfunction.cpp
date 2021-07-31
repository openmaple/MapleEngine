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

#include <ffi.h>
#include <cstring>

#include <unistd.h>
#include <sys/syscall.h>
#define gettid() syscall(SYS_gettid)
#include <sys/mman.h>
#include <cmath>

#include "mfunction.h"
#include "mprimtype.h"
#include "massert.h" // for MASSERT

#define MPUSH(x) (operand_stack[++sp] = x)
#define MPOP()   (operand_stack[sp--])
#define MTOP()   (operand_stack[sp])

#define RETURNVAL  (operand_stack[0])
#define THROWVAL   (operand_stack[1])

#define INTERNALFUNC(x) extern "C" void x();
#include "internal_functions.def"
#undef INTERNALFUNC

#define NONDECLAREDINTRINSIC(x) static constexpr auto x = nullptr;
#define DECLAREDINTRINSIC(x) extern "C" void x(void *);
#include "declared_intrinsics.def"
#undef DECLAREDINTRINSIC
#undef NONDECLAREDINTRINSIC

#define DEBUGINTRINSIC(id)  if(debug_engine & kEngineDebugMethod) \
    fprintf(stderr, "Debug [%ld] IntrinsicId=%d, %s\n", gettid(), (int)id, intrinsic_table[id].func_name)
#define DEBUGDCALL(pc, msg) if(debug_engine & kEngineDebugMethod) \
    fprintf(stderr, "Debug [%ld] Direct call: %.*s : %s\n", gettid(), *(uint16_t *)pc, pc + 2, msg);
#define DEBUGOBJECT(addr) if(debug_engine & kEngineDebugAll) \
    fprintf(stderr, "Debug [%ld] New object: 0x%016llx\n", \
            gettid(), (unsigned long long)addr)

namespace maple {

    MFunction::MFunction(const method_header_t* const current_header,
                         const MFunction *func_caller,
                         bool is_shim)
        : header(current_header),
          caller(func_caller),
          try_catch_pc(nullptr) {
            pc = (uint8_t *)header + header->header_size;

            // Evaluation stack pointer
            if(is_shim) {
                sp = 0;
                // for all arguments
                operand_stack.resize(header->formals_num + 1, {.x.i64 = 0, PTY_void});
                var_names = nullptr;
            } else {
                sp = header->locals_num;
                // for all locals, return value, throw value and evaluation stack
                operand_stack.resize(sp + header->eval_depth + 1, {.x.i64 = 0, PTY_void});
                var_names = (char*)(&header->primtype_table) + header->formals_num*2 + header->locals_num*2; // *2 because formals and locals_num each have 2 bytes
                if(var_names >= (char*)pc)
                    var_names = nullptr;
            }
            // Add a mark of evaluation stack bottom
            operand_stack[sp].x.a64 = (uint8_t*)0xcafef00ddeadbeef;
        }

    MFunction::~MFunction() { }

    void MFunction::ResetSP() {
        sp = header->locals_num;
    }

    static ffi_type ffi_type_table[] = {
        ffi_type_void, // kPtyInvalid
#define EXPANDFFI1(x) x,
#define EXPANDFFI2(x) EXPANDFFI1(x)
#define PRIMTYPE(P) EXPANDFFI2(FFITYPE_##P)
#define LOAD_ALGO_PRIMARY_TYPE
#include "prim_types.def"
#undef PRIMTYPE
        ffi_type_void // kPtyDerived
    };

    typedef struct {
        const size_t       len;
        const char * const func_name;
        ffi_fp_t           func_pointer;
    } FuncTableTy;

    static const FuncTableTy func_table[] = {
#define INTERNALFUNC(x) { sizeof(#x) - 1, #x, (ffi_fp_t)x },
#include "internal_functions.def"
#undef INTERNALFUNC
        { 0, nullptr, nullptr } };

    void MFunction::direct_call(PrimType ret_ptyp, const uint32_t arg_num, uint8_t* const pc) {
        DEBUGDCALL(pc, "Starting...");
        size_t len = *(uint16_t *)pc;
        const char *str = (const char *)(pc + 2);
        const FuncTableTy *entryptr = func_table;
        while(entryptr->func_name) {
            if(entryptr->len == len && strncmp(entryptr->func_name, str, len) == 0) {
                DEBUGDCALL(pc, "Matched.");
                call_with_ffi(ret_ptyp, arg_num, entryptr->func_pointer);
                break;
            }
            entryptr++;
        }
        if(entryptr->func_name == nullptr) {
            sp -= arg_num;
            DEBUGDCALL(pc, "Error: Not matched.");
        }
    }

#if defined(__aarch64__)
#define MPLI_OFFSET 8
#elif defined(__x86_64__)
#define MPLI_OFFSET 0
#else
#error Unsuported arch.
#endif

    void MFunction::indirect_call(PrimType ret_ptyp, const uint32_t arg_num) {
        const uint32_t actual_num = arg_num - 1;
        uint8_t *fp = operand_stack[sp - actual_num].x.a64;
        MASSERT(fp != nullptr, "Indirect call with nullptr");
        if(*(uint32_t*)(fp + MPLI_OFFSET) == 0x494c504d) {
            method_header_t *header = (method_header_t *)(fp + MPLI_OFFSET + 4);
            DEBUGSYMBOL(header, "Calling Java method...");
            // Weak function for missing JNI native method has 0 formal parameter
            MASSERT(header->formals_num == 0 || header->formals_num == actual_num,
                    "Wrong number of arguments: formals=%d, actuals=%d", header->formals_num, actual_num);
            RETURNVAL = maple_invoke_method(header, this);
            //MASSERT(header->formals_num == 0 || RETURNVAL.ptyp == ret_ptyp || ret_ptyp == PTY_void,
            //        "Type mismatch: 0x%02x and 0x%02x", RETURNVAL.ptyp, ret_ptyp);
            RETURNVAL.ptyp = ret_ptyp; // Force the return type (See test case DCP0021)
            sp -= arg_num;
        } else {
            call_with_ffi(ret_ptyp, actual_num, (ffi_fp_t)fp);
            MPOP(); // function pointer
        }
    }

    static const FuncTableTy intrinsic_table[] = {
#define DEF_MIR_INTRINSIC(STR, NAME, INTRN_CLASS, RETURN_TYPE, ...) \
        { sizeof(#NAME) - 1, #NAME, (ffi_fp_t)NAME }, /* INTRN_##STR, INTRN_CLASS, RETURN_TYPE, NAME(__VA_ARGS__) */
#include "intrinsics.def"
#undef DEF_MIR_INTRINSIC
        };

    extern "C" uint32_t __MRT_prepare_invoke_clinit(void *classinfo, void *clinitArray[], void *classinfoParent[],
                                                    uint32_t index, uint32_t maxNum);
    extern "C" void __MRT_finalize_clinit(void *classinfo, uint32_t flag);
    extern "C" uint32_t __MRT_get_class_init_state(void *classinfo);
    extern "C" bool MRT_EnterSaferegion();
    extern "C" bool MRT_LeaveSaferegion();
    extern "C" bool __MRT_Reflect_ObjIsInstanceOfClassError(void *ex);

    void MFunction::invoke_intrinsic(PrimType ret_ptyp, const uint32_t arg_num, MIRIntrinsicID intrinsic) {
        DEBUGINTRINSIC(intrinsic);
        const FuncTableTy &entry = intrinsic_table[intrinsic];
        if(entry.func_pointer != nullptr) {
            call_with_ffi(ret_ptyp, arg_num, entry.func_pointer);
            if(intrinsic == INTRN_MCCNewObjFixedClass || intrinsic == INTRN_MCCNewObjFlexibleCname) {
                MValue &ret = RETURNVAL;
                DEBUGOBJECT(ret.x.a64);
                //for(int i = 0; i < 4; ++i)
                //    MCC_IncRef_NaiveRCFast(ret.x.a64); // Workaround for RC
            }
            return;
        }
        switch(intrinsic) {
            case INTRN_MPL_CLINIT_CHECK:
                {
                    MValue &val = MPOP();
                    void *classinfo = *((void **)val.x.a64);
                    DEBUGSYMBOL(val.x.a64, "CLINIT classinfo");
                    #define MAXSUPERNUM 128
                    #define CLINIT_NORMAL 0
                    #define CLINIT_SKIPPED 1
                    #define CLINIT_THROWN  2
                    #define CLASSINITSTATEUNINITIALIZED 0
                    #define CLASSINITSTATEINITIALIZING  1
                    #define CLASSINITSTATEFAILURE       2
                    void *clinitArray[MAXSUPERNUM];
                    void *classinfoParent[MAXSUPERNUM];
                    uint32_t sizeSuper= __MRT_prepare_invoke_clinit(classinfo, clinitArray, classinfoParent, 0, MAXSUPERNUM);
                    bool skipChildren = false;
                    maple::MException noClassFoundErrEh = nullptr;
                    maple::MException ex = nullptr;
                    for (uint32_t i = 0; i < sizeSuper; i++) {
                      if (skipChildren) {
                        __MRT_finalize_clinit(classinfoParent[i], CLINIT_SKIPPED); // skip an clinit
                      } else {
                        if (classinfoParent[i]) {
                          uint32_t clStatus = __MRT_get_class_init_state(classinfoParent[i]);
                          if (clStatus == CLASSINITSTATEFAILURE) {
                            MRT_ThrowNewException("java/lang/NoClassDefFoundError", nullptr);
                            noClassFoundErrEh = MRT_PendingException();
                            skipChildren = true;
                            continue;
                          }
                        }
                        uint8_t *fp = (uint8_t *)clinitArray[i];
                        if (fp) {
                          DEBUGSYMBOL(fp, "[Running CLINIT:");
                          try {
                            if(*(uint32_t*)(fp + MPLI_OFFSET) == 0x494c504d) {
                                method_header_t *header = (method_header_t *)(fp + MPLI_OFFSET + 4);
                                MASSERT(header->formals_num == 0, "Wrong number of arguments");
                                maple_invoke_method(header, this);
                            }
                            else
                                call_with_ffi(PTY_void, 0, (ffi_fp_t)fp);
                          }
                          catch(maple::MException e) {
                            ex = e;
                          }
                          if (ex) {
                            __MRT_finalize_clinit(classinfoParent[i], CLINIT_THROWN);
                            skipChildren = true;
                          } else {
                            __MRT_finalize_clinit(classinfoParent[i], CLINIT_NORMAL);
                          }
                          DEBUGSYMBOL(fp, "Finalized CLINIT]");
                        }
                      }
                    }
                    if (noClassFoundErrEh) {
                      MRT_ClearPendingException();
                      throw noClassFoundErrEh;
                    }
                    if(ex) {
                      if (__MRT_Reflect_ObjIsInstanceOfClassError(ex)) {
                        MRT_ClearPendingException();
                        // TODO: rc?
                        throw ex;
                      } else {
                        MCC_DecRef_NaiveRCFast(ex); // decrease RC for the exception
                        MRT_ThrowNewException("java/lang/ExceptionInInitializerError", nullptr);
                        maple::MException mex = MRT_PendingException();
                        MRT_ClearPendingException();
                        throw mex;
                      }
                    }
                    break;
                }
            case INTRN_MPL_CLEANUP_LOCALREFVARS_SKIP:  // Skip the last argument
                {
                    MPOP();
                    uint32_t i;
                    for(i = arg_num; i > 1; --i) {
                        MValue &val = MPOP();
                        if(val.x.a64)
                            MCC_DecRef_NaiveRCFast(val.x.a64);
                    }
                    break;
                }
            case INTRN_MPL_CLEANUP_LOCALREFVARS: // __mpl_cleanup_localrefvars
                {
                    uint32_t i;
                    for(i = arg_num; i > 0; --i) {
                        MValue &val = MPOP(); // Apply RC-Dec on this ref if it is not null
                        if(val.x.a64)
                            MCC_DecRef_NaiveRCFast(val.x.a64);
                    }
                    break;
                }
            case INTRN_MCCCallSlowNative:
                {
                    MRT_EnterSaferegion();
                    indirect_call(ret_ptyp, arg_num);
                    MRT_LeaveSaferegion();
                    break;
                }
            default:
                MASSERT(false, "Error: hit unsupported/unregistered intrinsic %d", (int)intrinsic);
        } // switch
    }

    void MFunction::throw_exception() {
        try {
        MCC_ThrowException(THROWVAL.x.a64);
        } catch(const MException e) {
            THROWVAL.x.a64 = reinterpret_cast<uint8_t *>(e);
        }
    }

    // Call statically-compiled method or C/C++ function with ffi
    void MFunction::call_with_ffi(PrimType ret_ptyp, const uint32_t actual_num, ffi_fp_t fp) {
        ffi_cif cif;
        ffi_type ffi_ret_type = ffi_type_table[ret_ptyp];
        ffi_type* arg_types[actual_num];
        DEBUGSYMBOL((void *)fp, "Calling function with ffi_call");
        void* args[actual_num];
        // Gather all args
        uint32_t idx = actual_num;
        while(idx > 0) {
            MValue &oprand = MPOP();
            // It is OK to use this location for ffi args
            // since no one else can change its value
            args[--idx] = &oprand.x;
            arg_types[idx] = ffi_type_table + oprand.ptyp;
        }

        // Check ffi status and call method if OK
        ffi_status status = ffi_prep_cif(&cif, FFI_DEFAULT_ABI, actual_num, &ffi_ret_type, arg_types);
        if(status == FFI_OK) {
            MValue &ret = RETURNVAL;
            ffi_call(&cif, fp, &ret.x, args);
            ret.ptyp = ret_ptyp;
        } else {
            MIR_FATAL("Failed to call method at %p", (void *)fp);
        }
    }
} // namespace maple

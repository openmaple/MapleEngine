#include <ostream>
#include <sstream>
#include <vector>
#include <map>
#include <atomic>
#include <memory>
#include <functional>
#include <cstdint>
#include <cstdarg>
#include <jni.h>

namespace maplert {
struct GCTIB_GCInfo {
  uint32_t header_proto;
  uint32_t n_bitmap_words;
  uint64_t bitmap_words[];
};
extern "C" struct GCTIB_GCInfo MCC_GCTIB___EmptyObject;
extern "C" struct GCTIB_GCInfo MCC_GCTIB___ArrayOfObject;
extern "C" struct GCTIB_GCInfo MCC_GCTIB___ArrayOfPrimitive;
}
namespace NameMangler {
static constexpr const char kJavaLangClassStr[] = "Ljava_2Flang_2FClass_3B";
static constexpr const char kJavaLangObjectStr[] = "Ljava_2Flang_2FObject_3B";
static constexpr const char kJavaLangStringStr[] = "Ljava_2Flang_2FString_3B";
static constexpr const char kThrowClassStr[] = "Ljava_2Flang_2FThrowable_3B";
static constexpr const char kJavaLangBooleanTrue[] = "Ljava_2Flang_2FBoolean_3B_7CTRUE";
static constexpr const char kJavaLangBooleanFalse[] = "Ljava_2Flang_2FBoolean_3B_7CFALSE";
static constexpr const char kJavaLangReflectAccessibleObject[] = "Ljava_2Flang_2Freflect_2FAccessibleObject_3B";
static constexpr const char kJavaLangReflectConstructor[] = "Ljava_2Flang_2Freflect_2FConstructor_3B";
static constexpr const char kJavaLangReflectExecutable[] = "Ljava_2Flang_2Freflect_2FExecutable_3B";
static constexpr const char kJavaLangReflectField[] = "Ljava_2Flang_2Freflect_2FField_3B";
static constexpr const char kJavaLangReflectMember[] = "Ljava_2Flang_2Freflect_2FMember_3B";
static constexpr const char kJavaLangReflectMethod[] = "Ljava_2Flang_2Freflect_2FMethod_3B";
static constexpr const char kJavaLangReflectType[] = "Ljava_2Flang_2Freflect_2FType_3B";
static constexpr const char kJavaLangReflectAnnotateElement[] = "Ljava_2Flang_2Freflect_2FAnnotatedElement_3B";
static constexpr const char kJavaLangReflectGenericDeclaration[] = "Ljava_2Flang_2Freflect_2FGenericDeclaration_3B";
static constexpr const char kJavaLangCharSequence[] = "Ljava_2Flang_2FCharSequence_3B";
static constexpr const char kJavaLangComparable[] = "Ljava_2Flang_2FComparable_3B";
static constexpr const char kJavaIoSerializable[] = "Ljava_2Fio_2FSerializable_3B";
extern bool doCompression;
std::string GetInternalNameLiteral(const char *name);
std::string GetOriginalNameLiteral(const char *name);
std::string EncodeName(const std::string& name);
std::string EncodeName(const char* name);
std::string DecodeName(const std::string& name);
std::string DecodeName(const char* name);
void DecodeMapleNameToJavaDescriptor(std::string &nameIn, std::string &nameOut);
std::string NativeJavaName(const char* name, bool overloaded = true);
__attribute__((visibility("default"))) unsigned UTF16ToUTF8(std::string &str, const std::u16string &str16, unsigned short num = 0, bool isbigendian = false);
__attribute__((visibility("default"))) unsigned UTF8ToUTF16(std::u16string &str16, const std::string &str, unsigned short num = 0, bool isbigendian = false);
uint64_t GetUleb128Encode(uint64_t val);
uint64_t GetSleb128Encode(int64_t val);
uint64_t GetUleb128Decode(uint64_t val);
int64_t GetSleb128Decode(uint64_t val);
size_t GetUleb128Size(uint64_t val);
size_t GetSleb128Size( int32_t val);
}
namespace maplert {
long __MRT_Envconf(const char *name, long default_);
}
template <typename T, size_t N>
char(&ArraySizeHelper(T(&array)[N]))[N];
template <typename... T>
void UNUSED(const T&...) {
}
constexpr bool kMemoryToolIsValgrind = false;
namespace maple {
enum LogSeverity {
  VERBOSE,
  DEBUGY,
  INFO,
  WARNING,
  ERROR,
  FATAL_WITHOUT_ABORT,
  FATAL,
};
enum LogId {
  DEFAULT,
  MAIN,
  SYSTEM,
};
using LogFunction = std::function<void(LogSeverity, const char*, const char*,
                                       unsigned int, const char*)>;
using AbortFunction = std::function<void(const char*)>;
void DefaultAborter(const char* abort_message);
void InitLogging(AbortFunction&& aborter = DefaultAborter);
void SetAborter(AbortFunction&& aborter);
static constexpr bool kEnableDChecks = true;
template <typename LHS, typename RHS>
struct EagerEvaluator {
  constexpr EagerEvaluator(LHS l, RHS r) : lhs(l), rhs(r) {
  }
  LHS lhs;
  RHS rhs;
};
template <typename LHS, typename RHS>
constexpr EagerEvaluator<LHS, RHS> MakeEagerEvaluator(LHS lhs, RHS rhs) {
  return EagerEvaluator<LHS, RHS>(lhs, rhs);
}
class LogMessageData;
class LogMessage {
 public:
  LogMessage(const char* file, unsigned int line, LogSeverity severity, const char* tag);
  ~LogMessage();
  std::ostream& stream();
  static void LogLine(const char* file, unsigned int line, LogSeverity severity,
                      const char* tag, const char* msg);
 private:
  const std::unique_ptr<LogMessageData> data_;
  LogMessage(const LogMessage&) = delete; void operator=(const LogMessage&) = delete;
};
LogSeverity GetMinimumLogSeverity();
LogSeverity SetMinimumLogSeverity(LogSeverity new_severity);
class ScopedLogSeverity {
 public:
  explicit ScopedLogSeverity(LogSeverity level);
  ~ScopedLogSeverity();
 private:
  LogSeverity old_;
};
}
namespace maple {
using AbortFunc = void(const char*);
struct LogVerbosity {
  bool jni;
  bool binding;
  bool startup;
  bool jvm;
  bool thread;
  bool classloader;
  bool mpllinker;
  bool signals;
  bool gc;
  bool rc;
  bool eh;
  bool reflect;
  bool methodtrace;
  bool classinit;
  bool printf;
  bool monitor;
  bool profiler;
  bool rcreleasetrace;
  bool oomchecker;
  bool jsan;
  bool nativeleak;
  bool addr2line;
  bool systrace_lock_logging;
  bool gcbindcpu;
  bool rcverify;
  bool opengclog;
  bool openrctracelog;
  bool openrplog;
  bool opencyclelog;
  bool opennativelog;
  bool allocatorfragmentlog;
  bool dumpgarbage;
  bool dumpheap;
  bool dumpheapbeforegc;
  bool dumpheapaftergc;
  bool dumpheapsimple;
  bool dumpgcinfoonsigquit;
  bool jdwp;
  bool decouple;
};
extern LogVerbosity gLogVerbosity;
extern const char *const endl;
extern unsigned int gAborting;
extern void InitLogging(char* argv[], AbortFunc& default_aborter);
extern const char* GetCmdLine();
extern const char* ProgramInvocationName();
extern const char* ProgramInvocationShortName();
}
namespace maplert {
void __MRT_LogSlow(const char *component, int level, const char *fmt...)
  __attribute__((__format__ (__printf__, 3, 4)));
}
namespace maple {
typedef uintptr_t address_t;
typedef std::function<void(address_t)> root_object_func;
typedef std::function<void(uint32_t, address_t)> irt_visit_func;
typedef std::function<void(address_t*)> root_object_addr_func;
class GCRootsVisitor{
public:
  static void AcquireThreadListLock();
  static bool TryAcquireThreadListLock();
  static void ReleaseThreadListLock();
  static bool IsThreadListLockLockBySelf();
  static void Visit(root_object_func& f);
  static void VisitLocalRef(root_object_func& f);
  static void VisitGlobalRef(root_object_func& f);
  static void VisitThreadException(root_object_func& f);
  static void Visit(root_object_addr_func& f __attribute__((__unused__))) {
  }
  static void ClearWeakGRTReference(uint32_t index, jobject obj);
  static void VisitWeakGRT(irt_visit_func& f);
  static void VisitWeakGRTConcurrent(irt_visit_func& f);
};
enum RootType {
  kRootUnknown = 0,
  kRootJNIGlobal,
  kRootJNILocal,
  kRootJavaFrame,
  kRootNativeStack,
  kRootStickyClass,
  kRootThreadBlock,
  kRootMonitorUsed,
  kRootThreadObject,
  kRootInternedString,
  kRootFinalizing,
  kRootDebugger,
  kRootReferenceCleanup,
  kRootVMInternal,
  kRootJNIMonitor,
};
inline std::ostream& operator<<(std::ostream& os, const RootType& root_type) {
  os << (uint32_t)root_type;
  return os;
}
class RootInfo {
 public:
  explicit RootInfo(RootType type, uint32_t thread_id = 0)
     : type_(type), thread_id_(thread_id) {
  }
  RootInfo(const RootInfo&) = default;
  virtual ~RootInfo() {
  }
  RootType GetType() const {
    return type_;
  }
  uint32_t GetThreadId() const {
    return thread_id_;
  }
  virtual void Describe(std::ostream& os) const {
    os << "Type=" << type_ << " thread_id=" << thread_id_;
  }
  std::string ToString() const;
 private:
  const RootType type_;
  const uint32_t thread_id_;
};
inline std::ostream& operator<<(std::ostream& os, const RootInfo& root_info) {
  root_info.Describe(os);
  return os;
}
inline std::string RootInfo::ToString() const {
  std::ostringstream oss;
  Describe(oss);
  return oss.str();
}
enum VisitRootFlags : uint8_t {
  kVisitRootFlagAllRoots = 0x1,
  kVisitRootFlagNewRoots = 0x2,
  kVisitRootFlagStartLoggingNewRoots = 0x4,
  kVisitRootFlagStopLoggingNewRoots = 0x8,
  kVisitRootFlagClearRootLog = 0x10,
  kVisitRootFlagClassLoader = 0x20,
  kVisitRootFlagPrecise = 0x80,
};
class RootVisitor {
 public:
  virtual ~RootVisitor() { }
  virtual void VisitRoot(jobject root, const RootInfo& info) {
    VisitRoots(&root, 1, info);
  }
  void VisitRootIfNonNull(jobject root, const RootInfo& info) {
    if (root != nullptr) {
      VisitRoot(root, info);
    }
  }
  virtual void VisitRoots(jobject* roots, size_t count, const RootInfo& info) = 0;
};
class SingleRootVisitor : public RootVisitor {
 private:
  void VisitRoots(jobject* roots, size_t count, const RootInfo& info) override {
    for (size_t i = 0; i < count; ++i) {
      VisitRoot(roots[i], info);
    }
  }
  virtual void VisitRoot(jobject root, const RootInfo& info) = 0;
};
}
namespace maplert {
typedef uintptr_t address_t;
typedef intptr_t offset_t;
static constexpr address_t kDummyAddress = static_cast<address_t>(-1);
typedef address_t reffield_t;
template<class T>
inline T& AddrToLVal(address_t addr) {
  return *reinterpret_cast<T*>(addr);
}
template<class T>
inline std::atomic<T>& AddrToLValAtomic(address_t addr) {
  return *reinterpret_cast<std::atomic<T>*>(addr);
}
static inline address_t AtomicExchangeObjRef(address_t* slot, address_t newValue) {
  return std::atomic_exchange_explicit(
           reinterpret_cast<std::atomic<address_t>*>(slot),
           newValue, std::memory_order_relaxed);
}
static inline address_t RefFieldToAddress(reffield_t reffield) {
  return static_cast<address_t>(reffield);
}
static inline reffield_t AddressToRefField(address_t addr) {
  return static_cast<reffield_t>(addr);
}
static inline address_t LoadRefField(address_t *fieldAddr) {
  return static_cast<address_t>(*reinterpret_cast<reffield_t *>(fieldAddr));
}
static inline address_t LoadRefField(address_t obj, std::size_t offset) {
  return LoadRefField(reinterpret_cast<address_t *>(obj+offset));
}
static inline void StoreRefField(address_t *fieldAddr, address_t newVal) {
  *reinterpret_cast<reffield_t *>(fieldAddr) = static_cast<reffield_t>(newVal);
}
static inline void StoreRefField(address_t obj, std::size_t offset, address_t newVal) {
  StoreRefField(reinterpret_cast<address_t *>(obj+offset), newVal);
}
}
namespace maplert {
typedef std::function<void()> GCFinishCallbackFunc;
}
namespace maplert {
const float kGCWaterLevelLow = 1.1;
const float kGCWaterLevel = 1.2;
const float kHeapWaterLevel = 0.5;
const uint64_t kInitSystemGCThreshold = 80 * 1024 * 1024;
const uint64_t kInitGCThreshold = 20 * 1024 * 1024;
const uint64_t kMinOOMInitalNs = 600000 * 1000UL * 1000UL;
const uint64_t kMinUserTriggerIntervalNs = 120000 * 1000UL * 1000UL;
enum GCReason : unsigned int {
  kMRTGCReasonUser = 0,
  kMRTGCReasonUserCheck = 1,
  kMRTGCReasonUserNi = 2,
  kMRTGCReasonOOM = 3,
  kMRTGCReasonHeuristic = 4,
  kMRTGCReasonForceGC = 5,
  kMRTGCReasonNative = 6,
};
class GCReasonConfig {
 public:
  GCReason reason;
  const char *name;
  uint64_t lastTriggerTimestamp;
  const uint64_t minIntervelNs;
  void (*tooFrequentCallback)();
  bool IsTooFrequentTriggered();
  void SetLastTriggerTime(uint64_t timestamp);
  inline bool IsBlockingGCReason() const {
    return ((reason != kMRTGCReasonHeuristic) && (reason != kMRTGCReasonNative));
  }
};
extern GCReasonConfig reasonCfgs[];
}
typedef uintptr_t address_t;
namespace maplert {
extern "C" {
__attribute__((visibility("default"))) void MRT_IncRef(address_t obj);
__attribute__((visibility("default"))) void MRT_DecRef(address_t obj);
__attribute__((visibility("default"))) void MRT_IncDecRef(address_t incObj, address_t decObj);
__attribute__((visibility("default"))) void MRT_DecRefUnsync(address_t obj);
__attribute__((visibility("default"))) void MRT_IncResurrectWeak(address_t obj);
__attribute__((visibility("default"))) void MRT_ReleaseObj(address_t obj);
__attribute__((visibility("default"))) address_t MRT_IncRef_NaiveRCFast(address_t obj);
__attribute__((visibility("default"))) void MRT_DecRef_NaiveRCFast(address_t obj);
__attribute__((visibility("default"))) void MRT_IncDecRef_NaiveRCFast(address_t inc_addr, address_t dec_addr);
__attribute__((visibility("default"))) void MRT_WeakRefGetBarrier(address_t referent);
__attribute__((visibility("default"))) uint32_t MRT_RefCount(address_t obj);
__attribute__((visibility("default"))) bool MRT_CheckHeapObj(uintptr_t obj);
__attribute__((visibility("default"))) bool MRT_IsGarbage(address_t obj);
__attribute__((visibility("default"))) bool MRT_IsValidOffHeapObject(jobject obj);
__attribute__((visibility("default"))) bool MRT_EnterSaferegion();
__attribute__((visibility("default"))) bool MRT_LeaveSaferegion();
__attribute__((visibility("default"))) address_t MRT_GetHeapLowerBound();
__attribute__((visibility("default"))) address_t MRT_GetHeapUpperBound();
__attribute__((visibility("default"))) bool MRT_IsValidObjAddr(address_t obj);
__attribute__((visibility("default"))) bool MRT_GCInitGlobal(size_t heap_start_size,
                                 size_t heap_size,
                                 size_t heap_growth_limit,
                                 size_t heap_min_free,
                                 size_t heap_max_free,
                                 float heap_target_utilization,
                                 bool ignore_max_footprint);
__attribute__((visibility("default"))) bool MRT_GCFiniGlobal();
__attribute__((visibility("default"))) bool MRT_GCInitThreadLocal(bool is_main);
__attribute__((visibility("default"))) bool MRT_GCFiniThreadLocal();
__attribute__((visibility("default"))) void MRT_GCStart();
__attribute__((visibility("default"))) void MRT_GCPreFork();
__attribute__((visibility("default"))) void MRT_GCPostForkChild(bool is_system);
__attribute__((visibility("default"))) void MRT_GCPostForkCommon(bool isZygote);
__attribute__((visibility("default"))) void MRT_GCLogPostFork();
__attribute__((visibility("default"))) void MRT_getGcCounts(uint64_t &gc_count, uint64_t &max_gc_ms);
__attribute__((visibility("default"))) void MRT_getMemLeak(uint64_t &avg_leak, uint64_t &peak_leak);
__attribute__((visibility("default"))) void MRT_getMemAlloc(float &util, uint64_t &abnormal_count);
__attribute__((visibility("default"))) void MRT_getRCParam(uint64_t &abnormal_count);
__attribute__((visibility("default"))) void MRT_getCyclePattern(std::ostream& os);
inline unsigned long MRT_GetGCUsedHeapMemoryTotal() {return 0;};
__attribute__((visibility("default"))) void MRT_ResetHeapStats();
__attribute__((visibility("default"))) uint64_t MRT_AllocSize();
__attribute__((visibility("default"))) uint64_t MRT_AllocCount();
__attribute__((visibility("default"))) uint64_t MRT_FreeSize();
__attribute__((visibility("default"))) uint64_t MRT_FreeCount();
__attribute__((visibility("default"))) uint64_t MRT_TotalMemory();
__attribute__((visibility("default"))) uint64_t MRT_MaxMemory();
__attribute__((visibility("default"))) uint64_t MRT_FreeMemory();
__attribute__((visibility("default"))) void MRT_SetHeapProfile(int hp);
__attribute__((visibility("default"))) void MRT_ShowHeapUsage();
__attribute__((visibility("default"))) void MRT_GetInstances(jclass klass, bool include_assignable,
    size_t max_count, std::vector<jobject>& instances);
__attribute__((visibility("default"))) void MRT_Debug_Cleanup();
__attribute__((visibility("default"))) void MRT_RegisterGCRoots(address_t* gcroots[], size_t len);
__attribute__((visibility("default"))) void MRT_SetReferenceProcessMode(bool immediate);
__attribute__((visibility("default"))) void *MRT_ProcessReferences(void *args);
__attribute__((visibility("default"))) void MRT_TriggerRefProcessor();
__attribute__((visibility("default"))) bool MRT_IsRefProcessorWorking();
__attribute__((visibility("default"))) void MRT_StopProcessReferences(bool doFinalizeOnStop=false);
__attribute__((visibility("default"))) void MRT_WaitProcessReferencesStopped();
__attribute__((visibility("default"))) void MRT_WaitProcessReferencesStarted();
__attribute__((visibility("default"))) void MRT_WaitGCStopped();
__attribute__((visibility("default"))) void MRT_CheckSaferegion(bool expect, const char* msg);
__attribute__((visibility("default"))) void MRT_DumpRCAndGCPerformanceInfo(std::ostream& os);
__attribute__((visibility("default"))) void MRT_TriggerGC(maplert::GCReason reason);
__attribute__((visibility("default"))) bool MRT_SetTriggerGCEnabled(bool enabled);
__attribute__((visibility("default"))) bool MRT_IsTriggerGCEnabled();
__attribute__((visibility("default"))) bool MRT_SetAllowUserGCBeforeOOM(bool allowed);
__attribute__((visibility("default"))) bool MRT_IsAllowUserGCBeforeOOM();
__attribute__((visibility("default"))) bool MRT_IsGcThread();
__attribute__((visibility("default"))) bool MRT_IsValidObjectAddress(address_t obj);
__attribute__((visibility("default"))) void MRT_VisitAllocatedObjects(maple::root_object_func f);
__attribute__((visibility("default"))) void MRT_Trim(bool aggressive);
__attribute__((visibility("default"))) void MRT_RequestTrim();
__attribute__((visibility("default"))) void MRT_SetHwBlobClass(jclass cls);
__attribute__((visibility("default"))) void MRT_SetSurfaceControlClass(jclass cls);
__attribute__((visibility("default"))) void MRT_DumpDynamicCyclePatterns(std::ostream& os, size_t limit);
__attribute__((visibility("default"))) void MRT_SetHwBlobClass(jclass cls);
__attribute__((visibility("default"))) void MRT_SetSurfaceControlClass(jclass cls);
__attribute__((visibility("default"))) bool MRT_HasDynamicLoadPattern(jclass cls);
__attribute__((visibility("default"))) void MRT_SetHasDynamicLoadPattern(jclass cls);
__attribute__((visibility("default"))) void MRT_SendCyclePatternJob(std::function<void()> job);
__attribute__((visibility("default"))) void MRT_SetPeriodicSaveCpJob(std::function<void()> job);
__attribute__((visibility("default"))) void MRT_SetPeriodicLearnCpJob(std::function<void()> job);
__attribute__((visibility("default"))) bool MRT_IsCyclePatternUpdated();
__attribute__((visibility("default"))) void MRT_SetGCFinishCallback(maplert::GCFinishCallbackFunc callback);
__attribute__((visibility("default"))) void MRT_DumpStaticField(std::ostream& os);
__attribute__((visibility("default"))) void MRT_logRefqueuesSize();
__attribute__((visibility("default"))) void MRT_PreWriteRefField(address_t obj);
__attribute__((visibility("default"))) size_t MRT_GetNativeAllocBytes();
__attribute__((visibility("default"))) void MRT_SetNativeAllocBytes(size_t size);
__attribute__((visibility("default"))) address_t MRT_LoadVolatileField(address_t obj, address_t* field_addr);
__attribute__((visibility("default"))) address_t MRT_LoadRefFieldCommon(address_t obj, address_t* field_addr);
__attribute__((visibility("default"))) void MRT_ClassInstanceNum(std::map<std::string, long> &objNameCntMp);
__attribute__((visibility("default"))) jobject MRT_Reference_getReferent(jobject javaThis);
__attribute__((visibility("default"))) void MRT_Reference_clearReferent(jobject javaThis);
__attribute__((visibility("default"))) bool MRT_Reference_runFinalization();
}
}
typedef void* mrt_class_t;
typedef void* mrt_throwable_t;
namespace maplert {
extern "C" {
  void* MRT_GetPollingPage();
  __attribute__ ((always_inline)) static inline void MRT_Yieldpoint() {
    __attribute__ ((unused))
    volatile register void* x = *(reinterpret_cast<void**>(MRT_GetPollingPage()));
  }
  uint64_t MRT_TotalHeapObj();
  void MRT_Trim(bool aggressive);
  address_t __MRT_Perm_NewObj(size_t size);
  address_t __MRT_NewObj_internal(size_t size, size_t align);
  address_t MRT_NewObj(size_t size, size_t align);
  address_t __MRT_NewObj_flexible_internal(
      size_t fixed_size, size_t elem_size, size_t n_elems, size_t align);
  void MRT_SubObjBytes(size_t cor_size);
  void MRT_FreeObj(address_t obj);
  void MRT_ResetHeapStats();
  void MRT_PrintRCStats();
  void MRT_ResetRCStats();
  size_t MRT_GetNativeAllocBytes();
  void MRT_SetNativeAllocBytes(size_t size);
  address_t MRT_LoadVolatileField(address_t obj, address_t* field_addr);
  address_t MRT_LoadRefFieldCommon(address_t obj, address_t* field_addr);
  void MRT_ClassInstanceNum(std::map<std::string, long> &objNameCntMp);
  void MRT_WriteRefField(address_t obj, address_t* field, address_t value);
  void MRT_WriteRefFieldNoRC(address_t obj, address_t* field, address_t value);
  void MRT_WriteRefFieldNoDec(address_t obj, address_t* field, address_t value);
  void MRT_WriteRefFieldNoInc(address_t obj, address_t* field, address_t value);
  void MRT_WriteVolatileField(address_t obj, address_t* obj_addr, address_t value);
  void MRT_WriteVolatileFieldNoInc(address_t obj, address_t* obj_addr, address_t value);
  void MRT_WriteVolatileFieldNoDec(address_t obj, address_t* obj_addr, address_t value);
  void MRT_WriteVolatileFieldNoRC(address_t obj, address_t* obj_addr, address_t value);
  void MRT_WriteWeakField(address_t obj, address_t* field_addr, address_t value);
  address_t MRT_LoadWeakField(address_t obj, address_t* field_addr);
  void MRT_WriteReferentField(address_t obj, address_t* field_addr, address_t value, bool isResurrectWeak);
  address_t MRT_LoadReferentField(address_t obj, address_t* field_addr);
  address_t MRT_ForceLoadReferentField(address_t obj, address_t* fieldAddr);
  void MRT_WriteRefVar(address_t* var, address_t value);
  void MRT_WriteRefVarNoInc(address_t* var, address_t value);
  void MRT_PreRenewObject(address_t obj);
  void MRT_SetTracingObject(address_t obj);
  void MRT_ReleaseRefVar(address_t obj);
  bool MRT_IsValidObjectAddress(address_t obj);
  void MRT_TriggerGC(maplert::GCReason reason);
  bool MRT_IsGcThread();
  bool MRT_IsGcRunning();
  bool MRT_FastIsValidObjAddr(address_t obj);
  void MRT_CheckLeakAndCycle(bool dump_heap);
  void MRT_Debug_ShowCurrentMutators();
  uint64_t MRT_AllocSize();
  uint64_t MRT_AllocCount();
  uint64_t MRT_FreeSize();
  uint64_t MRT_FreeCount();
  void MRT_DumpHeap(uint32_t index, const std::string& msg);
  void MRT_DumpRCAndGCPerformanceInfo(std::ostream& os);
  void MRT_DumpRCAndGCPerformanceInfo_Stderr();
  void MRT_VisitAllocatedObjects(maple::root_object_func f);
  bool MRT_SetTriggerGCEnabled(bool enabled);
  bool MRT_IsTriggerGCEnabled();
  bool MRT_SetAllowUserGCBeforeOOM(bool allowed);
  bool MRT_IsAllowUserGCBeforeOOM();
  bool MRT_SetAllowCreateCPFromBT(bool allowed);
  bool MRT_IsAllowCreateCPFromBT();
  void MRT_DumpDynamicCyclePatterns(std::ostream& os, size_t limit);
  void MRT_SendCyclePatternJob(std::function<void()> job);
  void MRT_SetPeriodicSaveCpJob(std::function<void()> job);
  void MRT_SetPeriodicLearnCpJob(std::function<void()> job);
  void MRT_SendSaveCpJob();
  bool MRT_IsCyclePatternUpdated();
  void MRT_SetHwBlobClass(jclass cls);
  void MRT_SetSurfaceControlClass(jclass cls);
  void MRT_SetGCFinishCallback(maplert::GCFinishCallbackFunc callback);
  void MRT_DumpStaticField(std::ostream& os);
  void MRT_HandleJavaSignal();
  void MRT_WaitGCStopped();
}
}
namespace maplert {
}
namespace maplert {
}
namespace maplert {
extern "C" {
__attribute__((visibility("default"))) address_t MRT_LoadRefField(address_t obj, address_t* field_addr);
__attribute__((visibility("default"))) void MRT_WriteRefField(address_t obj, address_t* field, address_t value);
__attribute__((visibility("default"))) address_t MRT_LoadVolatileField(address_t obj, address_t* field_addr);
__attribute__((visibility("default"))) void MRT_WriteVolatileField(address_t obj, address_t* obj_addr, address_t value);
__attribute__((visibility("default"))) void MRT_WriteWeakField(address_t obj, address_t* field_addr, address_t value);
__attribute__((visibility("default"))) address_t MRT_LoadWeakField(address_t obj, address_t* field_addr);
__attribute__((visibility("default"))) void MRT_WriteReferentField(address_t obj, address_t* field_addr, address_t value,
                                            bool isResurrectWeak);
__attribute__((visibility("default"))) address_t MRT_LoadReferentField(address_t obj, address_t* field_addr);
__attribute__((visibility("default"))) address_t MRT_ForceLoadReferentField(address_t obj, address_t* field_addr);
static inline bool __MRT_load_jboolean(address_t obj, size_t offset) {
  ;
  return *(bool*)(obj + offset);
}
static inline bool __MRT_load_jboolean_volatile(address_t obj, size_t offset) {
  ;
  return reinterpret_cast<std::atomic<bool>*>(obj + offset)->load(std::memory_order_seq_cst);
}
static inline int8_t __MRT_load_jbyte(address_t obj, size_t offset) {
  ;
  return *(int8_t*)(obj + offset);
}
static inline int8_t __MRT_load_jbyte_volatile(address_t obj, size_t offset) {
  ;
  return reinterpret_cast<std::atomic<int8_t>*>(obj + offset)->load(std::memory_order_seq_cst);
}
static inline int16_t __MRT_load_jshort(address_t obj, size_t offset) {
  ;
  return *(int16_t*)(obj + offset);
}
static inline int16_t __MRT_load_jshort_volatile(address_t obj, size_t offset) {
  ;
  return reinterpret_cast<std::atomic<int16_t>*>(obj + offset)->load(std::memory_order_seq_cst);
}
static inline uint16_t __MRT_load_jchar(address_t obj, size_t offset) {
  ;
  return *(uint16_t*)(obj + offset);
}
static inline uint16_t __MRT_load_jchar_volatile(address_t obj, size_t offset) {
  ;
  return reinterpret_cast<std::atomic<uint16_t>*>(obj + offset)->load(std::memory_order_seq_cst);
}
static inline int32_t __MRT_load_jint(address_t obj, size_t offset) {
  ;
  return *(int32_t*)(obj + offset);
}
static inline int32_t __MRT_load_jint_volatile(address_t obj, size_t offset) {
  ;
  return reinterpret_cast<std::atomic<int32_t>*>(obj + offset)->load(std::memory_order_seq_cst);
}
static inline int64_t __MRT_load_jlong(address_t obj, size_t offset) {
  ;
  return *(int64_t*)(obj + offset);
}
static inline int64_t __MRT_load_jlong_volatile(address_t obj, size_t offset) {
  ;
  return reinterpret_cast<std::atomic<int64_t>*>(obj + offset)->load(std::memory_order_seq_cst);
}
static inline float __MRT_load_jfloat(address_t obj, size_t offset) {
  ;
  return *(float*)(obj + offset);
}
static inline float __MRT_load_jfloat_volatile(address_t obj, size_t offset) {
  ;
  return reinterpret_cast<std::atomic<float>*>(obj + offset)->load(std::memory_order_seq_cst);
}
static inline double __MRT_load_jdouble(address_t obj, size_t offset) {
  ;
  return *(double*)(obj + offset);
}
static inline double __MRT_load_jdouble_volatile(address_t obj, size_t offset) {
  ;
  return reinterpret_cast<std::atomic<double>*>(obj + offset)->load(std::memory_order_seq_cst);
}
static inline uintptr_t __MRT_load_jobject(address_t obj, size_t offset) {
  ;
  return (uintptr_t)LoadRefField(obj, offset);
}
static inline uintptr_t __MRT_load_jobject_inc(address_t obj, size_t offset) {
  ;
  uintptr_t res = (uintptr_t)MRT_LoadRefField(obj, (address_t*)(obj + offset));
  return res;
}
static inline uintptr_t __MRT_load_jobject_inc_volatile(address_t obj, size_t offset) {
  ;
  uintptr_t res = (uintptr_t)MRT_LoadVolatileField(obj, (address_t*)(obj + offset));
  return res;
}
static inline uintptr_t __MRT_load_jobject_inc_referent(address_t obj, size_t offset) {
  ;
  uintptr_t res = (uintptr_t)MRT_LoadReferentField(obj, (address_t*)(obj + offset));
  return res;
}
static inline uintptr_t __MRT_force_load_jobject_inc_referent(address_t obj, size_t offset) {
  ;
  uintptr_t res = (uintptr_t)MRT_ForceLoadReferentField(obj, (address_t*)(obj + offset));
  return res;
}
static inline void __MRT_store_jboolean(address_t obj, size_t offset, bool value) {
  ;
  *(bool*)(obj + offset) = value;
}
static inline void __MRT_store_jboolean_volatile(address_t obj, size_t offset, bool value) {
  ;
  reinterpret_cast<std::atomic<bool>*>(obj + offset)->store(value, std::memory_order_seq_cst);
}
static inline void __MRT_store_jbyte(address_t obj, size_t offset, int8_t value) {
  ;
  *(int8_t*)(obj + offset) = value;
}
static inline void __MRT_store_jbyte_volatile(address_t obj, size_t offset, int8_t value) {
  ;
  reinterpret_cast<std::atomic<int8_t>*>(obj + offset)->store(value, std::memory_order_seq_cst);
}
static inline void __MRT_store_jshort(address_t obj, size_t offset, int16_t value) {
  ;
  *(int16_t*)(obj + offset) = value;
}
static inline void __MRT_store_jshort_volatile(address_t obj, size_t offset, int16_t value) {
  ;
  reinterpret_cast<std::atomic<int16_t>*>(obj + offset)->store(value, std::memory_order_seq_cst);
}
static inline void __MRT_store_jchar(address_t obj, size_t offset, uint16_t value) {
  ;
  *(uint16_t*)(obj + offset) = value;
}
static inline void __MRT_store_jchar_volatile(address_t obj, size_t offset, uint16_t value) {
  ;
  reinterpret_cast<std::atomic<uint16_t>*>(obj + offset)->store(value, std::memory_order_seq_cst);
}
static inline void __MRT_store_jint(address_t obj, size_t offset, int32_t value) {
  ;
  *(int32_t*)(obj + offset) = value;
}
static inline void __MRT_store_jint_volatile(address_t obj, size_t offset, int32_t value) {
  ;
  reinterpret_cast<std::atomic<int32_t>*>(obj + offset)->store(value, std::memory_order_seq_cst);
}
static inline void __MRT_store_ordered_jint(address_t obj, size_t offset, int32_t value) {
  ;
  reinterpret_cast<std::atomic<int32_t>*>(obj + offset)->store(value, std::memory_order_release);
}
static inline void __MRT_store_jlong(address_t obj, size_t offset, int64_t value) {
  ;
  *(int64_t*)(obj + offset) = value;
}
static inline void __MRT_store_jlong_volatile(address_t obj, size_t offset, int64_t value) {
  ;
  reinterpret_cast<std::atomic<int64_t>*>(obj + offset)->store(value, std::memory_order_seq_cst);
}
static inline void __MRT_store_ordered_jlong(address_t obj, size_t offset, int64_t value) {
  ;
  reinterpret_cast<std::atomic<int64_t>*>(obj + offset)->store(value, std::memory_order_release);
}
static inline void __MRT_store_jfloat(address_t obj, size_t offset, float value) {
  ;
  *(float*)(obj + offset) = value;
}
static inline void __MRT_store_jfloat_volatile(address_t obj, size_t offset, float value) {
  ;
  reinterpret_cast<std::atomic<float>*>(obj + offset)->store(value, std::memory_order_seq_cst);
}
static inline void __MRT_store_jdouble(address_t obj, size_t offset, double value) {
  ;
  *(double*)(obj + offset) = value;
}
static inline void __MRT_store_jdouble_volatile(address_t obj, size_t offset, double value) {
  ;
  reinterpret_cast<std::atomic<double>*>(obj + offset)->store(value, std::memory_order_seq_cst);
}
static inline void __MRT_store_jobject(address_t obj, size_t offset, uintptr_t value) {
  ;
  MRT_WriteRefField(obj, (address_t*)(obj + offset), (address_t)(value));
}
static inline void __MRT_store_meta_jobject(address_t obj, size_t offset, uintptr_t value) {
  ;
  StoreRefField(obj, offset, (address_t)value);
}
static inline void __MRT_store_jobject_norc(address_t obj, size_t offset, uintptr_t value) {
  ;
  StoreRefField(obj, offset, (address_t)value);
}
static inline void __MRT_store_jobject_volatile(address_t obj, size_t offset, uintptr_t value) {
  ;
  MRT_WriteVolatileField(obj, (address_t*)(obj + offset), (address_t)(value));
}
static inline void __MRT_write_referent(address_t obj, size_t offset, uintptr_t value, bool isResurrectWeak) {
  ;
  MRT_WriteReferentField(obj, (address_t*)(obj + offset), (address_t)value, isResurrectWeak);
}
}
}
namespace maplert {
void __MRT_Panic() __attribute__((noreturn));
void __MRT_AssertBreakPoint() __attribute__((noinline));
}
extern "C" {
using metaref_t = uintptr_t;
using metaref_offset_t = int64_t;
inline void MRT_SetMetarefOffset(metaref_offset_t *pOffset, char *addr) {
  (*pOffset) = (metaref_offset_t)(addr - (char*)pOffset);
}
inline void *MRT_GetAddressFromMetarefOffset(metaref_offset_t *pOffset) {
  return (void*)((char*)pOffset + (*pOffset));
}
typedef struct {
  union {
    uint64_t realOffset;
    void *absAddress;
  } offset;
  metaref_offset_t declaringclass;
  uint16_t flag;
  uint16_t padding;
  int32_t fieldname;
  int32_t annotation;
  uint32_t padding1;
} FieldMetadataRO;
typedef metaref_offset_t FieldMetadataRORef;
typedef struct {
  metaref_t shadow;
  int32_t monitor;
  uint32_t mod;
  FieldMetadataRORef fieldinforo;
  union {
    struct {
      int32_t taboffset;
      int32_t offset;
    } name;
    void *klass;
  } type;
} FieldMetadata;
typedef struct {
  uint32_t mod;
  int32_t type_name;
  int32_t offset;
  uint16_t flag;
  uint16_t padding;
  int32_t fieldname;
  int32_t annotation;
} FieldMetadataCompact;
typedef struct {
  int32_t methodname;
  int32_t signaturename;
  void *addr;
  char *annotationvalue;
  metaref_offset_t declaringclass;
  uint16_t flag;
  uint16_t argsize;
  int32_t padding;
  int64_t method_in_vtab_index;
} MethodMetadataRO;
typedef metaref_offset_t MethodMetadataRORef;
typedef struct {
  metaref_t shadow;
  int32_t monitor;
  uint32_t mod;
  MethodMetadataRORef methodinforo;
} MethodMetadata;
typedef struct {
  uint32_t mod;
  int32_t methodname;
  int32_t signaturename;
  int32_t addr;
  int32_t annotationvalue;
  int32_t declaringclass;
  uint16_t flag;
  uint16_t argsize;
  int32_t method_in_vtab_index;
} MethodMetadataCompact;
typedef struct {
  uint32_t metadataOffset;
  uint16_t localRefOffset;
  uint16_t localRefNumber;
} MethodDesc;
typedef struct {
  char *classname;
  FieldMetadata *fields;
  MethodMetadata *methods;
  union {
    struct CLASSMETADATA **superclass;
    struct CLASSMETADATA *componentclass;
  } familyclass;
  uint16_t numoffields;
  uint16_t numofmethods;
  uint16_t flag;
  uint16_t numofsuperclasses;
  uint32_t padding;
  uint32_t mod;
  char *annotation;
} ClassMetadataRO;
typedef struct CLASSMETADATA {
  metaref_t shadow;
  int32_t monitor;
  uint16_t clindex;
  union {
    uint16_t objsize;
    uint16_t componentsize;
  } sizeinfo;
  void *itab;
  void *vtab;
  void *gctib;
  ClassMetadataRO *classinforo;
  union {
    intptr_t initstate;
    intptr_t staticfields;
  };
} ClassMetadata;
static inline intptr_t ClassMetadataOffsetOfInitFlag() {
  ClassMetadata* base = (ClassMetadata*)(0);
  return (intptr_t)(&(base->initstate));
}
static inline intptr_t ClassMetadataOffsetOfStaticFields() {
  ClassMetadata* base = (ClassMetadata*)(0);
  return (intptr_t)(&(base->staticfields));
}
}
template<typename M, typename C>
static inline void MRT_SetMetadataShadow(M *meta, C cls) {
  meta->shadow = (metaref_t)(uintptr_t)cls;
}
template<typename M, typename C>
static inline void MRT_SetMetadataDeclaringClass(M *meta, C cls) {
  meta->declaringclass = (metaref_offset_t)(intptr_t)cls;
}
extern "C" {
__attribute__((noreturn)) void abort_message(const char* format, ...) __attribute__((format(printf, 1, 2)));
}
namespace maplert {
extern "C" {
typedef enum {
  _URC_NO_REASON = 0,
  _URC_FOREIGN_EXCEPTION_CAUGHT = 1,
  _URC_FATAL_PHASE2_ERROR = 2,
  _URC_FATAL_PHASE1_ERROR = 3,
  _URC_NORMAL_STOP = 4,
  _URC_END_OF_STACK = 5,
  _URC_HANDLER_FOUND = 6,
  _URC_INSTALL_CONTEXT = 7,
  _URC_CONTINUE_UNWIND = 8,
} _Unwind_Reason_Code;
typedef enum {
  _UA_SEARCH_PHASE = 1,
  _UA_CLEANUP_PHASE = 2,
  _UA_HANDLER_FRAME = 4,
  _UA_FORCE_UNWIND = 8,
  _UA_END_OF_STACK = 16
} _Unwind_Action;
using _Unwind_Word = uintptr_t;
using _Unwind_Sword = intptr_t;
using _Unwind_Ptr = uintptr_t;
using _Unwind_Internal_Ptr = uintptr_t;
using _Unwind_Exception_Class = uint64_t;
using _sleb128_t = intptr_t;
using _uleb128_t = uintptr_t;
struct _Unwind_Context;
struct _Unwind_Exception;
typedef void (*_Unwind_Exception_Cleanup_Fn)(_Unwind_Reason_Code,
                                             struct _Unwind_Exception *);
struct _Unwind_Exception {
  _Unwind_Exception_Class exception_class;
  _Unwind_Exception_Cleanup_Fn exception_cleanup;
  _Unwind_Word private_1;
  _Unwind_Word private_2;
} __attribute__((__aligned__));
typedef _Unwind_Reason_Code (*_Unwind_Stop_Fn)(int, _Unwind_Action,
                                               _Unwind_Exception_Class,
                                               struct _Unwind_Exception *,
                                               struct _Unwind_Context *,
                                               void *);
typedef _Unwind_Reason_Code (*_Unwind_Personality_Fn)(
    int, _Unwind_Action, _Unwind_Exception_Class, struct _Unwind_Exception *,
    struct _Unwind_Context *);
typedef _Unwind_Personality_Fn __personality_routine;
typedef _Unwind_Reason_Code (*_Unwind_Trace_Fn)(struct _Unwind_Context *,
                                                void *);
_Unwind_Word _Unwind_GetGR(struct _Unwind_Context *, int);
void _Unwind_SetGR(struct _Unwind_Context *, int, _Unwind_Word);
_Unwind_Word _Unwind_GetIP(struct _Unwind_Context *);
void _Unwind_SetIP(struct _Unwind_Context *, _Unwind_Word);
_Unwind_Word _Unwind_GetIPInfo(struct _Unwind_Context *, int *);
_Unwind_Word _Unwind_GetCFA(struct _Unwind_Context *);
_Unwind_Word _Unwind_GetBSP(struct _Unwind_Context *);
void *_Unwind_GetLanguageSpecificData(struct _Unwind_Context *);
_Unwind_Ptr _Unwind_GetRegionStart(struct _Unwind_Context *);
_Unwind_Reason_Code _Unwind_RaiseException(struct _Unwind_Exception *);
_Unwind_Reason_Code _Unwind_ForcedUnwind(struct _Unwind_Exception *,
                                         _Unwind_Stop_Fn, void *);
void _Unwind_DeleteException(struct _Unwind_Exception *);
void _Unwind_Resume(struct _Unwind_Exception *);
_Unwind_Reason_Code _Unwind_Resume_or_Rethrow(struct _Unwind_Exception *);
_Unwind_Reason_Code _Unwind_Backtrace(_Unwind_Trace_Fn, void *);
extern __attribute__((__visibility__("default"))) void *__cxa_begin_catch(void *exceptionObject) noexcept;
}
struct scan_results {
    int64_t ttypeIndex;
    const uint8_t* actionRecord;
    const uint8_t* languageSpecificData;
    uintptr_t landingPad;
    jthrowable thrownObject;
    bool caughtByJava;
    _Unwind_Reason_Code unwindReason;
};
#pragma GCC visibility push(hidden)
static const uint64_t kOurExceptionClass = 0x4d504c4a41564100;
static const uint64_t get_vendor_and_language = 0xFFFFFFFFFFFFFF00;
#pragma GCC visibility pop
__attribute__((visibility("hidden"), noreturn)) void __terminate() noexcept;
extern "C" {
__attribute__((__visibility__("default"))) _Unwind_Reason_Code __mpl_personality_v0 (int version,
    _Unwind_Action actions, uint64_t exceptionClass, _Unwind_Exception* unwind_exception, _Unwind_Context* context);
}
}
namespace maplert {
extern "C" {
__attribute__((visibility("default"))) void MCC_ThrowNullArrayNullPointerException();
__attribute__((visibility("default"))) void MCC_CheckThrowPendingException();
__attribute__((visibility("default"))) void MCC_ThrowNullPointerException();
__attribute__((visibility("default"))) void MCC_ThrowArithmeticException();
__attribute__((visibility("default"))) void MCC_ThrowInterruptedException();
__attribute__((visibility("default"))) void MCC_ThrowClassCastException(const char *msg);
__attribute__((visibility("default"))) void MCC_ThrowArrayIndexOutOfBoundsException(const char *msg);
__attribute__((visibility("default"))) void MCC_ThrowUnsatisfiedLinkError();
__attribute__((visibility("default"))) void MCC_ThrowSecurityException();
__attribute__((visibility("default"))) void MCC_ThrowExceptionInInitializerError(mrt_throwable_t cause);
__attribute__((visibility("default"))) void MCC_ThrowNoClassDefFoundError(mrt_class_t classinfo);
__attribute__((visibility("default"))) void MCC_ThrowStringIndexOutOfBoundsException();
__attribute__((visibility("default"))) void* MCC_JavaBeginCatch (_Unwind_Exception *unwind_exception);
__attribute__((visibility("default"))) void MCC_ThrowException(mrt_throwable_t obj);
__attribute__((visibility("default"))) void MCC_ThrowPendingException();
__attribute__((visibility("default"))) void MCC_RethrowException(mrt_throwable_t obj);
__attribute__((visibility("default"))) void MCC_SetRiskyUnwindContext(uint32_t *pc, void *fp);
__attribute__((visibility("default"))) void MCC_SetReliableUnwindContext();
__attribute__((visibility("default"))) bool MCC_String_Equals_NotallCompress(jstring thisStr, jstring anotherStr);
__attribute__((visibility("default"))) jstring MCC_CStrToJStr(const char *ca, jint len);
__attribute__((visibility("default"))) char* MCC_GetStringUTFChars(jstring jstr, jboolean* isCopy);
__attribute__((visibility("default"))) jstring MCC_GetOrInsertLiteral(jstring literal);
__attribute__((visibility("default"))) jstring MCC_StringAppend(uint64_t toStringFlag, ...);
__attribute__((visibility("default"))) jstring MCC_StringAppend_StringString(jstring strObj_1, jstring strObj_2);
__attribute__((visibility("default"))) jstring MCC_StringAppend_StringInt(jstring strObj_1, jint intVaule);
__attribute__((visibility("default"))) jstring MCC_StringAppend_StringJcharString(jstring strObj_1,
                                                      uint16_t charVaule, jstring strObj_2);
__attribute__((visibility("default"))) jclass MCC_GetClass(jclass klass, const char* className);
__attribute__((visibility("default"))) jobject MCC_GetCurrentClassLoader(jobject obj);
__attribute__((visibility("default"))) void MCC_PreClinitCheck(ClassMetadata *classinfo __attribute__((unused)));
__attribute__((visibility("default"))) void MCC_PostClinitCheck(ClassMetadata *classinfo __attribute__((unused)));
__attribute__((visibility("default"))) jint MCC_Get_Array_Length(jobjectArray java_array);
__attribute__((visibility("default"))) void MCC_Array_Boundary_Check(jobjectArray javaArray, jint index);
__attribute__((visibility("default"))) void MCC_Reflect_ThrowCastException(void* sourceinfo, jobject target_object, jint isArray);
__attribute__((visibility("default"))) void MCC_Reflect_Check_Casting_Array(jclass source_class,
    jobject target_object, jint isArray, const char* sourceName);
__attribute__((visibility("default"))) void MCC_Reflect_Check_Arraystore(jobject array_object, jobject elem_object);
__attribute__((visibility("default"))) void MCC_Reflect_Check_Casting_NoArray(jclass sourceClass, jobject targetClass);
__attribute__((visibility("default"))) jboolean MCC_Reflect_IsInstance(jobject, jobject);
__attribute__((visibility("default"))) void *MCC_getFuncPtrFromItabSlow32(uint32_t *obj,
    uint32_t hashCode, uint32_t secondhashCode, char *signature);
__attribute__((visibility("default"))) void *MCC_getFuncPtrFromItabSlow64(unsigned long *obj,
    uint32_t hashCode, uint32_t secondhashCode, char *signature);
__attribute__((visibility("default"))) void *MCC_getFuncPtrFromItabSecondHash64(unsigned long *itab,
    unsigned long hashCode, char* signature);
__attribute__((visibility("default"))) void *MCC_getFuncPtrFromItabSecondHash32(int32_t *itab, int32_t hashCode, char* signature);
__attribute__((visibility("default"))) void *MCC_getFuncPtrFromVtab64(unsigned long *obj, uint32_t offset);
__attribute__((visibility("default"))) void *MCC_getFuncPtrFromVtab32(int32_t *obj, uint32_t offset);
__attribute__((visibility("default"))) void MCC_ArrayMap_String_Int_put (jstring key, jint value);
__attribute__((visibility("default"))) jint MCC_ArrayMap_String_Int_size ();
__attribute__((visibility("default"))) jint MCC_ArrayMap_String_Int_getOrDefault (jstring key, jint defaultValue);
__attribute__((visibility("default"))) void MCC_ArrayMap_String_Int_clear();
__attribute__((visibility("default"))) address_t MCC_IncRef_NaiveRCFast(address_t obj);
__attribute__((visibility("default"))) address_t MCC_LoadRefField_NaiveRCFast(address_t obj, address_t* filed_address);
__attribute__((visibility("default"))) void MCC_DecRef_NaiveRCFast(address_t obj);
__attribute__((visibility("default"))) void MCC_IncDecRef_NaiveRCFast(address_t inc_addr, address_t dec_addr);
__attribute__((visibility("default"))) void MCC_CleanupLocalStackRef_NaiveRCFast(address_t* local_start, size_t count);
__attribute__((visibility("default"))) void MCC_CleanupLocalStackRefSkip_NaiveRCFast(address_t* local_start, size_t count, size_t skip);
__attribute__((visibility("default"))) void MCC_CleanupNonescapedVar(address_t obj);
__attribute__((visibility("default"))) void MCC_ClearLocalStackRef(address_t* var);
__attribute__((visibility("default"))) address_t MCC_NewObj(size_t size, size_t align);
__attribute__((visibility("default"))) address_t MCC_NewObj_flexible(size_t fixed_size, size_t elem_size, size_t n_elems, size_t align);
__attribute__((visibility("default"))) address_t MCC_LoadRefStatic(address_t* field_addr);
__attribute__((visibility("default"))) address_t MCC_LoadVolatileStaticField(address_t* field_addr);
__attribute__((visibility("default"))) address_t MCC_LoadReferentField(address_t obj, address_t* field_addr);
__attribute__((visibility("default"))) void MCC_WriteRefFieldStatic(address_t* field, address_t value);
__attribute__((visibility("default"))) void MCC_WriteRefFieldStaticNoInc(address_t* field, address_t value);
__attribute__((visibility("default"))) void MCC_WriteRefFieldStaticNoDec(address_t* field, address_t value);
__attribute__((visibility("default"))) void MCC_WriteRefFieldStaticNoRC(address_t* field, address_t value);
__attribute__((visibility("default"))) void MCC_WriteVolatileStaticField(address_t* obj_addr, address_t value);
__attribute__((visibility("default"))) void MCC_WriteVolatileStaticFieldNoInc(address_t* obj_addr, address_t value);
__attribute__((visibility("default"))) void MCC_WriteVolatileStaticFieldNoDec(address_t* obj_addr, address_t value);
__attribute__((visibility("default"))) void MCC_WriteVolatileStaticFieldNoRC(address_t* obj_addr, address_t value);
__attribute__((visibility("default"))) address_t MCC_LoadVolatileField(address_t obj, address_t* field_addr);
__attribute__((visibility("default"))) void MCC_PreWriteRefField(address_t obj);
__attribute__((visibility("default"))) void MCC_WriteRefField(address_t obj, address_t* field, address_t value);
__attribute__((visibility("default"))) void MCC_WriteRefFieldNoRC(address_t obj, address_t* field, address_t value);
__attribute__((visibility("default"))) void MCC_WriteRefFieldNoDec(address_t obj, address_t* field, address_t value);
__attribute__((visibility("default"))) void MCC_WriteRefFieldNoInc(address_t obj, address_t* field, address_t value);
__attribute__((visibility("default"))) void MCC_WriteVolatileField(address_t obj, address_t* obj_addr, address_t value);
__attribute__((visibility("default"))) void MCC_WriteVolatileFieldNoInc(address_t obj, address_t* obj_addr, address_t value);
__attribute__((visibility("default"))) void MCC_WriteWeakField(address_t obj, address_t* field_addr, address_t value);
__attribute__((visibility("default"))) address_t MCC_LoadWeakField(address_t obj, address_t* field_addr);
__attribute__((visibility("default"))) void MCC_WriteVolatileWeakField(address_t obj, address_t* field_addr, address_t value);
__attribute__((visibility("default"))) address_t MCC_LoadVolatileWeakField(address_t obj, address_t* field_addr);
__attribute__((visibility("default"))) void MCC_WriteReferent(address_t obj, address_t value);
__attribute__((visibility("default"))) void MCC_InitializeLocalStackRef(address_t* local_start, size_t count);
__attribute__((visibility("default"))) void MCC_CleanupLocalStackRef(address_t* local_start, size_t count);
__attribute__((visibility("default"))) void MCC_CleanupLocalStackRefSkip(address_t* local_start, size_t count, size_t skip);
__attribute__((visibility("default"))) void* MCC_CallFastNativeExt(...);
__attribute__((visibility("default"))) void* MCC_CallFastNative(void* func);
__attribute__((visibility("default"))) void* MCC_CallSlowNative0(void* func);
__attribute__((visibility("default"))) void* MCC_CallSlowNative1(void* func, uint64_t arg1);
__attribute__((visibility("default"))) void* MCC_CallSlowNative2(void* func, uint64_t arg1, uint64_t arg2);
__attribute__((visibility("default"))) void* MCC_CallSlowNative3(void* func, uint64_t arg1, uint64_t arg2, uint64_t arg3);
__attribute__((visibility("default"))) void* MCC_CallSlowNative4(void* func, uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4);
__attribute__((visibility("default"))) void* MCC_CallSlowNative5(void* func, uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4, uint64_t arg5);
__attribute__((visibility("default"))) void* MCC_CallSlowNative6(void* func, uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4, uint64_t arg5, uint64_t arg6);
__attribute__((visibility("default"))) void* MCC_CallSlowNative7(void* func, uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4, uint64_t arg5, uint64_t arg6, uint64_t arg7);
__attribute__((visibility("default"))) void* MCC_CallSlowNative8(void* func, uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4, uint64_t arg5, uint64_t arg6, uint64_t arg7, uint64_t arg8);
__attribute__((visibility("default"))) void* MCC_CallSlowNativeExt(void* func);
__attribute__((visibility("default"))) void MCC_RecordStaticField(address_t* field, const char *name);
__attribute__((visibility("default"))) void MCC_SyncExitFast(void* obj);
__attribute__((visibility("default"))) void MCC_SyncEnterFast2(void* obj);
__attribute__((visibility("default"))) void MCC_SyncEnterFast0(void* obj);
__attribute__((visibility("default"))) void MCC_SyncEnterFast1(void* obj);
__attribute__((visibility("default"))) void MCC_SyncEnterFast3(void* obj);
__attribute__((visibility("default"))) JNIEnv* MCC_PreNativeCall(jobject caller);
__attribute__((visibility("default"))) void MCC_PostNativeCall(JNIEnv* env);
__attribute__((visibility("default"))) int32_t MCC_JavaArrayLength(void* p);
__attribute__((visibility("default"))) void MCC_JavaArrayFill(void* d, void* s, int32_t len);
__attribute__((visibility("default"))) void* MCC_JavaCheckCast(void* i, void* c);
__attribute__((visibility("default"))) void* MCC_GetReferenceToClass(void* c);
__attribute__((visibility("default"))) bool MCC_JavaInstanceOf(void* i, void* c);
__attribute__((visibility("default"))) void MCC_JavaInterfaceCall(void* dummy);
__attribute__((visibility("default"))) jvalue MCC_JavaPolymorphicCall(void* calleeName,
    void* protoString, int paramNum, jobject methodHandle, ...);
__attribute__((visibility("default"))) void MCC_SetJavaClass(address_t objaddr, address_t klass);
__attribute__((visibility("default"))) address_t MCC_NewObj_fixed_class(address_t klass);
__attribute__((visibility("default"))) address_t MCC_Reflect_ThrowInstantiationError();
__attribute__((visibility("default"))) address_t MCC_NewObj_flexible_cname(size_t elem_size, size_t n_elems,
    const char *classNameOrclassObj, address_t caller_obj, unsigned long isClassObj);
__attribute__((visibility("default"))) address_t MCC_NewPermanentObject(address_t klass);
__attribute__((visibility("default"))) address_t MCC_NewPermanentArray(size_t elemSize, size_t nElems,
    const char *classNameOrclassObj, address_t callerObj, unsigned long isClassObj);
__attribute__((visibility("default"))) void MCC_CheckObjMem(void* obj);
__attribute__((visibility("default"))) jobject MCC_CannotFindNativeMethod(const char* signature);
__attribute__((visibility("default"))) jstring MCC_CannotFindNativeMethod_S(const char* signature);
__attribute__((visibility("default"))) jobject MCC_CannotFindNativeMethod_A(const char* signature);
__attribute__((visibility("default"))) void* MCC_FindNativeMethodPtr(uint64_t** regFuncTabAddr);
__attribute__((visibility("default"))) void* MCC_FindNativeMethodPtrWithoutException(uint64_t** regFuncTabAddr);
__attribute__((visibility("default"))) void* MCC_DummyNativeMethodPtr();
}
}
namespace maplert {
template<class ... Types>
__attribute__((always_inline)) inline void __MRT_DummyUse(Types ... args __attribute__((unused))) {
}
extern "C" {
address_t __MRT_chelper_newobj_0(size_t size);
address_t __MRT_chelper_newobj_flexible_0(size_t elem_size, size_t len);
address_t __MRT_chelper_newobj_flexible_1(size_t elem_size, size_t len, address_t klass);
void __MRT_SetJavaClass(address_t objaddr, address_t klass);
void __MRT_SetJavaArrayClass(address_t objaddr, address_t klass);
void MRT_Reflect_ThrowNegtiveArraySizeException();
}
}
using namespace std;
namespace maplert {
extern "C" {
extern void *__cinf_Ljava_2Flang_2FObject_3B;
extern void *__cinf_Ljava_2Flang_2Freflect_2FField_3B;
extern void *__cinf_Ljava_2Flang_2FCharSequence_3B;
extern void *__cinf_Ljava_2Flang_2FThreadLocal_24ThreadLocalMap_24Entry_3B;
extern void *__cinf_Ljava_2Futil_2FHashtable_24Entry_3B;
extern void *__cinf_Ljava_2Futil_2FFormatter_24Flags_3B;
extern void *__cinf_Ljava_2Futil_2FHashMap_24Node_3B;
extern void *__cinf_Ljava_2Futil_2FFormatter_24FormatString_3B;
extern void *__cinf_Ljava_2Flang_2FString_3B;
extern void *__cinf_Ljava_2Flang_2FClass_3B;
extern void *__cinf_Ljava_2Flang_2Freflect_2FMethod_3B;
extern void *__cinf_Ljava_2Flang_2Fannotation_2FAnnotation_3B;
extern void *__cinf_Ljava_2Flang_2Freflect_2FConstructor_3B;
extern void *__cinf_Ljava_2Flang_2Freflect_2FParameter_3B;
extern void (*Ljava_2Flang_2FObject_3B_7Cclone_7C_28_29Ljava_2Flang_2FObject_3B)();
extern void (*Ljava_2Flang_2FObject_3B_7Cequals_7C_28Ljava_2Flang_2FObject_3B_29Z)();
extern void (*Ljava_2Flang_2FObject_3B_7Cfinalize_7C_28_29V)();
extern void (*Ljava_2Flang_2FObject_3B_7CgetClass_7C_28_29Ljava_2Flang_2FClass_3B)();
extern void (*Ljava_2Flang_2FObject_3B_7ChashCode_7C_28_29I)();
extern void (*Ljava_2Flang_2FObject_3B_7Cnotify_7C_28_29V)();
extern void (*Ljava_2Flang_2FObject_3B_7CnotifyAll_7C_28_29V)();
extern void (*Ljava_2Flang_2FObject_3B_7CtoString_7C_28_29Ljava_2Flang_2FString_3B)();
extern void (*Ljava_2Flang_2FObject_3B_7Cwait_7C_28_29V)();
extern void (*Ljava_2Flang_2FObject_3B_7Cwait_7C_28J_29V)();
extern void (*Ljava_2Flang_2FObject_3B_7Cwait_7C_28JI_29V)();
extern void (*__itb_Ljava_2Flang_2FObject_3B)();
extern void (*__vtb_Ljava_2Flang_2FObject_3B)();
extern void *__fields__Ljava_2Flang_2FObject_3B;
extern void *__methods__Ljava_2Flang_2FObject_3B;
extern void *MCC_GCTIB__Ljava_2Flang_2FObject_3B;
ClassMetadataRO __primitiveclassinforo__Z = { ((char *)"Z"), 0, 0, {((ClassMetadata **)0xabcd)}, 0, 0, 0x0001, 0, 0, 0x00000411, 0}; ClassMetadata __attribute__((visibility("default"))) __pinf_Z = { ((uintptr_t)(&__cinf_Ljava_2Flang_2FClass_3B)), 0, 0, {1}, ((void *)0xabcd), ((void *)(&__vtb_Ljava_2Flang_2FObject_3B)), ((void *)((char*)(((void *)(&MCC_GCTIB__Ljava_2Flang_2FObject_3B))) - (char*)(&(__pinf_Z.gctib)))), (ClassMetadataRO*)(&__primitiveclassinforo__Z), {0} }; ClassMetadataRO __primitiveclassinforo__AZ = { ((char *)"[Z"), 0, 0, {((ClassMetadata **)(&__pinf_Z))}, 0, 0, 0x0002, 0, 0, 0x00000411, 0 }; ClassMetadata __attribute__((visibility("default"))) __pinf_AZ = { ((uintptr_t)(&__cinf_Ljava_2Flang_2FClass_3B)), 0, 0, {1}, 0, ((void *)(&__vtb_Ljava_2Flang_2FObject_3B)), ((void *)((char*)(((void *)(&MCC_GCTIB___ArrayOfPrimitive))) - (char*)(&(__pinf_AZ.gctib)))), (ClassMetadataRO*)(&__primitiveclassinforo__AZ), {0} }; ClassMetadataRO __primitiveclassinforo__AAZ = { ((char *)"[[Z"), 0, 0, {((ClassMetadata **)(&__pinf_AZ))}, 0, 0, 0x0002, 0, 0, 0x00000411, 0 }; ClassMetadata __attribute__((visibility("default"))) __pinf_AAZ = { ((uintptr_t)(&__cinf_Ljava_2Flang_2FClass_3B)), 0, 0, {8}, 0, ((void *)(&__vtb_Ljava_2Flang_2FObject_3B)), ((void *)((char*)(((void *)(&MCC_GCTIB___ArrayOfObject))) - (char*)(&(__pinf_AAZ.gctib)))), (ClassMetadataRO*)(&__primitiveclassinforo__AAZ), {0} }; ClassMetadataRO __primitiveclassinforo__AAAZ = { ((char *)"[[[Z"), 0, 0, {((ClassMetadata **)(&__pinf_AAZ))}, 0, 0, 0x0002, 0, 0, 0x00000411, 0 }; ClassMetadata __attribute__((visibility("default"))) __pinf_AAAZ = { ((uintptr_t)(&__cinf_Ljava_2Flang_2FClass_3B)), 0, 0, {8}, 0, ((void *)(&__vtb_Ljava_2Flang_2FObject_3B)), ((void *)((char*)(((void *)(&MCC_GCTIB___ArrayOfObject))) - (char*)(&(__pinf_AAAZ.gctib)))), (ClassMetadataRO*)(&__primitiveclassinforo__AAAZ), {0} }; ClassMetadata *__mrt_pclasses_Z[4] = { &__pinf_Z, &__pinf_AZ, &__pinf_AAZ, &__pinf_AAAZ };
ClassMetadataRO __primitiveclassinforo__B = { ((char *)"B"), 0, 0, {((ClassMetadata **)0xabcd)}, 0, 0, 0x0001, 0, 0, 0x00000411, 0}; ClassMetadata __attribute__((visibility("default"))) __pinf_B = { ((uintptr_t)(&__cinf_Ljava_2Flang_2FClass_3B)), 0, 0, {1}, ((void *)0xabcd), ((void *)(&__vtb_Ljava_2Flang_2FObject_3B)), ((void *)((char*)(((void *)(&MCC_GCTIB__Ljava_2Flang_2FObject_3B))) - (char*)(&(__pinf_B.gctib)))), (ClassMetadataRO*)(&__primitiveclassinforo__B), {0} }; ClassMetadataRO __primitiveclassinforo__AB = { ((char *)"[B"), 0, 0, {((ClassMetadata **)(&__pinf_B))}, 0, 0, 0x0002, 0, 0, 0x00000411, 0 }; ClassMetadata __attribute__((visibility("default"))) __pinf_AB = { ((uintptr_t)(&__cinf_Ljava_2Flang_2FClass_3B)), 0, 0, {1}, 0, ((void *)(&__vtb_Ljava_2Flang_2FObject_3B)), ((void *)((char*)(((void *)(&MCC_GCTIB___ArrayOfPrimitive))) - (char*)(&(__pinf_AB.gctib)))), (ClassMetadataRO*)(&__primitiveclassinforo__AB), {0} }; ClassMetadataRO __primitiveclassinforo__AAB = { ((char *)"[[B"), 0, 0, {((ClassMetadata **)(&__pinf_AB))}, 0, 0, 0x0002, 0, 0, 0x00000411, 0 }; ClassMetadata __attribute__((visibility("default"))) __pinf_AAB = { ((uintptr_t)(&__cinf_Ljava_2Flang_2FClass_3B)), 0, 0, {8}, 0, ((void *)(&__vtb_Ljava_2Flang_2FObject_3B)), ((void *)((char*)(((void *)(&MCC_GCTIB___ArrayOfObject))) - (char*)(&(__pinf_AAB.gctib)))), (ClassMetadataRO*)(&__primitiveclassinforo__AAB), {0} }; ClassMetadataRO __primitiveclassinforo__AAAB = { ((char *)"[[[B"), 0, 0, {((ClassMetadata **)(&__pinf_AAB))}, 0, 0, 0x0002, 0, 0, 0x00000411, 0 }; ClassMetadata __attribute__((visibility("default"))) __pinf_AAAB = { ((uintptr_t)(&__cinf_Ljava_2Flang_2FClass_3B)), 0, 0, {8}, 0, ((void *)(&__vtb_Ljava_2Flang_2FObject_3B)), ((void *)((char*)(((void *)(&MCC_GCTIB___ArrayOfObject))) - (char*)(&(__pinf_AAAB.gctib)))), (ClassMetadataRO*)(&__primitiveclassinforo__AAAB), {0} }; ClassMetadata *__mrt_pclasses_B[4] = { &__pinf_B, &__pinf_AB, &__pinf_AAB, &__pinf_AAAB };
ClassMetadataRO __primitiveclassinforo__S = { ((char *)"S"), 0, 0, {((ClassMetadata **)0xabcd)}, 0, 0, 0x0001, 0, 0, 0x00000411, 0}; ClassMetadata __attribute__((visibility("default"))) __pinf_S = { ((uintptr_t)(&__cinf_Ljava_2Flang_2FClass_3B)), 0, 0, {2}, ((void *)0xabcd), ((void *)(&__vtb_Ljava_2Flang_2FObject_3B)), ((void *)((char*)(((void *)(&MCC_GCTIB__Ljava_2Flang_2FObject_3B))) - (char*)(&(__pinf_S.gctib)))), (ClassMetadataRO*)(&__primitiveclassinforo__S), {0} }; ClassMetadataRO __primitiveclassinforo__AS = { ((char *)"[S"), 0, 0, {((ClassMetadata **)(&__pinf_S))}, 0, 0, 0x0002, 0, 0, 0x00000411, 0 }; ClassMetadata __attribute__((visibility("default"))) __pinf_AS = { ((uintptr_t)(&__cinf_Ljava_2Flang_2FClass_3B)), 0, 0, {2}, 0, ((void *)(&__vtb_Ljava_2Flang_2FObject_3B)), ((void *)((char*)(((void *)(&MCC_GCTIB___ArrayOfPrimitive))) - (char*)(&(__pinf_AS.gctib)))), (ClassMetadataRO*)(&__primitiveclassinforo__AS), {0} }; ClassMetadataRO __primitiveclassinforo__AAS = { ((char *)"[[S"), 0, 0, {((ClassMetadata **)(&__pinf_AS))}, 0, 0, 0x0002, 0, 0, 0x00000411, 0 }; ClassMetadata __attribute__((visibility("default"))) __pinf_AAS = { ((uintptr_t)(&__cinf_Ljava_2Flang_2FClass_3B)), 0, 0, {8}, 0, ((void *)(&__vtb_Ljava_2Flang_2FObject_3B)), ((void *)((char*)(((void *)(&MCC_GCTIB___ArrayOfObject))) - (char*)(&(__pinf_AAS.gctib)))), (ClassMetadataRO*)(&__primitiveclassinforo__AAS), {0} }; ClassMetadataRO __primitiveclassinforo__AAAS = { ((char *)"[[[S"), 0, 0, {((ClassMetadata **)(&__pinf_AAS))}, 0, 0, 0x0002, 0, 0, 0x00000411, 0 }; ClassMetadata __attribute__((visibility("default"))) __pinf_AAAS = { ((uintptr_t)(&__cinf_Ljava_2Flang_2FClass_3B)), 0, 0, {8}, 0, ((void *)(&__vtb_Ljava_2Flang_2FObject_3B)), ((void *)((char*)(((void *)(&MCC_GCTIB___ArrayOfObject))) - (char*)(&(__pinf_AAAS.gctib)))), (ClassMetadataRO*)(&__primitiveclassinforo__AAAS), {0} }; ClassMetadata *__mrt_pclasses_S[4] = { &__pinf_S, &__pinf_AS, &__pinf_AAS, &__pinf_AAAS };
ClassMetadataRO __primitiveclassinforo__C = { ((char *)"C"), 0, 0, {((ClassMetadata **)0xabcd)}, 0, 0, 0x0001, 0, 0, 0x00000411, 0}; ClassMetadata __attribute__((visibility("default"))) __pinf_C = { ((uintptr_t)(&__cinf_Ljava_2Flang_2FClass_3B)), 0, 0, {2}, ((void *)0xabcd), ((void *)(&__vtb_Ljava_2Flang_2FObject_3B)), ((void *)((char*)(((void *)(&MCC_GCTIB__Ljava_2Flang_2FObject_3B))) - (char*)(&(__pinf_C.gctib)))), (ClassMetadataRO*)(&__primitiveclassinforo__C), {0} }; ClassMetadataRO __primitiveclassinforo__AC = { ((char *)"[C"), 0, 0, {((ClassMetadata **)(&__pinf_C))}, 0, 0, 0x0002, 0, 0, 0x00000411, 0 }; ClassMetadata __attribute__((visibility("default"))) __pinf_AC = { ((uintptr_t)(&__cinf_Ljava_2Flang_2FClass_3B)), 0, 0, {2}, 0, ((void *)(&__vtb_Ljava_2Flang_2FObject_3B)), ((void *)((char*)(((void *)(&MCC_GCTIB___ArrayOfPrimitive))) - (char*)(&(__pinf_AC.gctib)))), (ClassMetadataRO*)(&__primitiveclassinforo__AC), {0} }; ClassMetadataRO __primitiveclassinforo__AAC = { ((char *)"[[C"), 0, 0, {((ClassMetadata **)(&__pinf_AC))}, 0, 0, 0x0002, 0, 0, 0x00000411, 0 }; ClassMetadata __attribute__((visibility("default"))) __pinf_AAC = { ((uintptr_t)(&__cinf_Ljava_2Flang_2FClass_3B)), 0, 0, {8}, 0, ((void *)(&__vtb_Ljava_2Flang_2FObject_3B)), ((void *)((char*)(((void *)(&MCC_GCTIB___ArrayOfObject))) - (char*)(&(__pinf_AAC.gctib)))), (ClassMetadataRO*)(&__primitiveclassinforo__AAC), {0} }; ClassMetadataRO __primitiveclassinforo__AAAC = { ((char *)"[[[C"), 0, 0, {((ClassMetadata **)(&__pinf_AAC))}, 0, 0, 0x0002, 0, 0, 0x00000411, 0 }; ClassMetadata __attribute__((visibility("default"))) __pinf_AAAC = { ((uintptr_t)(&__cinf_Ljava_2Flang_2FClass_3B)), 0, 0, {8}, 0, ((void *)(&__vtb_Ljava_2Flang_2FObject_3B)), ((void *)((char*)(((void *)(&MCC_GCTIB___ArrayOfObject))) - (char*)(&(__pinf_AAAC.gctib)))), (ClassMetadataRO*)(&__primitiveclassinforo__AAAC), {0} }; ClassMetadata *__mrt_pclasses_C[4] = { &__pinf_C, &__pinf_AC, &__pinf_AAC, &__pinf_AAAC };
ClassMetadataRO __primitiveclassinforo__I = { ((char *)"I"), 0, 0, {((ClassMetadata **)0xabcd)}, 0, 0, 0x0001, 0, 0, 0x00000411, 0}; ClassMetadata __attribute__((visibility("default"))) __pinf_I = { ((uintptr_t)(&__cinf_Ljava_2Flang_2FClass_3B)), 0, 0, {4}, ((void *)0xabcd), ((void *)(&__vtb_Ljava_2Flang_2FObject_3B)), ((void *)((char*)(((void *)(&MCC_GCTIB__Ljava_2Flang_2FObject_3B))) - (char*)(&(__pinf_I.gctib)))), (ClassMetadataRO*)(&__primitiveclassinforo__I), {0} }; ClassMetadataRO __primitiveclassinforo__AI = { ((char *)"[I"), 0, 0, {((ClassMetadata **)(&__pinf_I))}, 0, 0, 0x0002, 0, 0, 0x00000411, 0 }; ClassMetadata __attribute__((visibility("default"))) __pinf_AI = { ((uintptr_t)(&__cinf_Ljava_2Flang_2FClass_3B)), 0, 0, {4}, 0, ((void *)(&__vtb_Ljava_2Flang_2FObject_3B)), ((void *)((char*)(((void *)(&MCC_GCTIB___ArrayOfPrimitive))) - (char*)(&(__pinf_AI.gctib)))), (ClassMetadataRO*)(&__primitiveclassinforo__AI), {0} }; ClassMetadataRO __primitiveclassinforo__AAI = { ((char *)"[[I"), 0, 0, {((ClassMetadata **)(&__pinf_AI))}, 0, 0, 0x0002, 0, 0, 0x00000411, 0 }; ClassMetadata __attribute__((visibility("default"))) __pinf_AAI = { ((uintptr_t)(&__cinf_Ljava_2Flang_2FClass_3B)), 0, 0, {8}, 0, ((void *)(&__vtb_Ljava_2Flang_2FObject_3B)), ((void *)((char*)(((void *)(&MCC_GCTIB___ArrayOfObject))) - (char*)(&(__pinf_AAI.gctib)))), (ClassMetadataRO*)(&__primitiveclassinforo__AAI), {0} }; ClassMetadataRO __primitiveclassinforo__AAAI = { ((char *)"[[[I"), 0, 0, {((ClassMetadata **)(&__pinf_AAI))}, 0, 0, 0x0002, 0, 0, 0x00000411, 0 }; ClassMetadata __attribute__((visibility("default"))) __pinf_AAAI = { ((uintptr_t)(&__cinf_Ljava_2Flang_2FClass_3B)), 0, 0, {8}, 0, ((void *)(&__vtb_Ljava_2Flang_2FObject_3B)), ((void *)((char*)(((void *)(&MCC_GCTIB___ArrayOfObject))) - (char*)(&(__pinf_AAAI.gctib)))), (ClassMetadataRO*)(&__primitiveclassinforo__AAAI), {0} }; ClassMetadata *__mrt_pclasses_I[4] = { &__pinf_I, &__pinf_AI, &__pinf_AAI, &__pinf_AAAI };
ClassMetadataRO __primitiveclassinforo__F = { ((char *)"F"), 0, 0, {((ClassMetadata **)0xabcd)}, 0, 0, 0x0001, 0, 0, 0x00000411, 0}; ClassMetadata __attribute__((visibility("default"))) __pinf_F = { ((uintptr_t)(&__cinf_Ljava_2Flang_2FClass_3B)), 0, 0, {4}, ((void *)0xabcd), ((void *)(&__vtb_Ljava_2Flang_2FObject_3B)), ((void *)((char*)(((void *)(&MCC_GCTIB__Ljava_2Flang_2FObject_3B))) - (char*)(&(__pinf_F.gctib)))), (ClassMetadataRO*)(&__primitiveclassinforo__F), {0} }; ClassMetadataRO __primitiveclassinforo__AF = { ((char *)"[F"), 0, 0, {((ClassMetadata **)(&__pinf_F))}, 0, 0, 0x0002, 0, 0, 0x00000411, 0 }; ClassMetadata __attribute__((visibility("default"))) __pinf_AF = { ((uintptr_t)(&__cinf_Ljava_2Flang_2FClass_3B)), 0, 0, {4}, 0, ((void *)(&__vtb_Ljava_2Flang_2FObject_3B)), ((void *)((char*)(((void *)(&MCC_GCTIB___ArrayOfPrimitive))) - (char*)(&(__pinf_AF.gctib)))), (ClassMetadataRO*)(&__primitiveclassinforo__AF), {0} }; ClassMetadataRO __primitiveclassinforo__AAF = { ((char *)"[[F"), 0, 0, {((ClassMetadata **)(&__pinf_AF))}, 0, 0, 0x0002, 0, 0, 0x00000411, 0 }; ClassMetadata __attribute__((visibility("default"))) __pinf_AAF = { ((uintptr_t)(&__cinf_Ljava_2Flang_2FClass_3B)), 0, 0, {8}, 0, ((void *)(&__vtb_Ljava_2Flang_2FObject_3B)), ((void *)((char*)(((void *)(&MCC_GCTIB___ArrayOfObject))) - (char*)(&(__pinf_AAF.gctib)))), (ClassMetadataRO*)(&__primitiveclassinforo__AAF), {0} }; ClassMetadataRO __primitiveclassinforo__AAAF = { ((char *)"[[[F"), 0, 0, {((ClassMetadata **)(&__pinf_AAF))}, 0, 0, 0x0002, 0, 0, 0x00000411, 0 }; ClassMetadata __attribute__((visibility("default"))) __pinf_AAAF = { ((uintptr_t)(&__cinf_Ljava_2Flang_2FClass_3B)), 0, 0, {8}, 0, ((void *)(&__vtb_Ljava_2Flang_2FObject_3B)), ((void *)((char*)(((void *)(&MCC_GCTIB___ArrayOfObject))) - (char*)(&(__pinf_AAAF.gctib)))), (ClassMetadataRO*)(&__primitiveclassinforo__AAAF), {0} }; ClassMetadata *__mrt_pclasses_F[4] = { &__pinf_F, &__pinf_AF, &__pinf_AAF, &__pinf_AAAF };
ClassMetadataRO __primitiveclassinforo__D = { ((char *)"D"), 0, 0, {((ClassMetadata **)0xabcd)}, 0, 0, 0x0001, 0, 0, 0x00000411, 0}; ClassMetadata __attribute__((visibility("default"))) __pinf_D = { ((uintptr_t)(&__cinf_Ljava_2Flang_2FClass_3B)), 0, 0, {8}, ((void *)0xabcd), ((void *)(&__vtb_Ljava_2Flang_2FObject_3B)), ((void *)((char*)(((void *)(&MCC_GCTIB__Ljava_2Flang_2FObject_3B))) - (char*)(&(__pinf_D.gctib)))), (ClassMetadataRO*)(&__primitiveclassinforo__D), {0} }; ClassMetadataRO __primitiveclassinforo__AD = { ((char *)"[D"), 0, 0, {((ClassMetadata **)(&__pinf_D))}, 0, 0, 0x0002, 0, 0, 0x00000411, 0 }; ClassMetadata __attribute__((visibility("default"))) __pinf_AD = { ((uintptr_t)(&__cinf_Ljava_2Flang_2FClass_3B)), 0, 0, {8}, 0, ((void *)(&__vtb_Ljava_2Flang_2FObject_3B)), ((void *)((char*)(((void *)(&MCC_GCTIB___ArrayOfPrimitive))) - (char*)(&(__pinf_AD.gctib)))), (ClassMetadataRO*)(&__primitiveclassinforo__AD), {0} }; ClassMetadataRO __primitiveclassinforo__AAD = { ((char *)"[[D"), 0, 0, {((ClassMetadata **)(&__pinf_AD))}, 0, 0, 0x0002, 0, 0, 0x00000411, 0 }; ClassMetadata __attribute__((visibility("default"))) __pinf_AAD = { ((uintptr_t)(&__cinf_Ljava_2Flang_2FClass_3B)), 0, 0, {8}, 0, ((void *)(&__vtb_Ljava_2Flang_2FObject_3B)), ((void *)((char*)(((void *)(&MCC_GCTIB___ArrayOfObject))) - (char*)(&(__pinf_AAD.gctib)))), (ClassMetadataRO*)(&__primitiveclassinforo__AAD), {0} }; ClassMetadataRO __primitiveclassinforo__AAAD = { ((char *)"[[[D"), 0, 0, {((ClassMetadata **)(&__pinf_AAD))}, 0, 0, 0x0002, 0, 0, 0x00000411, 0 }; ClassMetadata __attribute__((visibility("default"))) __pinf_AAAD = { ((uintptr_t)(&__cinf_Ljava_2Flang_2FClass_3B)), 0, 0, {8}, 0, ((void *)(&__vtb_Ljava_2Flang_2FObject_3B)), ((void *)((char*)(((void *)(&MCC_GCTIB___ArrayOfObject))) - (char*)(&(__pinf_AAAD.gctib)))), (ClassMetadataRO*)(&__primitiveclassinforo__AAAD), {0} }; ClassMetadata *__mrt_pclasses_D[4] = { &__pinf_D, &__pinf_AD, &__pinf_AAD, &__pinf_AAAD };
ClassMetadataRO __primitiveclassinforo__J = { ((char *)"J"), 0, 0, {((ClassMetadata **)0xabcd)}, 0, 0, 0x0001, 0, 0, 0x00000411, 0}; ClassMetadata __attribute__((visibility("default"))) __pinf_J = { ((uintptr_t)(&__cinf_Ljava_2Flang_2FClass_3B)), 0, 0, {8}, ((void *)0xabcd), ((void *)(&__vtb_Ljava_2Flang_2FObject_3B)), ((void *)((char*)(((void *)(&MCC_GCTIB__Ljava_2Flang_2FObject_3B))) - (char*)(&(__pinf_J.gctib)))), (ClassMetadataRO*)(&__primitiveclassinforo__J), {0} }; ClassMetadataRO __primitiveclassinforo__AJ = { ((char *)"[J"), 0, 0, {((ClassMetadata **)(&__pinf_J))}, 0, 0, 0x0002, 0, 0, 0x00000411, 0 }; ClassMetadata __attribute__((visibility("default"))) __pinf_AJ = { ((uintptr_t)(&__cinf_Ljava_2Flang_2FClass_3B)), 0, 0, {8}, 0, ((void *)(&__vtb_Ljava_2Flang_2FObject_3B)), ((void *)((char*)(((void *)(&MCC_GCTIB___ArrayOfPrimitive))) - (char*)(&(__pinf_AJ.gctib)))), (ClassMetadataRO*)(&__primitiveclassinforo__AJ), {0} }; ClassMetadataRO __primitiveclassinforo__AAJ = { ((char *)"[[J"), 0, 0, {((ClassMetadata **)(&__pinf_AJ))}, 0, 0, 0x0002, 0, 0, 0x00000411, 0 }; ClassMetadata __attribute__((visibility("default"))) __pinf_AAJ = { ((uintptr_t)(&__cinf_Ljava_2Flang_2FClass_3B)), 0, 0, {8}, 0, ((void *)(&__vtb_Ljava_2Flang_2FObject_3B)), ((void *)((char*)(((void *)(&MCC_GCTIB___ArrayOfObject))) - (char*)(&(__pinf_AAJ.gctib)))), (ClassMetadataRO*)(&__primitiveclassinforo__AAJ), {0} }; ClassMetadataRO __primitiveclassinforo__AAAJ = { ((char *)"[[[J"), 0, 0, {((ClassMetadata **)(&__pinf_AAJ))}, 0, 0, 0x0002, 0, 0, 0x00000411, 0 }; ClassMetadata __attribute__((visibility("default"))) __pinf_AAAJ = { ((uintptr_t)(&__cinf_Ljava_2Flang_2FClass_3B)), 0, 0, {8}, 0, ((void *)(&__vtb_Ljava_2Flang_2FObject_3B)), ((void *)((char*)(((void *)(&MCC_GCTIB___ArrayOfObject))) - (char*)(&(__pinf_AAAJ.gctib)))), (ClassMetadataRO*)(&__primitiveclassinforo__AAAJ), {0} }; ClassMetadata *__mrt_pclasses_J[4] = { &__pinf_J, &__pinf_AJ, &__pinf_AAJ, &__pinf_AAAJ };
ClassMetadataRO __primitiveclassinforo__V = { ((char *)"V"), 0, 0, {((ClassMetadata **)0xabcd)}, 0, 0, 0x0001, 0, 0, 0x00000411, 0}; ClassMetadata __attribute__((visibility("default"))) __pinf_V = { ((uintptr_t)(&__cinf_Ljava_2Flang_2FClass_3B)), 0, 0, {0}, ((void *)0xabcd), ((void *)(&__vtb_Ljava_2Flang_2FObject_3B)), ((void *)((char*)(((void *)(&MCC_GCTIB__Ljava_2Flang_2FObject_3B))) - (char*)(&(__pinf_V.gctib)))), (ClassMetadataRO*)(&__primitiveclassinforo__V), {0} }; ClassMetadataRO __primitiveclassinforo__AV = { ((char *)"[V"), 0, 0, {((ClassMetadata **)(&__pinf_V))}, 0, 0, 0x0002, 0, 0, 0x00000411, 0 }; ClassMetadata __attribute__((visibility("default"))) __pinf_AV = { ((uintptr_t)(&__cinf_Ljava_2Flang_2FClass_3B)), 0, 0, {0}, 0, ((void *)(&__vtb_Ljava_2Flang_2FObject_3B)), ((void *)((char*)(((void *)(&MCC_GCTIB___ArrayOfPrimitive))) - (char*)(&(__pinf_AV.gctib)))), (ClassMetadataRO*)(&__primitiveclassinforo__AV), {0} }; ClassMetadataRO __primitiveclassinforo__AAV = { ((char *)"[[V"), 0, 0, {((ClassMetadata **)(&__pinf_AV))}, 0, 0, 0x0002, 0, 0, 0x00000411, 0 }; ClassMetadata __attribute__((visibility("default"))) __pinf_AAV = { ((uintptr_t)(&__cinf_Ljava_2Flang_2FClass_3B)), 0, 0, {8}, 0, ((void *)(&__vtb_Ljava_2Flang_2FObject_3B)), ((void *)((char*)(((void *)(&MCC_GCTIB___ArrayOfObject))) - (char*)(&(__pinf_AAV.gctib)))), (ClassMetadataRO*)(&__primitiveclassinforo__AAV), {0} }; ClassMetadataRO __primitiveclassinforo__AAAV = { ((char *)"[[[V"), 0, 0, {((ClassMetadata **)(&__pinf_AAV))}, 0, 0, 0x0002, 0, 0, 0x00000411, 0 }; ClassMetadata __attribute__((visibility("default"))) __pinf_AAAV = { ((uintptr_t)(&__cinf_Ljava_2Flang_2FClass_3B)), 0, 0, {8}, 0, ((void *)(&__vtb_Ljava_2Flang_2FObject_3B)), ((void *)((char*)(((void *)(&MCC_GCTIB___ArrayOfObject))) - (char*)(&(__pinf_AAAV.gctib)))), (ClassMetadataRO*)(&__primitiveclassinforo__AAAV), {0} }; ClassMetadata *__mrt_pclasses_V[4] = { &__pinf_V, &__pinf_AV, &__pinf_AAV, &__pinf_AAAV };
ClassMetadataRO __cinfro__ALjava_2Flang_2FObject_3B = { ((char *)"[Ljava/lang/Object;"), 0, 0, {((ClassMetadata **)(&__cinf_Ljava_2Flang_2FObject_3B))}, 0, 0, 0x0002, 0, 0, 0x00000411, 0}; ClassMetadata __attribute__((visibility("default"))) __cinf_ALjava_2Flang_2FObject_3B = { ((uintptr_t)(&__cinf_Ljava_2Flang_2FClass_3B)), 0, 0, {8}, 0, ((void *)(&__vtb_Ljava_2Flang_2FObject_3B)), ((void *)((char*)(((void *)(&MCC_GCTIB___ArrayOfObject))) - (char*)(&(__cinf_ALjava_2Flang_2FObject_3B.gctib)))), (ClassMetadataRO*)(&__cinfro__ALjava_2Flang_2FObject_3B), {0} };
ClassMetadataRO __cinfro__ALjava_2Flang_2FClass_3B = { ((char *)"[Ljava/lang/Class;"), 0, 0, {((ClassMetadata **)(&__cinf_Ljava_2Flang_2FClass_3B))}, 0, 0, 0x0002, 0, 0, 0x00000411, 0}; ClassMetadata __attribute__((visibility("default"))) __cinf_ALjava_2Flang_2FClass_3B = { ((uintptr_t)(&__cinf_Ljava_2Flang_2FClass_3B)), 0, 0, {8}, 0, ((void *)(&__vtb_Ljava_2Flang_2FObject_3B)), ((void *)((char*)(((void *)(&MCC_GCTIB___ArrayOfObject))) - (char*)(&(__cinf_ALjava_2Flang_2FClass_3B.gctib)))), (ClassMetadataRO*)(&__cinfro__ALjava_2Flang_2FClass_3B), {0} };
ClassMetadataRO __cinfro__ALjava_2Flang_2FString_3B = { ((char *)"[Ljava/lang/String;"), 0, 0, {((ClassMetadata **)(&__cinf_Ljava_2Flang_2FString_3B))}, 0, 0, 0x0002, 0, 0, 0x00000411, 0}; ClassMetadata __attribute__((visibility("default"))) __cinf_ALjava_2Flang_2FString_3B = { ((uintptr_t)(&__cinf_Ljava_2Flang_2FClass_3B)), 0, 0, {8}, 0, ((void *)(&__vtb_Ljava_2Flang_2FObject_3B)), ((void *)((char*)(((void *)(&MCC_GCTIB___ArrayOfObject))) - (char*)(&(__cinf_ALjava_2Flang_2FString_3B.gctib)))), (ClassMetadataRO*)(&__cinfro__ALjava_2Flang_2FString_3B), {0} };
ClassMetadataRO __cinfro__ALjava_2Futil_2FFormatter_24Flags_3B = { ((char *)"[Ljava/util/Formatter$Flags;"), 0, 0, {((ClassMetadata **)(&__cinf_Ljava_2Futil_2FFormatter_24Flags_3B))}, 0, 0, 0x0002, 0, 0, 0x00000411, 0}; ClassMetadata __attribute__((visibility("default"))) __cinf_ALjava_2Futil_2FFormatter_24Flags_3B = { ((uintptr_t)(&__cinf_Ljava_2Flang_2FClass_3B)), 0, 0, {8}, 0, ((void *)(&__vtb_Ljava_2Flang_2FObject_3B)), ((void *)((char*)(((void *)(&MCC_GCTIB___ArrayOfObject))) - (char*)(&(__cinf_ALjava_2Futil_2FFormatter_24Flags_3B.gctib)))), (ClassMetadataRO*)(&__cinfro__ALjava_2Futil_2FFormatter_24Flags_3B), {0} };
ClassMetadataRO __cinfro__ALjava_2Futil_2FHashMap_24Node_3B = { ((char *)"[Ljava/util/HashMap$Node;"), 0, 0, {((ClassMetadata **)(&__cinf_Ljava_2Futil_2FHashMap_24Node_3B))}, 0, 0, 0x0002, 0, 0, 0x00000411, 0}; ClassMetadata __attribute__((visibility("default"))) __cinf_ALjava_2Futil_2FHashMap_24Node_3B = { ((uintptr_t)(&__cinf_Ljava_2Flang_2FClass_3B)), 0, 0, {8}, 0, ((void *)(&__vtb_Ljava_2Flang_2FObject_3B)), ((void *)((char*)(((void *)(&MCC_GCTIB___ArrayOfObject))) - (char*)(&(__cinf_ALjava_2Futil_2FHashMap_24Node_3B.gctib)))), (ClassMetadataRO*)(&__cinfro__ALjava_2Futil_2FHashMap_24Node_3B), {0} };
ClassMetadataRO __cinfro__ALjava_2Futil_2FFormatter_24FormatString_3B = { ((char *)"[Ljava/util/Formatter$FormatString;"), 0, 0, {((ClassMetadata **)(&__cinf_Ljava_2Futil_2FFormatter_24FormatString_3B))}, 0, 0, 0x0002, 0, 0, 0x00000411, 0}; ClassMetadata __attribute__((visibility("default"))) __cinf_ALjava_2Futil_2FFormatter_24FormatString_3B = { ((uintptr_t)(&__cinf_Ljava_2Flang_2FClass_3B)), 0, 0, {8}, 0, ((void *)(&__vtb_Ljava_2Flang_2FObject_3B)), ((void *)((char*)(((void *)(&MCC_GCTIB___ArrayOfObject))) - (char*)(&(__cinf_ALjava_2Futil_2FFormatter_24FormatString_3B.gctib)))), (ClassMetadataRO*)(&__cinfro__ALjava_2Futil_2FFormatter_24FormatString_3B), {0} };
ClassMetadataRO __cinfro__ALjava_2Flang_2FCharSequence_3B = { ((char *)"[Ljava/lang/CharSequence;"), 0, 0, {((ClassMetadata **)(&__cinf_Ljava_2Flang_2FCharSequence_3B))}, 0, 0, 0x0002, 0, 0, 0x00000411, 0}; ClassMetadata __attribute__((visibility("default"))) __cinf_ALjava_2Flang_2FCharSequence_3B = { ((uintptr_t)(&__cinf_Ljava_2Flang_2FClass_3B)), 0, 0, {8}, 0, ((void *)(&__vtb_Ljava_2Flang_2FObject_3B)), ((void *)((char*)(((void *)(&MCC_GCTIB___ArrayOfObject))) - (char*)(&(__cinf_ALjava_2Flang_2FCharSequence_3B.gctib)))), (ClassMetadataRO*)(&__cinfro__ALjava_2Flang_2FCharSequence_3B), {0} };
ClassMetadataRO __cinfro__ALjava_2Flang_2FThreadLocal_24ThreadLocalMap_24Entry_3B = { ((char *)"[Ljava/lang/ThreadLocal$ThreadLocalMap$Entry;"), 0, 0, {((ClassMetadata **)(&__cinf_Ljava_2Flang_2FThreadLocal_24ThreadLocalMap_24Entry_3B))}, 0, 0, 0x0002, 0, 0, 0x00000411, 0}; ClassMetadata __attribute__((visibility("default"))) __cinf_ALjava_2Flang_2FThreadLocal_24ThreadLocalMap_24Entry_3B = { ((uintptr_t)(&__cinf_Ljava_2Flang_2FClass_3B)), 0, 0, {8}, 0, ((void *)(&__vtb_Ljava_2Flang_2FObject_3B)), ((void *)((char*)(((void *)(&MCC_GCTIB___ArrayOfObject))) - (char*)(&(__cinf_ALjava_2Flang_2FThreadLocal_24ThreadLocalMap_24Entry_3B.gctib)))), (ClassMetadataRO*)(&__cinfro__ALjava_2Flang_2FThreadLocal_24ThreadLocalMap_24Entry_3B), {0} };
ClassMetadataRO __cinfro__ALjava_2Futil_2FHashtable_24Entry_3B = { ((char *)"[Ljava/util/Hashtable$Entry;"), 0, 0, {((ClassMetadata **)(&__cinf_Ljava_2Futil_2FHashtable_24Entry_3B))}, 0, 0, 0x0002, 0, 0, 0x00000411, 0}; ClassMetadata __attribute__((visibility("default"))) __cinf_ALjava_2Futil_2FHashtable_24Entry_3B = { ((uintptr_t)(&__cinf_Ljava_2Flang_2FClass_3B)), 0, 0, {8}, 0, ((void *)(&__vtb_Ljava_2Flang_2FObject_3B)), ((void *)((char*)(((void *)(&MCC_GCTIB___ArrayOfObject))) - (char*)(&(__cinf_ALjava_2Futil_2FHashtable_24Entry_3B.gctib)))), (ClassMetadataRO*)(&__cinfro__ALjava_2Futil_2FHashtable_24Entry_3B), {0} };
}
extern "C" {
extern void* Ljava_2Fio_2FUnixFileSystem_3B_7C_3Cinit_3E_7C_28_29V;
extern void* __cinf_Ljava_2Flang_2Fref_2FWeakReference_3B;
void* gsym__vtb_Ljava_2Flang_2FObject_3B = &__vtb_Ljava_2Flang_2FObject_3B;
void* gsym__cinf_Ljava_2Flang_2FString_3B = &__cinf_Ljava_2Flang_2FString_3B;
void* gsym__cinf_Ljava_2Flang_2FClass_3B = &__cinf_Ljava_2Flang_2FClass_3B;
void* gsym__cinf_Ljava_2Flang_2Fref_2FWeakReference_3B = &__cinf_Ljava_2Flang_2Fref_2FWeakReference_3B;
void* gsym_Ljava_2Fio_2FUnixFileSystem_3B_7C_3Cinit_3E_7C_28_29V = &Ljava_2Fio_2FUnixFileSystem_3B_7C_3Cinit_3E_7C_28_29V;
};
extern "C" jobject Ljava_2Flang_2FClass_3B_7CgetAnnotation_7C_28Ljava_2Flang_2FClass_3B_29Ljava_2Flang_2Fannotation_2FAnnotation_3B_ (jobject jThis) ;
extern "C" jobject Ljava_2Flang_2FClass_3B_7CgetAnnotation_7C_28Ljava_2Flang_2FClass_3B_29Ljava_2Flang_2Fannotation_2FAnnotation_3B (jobject jThis) {
return Ljava_2Flang_2FClass_3B_7CgetAnnotation_7C_28Ljava_2Flang_2FClass_3B_29Ljava_2Flang_2Fannotation_2FAnnotation_3B_(jThis);
}
extern "C" jobject Ljava_2Flang_2FClass_3B_7CgetClasses_7C_28_29ALjava_2Flang_2FClass_3B_(jobject javaThis) ;
extern "C" jobject Ljava_2Flang_2FClass_3B_7CgetClasses_7C_28_29ALjava_2Flang_2FClass_3B(jobject javaThis) {
return Ljava_2Flang_2FClass_3B_7CgetClasses_7C_28_29ALjava_2Flang_2FClass_3B_(javaThis);
}
extern "C" jobject Ljava_2Flang_2FClass_3B_7CgetClassLoader_7C_28_29Ljava_2Flang_2FClassLoader_3B_ (jobject javaThis) ;
extern "C" jobject Ljava_2Flang_2FClass_3B_7CgetClassLoader_7C_28_29Ljava_2Flang_2FClassLoader_3B (jobject javaThis) {
return Ljava_2Flang_2FClass_3B_7CgetClassLoader_7C_28_29Ljava_2Flang_2FClassLoader_3B_(javaThis);
}
extern "C" jclass Ljava_2Flang_2FClass_3B_7CgetComponentType_7C_28_29Ljava_2Flang_2FClass_3B_(jobject javaThis) ;
extern "C" jclass Ljava_2Flang_2FClass_3B_7CgetComponentType_7C_28_29Ljava_2Flang_2FClass_3B(jobject javaThis) {
return Ljava_2Flang_2FClass_3B_7CgetComponentType_7C_28_29Ljava_2Flang_2FClass_3B_(javaThis);
}
extern "C" jclass Ljava_2Flang_2FClass_3B_7CgetConstructor_7C_28ALjava_2Flang_2FClass_3B_29Ljava_2Flang_2Freflect_2FConstructor_3B_ (jobject Class_arg0, jobject AClass_arg1) ;
extern "C" jclass Ljava_2Flang_2FClass_3B_7CgetConstructor_7C_28ALjava_2Flang_2FClass_3B_29Ljava_2Flang_2Freflect_2FConstructor_3B (jobject Class_arg0, jobject AClass_arg1) {
return Ljava_2Flang_2FClass_3B_7CgetConstructor_7C_28ALjava_2Flang_2FClass_3B_29Ljava_2Flang_2Freflect_2FConstructor_3B_(Class_arg0, AClass_arg1);
}
extern "C" jobject Ljava_2Flang_2FClass_3B_7CgetConstructors_7C_28_29ALjava_2Flang_2Freflect_2FConstructor_3B_ (jobject javaThis) ;
extern "C" jobject Ljava_2Flang_2FClass_3B_7CgetConstructors_7C_28_29ALjava_2Flang_2Freflect_2FConstructor_3B (jobject javaThis) {
return Ljava_2Flang_2FClass_3B_7CgetConstructors_7C_28_29ALjava_2Flang_2Freflect_2FConstructor_3B_(javaThis);
}
extern "C" jobject Ljava_2Flang_2FClass_3B_7CgetDeclaredAnnotation_7C_28Ljava_2Flang_2FClass_3B_29Ljava_2Flang_2Fannotation_2FAnnotation_3B_(jobject javaThis, jobject annotationClass) ;
extern "C" jobject Ljava_2Flang_2FClass_3B_7CgetDeclaredAnnotation_7C_28Ljava_2Flang_2FClass_3B_29Ljava_2Flang_2Fannotation_2FAnnotation_3B(jobject javaThis, jobject annotationClass) {
return Ljava_2Flang_2FClass_3B_7CgetDeclaredAnnotation_7C_28Ljava_2Flang_2FClass_3B_29Ljava_2Flang_2Fannotation_2FAnnotation_3B_(javaThis, annotationClass);
}
extern "C" jobject Ljava_2Flang_2FClass_3B_7CgetDeclaredAnnotations_7C_28_29ALjava_2Flang_2Fannotation_2FAnnotation_3B_(jobject javaThis) ;
extern "C" jobject Ljava_2Flang_2FClass_3B_7CgetDeclaredAnnotations_7C_28_29ALjava_2Flang_2Fannotation_2FAnnotation_3B(jobject javaThis) {
return Ljava_2Flang_2FClass_3B_7CgetDeclaredAnnotations_7C_28_29ALjava_2Flang_2Fannotation_2FAnnotation_3B_(javaThis);
}
extern "C" jobject Ljava_2Flang_2FClass_3B_7CgetDeclaredClasses_7C_28_29ALjava_2Flang_2FClass_3B_(jobject javaThis) ;
extern "C" jobject Ljava_2Flang_2FClass_3B_7CgetDeclaredClasses_7C_28_29ALjava_2Flang_2FClass_3B(jobject javaThis) {
return Ljava_2Flang_2FClass_3B_7CgetDeclaredClasses_7C_28_29ALjava_2Flang_2FClass_3B_(javaThis);
}
extern "C" jclass Ljava_2Flang_2FClass_3B_7CgetDeclaredConstructor_7C_28ALjava_2Flang_2FClass_3B_29Ljava_2Flang_2Freflect_2FConstructor_3B_ (jobject javaThis, jobject parameterTypes) ;
extern "C" jclass Ljava_2Flang_2FClass_3B_7CgetDeclaredConstructor_7C_28ALjava_2Flang_2FClass_3B_29Ljava_2Flang_2Freflect_2FConstructor_3B (jobject javaThis, jobject parameterTypes) {
return Ljava_2Flang_2FClass_3B_7CgetDeclaredConstructor_7C_28ALjava_2Flang_2FClass_3B_29Ljava_2Flang_2Freflect_2FConstructor_3B_(javaThis, parameterTypes);
}
extern "C" jobject Ljava_2Flang_2FClass_3B_7CgetDeclaredConstructors_7C_28_29ALjava_2Flang_2Freflect_2FConstructor_3B_ (jobject javaThis) ;
extern "C" jobject Ljava_2Flang_2FClass_3B_7CgetDeclaredConstructors_7C_28_29ALjava_2Flang_2Freflect_2FConstructor_3B (jobject javaThis) {
return Ljava_2Flang_2FClass_3B_7CgetDeclaredConstructors_7C_28_29ALjava_2Flang_2Freflect_2FConstructor_3B_(javaThis);
}
extern "C" jobject Ljava_2Flang_2FClass_3B_7CgetDeclaredField_7C_28Ljava_2Flang_2FString_3B_29Ljava_2Flang_2Freflect_2FField_3B_(jobject javaThis, jstring jStr) ;
extern "C" jobject Ljava_2Flang_2FClass_3B_7CgetDeclaredField_7C_28Ljava_2Flang_2FString_3B_29Ljava_2Flang_2Freflect_2FField_3B(jobject javaThis, jstring jStr) {
return Ljava_2Flang_2FClass_3B_7CgetDeclaredField_7C_28Ljava_2Flang_2FString_3B_29Ljava_2Flang_2Freflect_2FField_3B_(javaThis, jStr);
}
extern "C" jobject Ljava_2Flang_2FClass_3B_7CgetDeclaredFields_7C_28_29ALjava_2Flang_2Freflect_2FField_3B_(jobject javaThis) ;
extern "C" jobject Ljava_2Flang_2FClass_3B_7CgetDeclaredFields_7C_28_29ALjava_2Flang_2Freflect_2FField_3B(jobject javaThis) {
return Ljava_2Flang_2FClass_3B_7CgetDeclaredFields_7C_28_29ALjava_2Flang_2Freflect_2FField_3B_(javaThis);
}
extern "C" jclass Ljava_2Flang_2FClass_3B_7CgetDeclaredMethod_7C_28Ljava_2Flang_2FString_3BALjava_2Flang_2FClass_3B_29Ljava_2Flang_2Freflect_2FMethod_3B_ (jobject javaThis, jstring name, jobject parameterTypes) ;
extern "C" jclass Ljava_2Flang_2FClass_3B_7CgetDeclaredMethod_7C_28Ljava_2Flang_2FString_3BALjava_2Flang_2FClass_3B_29Ljava_2Flang_2Freflect_2FMethod_3B (jobject javaThis, jstring name, jobject parameterTypes) {
return Ljava_2Flang_2FClass_3B_7CgetDeclaredMethod_7C_28Ljava_2Flang_2FString_3BALjava_2Flang_2FClass_3B_29Ljava_2Flang_2Freflect_2FMethod_3B_(javaThis, name, parameterTypes);
}
extern "C" jobject Ljava_2Flang_2FClass_3B_7CgetDeclaredMethods_7C_28_29ALjava_2Flang_2Freflect_2FMethod_3B_ (jobject javaThis) ;
extern "C" jobject Ljava_2Flang_2FClass_3B_7CgetDeclaredMethods_7C_28_29ALjava_2Flang_2Freflect_2FMethod_3B (jobject javaThis) {
return Ljava_2Flang_2FClass_3B_7CgetDeclaredMethods_7C_28_29ALjava_2Flang_2Freflect_2FMethod_3B_(javaThis);
}
extern "C" jobject Ljava_2Flang_2FClass_3B_7CgetDeclaringClass_7C_28_29Ljava_2Flang_2FClass_3B_(jobject javaThis) ;
extern "C" jobject Ljava_2Flang_2FClass_3B_7CgetDeclaringClass_7C_28_29Ljava_2Flang_2FClass_3B(jobject javaThis) {
return Ljava_2Flang_2FClass_3B_7CgetDeclaringClass_7C_28_29Ljava_2Flang_2FClass_3B_(javaThis);
}
extern "C" jobject Ljava_2Flang_2FClass_3B_7CgetEnclosingClass_7C_28_29Ljava_2Flang_2FClass_3B_(jobject javaThis) ;
extern "C" jobject Ljava_2Flang_2FClass_3B_7CgetEnclosingClass_7C_28_29Ljava_2Flang_2FClass_3B(jobject javaThis) {
return Ljava_2Flang_2FClass_3B_7CgetEnclosingClass_7C_28_29Ljava_2Flang_2FClass_3B_(javaThis);
}
extern "C" jobject Ljava_2Flang_2FClass_3B_7CgetEnclosingConstructor_7C_28_29Ljava_2Flang_2Freflect_2FConstructor_3B_ (jobject jThis) ;
extern "C" jobject Ljava_2Flang_2FClass_3B_7CgetEnclosingConstructor_7C_28_29Ljava_2Flang_2Freflect_2FConstructor_3B (jobject jThis) {
return Ljava_2Flang_2FClass_3B_7CgetEnclosingConstructor_7C_28_29Ljava_2Flang_2Freflect_2FConstructor_3B_(jThis);
}
extern "C" jobject Ljava_2Flang_2FClass_3B_7CgetEnclosingMethod_7C_28_29Ljava_2Flang_2Freflect_2FMethod_3B_(jobject jThis) ;
extern "C" jobject Ljava_2Flang_2FClass_3B_7CgetEnclosingMethod_7C_28_29Ljava_2Flang_2Freflect_2FMethod_3B(jobject jThis) {
return Ljava_2Flang_2FClass_3B_7CgetEnclosingMethod_7C_28_29Ljava_2Flang_2Freflect_2FMethod_3B_(jThis);
}
extern "C" jobject Ljava_2Flang_2FClass_3B_7CgetField_7C_28Ljava_2Flang_2FString_3B_29Ljava_2Flang_2Freflect_2FField_3B_ (jobject javaThis, jstring name) ;
extern "C" jobject Ljava_2Flang_2FClass_3B_7CgetField_7C_28Ljava_2Flang_2FString_3B_29Ljava_2Flang_2Freflect_2FField_3B (jobject javaThis, jstring name) {
return Ljava_2Flang_2FClass_3B_7CgetField_7C_28Ljava_2Flang_2FString_3B_29Ljava_2Flang_2Freflect_2FField_3B_(javaThis, name);
}
extern "C" jobject Ljava_2Flang_2FClass_3B_7CgetFields_7C_28_29ALjava_2Flang_2Freflect_2FField_3B_(jobject javaThis) ;
extern "C" jobject Ljava_2Flang_2FClass_3B_7CgetFields_7C_28_29ALjava_2Flang_2Freflect_2FField_3B(jobject javaThis) {
return Ljava_2Flang_2FClass_3B_7CgetFields_7C_28_29ALjava_2Flang_2Freflect_2FField_3B_(javaThis);
}
extern "C" jobject Ljava_2Flang_2FClass_3B_7CgetInterfaces_7C_28_29ALjava_2Flang_2FClass_3B_(jobject javaThis) ;
extern "C" jobject Ljava_2Flang_2FClass_3B_7CgetInterfaces_7C_28_29ALjava_2Flang_2FClass_3B(jobject javaThis) {
return Ljava_2Flang_2FClass_3B_7CgetInterfaces_7C_28_29ALjava_2Flang_2FClass_3B_(javaThis);
}
extern "C" jclass Ljava_2Flang_2FClass_3B_7CgetMethod_7C_28Ljava_2Flang_2FString_3BALjava_2Flang_2FClass_3B_29Ljava_2Flang_2Freflect_2FMethod_3B_ (jobject javaThis, jstring name, jobject parameterTypes) ;
extern "C" jclass Ljava_2Flang_2FClass_3B_7CgetMethod_7C_28Ljava_2Flang_2FString_3BALjava_2Flang_2FClass_3B_29Ljava_2Flang_2Freflect_2FMethod_3B (jobject javaThis, jstring name, jobject parameterTypes) {
return Ljava_2Flang_2FClass_3B_7CgetMethod_7C_28Ljava_2Flang_2FString_3BALjava_2Flang_2FClass_3B_29Ljava_2Flang_2Freflect_2FMethod_3B_(javaThis, name, parameterTypes);
}
extern "C" jobject Ljava_2Flang_2FClass_3B_7CgetMethods_7C_28_29ALjava_2Flang_2Freflect_2FMethod_3B_(jobject javaThis) ;
extern "C" jobject Ljava_2Flang_2FClass_3B_7CgetMethods_7C_28_29ALjava_2Flang_2Freflect_2FMethod_3B(jobject javaThis) {
return Ljava_2Flang_2FClass_3B_7CgetMethods_7C_28_29ALjava_2Flang_2Freflect_2FMethod_3B_(javaThis);
}
extern "C" jint Ljava_2Flang_2FClass_3B_7CgetModifiers_7C_28_29I_(jobject javaThis) ;
extern "C" jint Ljava_2Flang_2FClass_3B_7CgetModifiers_7C_28_29I(jobject javaThis) {
return Ljava_2Flang_2FClass_3B_7CgetModifiers_7C_28_29I_(javaThis);
}
extern "C" jobject Ljava_2Flang_2FClass_3B_7CgetName_7C_28_29Ljava_2Flang_2FString_3B_ (jobject javaThis) ;
extern "C" jobject Ljava_2Flang_2FClass_3B_7CgetName_7C_28_29Ljava_2Flang_2FString_3B (jobject javaThis) {
return Ljava_2Flang_2FClass_3B_7CgetName_7C_28_29Ljava_2Flang_2FString_3B_(javaThis);
}
extern "C" jclass Ljava_2Flang_2FClass_3B_7CgetSuperclass_7C_28_29Ljava_2Flang_2FClass_3B_ (jobject javaThis) ;
extern "C" jclass Ljava_2Flang_2FClass_3B_7CgetSuperclass_7C_28_29Ljava_2Flang_2FClass_3B (jobject javaThis) {
return Ljava_2Flang_2FClass_3B_7CgetSuperclass_7C_28_29Ljava_2Flang_2FClass_3B_(javaThis);
}
extern "C" jboolean Ljava_2Flang_2FClass_3B_7CisAnonymousClass_7C_28_29Z_(jobject javaThis) ;
extern "C" jboolean Ljava_2Flang_2FClass_3B_7CisAnonymousClass_7C_28_29Z(jobject javaThis) {
return Ljava_2Flang_2FClass_3B_7CisAnonymousClass_7C_28_29Z_(javaThis);
}
extern "C" jboolean Ljava_2Flang_2FClass_3B_7CisArray_7C_28_29Z_(jobject javaThis) ;
extern "C" jboolean Ljava_2Flang_2FClass_3B_7CisArray_7C_28_29Z(jobject javaThis) {
return Ljava_2Flang_2FClass_3B_7CisArray_7C_28_29Z_(javaThis);
}
extern "C" jboolean Ljava_2Flang_2FClass_3B_7CisAssignableFrom_7C_28Ljava_2Flang_2FClass_3B_29Z_ (jobject javaThis, jclass cls) ;
extern "C" jboolean Ljava_2Flang_2FClass_3B_7CisAssignableFrom_7C_28Ljava_2Flang_2FClass_3B_29Z (jobject javaThis, jclass cls) {
return Ljava_2Flang_2FClass_3B_7CisAssignableFrom_7C_28Ljava_2Flang_2FClass_3B_29Z_(javaThis, cls);
}
extern "C" jboolean Ljava_2Flang_2FClass_3B_7CisInterface_7C_28_29Z_ (jobject javaThis) ;
extern "C" jboolean Ljava_2Flang_2FClass_3B_7CisInterface_7C_28_29Z (jobject javaThis) {
return Ljava_2Flang_2FClass_3B_7CisInterface_7C_28_29Z_(javaThis);
}
extern "C" jboolean Ljava_2Flang_2FClass_3B_7CisPrimitive_7C_28_29Z_ (jobject javaThis) ;
extern "C" jboolean Ljava_2Flang_2FClass_3B_7CisPrimitive_7C_28_29Z (jobject javaThis) {
return Ljava_2Flang_2FClass_3B_7CisPrimitive_7C_28_29Z_(javaThis);
}
extern "C" jobject Ljava_2Flang_2FClass_3B_7CnewInstance_7C_28_29Ljava_2Flang_2FObject_3B_(jobject javaThis) ;
extern "C" jobject Ljava_2Flang_2FClass_3B_7CnewInstance_7C_28_29Ljava_2Flang_2FObject_3B(jobject javaThis) {
return Ljava_2Flang_2FClass_3B_7CnewInstance_7C_28_29Ljava_2Flang_2FObject_3B_(javaThis);
}
extern "C" void Ljava_2Flang_2FClassLoader_3B_7CloadLibrary_7C_28Ljava_2Flang_2FClass_3BLjava_2Flang_2FString_3BZ_29V_(jclass klass, jstring jstr, jboolean flag) ;
extern "C" void Ljava_2Flang_2FClassLoader_3B_7CloadLibrary_7C_28Ljava_2Flang_2FClass_3BLjava_2Flang_2FString_3BZ_29V(jclass klass, jstring jstr, jboolean flag) {
Ljava_2Flang_2FClassLoader_3B_7CloadLibrary_7C_28Ljava_2Flang_2FClass_3BLjava_2Flang_2FString_3BZ_29V_(klass, jstr, flag);
return;
}
extern "C" jboolean Ljava_2Flang_2FClassLoader_3B_7CregisterAsParallelCapable_7C_28_29Z_(jclass klass) ;
extern "C" jboolean Ljava_2Flang_2FClassLoader_3B_7CregisterAsParallelCapable_7C_28_29Z(jclass klass) {
return Ljava_2Flang_2FClassLoader_3B_7CregisterAsParallelCapable_7C_28_29Z_(klass);
}
extern "C" void Ljava_2Flang_2Fref_2FReference_3B_7Cclear_7C_28_29V_(jobject javaThis) ;
extern "C" void Ljava_2Flang_2Fref_2FReference_3B_7Cclear_7C_28_29V(jobject javaThis) {
Ljava_2Flang_2Fref_2FReference_3B_7Cclear_7C_28_29V_(javaThis);
return;
}
extern "C" jobject Ljava_2Flang_2Fref_2FReference_3B_7Cget_7C_28_29Ljava_2Flang_2FObject_3B_(jobject javaThis) ;
extern "C" jobject Ljava_2Flang_2Fref_2FReference_3B_7Cget_7C_28_29Ljava_2Flang_2FObject_3B(jobject javaThis) {
return Ljava_2Flang_2Fref_2FReference_3B_7Cget_7C_28_29Ljava_2Flang_2FObject_3B_(javaThis);
}
extern "C" jboolean Ljava_2Flang_2Freflect_2FAccessibleObject_3B_7CisAccessible_7C_28_29Z_(jobject obj) ;
extern "C" jboolean Ljava_2Flang_2Freflect_2FAccessibleObject_3B_7CisAccessible_7C_28_29Z(jobject obj) {
return Ljava_2Flang_2Freflect_2FAccessibleObject_3B_7CisAccessible_7C_28_29Z_(obj);
}
extern "C" void Ljava_2Flang_2Freflect_2FAccessibleObject_3B_7CsetAccessible0_7C_28Ljava_2Flang_2Freflect_2FAccessibleObject_3BZ_29V_(jobject thisobj, jboolean flag) ;
extern "C" void Ljava_2Flang_2Freflect_2FAccessibleObject_3B_7CsetAccessible0_7C_28Ljava_2Flang_2Freflect_2FAccessibleObject_3BZ_29V(jobject thisobj, jboolean flag) {
Ljava_2Flang_2Freflect_2FAccessibleObject_3B_7CsetAccessible0_7C_28Ljava_2Flang_2Freflect_2FAccessibleObject_3BZ_29V_(thisobj, flag);
return;
}
extern "C" jclass Ljava_2Flang_2Freflect_2FConstructor_3B_7CgetDeclaringClass_7C_28_29Ljava_2Flang_2FClass_3B_(jobject javaThis) ;
extern "C" jclass Ljava_2Flang_2Freflect_2FConstructor_3B_7CgetDeclaringClass_7C_28_29Ljava_2Flang_2FClass_3B(jobject javaThis) {
return Ljava_2Flang_2Freflect_2FConstructor_3B_7CgetDeclaringClass_7C_28_29Ljava_2Flang_2FClass_3B_(javaThis);
}
extern "C" jobject Ljava_2Flang_2Freflect_2FConstructor_3B_7CgetExceptionTypes_7C_28_29ALjava_2Flang_2FClass_3B_(jobject javaThis) ;
extern "C" jobject Ljava_2Flang_2Freflect_2FConstructor_3B_7CgetExceptionTypes_7C_28_29ALjava_2Flang_2FClass_3B(jobject javaThis) {
return Ljava_2Flang_2Freflect_2FConstructor_3B_7CgetExceptionTypes_7C_28_29ALjava_2Flang_2FClass_3B_(javaThis);
}
extern "C" jint Ljava_2Flang_2Freflect_2FConstructor_3B_7CgetModifiers_7C_28_29I_(jobject javaThis) ;
extern "C" jint Ljava_2Flang_2Freflect_2FConstructor_3B_7CgetModifiers_7C_28_29I(jobject javaThis) {
return Ljava_2Flang_2Freflect_2FConstructor_3B_7CgetModifiers_7C_28_29I_(javaThis);
}
extern "C" jint Ljava_2Flang_2Freflect_2FConstructor_3B_7CgetParameterCount_7C_28_29I_(jobject javaThis) ;
extern "C" jint Ljava_2Flang_2Freflect_2FConstructor_3B_7CgetParameterCount_7C_28_29I(jobject javaThis) {
return Ljava_2Flang_2Freflect_2FConstructor_3B_7CgetParameterCount_7C_28_29I_(javaThis);
}
extern "C" jobject Ljava_2Flang_2Freflect_2FConstructor_3B_7CgetParameterTypes_7C_28_29ALjava_2Flang_2FClass_3B_(jobject javaThis) ;
extern "C" jobject Ljava_2Flang_2Freflect_2FConstructor_3B_7CgetParameterTypes_7C_28_29ALjava_2Flang_2FClass_3B(jobject javaThis) {
return Ljava_2Flang_2Freflect_2FConstructor_3B_7CgetParameterTypes_7C_28_29ALjava_2Flang_2FClass_3B_(javaThis);
}
extern "C" jboolean Ljava_2Flang_2Freflect_2FConstructor_3B_7CisSynthetic_7C_28_29Z_(jobject javaThis) ;
extern "C" jboolean Ljava_2Flang_2Freflect_2FConstructor_3B_7CisSynthetic_7C_28_29Z(jobject javaThis) {
return Ljava_2Flang_2Freflect_2FConstructor_3B_7CisSynthetic_7C_28_29Z_(javaThis);
}
extern "C" jboolean Ljava_2Flang_2Freflect_2FConstructor_3B_7CisVarArgs_7C_28_29Z_(jobject javaThis) ;
extern "C" jboolean Ljava_2Flang_2Freflect_2FConstructor_3B_7CisVarArgs_7C_28_29Z(jobject javaThis) {
return Ljava_2Flang_2Freflect_2FConstructor_3B_7CisVarArgs_7C_28_29Z_(javaThis);
}
extern "C" jobject Ljava_2Flang_2Freflect_2FConstructor_3B_7CnewInstance_7C_28ALjava_2Flang_2FObject_3B_29Ljava_2Flang_2FObject_3B_ (jobject javaThis, jobject javaArgs) ;
extern "C" jobject Ljava_2Flang_2Freflect_2FConstructor_3B_7CnewInstance_7C_28ALjava_2Flang_2FObject_3B_29Ljava_2Flang_2FObject_3B (jobject javaThis, jobject javaArgs) {
return Ljava_2Flang_2Freflect_2FConstructor_3B_7CnewInstance_7C_28ALjava_2Flang_2FObject_3B_29Ljava_2Flang_2FObject_3B_(javaThis, javaArgs);
}
extern "C" jobject Ljava_2Flang_2Freflect_2FExecutable_3B_7CdeclaredAnnotations_7C_28_29Ljava_2Futil_2FMap_3B_(jobject methodObj) ;
extern "C" jobject Ljava_2Flang_2Freflect_2FExecutable_3B_7CdeclaredAnnotations_7C_28_29Ljava_2Futil_2FMap_3B(jobject methodObj) {
return Ljava_2Flang_2Freflect_2FExecutable_3B_7CdeclaredAnnotations_7C_28_29Ljava_2Futil_2FMap_3B_(methodObj);
}
extern "C" jboolean Ljava_2Flang_2Freflect_2FExecutable_3B_7ChasRealParameterData_7C_28_29Z_(jobject methodObj) ;
extern "C" jboolean Ljava_2Flang_2Freflect_2FExecutable_3B_7ChasRealParameterData_7C_28_29Z(jobject methodObj) {
return Ljava_2Flang_2Freflect_2FExecutable_3B_7ChasRealParameterData_7C_28_29Z_(methodObj);
}
extern "C" jboolean Ljava_2Flang_2Freflect_2FExecutable_3B_7CisSynthetic_7C_28_29Z_(jobject methodObj) ;
extern "C" jboolean Ljava_2Flang_2Freflect_2FExecutable_3B_7CisSynthetic_7C_28_29Z(jobject methodObj) {
return Ljava_2Flang_2Freflect_2FExecutable_3B_7CisSynthetic_7C_28_29Z_(methodObj);
}
extern "C" jboolean Ljava_2Flang_2Freflect_2FExecutable_3B_7CisVarArgs_7C_28_29Z_(jobject methodObj) ;
extern "C" jboolean Ljava_2Flang_2Freflect_2FExecutable_3B_7CisVarArgs_7C_28_29Z(jobject methodObj) {
return Ljava_2Flang_2Freflect_2FExecutable_3B_7CisVarArgs_7C_28_29Z_(methodObj);
}
extern "C" jobject Ljava_2Flang_2Freflect_2FExecutable_3B_7CprivateGetParameters_7C_28_29ALjava_2Flang_2Freflect_2FParameter_3B_(jobject obj) ;
extern "C" jobject Ljava_2Flang_2Freflect_2FExecutable_3B_7CprivateGetParameters_7C_28_29ALjava_2Flang_2Freflect_2FParameter_3B(jobject obj) {
return Ljava_2Flang_2Freflect_2FExecutable_3B_7CprivateGetParameters_7C_28_29ALjava_2Flang_2Freflect_2FParameter_3B_(obj);
}
extern "C" jobject Ljava_2Flang_2Freflect_2FField_3B_7Cget_7C_28Ljava_2Flang_2FObject_3B_29Ljava_2Flang_2FObject_3B_(jobject jobj, jobject obj) ;
extern "C" jobject Ljava_2Flang_2Freflect_2FField_3B_7Cget_7C_28Ljava_2Flang_2FObject_3B_29Ljava_2Flang_2FObject_3B(jobject jobj, jobject obj) {
return Ljava_2Flang_2Freflect_2FField_3B_7Cget_7C_28Ljava_2Flang_2FObject_3B_29Ljava_2Flang_2FObject_3B_(jobj, obj);
}
extern "C" jobject Ljava_2Flang_2Freflect_2FField_3B_7CgetAnnotation_7C_28Ljava_2Flang_2FClass_3B_29Ljava_2Flang_2Fannotation_2FAnnotation_3B_ (jobject Field_arg0, jobject Class_arg) ;
extern "C" jobject Ljava_2Flang_2Freflect_2FField_3B_7CgetAnnotation_7C_28Ljava_2Flang_2FClass_3B_29Ljava_2Flang_2Fannotation_2FAnnotation_3B (jobject Field_arg0, jobject Class_arg) {
return Ljava_2Flang_2Freflect_2FField_3B_7CgetAnnotation_7C_28Ljava_2Flang_2FClass_3B_29Ljava_2Flang_2Fannotation_2FAnnotation_3B_(Field_arg0, Class_arg);
}
extern "C" jboolean Ljava_2Flang_2Freflect_2FField_3B_7CgetBoolean_7C_28Ljava_2Flang_2FObject_3B_29Z_ (jobject Field_arg0, jobject Object_arg1) ;
extern "C" jboolean Ljava_2Flang_2Freflect_2FField_3B_7CgetBoolean_7C_28Ljava_2Flang_2FObject_3B_29Z (jobject Field_arg0, jobject Object_arg1) {
return Ljava_2Flang_2Freflect_2FField_3B_7CgetBoolean_7C_28Ljava_2Flang_2FObject_3B_29Z_(Field_arg0, Object_arg1);
}
extern "C" jbyte Ljava_2Flang_2Freflect_2FField_3B_7CgetByte_7C_28Ljava_2Flang_2FObject_3B_29B_ (jobject Field_arg0, jobject Object_arg1) ;
extern "C" jbyte Ljava_2Flang_2Freflect_2FField_3B_7CgetByte_7C_28Ljava_2Flang_2FObject_3B_29B (jobject Field_arg0, jobject Object_arg1) {
return Ljava_2Flang_2Freflect_2FField_3B_7CgetByte_7C_28Ljava_2Flang_2FObject_3B_29B_(Field_arg0, Object_arg1);
}
extern "C" jchar Ljava_2Flang_2Freflect_2FField_3B_7CgetChar_7C_28Ljava_2Flang_2FObject_3B_29C_ (jobject Field_arg0, jobject Object_arg1) ;
extern "C" jchar Ljava_2Flang_2Freflect_2FField_3B_7CgetChar_7C_28Ljava_2Flang_2FObject_3B_29C (jobject Field_arg0, jobject Object_arg1) {
return Ljava_2Flang_2Freflect_2FField_3B_7CgetChar_7C_28Ljava_2Flang_2FObject_3B_29C_(Field_arg0, Object_arg1);
}
extern "C" jobject Ljava_2Flang_2Freflect_2FField_3B_7CgetDeclaredAnnotations_7C_28_29ALjava_2Flang_2Fannotation_2FAnnotation_3B_(jobject Field_arg0) ;
extern "C" jobject Ljava_2Flang_2Freflect_2FField_3B_7CgetDeclaredAnnotations_7C_28_29ALjava_2Flang_2Fannotation_2FAnnotation_3B(jobject Field_arg0) {
return Ljava_2Flang_2Freflect_2FField_3B_7CgetDeclaredAnnotations_7C_28_29ALjava_2Flang_2Fannotation_2FAnnotation_3B_(Field_arg0);
}
extern "C" jclass Ljava_2Flang_2Freflect_2FField_3B_7CgetDeclaringClass_7C_28_29Ljava_2Flang_2FClass_3B_ (jobject javaThis) ;
extern "C" jclass Ljava_2Flang_2Freflect_2FField_3B_7CgetDeclaringClass_7C_28_29Ljava_2Flang_2FClass_3B (jobject javaThis) {
return Ljava_2Flang_2Freflect_2FField_3B_7CgetDeclaringClass_7C_28_29Ljava_2Flang_2FClass_3B_(javaThis);
}
extern "C" jdouble Ljava_2Flang_2Freflect_2FField_3B_7CgetDouble_7C_28Ljava_2Flang_2FObject_3B_29D_ (jobject Field_arg0, jobject Object_arg1) ;
extern "C" jdouble Ljava_2Flang_2Freflect_2FField_3B_7CgetDouble_7C_28Ljava_2Flang_2FObject_3B_29D (jobject Field_arg0, jobject Object_arg1) {
return Ljava_2Flang_2Freflect_2FField_3B_7CgetDouble_7C_28Ljava_2Flang_2FObject_3B_29D_(Field_arg0, Object_arg1);
}
extern "C" jfloat Ljava_2Flang_2Freflect_2FField_3B_7CgetFloat_7C_28Ljava_2Flang_2FObject_3B_29F_ (jobject Field_arg0, jobject Object_arg1) ;
extern "C" jfloat Ljava_2Flang_2Freflect_2FField_3B_7CgetFloat_7C_28Ljava_2Flang_2FObject_3B_29F (jobject Field_arg0, jobject Object_arg1) {
return Ljava_2Flang_2Freflect_2FField_3B_7CgetFloat_7C_28Ljava_2Flang_2FObject_3B_29F_(Field_arg0, Object_arg1);
}
extern "C" jint Ljava_2Flang_2Freflect_2FField_3B_7CgetInt_7C_28Ljava_2Flang_2FObject_3B_29I_ (jobject Field_arg0, jobject Object_arg1) ;
extern "C" jint Ljava_2Flang_2Freflect_2FField_3B_7CgetInt_7C_28Ljava_2Flang_2FObject_3B_29I (jobject Field_arg0, jobject Object_arg1) {
return Ljava_2Flang_2Freflect_2FField_3B_7CgetInt_7C_28Ljava_2Flang_2FObject_3B_29I_(Field_arg0, Object_arg1);
}
extern "C" jlong Ljava_2Flang_2Freflect_2FField_3B_7CgetLong_7C_28Ljava_2Flang_2FObject_3B_29J_ (jobject Field_arg0, jobject Object_arg1) ;
extern "C" jlong Ljava_2Flang_2Freflect_2FField_3B_7CgetLong_7C_28Ljava_2Flang_2FObject_3B_29J (jobject Field_arg0, jobject Object_arg1) {
return Ljava_2Flang_2Freflect_2FField_3B_7CgetLong_7C_28Ljava_2Flang_2FObject_3B_29J_(Field_arg0, Object_arg1);
}
extern "C" jint Ljava_2Flang_2Freflect_2FField_3B_7CgetModifiers_7C_28_29I_ (jobject javaThis) ;
extern "C" jint Ljava_2Flang_2Freflect_2FField_3B_7CgetModifiers_7C_28_29I (jobject javaThis) {
return Ljava_2Flang_2Freflect_2FField_3B_7CgetModifiers_7C_28_29I_(javaThis);
}
extern "C" jstring Ljava_2Flang_2Freflect_2FField_3B_7CgetName_7C_28_29Ljava_2Flang_2FString_3B_ (jobject javaThis) ;
extern "C" jstring Ljava_2Flang_2Freflect_2FField_3B_7CgetName_7C_28_29Ljava_2Flang_2FString_3B (jobject javaThis) {
return Ljava_2Flang_2Freflect_2FField_3B_7CgetName_7C_28_29Ljava_2Flang_2FString_3B_(javaThis);
}
extern "C" jshort Ljava_2Flang_2Freflect_2FField_3B_7CgetShort_7C_28Ljava_2Flang_2FObject_3B_29S_ (jobject Field_arg0, jobject Object_arg1) ;
extern "C" jshort Ljava_2Flang_2Freflect_2FField_3B_7CgetShort_7C_28Ljava_2Flang_2FObject_3B_29S (jobject Field_arg0, jobject Object_arg1) {
return Ljava_2Flang_2Freflect_2FField_3B_7CgetShort_7C_28Ljava_2Flang_2FObject_3B_29S_(Field_arg0, Object_arg1);
}
extern "C" jclass Ljava_2Flang_2Freflect_2FField_3B_7CgetType_7C_28_29Ljava_2Flang_2FClass_3B_ (jobject javaThis) ;
extern "C" jclass Ljava_2Flang_2Freflect_2FField_3B_7CgetType_7C_28_29Ljava_2Flang_2FClass_3B (jobject javaThis) {
return Ljava_2Flang_2Freflect_2FField_3B_7CgetType_7C_28_29Ljava_2Flang_2FClass_3B_(javaThis);
}
extern "C" void Ljava_2Flang_2Freflect_2FField_3B_7Cset_7C_28Ljava_2Flang_2FObject_3BLjava_2Flang_2FObject_3B_29V_(jobject jobj, jobject obj, jobject value) ;
extern "C" void Ljava_2Flang_2Freflect_2FField_3B_7Cset_7C_28Ljava_2Flang_2FObject_3BLjava_2Flang_2FObject_3B_29V(jobject jobj, jobject obj, jobject value) {
Ljava_2Flang_2Freflect_2FField_3B_7Cset_7C_28Ljava_2Flang_2FObject_3BLjava_2Flang_2FObject_3B_29V_(jobj, obj, value);
return;
}
extern "C" void Ljava_2Flang_2Freflect_2FField_3B_7CsetBoolean_7C_28Ljava_2Flang_2FObject_3BZ_29V_ (jobject Field_arg0, jobject Object_arg1, jboolean arg2) ;
extern "C" void Ljava_2Flang_2Freflect_2FField_3B_7CsetBoolean_7C_28Ljava_2Flang_2FObject_3BZ_29V (jobject Field_arg0, jobject Object_arg1, jboolean arg2) {
Ljava_2Flang_2Freflect_2FField_3B_7CsetBoolean_7C_28Ljava_2Flang_2FObject_3BZ_29V_(Field_arg0, Object_arg1, arg2);
return;
}
extern "C" void Ljava_2Flang_2Freflect_2FField_3B_7CsetByte_7C_28Ljava_2Flang_2FObject_3BB_29V_ (jobject Field_arg0, jobject Object_arg1, jbyte arg2) ;
extern "C" void Ljava_2Flang_2Freflect_2FField_3B_7CsetByte_7C_28Ljava_2Flang_2FObject_3BB_29V (jobject Field_arg0, jobject Object_arg1, jbyte arg2) {
Ljava_2Flang_2Freflect_2FField_3B_7CsetByte_7C_28Ljava_2Flang_2FObject_3BB_29V_(Field_arg0, Object_arg1, arg2);
return;
}
extern "C" void Ljava_2Flang_2Freflect_2FField_3B_7CsetChar_7C_28Ljava_2Flang_2FObject_3BC_29V_ (jobject Field_arg0, jobject Object_arg1, jchar arg2) ;
extern "C" void Ljava_2Flang_2Freflect_2FField_3B_7CsetChar_7C_28Ljava_2Flang_2FObject_3BC_29V (jobject Field_arg0, jobject Object_arg1, jchar arg2) {
Ljava_2Flang_2Freflect_2FField_3B_7CsetChar_7C_28Ljava_2Flang_2FObject_3BC_29V_(Field_arg0, Object_arg1, arg2);
return;
}
extern "C" void Ljava_2Flang_2Freflect_2FField_3B_7CsetDouble_7C_28Ljava_2Flang_2FObject_3BD_29V_ (jobject Field_arg0, jobject Object_arg1, jdouble arg2) ;
extern "C" void Ljava_2Flang_2Freflect_2FField_3B_7CsetDouble_7C_28Ljava_2Flang_2FObject_3BD_29V (jobject Field_arg0, jobject Object_arg1, jdouble arg2) {
Ljava_2Flang_2Freflect_2FField_3B_7CsetDouble_7C_28Ljava_2Flang_2FObject_3BD_29V_(Field_arg0, Object_arg1, arg2);
return;
}
extern "C" void Ljava_2Flang_2Freflect_2FField_3B_7CsetFloat_7C_28Ljava_2Flang_2FObject_3BF_29V_ (jobject Field_arg0, jobject Object_arg1, jfloat arg2) ;
extern "C" void Ljava_2Flang_2Freflect_2FField_3B_7CsetFloat_7C_28Ljava_2Flang_2FObject_3BF_29V (jobject Field_arg0, jobject Object_arg1, jfloat arg2) {
Ljava_2Flang_2Freflect_2FField_3B_7CsetFloat_7C_28Ljava_2Flang_2FObject_3BF_29V_(Field_arg0, Object_arg1, arg2);
return;
}
extern "C" void Ljava_2Flang_2Freflect_2FField_3B_7CsetInt_7C_28Ljava_2Flang_2FObject_3BI_29V_ (jobject Field_arg0, jobject Object_arg1, jint arg2) ;
extern "C" void Ljava_2Flang_2Freflect_2FField_3B_7CsetInt_7C_28Ljava_2Flang_2FObject_3BI_29V (jobject Field_arg0, jobject Object_arg1, jint arg2) {
Ljava_2Flang_2Freflect_2FField_3B_7CsetInt_7C_28Ljava_2Flang_2FObject_3BI_29V_(Field_arg0, Object_arg1, arg2);
return;
}
extern "C" void Ljava_2Flang_2Freflect_2FField_3B_7CsetLong_7C_28Ljava_2Flang_2FObject_3BJ_29V_ (jobject Field_arg0, jobject Object_arg1, jlong arg2) ;
extern "C" void Ljava_2Flang_2Freflect_2FField_3B_7CsetLong_7C_28Ljava_2Flang_2FObject_3BJ_29V (jobject Field_arg0, jobject Object_arg1, jlong arg2) {
Ljava_2Flang_2Freflect_2FField_3B_7CsetLong_7C_28Ljava_2Flang_2FObject_3BJ_29V_(Field_arg0, Object_arg1, arg2);
return;
}
extern "C" void Ljava_2Flang_2Freflect_2FField_3B_7CsetShort_7C_28Ljava_2Flang_2FObject_3BS_29V_ (jobject Field_arg0, jobject Object_arg1, jshort arg2) ;
extern "C" void Ljava_2Flang_2Freflect_2FField_3B_7CsetShort_7C_28Ljava_2Flang_2FObject_3BS_29V (jobject Field_arg0, jobject Object_arg1, jshort arg2) {
Ljava_2Flang_2Freflect_2FField_3B_7CsetShort_7C_28Ljava_2Flang_2FObject_3BS_29V_(Field_arg0, Object_arg1, arg2);
return;
}
extern "C" jclass Ljava_2Flang_2Freflect_2FMethod_3B_7CgetDeclaringClass_7C_28_29Ljava_2Flang_2FClass_3B_(jobject javaThis) ;
extern "C" jclass Ljava_2Flang_2Freflect_2FMethod_3B_7CgetDeclaringClass_7C_28_29Ljava_2Flang_2FClass_3B(jobject javaThis) {
return Ljava_2Flang_2Freflect_2FMethod_3B_7CgetDeclaringClass_7C_28_29Ljava_2Flang_2FClass_3B_(javaThis);
}
extern "C" jobject Ljava_2Flang_2Freflect_2FMethod_3B_7CgetExceptionTypes_7C_28_29ALjava_2Flang_2FClass_3B_(jobject javaThis) ;
extern "C" jobject Ljava_2Flang_2Freflect_2FMethod_3B_7CgetExceptionTypes_7C_28_29ALjava_2Flang_2FClass_3B(jobject javaThis) {
return Ljava_2Flang_2Freflect_2FMethod_3B_7CgetExceptionTypes_7C_28_29ALjava_2Flang_2FClass_3B_(javaThis);
}
extern "C" jint Ljava_2Flang_2Freflect_2FMethod_3B_7CgetModifiers_7C_28_29I_(jobject javaThis) ;
extern "C" jint Ljava_2Flang_2Freflect_2FMethod_3B_7CgetModifiers_7C_28_29I(jobject javaThis) {
return Ljava_2Flang_2Freflect_2FMethod_3B_7CgetModifiers_7C_28_29I_(javaThis);
}
extern "C" jstring Ljava_2Flang_2Freflect_2FMethod_3B_7CgetName_7C_28_29Ljava_2Flang_2FString_3B_(jobject javaThis) ;
extern "C" jstring Ljava_2Flang_2Freflect_2FMethod_3B_7CgetName_7C_28_29Ljava_2Flang_2FString_3B(jobject javaThis) {
return Ljava_2Flang_2Freflect_2FMethod_3B_7CgetName_7C_28_29Ljava_2Flang_2FString_3B_(javaThis);
}
extern "C" jint Ljava_2Flang_2Freflect_2FMethod_3B_7CgetParameterCount_7C_28_29I_(jobject javaThis) ;
extern "C" jint Ljava_2Flang_2Freflect_2FMethod_3B_7CgetParameterCount_7C_28_29I(jobject javaThis) {
return Ljava_2Flang_2Freflect_2FMethod_3B_7CgetParameterCount_7C_28_29I_(javaThis);
}
extern "C" jobject Ljava_2Flang_2Freflect_2FMethod_3B_7CgetParameterTypes_7C_28_29ALjava_2Flang_2FClass_3B_(jobject javaThis) ;
extern "C" jobject Ljava_2Flang_2Freflect_2FMethod_3B_7CgetParameterTypes_7C_28_29ALjava_2Flang_2FClass_3B(jobject javaThis) {
return Ljava_2Flang_2Freflect_2FMethod_3B_7CgetParameterTypes_7C_28_29ALjava_2Flang_2FClass_3B_(javaThis);
}
extern "C" jclass Ljava_2Flang_2Freflect_2FMethod_3B_7CgetReturnType_7C_28_29Ljava_2Flang_2FClass_3B_(jobject javaThis) ;
extern "C" jclass Ljava_2Flang_2Freflect_2FMethod_3B_7CgetReturnType_7C_28_29Ljava_2Flang_2FClass_3B(jobject javaThis) {
return Ljava_2Flang_2Freflect_2FMethod_3B_7CgetReturnType_7C_28_29Ljava_2Flang_2FClass_3B_(javaThis);
}
extern "C" jobject Ljava_2Flang_2Freflect_2FMethod_3B_7Cinvoke_7C_28Ljava_2Flang_2FObject_3BALjava_2Flang_2FObject_3B_29Ljava_2Flang_2FObject_3B_(jobject javaThis, jobject Object_arg1, jobject AObject_arg2) ;
extern "C" jobject Ljava_2Flang_2Freflect_2FMethod_3B_7Cinvoke_7C_28Ljava_2Flang_2FObject_3BALjava_2Flang_2FObject_3B_29Ljava_2Flang_2FObject_3B(jobject javaThis, jobject Object_arg1, jobject AObject_arg2) {
return Ljava_2Flang_2Freflect_2FMethod_3B_7Cinvoke_7C_28Ljava_2Flang_2FObject_3BALjava_2Flang_2FObject_3B_29Ljava_2Flang_2FObject_3B_(javaThis, Object_arg1, AObject_arg2);
}
extern "C" jboolean Ljava_2Flang_2Freflect_2FMethod_3B_7CisBridge_7C_28_29Z_(jobject javaThis) ;
extern "C" jboolean Ljava_2Flang_2Freflect_2FMethod_3B_7CisBridge_7C_28_29Z(jobject javaThis) {
return Ljava_2Flang_2Freflect_2FMethod_3B_7CisBridge_7C_28_29Z_(javaThis);
}
extern "C" jboolean Ljava_2Flang_2Freflect_2FMethod_3B_7CisDefault_7C_28_29Z_(jobject javaThis) ;
extern "C" jboolean Ljava_2Flang_2Freflect_2FMethod_3B_7CisDefault_7C_28_29Z(jobject javaThis) {
return Ljava_2Flang_2Freflect_2FMethod_3B_7CisDefault_7C_28_29Z_(javaThis);
}
extern "C" jboolean Ljava_2Flang_2Freflect_2FMethod_3B_7CisSynthetic_7C_28_29Z_(jobject javaThis) ;
extern "C" jboolean Ljava_2Flang_2Freflect_2FMethod_3B_7CisSynthetic_7C_28_29Z(jobject javaThis) {
return Ljava_2Flang_2Freflect_2FMethod_3B_7CisSynthetic_7C_28_29Z_(javaThis);
}
extern "C" jboolean Ljava_2Flang_2Freflect_2FMethod_3B_7CisVarArgs_7C_28_29Z_(jobject javaThis) ;
extern "C" jboolean Ljava_2Flang_2Freflect_2FMethod_3B_7CisVarArgs_7C_28_29Z(jobject javaThis) {
return Ljava_2Flang_2Freflect_2FMethod_3B_7CisVarArgs_7C_28_29Z_(javaThis);
}
extern "C" jstring Ljava_2Flang_2Freflect_2FMethod_3B_7CtoString_7C_28_29Ljava_2Flang_2FString_3B_(jobject javaThis) ;
extern "C" jstring Ljava_2Flang_2Freflect_2FMethod_3B_7CtoString_7C_28_29Ljava_2Flang_2FString_3B(jobject javaThis) {
return Ljava_2Flang_2Freflect_2FMethod_3B_7CtoString_7C_28_29Ljava_2Flang_2FString_3B_(javaThis);
}
extern "C" jstring Ljava_2Flang_2FString_3B_7Cintern_7C_28_29Ljava_2Flang_2FString_3B_(jstring str) ;
extern "C" jstring Ljava_2Flang_2FString_3B_7Cintern_7C_28_29Ljava_2Flang_2FString_3B(jstring str) {
return Ljava_2Flang_2FString_3B_7Cintern_7C_28_29Ljava_2Flang_2FString_3B_(str);
}
extern "C" void Ljava_2Flang_2FShutdown_3B_7CbeforeHalt_7C_28_29V_(jclass klass) ;
extern "C" void Ljava_2Flang_2FShutdown_3B_7CbeforeHalt_7C_28_29V(jclass klass) {
Ljava_2Flang_2FShutdown_3B_7CbeforeHalt_7C_28_29V_(klass);
return;
}
extern "C" jclass Lsun_2Freflect_2FReflection_3B_7CgetCallerClass_7C_28_29Ljava_2Flang_2FClass_3B_() ;
extern "C" jclass Lsun_2Freflect_2FReflection_3B_7CgetCallerClass_7C_28_29Ljava_2Flang_2FClass_3B() {
return Lsun_2Freflect_2FReflection_3B_7CgetCallerClass_7C_28_29Ljava_2Flang_2FClass_3B_();
}
}

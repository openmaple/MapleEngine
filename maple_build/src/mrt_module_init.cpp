/*
  This file shouldn't be included in libmaplert, instead,
  it should be linked statically into each linking target (be it an executable or a shared library)
*/

extern "C" {

#define MRT_EXPORT __attribute__((visibility("default")))

extern char *__reflection_strtab_start__;
extern char *__reflection_strtab_end__;

extern char *__muid_range_tab_begin__;
extern char *__muid_range_tab_end__;

extern char *__muid_tab_start;
extern char *__muid_tab_end;
extern char *__eh_frame_start;
extern char *__eh_frame_end;

extern void *__maple_end__;

MRT_EXPORT char *MRT_GetMapleEnd() {
  return (char *)&__maple_end__;
}

// reflection strtab query interface;
MRT_EXPORT char *MRT_GetStrTabBegin() {
  return (char *)&__reflection_strtab_start__;
}

MRT_EXPORT char *MRT_GetStrTabEnd() {
  return (char *)&__reflection_strtab_end__;
}

MRT_EXPORT void *MRT_GetRangeTableBegin() {
  return (void*)&__muid_range_tab_begin__;
}

MRT_EXPORT void *MRT_GetRangeTableEnd() {
  return (void*)&__muid_range_tab_end__;
}

MRT_EXPORT void *MRT_GetMuidTabBegin() {
  return (void *)&__muid_tab_start;
}

MRT_EXPORT void *MRT_GetMuidTabEnd() {
  return (void *)&__muid_tab_end;
}

MRT_EXPORT void *MRT_GetEhframeStart() {
  return (void *)&__eh_frame_start;
}

MRT_EXPORT void *MRT_GetEhframeEnd() {
  return (void *)&__eh_frame_end;
}

// TODO: better to use __OPENJDK__ as the guard?
#ifdef __MUSL__ // using musl as the C library
// gcc/llvm generated code may need symbol __dso_handle
// which is not provided by __MUSL__. Add it here to satisfy the link requirement.
void *__dso_handle = nullptr;

#endif // __MUSL__
}

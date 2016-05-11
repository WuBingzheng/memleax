#ifndef _MLD_PTR_BACKTRACE_
#define _MLD_PTR_BACKTRACE_

#include <libunwind-ptrace.h>

int ptr_maps_build(const char *path, intptr_t start, intptr_t end, int exe_self);
int ptr_backtrace(unw_word_t *ips, int size);

#endif

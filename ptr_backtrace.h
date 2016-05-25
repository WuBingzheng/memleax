#ifndef _MLD_PTR_BACKTRACE_
#define _MLD_PTR_BACKTRACE_

#include <libunwind-ptrace.h>
#include <sys/types.h>

void ptr_maps_build(pid_t pid);
int ptr_backtrace(unw_word_t *ips, int size);

#endif

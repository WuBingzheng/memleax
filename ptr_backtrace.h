#ifndef MLD_PTR_BACKTRACE_H
#define MLD_PTR_BACKTRACE_H

#include <libunwind-ptrace.h>
#include <sys/types.h>

void ptr_maps_build(pid_t pid);
int ptr_backtrace(unw_word_t *ips, int size);

#endif

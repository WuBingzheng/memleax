#ifndef MLD_PTR_BACKTRACE_H
#define MLD_PTR_BACKTRACE_H

#include <libunwind-ptrace.h>
#include <sys/types.h>

/**
 * @brief ptr_maps_build
 * @param pid
 */
void ptr_maps_build(pid_t pid);

/**
 * @brief ptr_backtrace
 * @param ips
 * @param size
 * @return
 */
int ptr_backtrace(unw_word_t *ips, int size);

#endif

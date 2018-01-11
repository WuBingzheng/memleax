#ifndef MLD_DEBUG_LINE_H
#define MLD_DEBUG_LINE_H

#include <stdint.h>
#include <sys/types.h>

/**
 * @brief debug_line_build
 * @param pid
 */
void debug_line_build(pid_t pid);

/**
 * @brief debug_line_search
 * @param address
 * @param lineno
 * @return
 */
const char *debug_line_search(uintptr_t address, int *lineno);

#endif

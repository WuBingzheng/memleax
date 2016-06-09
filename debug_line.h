#ifndef MLD_DEBUG_LINE_H
#define MLD_DEBUG_LINE_H

#include <stdint.h>
#include <sys/types.h>

void debug_line_build(pid_t pid);
const char *debug_line_search(uintptr_t address, int *lineno);

#endif

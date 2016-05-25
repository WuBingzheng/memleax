#ifndef _MLD_DEBUG_LINE_
#define _MLD_DEBUG_LINE_

#include <stdint.h>
#include <sys/types.h>

void debug_line_build(pid_t pid);
const char *debug_line_search(uintptr_t address, int *lineno);

#endif

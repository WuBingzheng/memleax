#ifndef _MLD_DEBUG_LINE_
#define _MLD_DEBUG_LINE_

#include <stdint.h>

int debug_line_build(const char *path, intptr_t start, intptr_t end, int exe_self);
void debug_line_build_finish(void);
const char *debug_line_search(intptr_t address, int *lineno);

#endif

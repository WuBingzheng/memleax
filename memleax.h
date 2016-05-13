#ifndef _MEMLEAKDETECTOR_
#define _MEMLEAKDETECTOR_

#include <stdint.h>

extern pid_t g_current_thread;
extern uintptr_t g_current_entry;
extern int opt_backtrace_limit;

#define log_debug(...)

#endif

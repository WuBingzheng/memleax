#ifndef MLD_MEMLEAK_H
#define MLD_MEMLEAK_H

#include <stdint.h>

extern pid_t g_current_thread;
extern uintptr_t g_current_entry;
extern int opt_backtrace_limit;
extern const char *opt_debug_info_file;

#define log_debug(...)

#endif

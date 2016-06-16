#ifndef MLD_MEMLEAK_H
#define MLD_MEMLEAK_H

#include <stdint.h>

extern pid_t g_current_thread;
extern uintptr_t g_current_entry;
extern int opt_backtrace_limit;

void try_debug(int (*buildf)(const char*, size_t, size_t),
		const char *name, const char *path,
		size_t start, size_t end, int exe_self);

#define log_debug(...)

#endif

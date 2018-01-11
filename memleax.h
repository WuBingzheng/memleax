#ifndef MLD_MEMLEAK_H
#define MLD_MEMLEAK_H

#include <stdint.h>

/**
 * @brief g_current_thread
 */
extern pid_t g_current_thread;

/**
 * @brief g_current_entry
 */
extern uintptr_t g_current_entry;

/**
 * @brief opt_backtrace_limit
 */
extern int opt_backtrace_limit;

/**
 * @brief opt_debug_info_file
 */
extern const char *opt_debug_info_file;

/**
 *
 */
#define log_debug(...)

#endif

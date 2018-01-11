#ifndef MLD_CALLSTACK_H
#define MLD_CALLSTACK_H

#include "hash.h"
#include "ptr_backtrace.h"
#include <stdint.h>

#define BACKTRACE_MAX 50

/**
 * @brief The callstack_s struct
 */
struct callstack_s {
	int		id;

	int		alloc_count, free_count;
	int		expired_count, free_expired_count;
	int		alloc_size, free_size;
	int		expired_size, free_expired_size;
	int		free_min, free_max, free_total;
	int		unfree_max;

	int		ip_num;
	struct hlist_node hash_node;
	unw_word_t	ips[0];
};

/**
 * @brief callstack_current
 * @return
 */
struct callstack_s *callstack_current(void);

/**
 * @brief callstack_print
 * @param cs
 */
void callstack_print(struct callstack_s *cs);

/**
 * @brief callstack_report
 */
void callstack_report(void);

#endif

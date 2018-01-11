#ifndef MLD_CALLSTACK_H
#define MLD_CALLSTACK_H

#include "hash.h"
#include "ptr_backtrace.h"
#include <stdint.h>

#define BACKTRACE_MAX 50

struct callstack_s {
	int		id;

    size_t		alloc_count, free_count;
    size_t		expired_count, free_expired_count;
    size_t		alloc_size, free_size;
    size_t		expired_size, free_expired_size;
	int		free_min, free_max, free_total;
	int		unfree_max;

	int		ip_num;
	struct hlist_node hash_node;
	unw_word_t	ips[0];
};

struct callstack_s *callstack_current(void);
void callstack_print(struct callstack_s *cs);
void callstack_report(void);

#endif

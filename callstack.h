#ifndef _MLD_callstack_
#define _MLD_callstack_

#include "hash.h"
#include "ptr_backtrace.h"
#include <stdint.h>

#define BACKTRACE_MAX 50

struct callstack_s {
	struct hlist_node hash_node;
	unw_word_t	ips[BACKTRACE_MAX];
	int		ip_num;

	int		alloc_count, free_count;
	int		expired_count, free_expired_count;
	int		alloc_size, free_size;
	int		expired_size, free_expired_size;
	int		free_min, free_max, free_total;
	int		unfree_max;

	int		id;
	char		*string;
};

struct callstack_s *callstack_current(void);
const char *callstack_string(struct callstack_s *cs);
void callstack_report(void);

#endif

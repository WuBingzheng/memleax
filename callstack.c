/*
 * call stack utils
 *
 * Author: Wu Bingzheng
 *   Date: 2016-5
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "hash.h"
#include "array.h"
#include "symtab.h"
#include "callstack.h"
#include "ptr_backtrace.h"
#include "addr_maps.h"
#include "debug_line.h"
#include "memleax.h"
#include "memblock.h"

static struct hlist_head g_callstack_hash[HASH_SIZE];

struct callstack_s *callstack_current(void)
{
	unw_word_t ips[BACKTRACE_MAX];
	ips[0] = g_current_entry; /* push the API entry address */
	int ip_num = ptr_backtrace(ips + 1, opt_backtrace_limit - 1) + 1;

	/* search */
	struct hlist_node *p = hash_search(g_callstack_hash,
			ips, sizeof(unw_word_t) * ip_num);
	if (p != NULL) { /* found */
		return list_entry(p, struct callstack_s, hash_node);
	}

	/* not found, create new one */
	struct callstack_s *cs = malloc(sizeof(struct callstack_s) + sizeof(unw_word_t) * ip_num);
	if (cs == NULL) {
		return NULL;
	}
	bzero(cs, sizeof(struct callstack_s));

	memcpy(cs->ips, ips, sizeof(unw_word_t) * ip_num);
	cs->ip_num = ip_num;

	hash_add(g_callstack_hash, &cs->hash_node, sizeof(unw_word_t) * ip_num);
	return cs;
}

void callstack_print(struct callstack_s *cs)
{
	int offset, lineno, i;
	for (i = 0; i < cs->ip_num; i++) {
		uintptr_t address = cs->ips[i];
		if (address == 0) {
			break;
		}

		printf("    0x%016lx", address);

		printf("  %s", addr_maps_search(address));

		const char *proc_name = symtab_by_address(address, &offset);
		if (proc_name != NULL) {
			printf("  %s()+%d", proc_name, offset);
		}

		/* @address is return-address, so address-1 is calling-line */
		const char *file_name = debug_line_search(address - 1, &lineno);
		if (file_name != NULL) {
			if (proc_name == NULL) printf("  ?()");
			printf("  %s:%d", file_name, lineno);
		}

		printf("\n");

		if (proc_name && strcmp(proc_name, "main") == 0) {
			break;
		}
	}
}

static int callstack_cmp(const void *a, const void *b)
{
	const struct callstack_s *csa = *(const struct callstack_s **)a;
	const struct callstack_s *csb = *(const struct callstack_s **)b;

	return (csa->expired_count - csa->free_expired_count)
			- (csb->expired_count - csb->free_expired_count);
}
void callstack_report(void)
{
	memblock_count();

	/* sort */
	ARRAY(callstacks, struct callstack_s *, 1000);

	struct callstack_s **csp;
	struct callstack_s *cs;
	struct hlist_node *p;
	int i;
	hash_for_each(p, i, g_callstack_hash) {
		cs = list_entry(p, struct callstack_s, hash_node);
		if (cs->expired_count == cs->free_expired_count) {
			continue;
		}
		csp = array_push(&callstacks);
		*csp = cs;
	}

	if (callstacks.item_num == 0) {
		printf("== No expired memory blocks.\n\n");
		return;
	}

	array_sort(&callstacks, callstack_cmp);

	/* show */
	printf("== Callstack statistics: (in ascending order)\n\n");
	array_for_each(csp, &callstacks) {
		cs = *csp;
		printf("CallStack[%d]: may-leak=%d (%d bytes)\n"
				"    expired=%d (%d bytes), free_expired=%d (%d bytes)\n"
				"    alloc=%d (%d bytes), free=%d (%d bytes)\n"
				"    freed memory live time: min=%d max=%d average=%d\n"
				"    un-freed memory live time: max=%d\n",
				cs->id,
				cs->expired_count - cs->free_expired_count,
				cs->expired_size - cs->free_expired_size,
				cs->expired_count, cs->expired_size,
				cs->free_expired_count, cs->free_expired_size,
				cs->alloc_count, cs->alloc_size,
				cs->free_count, cs->free_size,
				cs->free_min, cs->free_max,
				cs->free_count ? cs->free_total / cs->free_count : 0,
				cs->unfree_max);
		callstack_print(cs);
		printf("\n");
	}
}

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "hash.h"
#include "symtab.h"
#include "callstack.h"
#include "ptr_backtrace.h"
#include "debug_line.h"
#include "memleax.h"
#include "memblock.h"

static struct hlist_head g_callstack_hash[HASH_SIZE];

#define CALLSTACK_MAX 10000
static int g_callstack_num = 0;
static struct callstack_s g_callstacks[CALLSTACK_MAX];

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
	if (g_callstack_num == CALLSTACK_MAX) {
		printf("error: too many callstacks\n");
		exit(3);
	}
	struct callstack_s *cs = &g_callstacks[g_callstack_num++];
	bzero(cs, sizeof(struct callstack_s));

	memcpy(cs->ips, ips, sizeof(unw_word_t) * ip_num);
	cs->ip_num = ip_num;

	hash_add(g_callstack_hash, &cs->hash_node, sizeof(unw_word_t) * ip_num);
	return cs;
}

const char *callstack_string(struct callstack_s *cs)
{
	if (cs->string != NULL) {
		return cs->string;
	}

	char buffer[10000], *p = buffer;
	int offset, lineno, i;
	for (i = 0; i < cs->ip_num; i++) {
		uintptr_t address = cs->ips[i];
		if (address == 0) {
			break;
		}

		p += sprintf(p, "    0x%016lx", address);

		const char *proc_name = symtab_by_address(address, &offset);
		if (proc_name != NULL) {
			p += sprintf(p, " %s()+%d", proc_name, offset);
		}

		/* @address is return-address, so address-1 is calling-line */
		const char *file_name = debug_line_search(address - 1, &lineno);
		if (file_name != NULL) {
			p += sprintf(p, " %s:%d", file_name, lineno);
		}

		p += sprintf(p, "\n");

		if (proc_name && strcmp(proc_name, "main") == 0) {
			break;
		}
	}

	cs->string = malloc(p - buffer + 1);
	memcpy(cs->string, buffer, p - buffer + 1);
	return cs->string;
}

static int callstack_cmp(const void *a, const void *b)
{
	const struct callstack_s *csa = a;
	const struct callstack_s *csb = b;

	return (csa->expired_count - csa->free_expired_count)
			- (csb->expired_count - csb->free_expired_count);
}
void callstack_report(void)
{
	memblock_count();
	qsort(g_callstacks, g_callstack_num,
			sizeof(struct callstack_s), callstack_cmp);

	int i;
	for (i = 0; i < g_callstack_num; i++) {
		struct callstack_s *cs = &g_callstacks[i];
		if (cs->expired_count == cs->free_expired_count) {
			continue;
		}

		printf("CallStack[%d]: may-leak=%d (%d bytes)\n"
				"    expired=%d (%d bytes), free_expired=%d (%d bytes)\n"
				"    alloc=%d (%d bytes), free=%d (%d bytes)\n"
				"    freed memory live time: min=%d max=%d average=%d\n"
				"    un-freed memory live time: max=%d\n%s\n",
				cs->id,
				cs->expired_count - cs->free_expired_count,
				cs->expired_size - cs->free_expired_size,
				cs->expired_count, cs->expired_size,
				cs->free_expired_count, cs->free_expired_size,
				cs->alloc_count, cs->alloc_size,
				cs->free_count, cs->free_size,
				cs->free_min, cs->free_max,
				cs->free_count ? cs->free_total / cs->free_count : 0,
				cs->unfree_max,
				callstack_string(cs));
	}
}

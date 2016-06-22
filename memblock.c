/*
 * memory block allocate, expire and free
 *
 * Author: Wu Bingzheng
 *   Date: 2016-5
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "hash.h"
#include "list.h"
#include "memblock.h"
#include "callstack.h"

struct memblock_s {
	struct list_head	list_node;
	struct hlist_node	hash_node;
	uintptr_t		pointer;
	size_t			size;
	time_t			create;
	int			expired;
	struct callstack_s	*callstack;
};

static struct hlist_head g_memblock_hash[HASH_SIZE];

static LIST_HEAD(g_memblock_active);
static LIST_HEAD(g_memblock_expire);

int memblock_new(uintptr_t pointer, size_t size)
{
	if (pointer == 0) {
		printf("Warning: alloc returns NULL at\n");
		callstack_print(callstack_current());
		return 0;
	}

	struct memblock_s *mb = malloc(sizeof(struct memblock_s));
	if (mb == NULL) {
		return -1;
	}
	mb->callstack = callstack_current();
	if (mb->callstack == NULL) {
		return -1;
	}
	mb->pointer = pointer;
	mb->size = size;
	mb->create = time(NULL);
	mb->expired = 0;

	hash_add(g_memblock_hash, &mb->hash_node, sizeof(uintptr_t));
	list_add_tail(&mb->list_node, &g_memblock_active);

	mb->callstack->alloc_count++;
	mb->callstack->alloc_size += size;
	return 0;
}

#define DONOT_SHOW_AFTER_FREE_EXPIRES 3
void memblock_delete(struct memblock_s *mb)
{
	if (mb == NULL) {
		return;
	}

	time_t free_time = time(NULL) - mb->create;
	struct callstack_s *cs = mb->callstack;
	if (mb->expired) {
		cs->free_expired_count++;
		cs->free_expired_size += mb->size;

		if (cs->free_expired_count <= DONOT_SHOW_AFTER_FREE_EXPIRES) {
			printf("CallStack[%d]: expired-memory frees after %ld seconds\n",
					cs->id, free_time);
		}
		if (cs->free_expired_count == DONOT_SHOW_AFTER_FREE_EXPIRES) {
			printf("Warning: too many expired-free at CallStack[%d]. "
					"will not show this CallStack later\n", cs->id);
		}
	}

	cs->free_count++;
	cs->free_size += mb->size;
	cs->free_total += free_time;
	if (free_time > cs->free_max) cs->free_max = free_time;
	if (cs->free_min == 0 || free_time < cs->free_min) cs->free_min = free_time;

	hash_delete(&mb->hash_node);
	list_del(&mb->list_node);
	free(mb);
}

void memblock_update_size(struct memblock_s *mb, size_t size)
{
	if (mb != NULL) {
		mb->callstack->alloc_size -= mb->size;
		mb->callstack->alloc_size += size;
		mb->size = size;
	}
}

struct memblock_s *memblock_search(uintptr_t pointer)
{
	struct hlist_node *p = hash_search(g_memblock_hash,
			&pointer, sizeof(uintptr_t));
	if (p == NULL) {
		return NULL;
	}
	return list_entry(p, struct memblock_s, hash_node);
}

static void memblock_expire_one(struct memblock_s *mb)
{
	/* memblock */
	mb->expired = 1;
	list_del(&mb->list_node);
	list_add_tail(&mb->list_node, &g_memblock_expire);

	/* callstack */
	struct callstack_s *cs = mb->callstack;
	cs->expired_count++;
	cs->expired_size += mb->size;

	static int callstack_id = 1;
	if (cs->id == 0) { /* first time */
		cs->id = callstack_id++;
		printf("CallStack[%d]: memory expires with %ld bytes, backtrace:\n",
				cs->id, mb->size);
		callstack_print(cs);
	} else if (cs->free_expired_count < DONOT_SHOW_AFTER_FREE_EXPIRES) {
		printf("CallStack[%d]: memory expires with %ld bytes, %d times again\n",
				cs->id, mb->size, cs->expired_count);
	}
}

int memblock_expire(time_t expire, int memblock_limit, int callstack_limit)
{
	time_t now = time(NULL);
	struct list_head *p, *safe;
	struct memblock_s *mb;
	list_for_each_safe(p, safe, &g_memblock_active) {
		mb = list_entry(p, struct memblock_s, list_node);
		if (now - mb->create < expire) {
			return expire - (now - mb->create) + 1;
		}

		memblock_expire_one(mb);

		/* check limits */
		struct callstack_s *cs = mb->callstack;
		if (cs->id >= callstack_limit + 1) {
			printf("\n== %d CallStacks gets memory leak.\n", callstack_limit);
			printf("-- Maybe you should set bigger expired time by `-e`.\n");
			return -1;
		}
		if (cs->expired_count >= memblock_limit) {
			printf("\n== %d memory blocks leaked at CallStack[%d].\n",
					memblock_limit, cs->id);
			return -1;
		}
	}

	return 0;
}

void memblock_count(void)
{
	time_t now = time(NULL);
	struct list_head *p;
	struct memblock_s *mb;
	struct callstack_s *cs;
	list_for_each(p, &g_memblock_expire) {
		mb = list_entry(p, struct memblock_s, list_node);
		cs = mb->callstack;
		if (cs->unfree_max == 0) {
			cs->unfree_max = now - mb->create;
		}
	}
	list_for_each(p, &g_memblock_active) {
		mb = list_entry(p, struct memblock_s, list_node);
		cs = mb->callstack;
		if (cs->unfree_max == 0) {
			cs->unfree_max = now - mb->create;
		}
	}
}

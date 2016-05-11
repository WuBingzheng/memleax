#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "hash.h"
#include "list.h"
#include "memblock.h"
#include "callstack.h"

static struct hlist_head g_memblock_hash[HASH_SIZE];

static LIST_HEAD(g_memblock_active);
static LIST_HEAD(g_memblock_expire);

struct memblock_s *memblock_new(intptr_t pointer, size_t size)
{
	if (pointer == 0) {
		printf("Warning: alloc returns NULL at\n%s",
				callstack_string(callstack_current()));
		return NULL;
	}

	struct memblock_s *mb = malloc(sizeof(struct memblock_s));
	mb->pointer = pointer;
	mb->size = size;
	mb->callstack = callstack_current();
	mb->create = time(NULL);
	mb->expired = 0;

	hash_add(g_memblock_hash, &mb->hash_node, sizeof(intptr_t));
	list_add_tail(&mb->list_node, &g_memblock_active);

	mb->callstack->alloc_count++;
	mb->callstack->alloc_size += size;
	return mb;
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

struct memblock_s *memblock_search(intptr_t pointer)
{
	struct hlist_node *p = hash_search(g_memblock_hash,
			&pointer, sizeof(intptr_t));
	if (p == NULL) {
		return NULL;
	}
	return list_entry(p, struct memblock_s, hash_node);
}

static void memblock_expire_one(struct memblock_s *mb)
{
	/* callstack */
	struct callstack_s *cs = mb->callstack;

	cs->expired_count++;
	cs->expired_size += mb->size;

	static int callstack_id = 1;
	if (cs->id == 0) { /* first time */
		cs->id = callstack_id++;
		printf("CallStack[%d]: memory expires with %ld bytes, backtrace:\n%s",
				cs->id, mb->size, callstack_string(cs));
	} else if (cs->free_expired_count < DONOT_SHOW_AFTER_FREE_EXPIRES) {
		printf("CallStack[%d]: memory expires with %ld bytes, %d times again\n",
				cs->id, mb->size, cs->expired_count);
	}

	/* memblock */
	mb->expired = 1;
	list_del(&mb->list_node);
	list_add_tail(&mb->list_node, &g_memblock_expire);
}

void memblock_expire(time_t expire)
{
	time_t now = time(NULL);
	struct list_head *p, *safe;
	struct memblock_s *mb;
	list_for_each_safe(p, safe, &g_memblock_active) {
		mb = list_entry(p, struct memblock_s, list_node);
		if (now - mb->create < expire) {
			break;
		}

		memblock_expire_one(mb);
	}
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

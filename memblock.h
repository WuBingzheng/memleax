#ifndef _MLD_MEMBLOCK_
#define _MLD_MEMBLOCK_

#include <unistd.h>
#include <time.h>
#include "list.h"
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

struct memblock_s *memblock_new(uintptr_t pointer, size_t size);
void memblock_delete(struct memblock_s *mb);
struct memblock_s *memblock_search(uintptr_t pointer);
int memblock_expire(time_t expire);
void memblock_count(void);

#endif

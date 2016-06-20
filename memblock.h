#ifndef MLD_MEMBLOCK_H
#define MLD_MEMBLOCK_H

#include <unistd.h>
#include <time.h>
#include "callstack.h"

struct memblock_s;

int memblock_new(uintptr_t pointer, size_t size);
void memblock_delete(struct memblock_s *mb);
void memblock_update_size(struct memblock_s *mb, size_t size);
struct memblock_s *memblock_search(uintptr_t pointer);
int memblock_expire(time_t expire, int memblock_limit, int callstack_limit);
void memblock_count(void);

#endif

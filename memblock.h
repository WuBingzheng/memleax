#ifndef _MLD_MEMBLOCK_
#define _MLD_MEMBLOCK_

#include <unistd.h>
#include <time.h>
#include "callstack.h"

struct memblock_s;

struct memblock_s *memblock_new(uintptr_t pointer, size_t size);
void memblock_delete(struct memblock_s *mb);
void memblock_update_size(struct memblock_s *mb, size_t size);
struct memblock_s *memblock_search(uintptr_t pointer);
int memblock_expire(time_t expire);
void memblock_count(void);

#endif

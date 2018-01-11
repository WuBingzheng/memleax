#ifndef MLD_MEMBLOCK_H
#define MLD_MEMBLOCK_H

#include <unistd.h>
#include <time.h>
#include "callstack.h"

/**
 *
 */
struct memblock_s;

/**
 * @brief memblock_new
 * @param pointer
 * @param size
 * @return
 */
int memblock_new(uintptr_t pointer, size_t size);

/**
 * @brief memblock_delete
 * @param mb
 */
void memblock_delete(struct memblock_s *mb);

/**
 * @brief memblock_update_size
 * @param mb
 * @param size
 */
void memblock_update_size(struct memblock_s *mb, size_t size);

/**
 * @brief memblock_search
 * @param pointer
 * @return
 */
struct memblock_s *memblock_search(uintptr_t pointer);

/**
 * @brief memblock_expire
 * @param expire
 * @param memblock_limit
 * @param callstack_limit
 * @return
 */
int memblock_expire(time_t expire, int memblock_limit, int callstack_limit);

/**
 * @brief memblock_count
 */
void memblock_count(void);

#endif

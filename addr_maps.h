#ifndef MLD_ADDR_MAPS_H
#define MLD_ADDR_MAPS_H

#include <stdint.h>
#include <sys/types.h>

/**
 * @brief addr_maps_build
 * @param pid
 */
void addr_maps_build(pid_t pid);

/**
 * @brief addr_maps_search
 * @param address
 * @return
 */
const char *addr_maps_search(uintptr_t address);

#endif

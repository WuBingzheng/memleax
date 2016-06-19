#ifndef MLD_ADDR_MAPS_H
#define MLD_ADDR_MAPS_H

#include <stdint.h>
#include <sys/types.h>

void addr_maps_build(pid_t pid);
const char *addr_maps_search(uintptr_t address);

#endif

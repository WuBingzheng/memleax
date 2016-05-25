#ifndef _MLD_PROC_MAPS_
#define _MLD_PROC_MAPS_

#include <sys/types.h>
#include <stddef.h>

const char *proc_maps(pid_t pid, size_t *start, size_t *end, int *exe_self);
pid_t proc_tasks(pid_t pid);

#endif

#ifndef MLD_PROC_MAPS_H
#define MLD_PROC_MAPS_H

#include <sys/types.h>
#include <stddef.h>

const char *proc_maps(pid_t pid, size_t *start, size_t *end, int *exe_self);
pid_t proc_tasks(pid_t pid);
int proc_task_check(pid_t pid, pid_t child);

#endif

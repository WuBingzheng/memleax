#ifndef MLD_PROC_MAPS_H
#define MLD_PROC_MAPS_H

#include <sys/types.h>
#include <stddef.h>

/**
 * @brief proc_maps
 * @param pid
 * @param start
 * @param end
 * @param exe_self
 * @return
 */
const char *proc_maps(pid_t pid, size_t *start, size_t *end, int *exe_self);

/**
 * @brief proc_tasks
 * @param pid
 * @return
 */
pid_t proc_tasks(pid_t pid);

/**
 * @brief proc_task_check
 * @param pid
 * @param child
 * @return
 */
int proc_task_check(pid_t pid, pid_t child);

#endif

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <dirent.h>

#include "proc_info.h"

const char *proc_maps(pid_t pid, size_t *start, size_t *end, int *exe_self)
{
	static FILE *filp = NULL;
	static char path[1024];

	/* we assume the first map is exe-self */
	if (exe_self) {
		*exe_self = (filp == NULL);
	}

	/* first, init */
	if (filp == NULL) {
		char pname[100];
		sprintf(pname, "/proc/%d/maps", pid);
		filp = fopen(pname, "r");
		if (filp == NULL) {
			perror("Error in open /proc/pid/maps");
			exit(3);
		}
	}

	char line[1024];
	char perms[5];
	int ia, ib, ic, id;
	while (fgets(line, sizeof(line), filp) != NULL) {
		sscanf(line, "%lx-%lx %s %x %x:%x %d %s",
				start, end, perms, &ia, &ib, &ic, &id, path);
		if (perms[2] == 'x' && path[0] == '/') {
			return path;
		}
	}

	fclose(filp);
	filp = NULL;
	return NULL;
}

pid_t proc_tasks(pid_t pid)
{
	static DIR *dirp = NULL;

	if (dirp == NULL) {
		char tname[100];
		sprintf(tname, "/proc/%d/task", pid);
		dirp = opendir(tname);
		if (dirp == NULL) {
			perror("Error in open /proc/pid/tasks");
			exit(3);
		}
	}

	struct dirent *e;
	while((e = readdir(dirp)) != NULL) {
		pid_t task_id = atoi(e->d_name);
		if (task_id != 0) {
			return task_id;
		}
	}

	closedir(dirp);
	return 0;
}

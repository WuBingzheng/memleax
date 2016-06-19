/*
 * address maps of libraries
 *
 * Author: Wu Bingzheng
 *   Date: 2016-5
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "addr_maps.h"
#include "proc_info.h"
#include "array.h"


struct addr_map_s {
	uintptr_t	start, end;
	char		name[100];
};

static ARRAY(g_addr_maps, struct addr_map_s, 50);

static void addr_maps_build_file(const char *path, size_t start, size_t end)
{
	struct addr_map_s *am = array_push(&g_addr_maps);
	am->start = start;
	am->end = end;

	const char *p = strrchr(path, '/');
	const char *q = strstr(p, ".so");
	if (q == NULL) {
		strncpy(am->name, p + 1, 100);
		am->name[99] = '\0';
	} else {
		memcpy(am->name, p + 1, q - p + 2);
		am->name[q - p + 2] = '\0';
	}
}

void addr_maps_build(pid_t pid)
{
	const char *path;
	size_t start, end;
	while ((path = proc_maps(pid, &start, &end, NULL)) != NULL) {
		addr_maps_build_file(path, start, end);
	}
}

const char *addr_maps_search(uintptr_t address)
{
	int min = 0, max = g_addr_maps.item_num - 1;
	struct addr_map_s *maps = g_addr_maps.data;
	while (min <= max) {
		int mid = (min + max) / 2;
		struct addr_map_s *am = &maps[mid];
		if (address < am->start) {
			max = mid - 1;
		} else if (address > am->end) {
			min = mid + 1;
		} else {
			return am->name;
		}
	}
	return "??";
}

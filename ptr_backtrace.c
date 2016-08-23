/*
 * build backtrace by libunwind
 *
 * Author: Wu Bingzheng
 *   Date: 2016-5
 */

#include <sys/ptrace.h>
#include <libunwind-ptrace.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <unistd.h>

#include "ptr_backtrace.h"
#include "proc_info.h"
#include "list.h"
#include "hash.h"
#include "memleax.h"
#include "ptrace_utils.h"


static LIST_HEAD(g_map_sections);
struct map_section_s {
	struct list_head	list_node;
	uintptr_t		start, end;
	unw_word_t		data[0];
};

static void ptr_maps_build_file(const char *path, size_t start, size_t end)
{
	/* create map-section */
	struct map_section_s *ms = malloc(sizeof(struct map_section_s) + end - start);

	/* read */
	int fd = open(path, O_RDONLY);
	if (fd < 0) {
		printf("Warning: error in open map of %s: %s\n", path, strerror(errno));
		return;
	}
	ssize_t rlen = read(fd, ms->data, end - start);
	if (rlen < 0) {
		printf("Warning: error in read map of %s: %s\n", path, strerror(errno));
		return;
	}

	/* read OK */
	close(fd);

	ms->start = start;
	ms->end = start + rlen;
	list_add_tail(&ms->list_node, &g_map_sections);
}

void ptr_maps_build(pid_t pid)
{
	const char *path;
	size_t start, end;
	while ((path = proc_maps(pid, &start, &end, NULL)) != NULL) {
		ptr_maps_build_file(path, start, end);
	}
}

static int _ptr_access_mem(unw_addr_space_t as, unw_word_t addr, unw_word_t *val,
		int write, void *arg)
{
	struct list_head *p;
	struct map_section_s *ms;
	list_for_each(p, &g_map_sections) {
		ms = list_entry(p, struct map_section_s, list_node);
		if (addr >= ms->start && addr <= ms->end) {
			*val = ms->data[(addr - ms->start) / sizeof(unw_word_t)];
			return 0;
		}
	}
	*val = ptrace_get_data(g_current_thread, addr);
#ifdef MLX_FREEBSD
	unw_word_t high = ptrace_get_data(g_current_thread, addr + 4);
	*val = (high << 32) | (*val & 0xFFFFFFFFUL);
#endif
	return 0;
}
static int _ptr_find_proc_info (unw_addr_space_t as, unw_word_t ip, unw_proc_info_t *pi,
		int need_unwind_info, void *arg)
{
	static struct hlist_head ip_info_hash[HASH_SIZE];
	struct ip_info_s {
		struct hlist_node	hash_node;
		unw_word_t		ip;
		unw_proc_info_t		pi;
		int			ret;
	};

	/* search the cache */
	struct hlist_node *p = hash_search(ip_info_hash, &ip, sizeof(ip));
	if (p != NULL) {
		struct ip_info_s *ii = list_entry(p, struct ip_info_s, hash_node);
		if (ii->ret < 0) return ii->ret;

		*pi = ii->pi;
		pi->unwind_info = malloc(pi->unwind_info_size);
		memcpy(pi->unwind_info, ii->pi.unwind_info, pi->unwind_info_size);
		return ii->ret;
	}

	struct ip_info_s *ii = malloc(sizeof(struct ip_info_s));
	ii->ip = ip;

	/* cache miss, query it */
	ii->ret = _UPT_find_proc_info(as, ip, pi, need_unwind_info, arg);
	if (ii->ret >= 0) { /* add to cache if success */
		ii->pi = *pi;
		ii->pi.unwind_info = malloc(pi->unwind_info_size);
		memcpy(ii->pi.unwind_info, pi->unwind_info, pi->unwind_info_size);
	}
	hash_add(ip_info_hash, &ii->hash_node, sizeof(ip));
	return ii->ret;
}

struct thread_space_s {
	struct list_head	list_node;
	unw_addr_space_t	uspace;
	void			*upt_info;
	pid_t			thread_id;
};
int ptr_backtrace(unw_word_t *ips, int size)
{
#define SPACE_MAX 10
	static LIST_HEAD(space_head);
	static struct thread_space_s spaces[SPACE_MAX];

	/* init at first time */
	if (list_empty(&space_head)) {
		_UPT_accessors.access_mem = _ptr_access_mem;
		_UPT_accessors.find_proc_info = _ptr_find_proc_info;

		int i;
		for (i = 0; i < SPACE_MAX; i++) {
			list_add(&spaces[i].list_node, &space_head);
		}
	}

	/* search current thread */
	struct thread_space_s *sp = NULL;
	struct list_head *p;
	list_for_each(p, &space_head) {
		sp = list_entry(p, struct thread_space_s, list_node);
		if (sp->thread_id == 0) {
			break;
		}
		if (sp->thread_id == g_current_thread) {
			break;
		}
	}

	/* if not found, create one */
	if (sp->thread_id != g_current_thread) {
		if (sp->thread_id != 0) { /* expire oldest */
			unw_destroy_addr_space(sp->uspace);
			_UPT_destroy(sp->upt_info);
		}

		sp->uspace = unw_create_addr_space(&_UPT_accessors, 0);
		sp->upt_info = _UPT_create(g_current_thread);
		sp->thread_id = g_current_thread;
	}

	/* update LRU */
	list_del(&sp->list_node);
	list_add(&sp->list_node, &space_head);

	/* build backtrace */
	int i = 0;
	unw_cursor_t cursor;
	unw_init_remote(&cursor, sp->uspace, sp->upt_info);
	do {
		unw_get_reg(&cursor, UNW_REG_IP, &ips[i++]);
	} while (i < size && unw_step(&cursor) > 0);

	return i;
}

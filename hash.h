/*
 * simple hash utils
 *
 * Author: Wu Bingzheng
 *   Date: 2016-5
 */

#ifndef MLD_HASH_H
#define MLD_HASH_H

#include "list.h"
#include <stdint.h>

#define HASH_SIZE 29989

static inline uint32_t hash_slot(void *key, int len)
{
	uint32_t *p = key;
	uint32_t n = 0;
	int i;
	for (i = 0; i < len / sizeof(uint32_t); i++) {
		n ^= p[i];
	}
	return n % HASH_SIZE;
}

static inline void hash_add(struct hlist_head *hash, struct hlist_node *node, int len)
{
	void *key = node + 1;
	hlist_add_head(node, &hash[hash_slot(key, len)]);
}

static inline void hash_delete(struct hlist_node *node)
{
	hlist_del(node);
}

static inline struct hlist_node *hash_search(
		struct hlist_head *hash, void *vkey, int len)
{
	uint32_t n = hash_slot(vkey, len);

	struct hlist_node *p;
	hlist_for_each(p, &hash[n]) {
		if (memcmp(p + 1, vkey, len) == 0) {
			return p;
		}
	}
	return NULL;
}

#define hash_for_each(p, i, hash) \
	for (i = 0; i < HASH_SIZE; i++) \
		hlist_for_each(p, &hash[i])

#endif

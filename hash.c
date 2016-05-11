#include <string.h>
#include <stdint.h>
#include "hash.h"

static uint32_t hash_slot(void *key, int len)
{
	uint32_t *p = key;
	uint32_t n = 0;
	int i;
	for (i = 0; i < len / sizeof(uint32_t); i++) {
		n ^= p[i];
	}
	return n % HASH_SIZE;
}

void hash_add(struct hlist_head *hash, struct hlist_node *node, int len)
{
	void *key = node + 1;
	hlist_add_head(node, &hash[hash_slot(key, len)]);
}

void hash_delete(struct hlist_node *node)
{
	hlist_del(node);
}

struct hlist_node *hash_search(struct hlist_head *hash, void *vkey, int len)
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

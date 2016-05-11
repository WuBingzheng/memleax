#ifndef _MLD_HASH_
#define _MLD_HASH_

#include "list.h"

#define HASH_SIZE 29989

void hash_add(struct hlist_head *hash, struct hlist_node *node, int len);
void hash_delete(struct hlist_node *node);
struct hlist_node *hash_search(struct hlist_head *hash, void *vkey, int len);

#endif

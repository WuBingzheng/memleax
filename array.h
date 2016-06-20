/*
 * simple array utils
 *
 * Author: Wu Bingzheng
 *   Date: 2016-5
 */

#ifndef MLD_ARRAY_H
#define MLD_ARRAY_H

struct array_s {
	int	item_max;
	int	item_num;
	int	item_size;
	void	*data;
};

#define ARRAY(name, type, init_max) \
	struct array_s name = {init_max, 0, sizeof(type), NULL}

#define array_out(pos, a)  \
	((char *)pos >= (char *)(a)->data + ((a)->item_size * (a)->item_num))

#define array_for_each(pos, a) \
	for (pos = (a)->data; !array_out(pos, a); pos++)

static inline void *array_push(struct array_s *a)
{
	if (a->data == NULL) {
		a->data = malloc(a->item_size * a->item_max);
	} else if (a->item_num == a->item_max) {
		a->item_max *= 2;
		a->data = realloc(a->data, a->item_size * a->item_max);
	}

	if (a->data == NULL) {
		return NULL;
	}

	return (char *)a->data + (a->item_size * a->item_num++);
}

static inline void array_sort(struct array_s *a, int (*compar)(const void *, const void *))
{
	qsort(a->data, a->item_num, a->item_size, compar);
}

static inline void *array_last(struct array_s *a)
{
	if (a->item_num == 0) {
		return NULL;
	}
	return (char *)a->data + (a->item_size * (a->item_num-1));
}

#endif

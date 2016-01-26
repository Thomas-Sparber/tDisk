#ifndef HELPERS_H
#define HELPERS_H

#include <linux/list.h>

/**
  * Copied from div64.c
 **/
inline static uint32_t __div64_32(uint64_t *n, uint32_t base)
{
	uint64_t rem = *n;
	uint64_t b = base;
	uint64_t res, d = 1;
	uint32_t high = rem >> 32;

	/* Reduce the thing a bit first */
	res = 0;
	if (high >= base) {
		high /= base;
		res = (uint64_t) high << 32;
		rem -= (uint64_t) (high*base) << 32;
	}

	while ((int64_t)b > 0 && b < rem) {
		b = b+b;
		d = d+d;
	}

	do {
		if (rem >= b) {
			rem -= b;
			res += d;
		}
		b >>= 1;
		d >>= 1;
	} while (d);

	*n = res;
	return rem;
}

inline static void hlist_insert_sorted(struct hlist_node *entry, struct hlist_head *head, int(*callback)(struct hlist_node*, struct hlist_node*))
{
	struct hlist_node *item;
	struct hlist_node *last = NULL;

	hlist_for_each(item, head) {
		if(callback(entry, item) < 0)
		{
			hlist_add_before(entry, item);
			return;
		}
		last = item;
	}

	if(!last)
		hlist_add_head(entry, head);
	else
		hlist_add_behind(entry, last);
}

inline static void hlist_insertsort(struct hlist_head *head, int(*callback)(struct hlist_node*, struct hlist_node*))
{
	struct hlist_head list_unsorted;
	struct hlist_node *item;
	struct hlist_node *is;

	hlist_move_list(head, &list_unsorted);

	hlist_for_each_safe(item, is, &list_unsorted) {
		hlist_del(item);
		hlist_insert_sorted(item, head, callback);
	}
}

#endif //HELPERS_H

/**
  *
  * tDisk Driver
  * @author Thomas Sparber (2015)
  *
 **/

#ifndef HELPERS_H
#define HELPERS_H

#include <linux/list.h>

#define GET_MACRO(_0, _1, _2, _3, _4, _5, _6, NAME, ...) NAME
#define MY_BUG_ON(...) GET_MACRO(__VA_ARGS__, MY_BUG_ON6, MY_BUG_ON5, MY_BUG_ON4, MY_BUG_ON3, MY_BUG_ON2, MY_BUG_ON1, MY_BUG_ON0)(__VA_ARGS__)

#define PRINT_INT(var)		"%d",	var
#define PRINT_UINT(var)		"%u",	var
#define PRINT_LONG(var)		"%l",	var
#define PRINT_ULONG(var)	"%lu",	var
#define PRINT_LL(var)		"%ll",	var
#define PRINT_ULL(var)		"%llu",	var
#define PRINT_FLOAT(var)	"%d",	var

#define MY_BUG_ON0(equation) \
	do { \
		if(unlikely(equation)) { \
			printk_ratelimited(KERN_ERR "tDisk BUG: " #equation "\n"); \
			BUG_ON(equation); \
		} \
	} while(0)
		
#define MY_BUG_ON2(equation, data_type, data) \
	do { \
		if(unlikely(equation)) { \
			printk_ratelimited(KERN_ERR "tDisk BUG: " #equation "\n"); \
			printk_ratelimited(KERN_ERR "tDisk BUG_DATA: " #data " = " data_type "\n", data); \
			BUG_ON(equation); \
		} \
	} while(0)
		
#define MY_BUG_ON4(equation, data_type1, data1, data_type2, data2) \
	do { \
		if(unlikely(equation)) { \
			printk_ratelimited(KERN_ERR "tDisk BUG: " #equation); \
			printk_ratelimited(KERN_ERR "tDisk BUG_DATA: " #data1 " = " data_type1 "\n", data1); \
			printk_ratelimited(KERN_ERR "tDisk BUG_DATA: " #data2 " = " data_type2 "\n", data2); \
			BUG_ON(equation); \
		} \
	} while(0)
		
#define MY_BUG_ON6(equation, data_type1, data1, data_type2, data2, data_type3, data3) \
	do { \
		if(unlikely(equation)) { \
			printk_ratelimited(KERN_ERR "tDisk BUG: " #equation); \
			printk_ratelimited(KERN_ERR "tDisk BUG_DATA: " #data1 " = " data_type1 "\n", data1); \
			printk_ratelimited(KERN_ERR "tDisk BUG_DATA: " #data2 " = " data_type2 "\n", data2); \
			printk_ratelimited(KERN_ERR "tDisk BUG_DATA: " #data3 " = " data_type3 "\n", data3); \
			BUG_ON(equation); \
		} \
	} while(0)
	

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

/**
  * Inserts the given entry into the given
  * hlist in the sorted position according to the
  * callback
 **/
inline static void hlist_insert_sorted(struct hlist_node *entry, struct hlist_head *head, int(*callback)(struct hlist_node*, struct hlist_node*))
{
	struct hlist_node *item;
	struct hlist_node *last = NULL;

	hlist_for_each(item, head)
	{
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

/**
  * Sorts the given hlist according to the callback
 **/
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

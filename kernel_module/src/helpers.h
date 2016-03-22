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

/**
  * This macro can be used to check for an error condition
  * and print some variables if that condition is true.
  * Also the module aborts because BUG_ON is called.
 **/
#define MY_BUG_ON(...) GET_MACRO(__VA_ARGS__, MY_BUG_ON6, MY_BUG_ON5, MY_BUG_ON4, MY_BUG_ON3, MY_BUG_ON2, MY_BUG_ON1, MY_BUG_ON0)(__VA_ARGS__)

/** Used by MY_BUG_ON to print an int variable (%d) **/
#define PRINT_INT(var)		"%d",	var

/** Used by MY_BUG_ON to print an uint variable (%u) **/
#define PRINT_UINT(var)		"%u",	var

/** Used by MY_BUG_ON to print a long variable (%l) **/
#define PRINT_LONG(var)		"%l",	var

/** Used by MY_BUG_ON to print a ulong variable (%lu) **/
#define PRINT_ULONG(var)	"%lu",	var

/** Used by MY_BUG_ON to print a long long variable (%ll) **/
#define PRINT_LL(var)		"%ll",	var

/** Used by MY_BUG_ON to print a ulong long variable (%llu) **/
#define PRINT_ULL(var)		"%llu",	var

/** Helper function of MY_BUG_ON **/
#define MY_BUG_ON0(equation) \
	do { \
		if(unlikely(equation)) { \
			printk_ratelimited(KERN_ERR "tDisk BUG: " #equation "\n"); \
			BUG_ON(equation); \
		} \
	} while(0)

/** Helper function of MY_BUG_ON **/
#define MY_BUG_ON2(equation, data_type, data) \
	do { \
		if(unlikely(equation)) { \
			printk_ratelimited(KERN_ERR "tDisk BUG: " #equation "\n"); \
			printk_ratelimited(KERN_ERR "tDisk BUG_DATA: " #data " = " data_type "\n", data); \
			BUG_ON(equation); \
		} \
	} while(0)

/** Helper function of MY_BUG_ON **/
#define MY_BUG_ON4(equation, data_type1, data1, data_type2, data2) \
	do { \
		if(unlikely(equation)) { \
			printk_ratelimited(KERN_ERR "tDisk BUG: " #equation); \
			printk_ratelimited(KERN_ERR "tDisk BUG_DATA: " #data1 " = " data_type1 "\n", data1); \
			printk_ratelimited(KERN_ERR "tDisk BUG_DATA: " #data2 " = " data_type2 "\n", data2); \
			BUG_ON(equation); \
		} \
	} while(0)

/** Helper function of MY_BUG_ON **/
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

#endif //HELPERS_H

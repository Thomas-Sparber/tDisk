/**
  *
  * tDisk Driver
  * @author Thomas Sparber (2015-2016)
  *
 **/

#ifndef HELPERS_H
#define HELPERS_H

#pragma GCC system_header
#include <linux/aio.h>
#include <linux/bio.h>
#include <linux/blk-mq.h>
#include <linux/blkdev.h>
#include <linux/fs.h>
#include <linux/file.h>
#include <linux/list.h>
#include <linux/uio.h>
#include <linux/version.h>

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/backing-dev.h>
#include <linux/bio.h>
#include <linux/blkdev.h>
#include <linux/kmemleak.h>
#include <linux/mm.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/workqueue.h>
#include <linux/smp.h>
#include <linux/llist.h>
#include <linux/list_sort.h>
#include <linux/cpu.h>
#include <linux/cache.h>
#include <linux/sched/sysctl.h>
#include <linux/delay.h>
#include <linux/crash_dump.h>

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

#ifndef __ASM_ARM_DIV64

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

#endif //__ASM_ARM_DIV64

/**
  * This function simply returns the division result
  * of the numbers and omits the mod
 **/
inline static uint64_t __div64_32_nomod(uint64_t n, uint32_t base)
{
	__div64_32(&n, base);
	return n;
}

#if LINUX_VERSION_CODE <= KERNEL_VERSION(3,19,0)

/**
  * Copied from read_write.c
 **/
inline ssize_t vfs_iter_write(struct file *file, struct iov_iter *iter, loff_t *ppos)
{
	struct kiocb kiocb;
	ssize_t ret;

	if (!file->f_op->write_iter)
	return -EINVAL;

	init_sync_kiocb(&kiocb, file);
	kiocb.ki_pos = *ppos;

	iter->type |= WRITE;
	ret = file->f_op->write_iter(&kiocb, iter);
	BUG_ON(ret == -EIOCBQUEUED);
	if (ret > 0)
	*ppos = kiocb.ki_pos;
	return ret;
}

/**
  * Copied from read_write.c
 **/
ssize_t vfs_iter_read(struct file *file, struct iov_iter *iter, loff_t *ppos)
{
	struct kiocb kiocb;
	ssize_t ret;

	if (!file->f_op->read_iter)
		return -EINVAL;

	init_sync_kiocb(&kiocb, file);
	kiocb.ki_pos = *ppos;

	iter->type |= READ;
	ret = file->f_op->read_iter(&kiocb, iter);
	BUG_ON(ret == -EIOCBQUEUED);
	if (ret > 0)
		*ppos = kiocb.ki_pos;
	return ret;
}

/**
  * Copied from iov_iter.c
 **/
inline void iov_iter_bvec(struct iov_iter *i, int direction, const struct bio_vec *bvec, unsigned long nr_segs, size_t count)
{
	BUG_ON(!(direction & ITER_BVEC));
	i->type = direction;
	i->bvec = bvec;
	i->nr_segs = nr_segs;
	i->iov_offset = 0;
	i->count = count;
}

#endif //LINUX_VERSION_CODE <= KERNEL_VERSION(3,19,0)

#endif //HELPERS_H

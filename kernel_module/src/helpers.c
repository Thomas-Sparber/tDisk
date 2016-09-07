#include "helpers.h"
#include "tdisk_tools_config.h"

#ifndef __ASM_ARM_DIV64
#ifndef __div64_32_DEFINED

uint32_t __div64_32(uint64_t *n, uint32_t base)
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
EXPORT_SYMBOL(__div64_32);

#endif //__div64_32_DEFINED
#endif //__ASM_ARM_DIV64

#ifndef vfs_iter_write_DEFINED

/**
  * Copied from read_write.c
 **/
ssize_t vfs_iter_write(struct file *file, struct iov_iter *iter, loff_t *ppos)
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
EXPORT_SYMBOL(vfs_iter_write);

#endif //vfs_iter_write_DEFINED

#ifndef vfs_iter_read_DEFINED

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
EXPORT_SYMBOL(vfs_iter_read);

#endif //vfs_iter_read_DEFINED

#ifndef iov_iter_bvec_DEFINED

/**
  * Copied from iov_iter.c
 **/
void iov_iter_bvec(struct iov_iter *i, int direction, const struct bio_vec *bvec, unsigned long nr_segs, size_t count)
{
	BUG_ON(!(direction & ITER_BVEC));
	i->type = direction;
	i->bvec = bvec;
	i->nr_segs = nr_segs;
	i->iov_offset = 0;
	i->count = count;
}
EXPORT_SYMBOL(iov_iter_bvec);

#endif //iov_iter_bvec_DEFINED

#ifndef TDISK_FILE_H
#define TDISK_FILE_H

#include <linux/timex.h>

#define RECORDS_SHIFT 16
#define PREVIOUS_RECORDS ((1 << RECORDS_SHIFT) - 1)
#define TIME_ONE_VALUE(val, mod) (val * (1 << RECORDS_SHIFT) + mod)

inline static loff_t file_get_size(struct file *file)
{
	return i_size_read(file->f_mapping->host) >> 9;
}

inline static int file_discard(struct file *file, struct request *rq, loff_t pos)
{
	//We use punch hole to reclaim the free space used by the image a.k.a. discard.
	int ret = 0;
	int mode = FALLOC_FL_PUNCH_HOLE | FALLOC_FL_KEEP_SIZE;

	if((!file->f_op->fallocate))
	{
		return -EOPNOTSUPP;
	}

	ret = file->f_op->fallocate(file, mode, pos, blk_rq_bytes(rq));

	if(unlikely(ret && ret != -EINVAL && ret != -EOPNOTSUPP))
		ret = -EIO;

	return ret;
}

inline static int file_flush(struct file *file)
{
	return vfs_fsync(file, 0);
}

inline static void file_update_performance(struct file *file, int direction, cycles_t time, struct device_performance *perf)
{
	unsigned long diff;

	if(perf == NULL)return;

	switch(direction)
	{
	case READ:
		if(perf->avg_read_time_cycles > time)
			diff = perf->avg_read_time_cycles-time;
		else diff = time-perf->avg_read_time_cycles;

		perf->avg_read_time_cycles *= PREVIOUS_RECORDS;
		perf->avg_read_time_cycles += time + perf->mod_avg_read;
		perf->stdev_read_time_cycles *= PREVIOUS_RECORDS;
		perf->stdev_read_time_cycles += diff + perf->mod_stdev_read;

		perf->mod_avg_read = perf->avg_read_time_cycles & PREVIOUS_RECORDS;
		perf->avg_read_time_cycles = perf->avg_read_time_cycles >> RECORDS_SHIFT;
		perf->mod_stdev_read = perf->stdev_read_time_cycles & PREVIOUS_RECORDS;
		perf->stdev_read_time_cycles = perf->stdev_read_time_cycles >> RECORDS_SHIFT;
		break;
	case WRITE:
		if(perf->avg_write_time_cycles > time)
			diff = perf->avg_write_time_cycles-time;
		else diff = time-perf->avg_write_time_cycles;

		perf->avg_write_time_cycles *= PREVIOUS_RECORDS;
		perf->avg_write_time_cycles += time + perf->mod_avg_write;
		perf->stdev_write_time_cycles *= PREVIOUS_RECORDS;
		perf->stdev_write_time_cycles += diff + perf->mod_stdev_write;

		perf->mod_avg_write = perf->avg_write_time_cycles & PREVIOUS_RECORDS;
		perf->avg_write_time_cycles = perf->avg_write_time_cycles >> RECORDS_SHIFT;
		perf->mod_stdev_write = perf->stdev_write_time_cycles & PREVIOUS_RECORDS;
		perf->stdev_write_time_cycles = perf->stdev_write_time_cycles >> RECORDS_SHIFT;
		break;
	}
}

inline static int file_write_bio_vec(struct file *file, struct bio_vec *bvec, loff_t *pos, struct device_performance *perf)
{
	int ret;
	struct iov_iter i;
	cycles_t time;

	iov_iter_bvec(&i, ITER_BVEC, bvec, 1, bvec->bv_len);

	time = get_cycles();
	file_start_write(file);
	ret = vfs_iter_write(file, &i, pos);
	file_end_write(file);
	file_update_performance(file, WRITE, get_cycles()-time, perf);

	return ret;
}

inline static int file_write_page(struct file *file, struct page *p, loff_t *pos, unsigned int length, struct device_performance *perf)
{
	struct bio_vec bvec = {
		.bv_page = p,
		.bv_len = length,
		.bv_offset = 0
	};

	return file_write_bio_vec(file, &bvec, pos, perf);
}

inline static int file_write_data(struct file *file, void *data, loff_t pos, unsigned int length, struct device_performance *perf)
{
	int len;
	int ret = 0;
	struct page *p = alloc_page(GFP_KERNEL);

	do
	{
		len = (length > PAGE_SIZE) ? PAGE_SIZE : length;

		memcpy(page_address(p), data, len);
		len = file_write_page(file, p, &pos, len, perf);

		if(unlikely(len < 0))
		{
			ret = len;
			break;
		}

		if(unlikely(len == 0))
		{
			ret = length;
			break;
		}

		data += len;
		length -= len;
	}
	while(length);

	__free_page(p);
	return ret;
}

inline static int file_read_bio_vec(struct file *file, struct bio_vec *bvec, loff_t *pos, struct device_performance *perf)
{
	int ret;
	struct iov_iter i;
	cycles_t time;
	iov_iter_bvec(&i, ITER_BVEC, bvec, 1, bvec->bv_len);

	time = get_cycles();
	ret = vfs_iter_read(file, &i, pos);
	//printk_ratelimited(KERN_DEBUG "tDisk: read start: %llu, end: %llu\n", time, get_cycles());
	file_update_performance(file, READ, get_cycles()-time, perf);

	return ret;
}

inline static int file_read_page(struct file *file, struct page *p, loff_t *pos, unsigned int length, struct device_performance *perf)
{
	struct bio_vec bvec = {
		.bv_page = p,
		.bv_len = length,
		.bv_offset = 0
	};

	return file_read_bio_vec(file, &bvec, pos, perf);
}

inline static int file_read_data(struct file *file, void *data, loff_t pos, unsigned int length, struct device_performance *perf)
{
	int len;
	int ret = 0;
	struct page *p = alloc_page(GFP_KERNEL);
	do
	{
		len = (length > PAGE_SIZE) ? PAGE_SIZE : length;
		len = file_read_page(file, p, &pos, len, perf);

		if(unlikely(len < 0))
		{
			ret = len;
			break;
		}

		memcpy(data, page_address(p), len);

		if(unlikely(len == 0))
		{
			ret = length;
			break;
		}

		data += len;
		length -= len;
	}
	while(length);

	__free_page(p);
	return ret;
}

#endif //TDISK_FILE_H

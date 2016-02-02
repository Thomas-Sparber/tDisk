/**
  *
  * tDisk Driver
  * @author Thomas Sparber (2015)
  *
 **/

#ifndef TDISK_FILE_H
#define TDISK_FILE_H

#include <linux/timex.h>
#include <linux/version.h>
#include "../include/tdisk/interface.h"

/**
  * Calculates the amount of measured records-1
  * @see file_update_performance
 **/
#define PREVIOUS_RECORDS ((1 << MEASUE_RECORDS_SHIFT) - 1)

/**
  * If the performance of ONE operation was measured
  * the reult is normally very low (e.g. 3 cycles) because
  * it was averaged over the last (1 << MEASUE_RECORDS_SHIFT)
  * operations.
  * This macro calculates the time back to the actual amount
  * of cycles.
 **/
#define TIME_ONE_VALUE(val, mod) (val * (1 << MEASUE_RECORDS_SHIFT) + mod)

/**
  * Returns whether the given file is a tDisk
 **/
static inline int file_is_tdisk(struct file *file, int MAJOR_NUMBER)
{
	struct inode *i = file->f_mapping->host;

	return i && S_ISBLK(i->i_mode) && MAJOR(i->i_rdev) == MAJOR_NUMBER;
}

/**
  * Returns the size of a file in 512 byte blocks
 **/
inline static loff_t file_get_size(struct file *file)
{
	return i_size_read(file->f_mapping->host) >> 9;
}

/**
  * Allocates the given space in the given file
 **/
inline static int file_alloc(struct file *file, loff_t pos, unsigned int length)
{
	//We use punch hole to reclaim the free space used by the image a.k.a. discard.
	int ret = 0;
	int mode = FALLOC_FL_PUNCH_HOLE | FALLOC_FL_KEEP_SIZE;

	if((!file->f_op->fallocate))
		return -EOPNOTSUPP;

	ret = file->f_op->fallocate(file, mode, pos, length);

	if(unlikely(ret && ret != -EINVAL && ret != -EOPNOTSUPP))
		ret = -EIO;

	return ret;
}

/**
  * Flushes the given file
 **/
inline static int file_flush(struct file *file)
{
	return vfs_fsync(file, 0);
}

/**
  * This function updates the performance data
  * of the given device. Actually it calculates
  * the average and standard deviation over the
  * last (1 << MEASUE_RECORDS_SHIFT) requests
 **/
inline static void file_update_performance(struct file *file, int direction, cycles_t time, struct device_performance *perf)
{
	unsigned long diff;

	//time is 0 on platforms which have no cycles measure
	WARN_ONCE(time == 0, "Your processor doesn't count cycles. Performances values may be wrong!");
	if(perf == NULL || time == 0)return;

	switch(direction)
	{
	case READ:
		//Avg difference
		if(perf->avg_read_time_cycles > time)
			diff = perf->avg_read_time_cycles-time;
		else diff = time-perf->avg_read_time_cycles;

		//The following lines calculate the following
		//equation using bitshift for better performance
		//avg = (avg*(records_count-1) + current_time) / (records_count)
		perf->avg_read_time_cycles *= PREVIOUS_RECORDS;
		perf->avg_read_time_cycles += time + perf->mod_avg_read;
		perf->stdev_read_time_cycles *= PREVIOUS_RECORDS;
		perf->stdev_read_time_cycles += diff + perf->mod_stdev_read;

		perf->mod_avg_read = perf->avg_read_time_cycles & PREVIOUS_RECORDS;
		perf->avg_read_time_cycles = perf->avg_read_time_cycles >> MEASUE_RECORDS_SHIFT;
		perf->mod_stdev_read = perf->stdev_read_time_cycles & PREVIOUS_RECORDS;
		perf->stdev_read_time_cycles = perf->stdev_read_time_cycles >> MEASUE_RECORDS_SHIFT;
		break;
	case WRITE:
		//Avg difference
		if(perf->avg_write_time_cycles > time)
			diff = perf->avg_write_time_cycles-time;
		else diff = time-perf->avg_write_time_cycles;

		//The following lines calculate the following
		//equation using bitshift for better performance
		//avg = (avg*(records_count-1) + current_time) / (records_count)
		perf->avg_write_time_cycles *= PREVIOUS_RECORDS;
		perf->avg_write_time_cycles += time + perf->mod_avg_write;
		perf->stdev_write_time_cycles *= PREVIOUS_RECORDS;
		perf->stdev_write_time_cycles += diff + perf->mod_stdev_write;

		perf->mod_avg_write = perf->avg_write_time_cycles & PREVIOUS_RECORDS;
		perf->avg_write_time_cycles = perf->avg_write_time_cycles >> MEASUE_RECORDS_SHIFT;
		perf->mod_stdev_write = perf->stdev_write_time_cycles & PREVIOUS_RECORDS;
		perf->stdev_write_time_cycles = perf->stdev_write_time_cycles >> MEASUE_RECORDS_SHIFT;
		break;
	}
}

/**
  * This function writes the given bio_vec to
  * file at the given position. It also measures the performance
 **/
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

/**
  * This function writes the given page to
  * file at the given position. It also measures the performance
 **/
inline static int file_write_page(struct file *file, struct page *p, loff_t *pos, unsigned int length, struct device_performance *perf)
{
	struct bio_vec bvec = {
		.bv_page = p,
		.bv_len = length,
		.bv_offset = 0
	};

	return file_write_bio_vec(file, &bvec, pos, perf);
}

/**
  * This function writes the given bytes to
  * file at the given position. It also measures the performance
 **/
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

/**
  * This function reads the given bio_vec from
  * file at the given position. It also measures the performance
 **/
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

/**
  * This function reads the given page from
  * file at the given position. It also measures the performance
 **/
inline static int file_read_page(struct file *file, struct page *p, loff_t *pos, unsigned int length, struct device_performance *perf)
{
	struct bio_vec bvec = {
		.bv_page = p,
		.bv_len = length,
		.bv_offset = 0
	};

	return file_read_bio_vec(file, &bvec, pos, perf);
}

/**
  * This function reads the given bytes from
  * file at the given position. It also measures the performance
 **/
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

/*************************** AIO *******************************/

struct aio_data
{
	struct kiocb iocb;
	void *private_data;
	void (*callback)(void*,long);

	int rw;
	struct device_performance *perf;
}; //end struct aio_data

inline static void lo_rw_aio_complete(struct kiocb *iocb, long ret, long ret2)
{
	struct aio_data *data = container_of(iocb, struct aio_data, iocb);

	if(data->callback)data->callback(data->private_data, ret);
	kfree(data);
}

/*inline static int lo_rw_aio(struct file *file, struct aio_data *data, loff_t pos, bool rw)
{
	struct iov_iter iter;
	struct bio_vec *bvec;
	struct bio *bio = data->rq->bio;
	int ret;

	//nomerge for request queue
	WARN_ON(data->rq->bio != data->rq->biotail);

	bvec = __bvec_iter_bvec(bio->bi_io_vec, bio->bi_iter);
	iov_iter_bvec(&iter, ITER_BVEC | rw, bvec, bio_segments(bio), blk_rq_bytes(data->rq));

	data->iocb.ki_pos = pos;
	data->iocb.ki_filp = file;
	data->iocb.ki_complete = lo_rw_aio_complete;
	data->iocb.ki_flags = IOCB_DIRECT;

	if(rw == WRITE)
		ret = file->f_op->write_iter(&data->iocb, &iter);
	else
		ret = file->f_op->read_iter(&data->iocb, &iter);

	if(ret != -EIOCBQUEUED)data->iocb.ki_complete(&data->iocb, ret, 0);
	return 0;
}*/

inline static void file_aio_request_callback(void *private_data, long bytes)
{
	struct request *rq = private_data;

	if(bytes > 0)
	{
		//Handle partial reads
		if(!(rq->cmd_flags & REQ_WRITE))
		{
			if(unlikely(bytes < blk_rq_bytes(rq)))
			{
				struct bio *bio = rq->bio;

				bio_advance(bio, bytes);
				zero_fill_bio(bio);
			}
		}

		bytes = 0;
	}
	else bytes = -EIO;

#if LINUX_VERSION_CODE <= KERNEL_VERSION(4,1,14)
	rq->errors = bytes;
	blk_mq_complete_request(rq);
#else
	blk_mq_complete_request(rq, bytes);
#endif //LINUX_VERSION_CODE <= KERNEL_VERSION(4,1,14)
}

inline static void file_aio_prepare_request(struct request *rq, struct iov_iter *iter, int rw)
{
	struct bio_vec *bvec;
	struct bio *bio = rq->bio;

	//nomerge for request queue
	WARN_ON(rq->bio != rq->biotail);

	bvec = __bvec_iter_bvec(bio->bi_io_vec, bio->bi_iter);
	iov_iter_bvec(iter, ITER_BVEC | rw, bvec, bio_segments(bio), blk_rq_bytes(rq));
}

inline static void file_aio_init_aio_data(struct aio_data *data, struct file *file, loff_t pos, void *private_data, void (*callback)(void*,long))
{
	data->iocb.ki_pos = pos;
	data->iocb.ki_filp = file;
	data->iocb.ki_complete = lo_rw_aio_complete;
	data->iocb.ki_flags = IOCB_DIRECT;

	data->private_data = private_data;
	data->callback = callback;
}

inline static int file_aio_read_request(struct file *file, struct request *rq, loff_t pos)
{
	int ret;
	struct iov_iter iter;
	struct aio_data *data = kmalloc(sizeof(struct aio_data), GFP_KERNEL);

	file_aio_prepare_request(rq, &iter, READ);

	if(!data)return -ENOMEM;

	file_aio_init_aio_data(data, file, pos, rq, file_aio_request_callback);

	//if(rw == WRITE)
	//	ret = file->f_op->write_iter(&data->iocb, &iter);
	//else
		ret = file->f_op->read_iter(&data->iocb, &iter);

	if(ret != -EIOCBQUEUED)data->iocb.ki_complete(&data->iocb, ret, 0);
	return 0;
}

#endif //TDISK_FILE_H

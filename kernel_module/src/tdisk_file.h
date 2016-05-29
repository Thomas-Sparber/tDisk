/**
  *
  * tDisk Driver
  * @author Thomas Sparber (2015-2016)
  *
 **/

#ifndef TDISK_FILE_H
#define TDISK_FILE_H

#include <tdisk/config.h>
#include <tdisk/interface.h>
#include "tdisk_performance.h"

#ifdef USE_FILES

#pragma GCC system_header
#include <linux/falloc.h>
#include <linux/file.h>
#include <linux/uio.h>
#include <linux/version.h>

/**
  * Returns whether the given file is a tDisk
 **/
static inline int file_is_tdisk(struct file *file, int MAJOR_NUMBER)
{
	struct inode *i = file->f_mapping->host;

	return i && S_ISBLK(i->i_mode) && MAJOR(i->i_rdev) == MAJOR_NUMBER;
}

/**
  * Returns the size of a file 
 **/
inline static loff_t file_get_size(struct file *file)
{
	return i_size_read(file->f_mapping->host);
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
  * This function writes the given bio_vec to
  * file at the given position. It also measures the performance
 **/
inline static int file_write_bio_vec(struct file *file, struct bio_vec *bvec, loff_t *pos, struct device_performance *perf)
{
	int ret;
	struct iov_iter i;

#ifdef MEASURE_PERFORMANCE
	cycles_t time = get_cycles();
#else
#pragma message "Performance measurement is disabled"
#endif //MEASURE_PERFORMANCE

	iov_iter_bvec(&i, ITER_BVEC, bvec, 1, bvec->bv_len);

	file_start_write(file);
	ret = vfs_iter_write(file, &i, pos);
	file_end_write(file);

#ifdef MEASURE_PERFORMANCE
	update_performance(WRITE, get_cycles()-time, perf);
#else
#pragma message "Performance measurement is disabled"
#endif //MEASURE_PERFORMANCE

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

#ifdef MEASURE_PERFORMANCE
	cycles_t time = get_cycles();
#else
#pragma message "Performance measurement is disabled"
#endif //MEASURE_PERFORMANCE

	iov_iter_bvec(&i, ITER_BVEC, bvec, 1, bvec->bv_len);

	ret = vfs_iter_read(file, &i, pos);

#ifdef MEASURE_PERFORMANCE
	update_performance(READ, get_cycles()-time, perf);
#else
#pragma message "Performance measurement is disabled"
#endif //MEASURE_PERFORMANCE

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
	cycles_t time;
}; //end struct aio_data

struct multi_aio_data
{
	atomic_t ret;
	atomic_t remaining;
	long length;
	void *private_data;
	void (*callback)(void*,long);
}; //end struct aio_data

inline static void file_aio_complete(struct kiocb *iocb, long ret, long ret2)
{
	printk(KERN_DEBUG "tDisk: AIO completed, ret: %ld\n", ret);
	struct aio_data *data = container_of(iocb, struct aio_data, iocb);

#ifdef MEASURE_PERFORMANCE
	update_performance(data->rw, get_cycles()-data->time, data->perf);
#else
#pragma message "Performance measurement is disabled"
#endif //MEASURE_PERFORMANCE

	if(data->callback)data->callback(data->private_data, ret);
	kfree(data);
}

inline static void file_multi_aio_complete(void *private_data, long ret)
{
	struct multi_aio_data *data = private_data;

	if(ret < 0)atomic_inc(&data->ret);

	printk(KERN_DEBUG "tDisk: Multi AIO completed, ret: %ld, %d msissing\n", ret, atomic_read(&data->remaining)-1);
	if(atomic_dec_and_test(&data->remaining))
	{
		if(unlikely(atomic_read(&data->ret) != 0))data->length = -EIO;

		if(data->callback)data->callback(data->private_data, data->length);

		kfree(data);
	}
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
	data->iocb.ki_complete = file_aio_complete;
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

/**
  * This function writes the given bio_vec asyncronously to
  * file at the given position. It also measures the performance
 **/
inline static int file_write_bio_vec_async(struct file *file, struct bio_vec *bvec, loff_t pos, struct device_performance *perf, void *private_data, void (*callback)(void*,long))
{
	int ret;
	struct iov_iter iter;
	struct aio_data *data = kmalloc(sizeof(struct aio_data), GFP_KERNEL);

	if(!data)
	{
		if(callback)callback(private_data, -ENOMEM);
		return;
	}

	iov_iter_bvec(&iter, ITER_BVEC/* | WRITE*/, bvec, 1, bvec->bv_len);

	data->iocb.ki_pos = pos;
	data->iocb.ki_filp = file;
	data->iocb.ki_complete = file_aio_complete;
	//data->iocb.ki_flags = IOCB_DIRECT;

	data->perf = perf;
	data->private_data = private_data;
	data->callback = callback;

#ifdef MEASURE_PERFORMANCE
	data->time = get_cycles();
#else
#pragma message "Performance measurement is disabled"
#endif //MEASURE_PERFORMANCE

	printk(KERN_DEBUG "tDisk: Sending async read file request\n");
	ret = file->f_op->write_iter(&data->iocb, &iter);
	printk(KERN_DEBUG "tDisk: Done Sending async write file request: %d\n", ret);

	if(ret != -EIOCBQUEUED)
	{
		if(callback)callback(private_data, ret);
	}
}

/**
  * This function writes the given page asyncronously to
  * file at the given position. It also measures the performance
 **/
inline static void file_write_page_async(struct file *file, struct page *p, loff_t pos, unsigned int length, struct device_performance *perf, void *private_data, void (*callback)(void*,long))
{
	struct bio_vec bvec = {
		.bv_page = p,
		.bv_len = length,
		.bv_offset = 0
	};

	file_write_bio_vec_async(file, &bvec, pos, perf, private_data, callback);
}

/**
  * This function writes the given bytes asyncronously to
  * file at the given position. It also measures the performance
 **/
inline static void file_write_data_async(struct file *file, void *data, loff_t pos, unsigned int length, struct device_performance *perf, void *private_data, void (*callback)(void*,long))
{
	int len;
	struct page *p = alloc_page(GFP_KERNEL);
	struct multi_aio_data *multi_data = kmalloc(sizeof(struct multi_aio_data), GFP_KERNEL);

	atomic_set(&multi_data->ret, 0);
	atomic_set(&multi_data->remaining, 1);
	multi_data->length = length;
	multi_data->private_data = private_data;
	multi_data->callback = callback;

	do
	{
		len = (length > PAGE_SIZE) ? PAGE_SIZE : length;

		memcpy(page_address(p), data, len);
		atomic_inc(&multi_data->remaining);
		file_write_page_async(file, p, pos, len, perf, multi_data, &file_multi_aio_complete);

		data += len;
		length -= len;
	}
	while(length);

	__free_page(p);

	file_multi_aio_complete(multi_data, 0);
}

/**
  * This function reads the given bio_vec asyncronously to
  * file at the given position. It also measures the performance
 **/
inline static int file_read_bio_vec_async(struct file *file, struct bio_vec *bvec, loff_t pos, struct device_performance *perf, void *private_data, void (*callback)(void*,long))
{
	int ret;
	struct iov_iter iter;
	struct aio_data *data = kmalloc(sizeof(struct aio_data), GFP_KERNEL);

	if(!data)
	{
		if(callback)callback(private_data, -ENOMEM);
		return;
	}

	iov_iter_bvec(&iter, ITER_BVEC/* | READ*/, bvec, 1, bvec->bv_len);

	data->iocb.ki_pos = pos;
	data->iocb.ki_filp = file;
	data->iocb.ki_complete = file_aio_complete;
	//data->iocb.ki_flags = IOCB_DIRECT;

	data->perf = perf;
	data->private_data = private_data;
	data->callback = callback;

#ifdef MEASURE_PERFORMANCE
	data->time = get_cycles();
#else
#pragma message "Performance measurement is disabled"
#endif //MEASURE_PERFORMANCE

	printk(KERN_DEBUG "tDisk: Sending async read file request\n");
	ret = file->f_op->read_iter(&data->iocb, &iter);
	printk(KERN_DEBUG "tDisk: Done Sending async read file request: %d\n", ret);

	if(ret != -EIOCBQUEUED)
	{
		printk(KERN_DEBUG "tDisk: Async file request returned an error: %d\n", ret);
		if(callback)callback(private_data, ret);
	}
}

/**
  * This function reads the given page asyncronously to
  * file at the given position. It also measures the performance
 **/
inline static void file_read_page_async(struct file *file, struct page *p, loff_t pos, unsigned int length, struct device_performance *perf, void *private_data, void (*callback)(void*,long))
{
	struct bio_vec bvec = {
		.bv_page = p,
		.bv_len = length,
		.bv_offset = 0
	};

	file_read_bio_vec_async(file, &bvec, pos, perf, private_data, callback);
}

/**
  * This function reads the given bytes asyncronously to
  * file at the given position. It also measures the performance
 **/
inline static void file_read_data_async(struct file *file, void *data, loff_t pos, unsigned int length, struct device_performance *perf, void *private_data, void (*callback)(void*,long))
{
	int len;
	struct page *p = alloc_page(GFP_KERNEL);
	struct multi_aio_data *multi_data = kmalloc(sizeof(struct multi_aio_data), GFP_KERNEL);

	atomic_set(&multi_data->ret, 0);
	atomic_set(&multi_data->remaining, 1);
	multi_data->length = length;
	multi_data->private_data = private_data;
	multi_data->callback = callback;

	do
	{
		len = (length > PAGE_SIZE) ? PAGE_SIZE : length;

		memcpy(page_address(p), data, len);
		atomic_inc(&multi_data->remaining);
		file_read_page_async(file, p, pos, len, perf, multi_data, &file_multi_aio_complete);

		data += len;
		length -= len;
	}
	while(length);

	__free_page(p);

	file_multi_aio_complete(multi_data, 0);
}

#else
#pragma message "Files are disabled"
#endif //USE_FILES

#endif //TDISK_FILE_H

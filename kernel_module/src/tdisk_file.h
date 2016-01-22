#ifndef TDISK_FILE_H
#define TDISK_FILE_H

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

inline static int file_write_bio_vec(struct file *file, struct bio_vec *bvec, loff_t *pos)
{
	int ret;
	struct iov_iter i;

	iov_iter_bvec(&i, ITER_BVEC, bvec, 1, bvec->bv_len);
	file_start_write(file);
	ret = vfs_iter_write(file, &i, pos);
	file_end_write(file);

	return ret;
}

inline static int file_write_page(struct file *file, struct page *p, loff_t *pos, unsigned int length)
{
	struct bio_vec bvec = {
		.bv_page = p,
		.bv_len = length,
		.bv_offset = 0
	};

	return file_write_bio_vec(file, &bvec, pos);
}

inline static int file_write_data(struct file *file, void *data, loff_t pos, unsigned int length)
{
	int len;
	int ret = 0;
	struct page *p = alloc_page(GFP_KERNEL);

	do
	{
		len = (length > PAGE_SIZE) ? PAGE_SIZE : length;

		memcpy(page_address(p), data, len);
		len = file_write_page(file, p, &pos, len);

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

inline static int file_read_bio_vec(struct file *file, struct bio_vec *bvec, loff_t *pos)
{
	struct iov_iter i;
	iov_iter_bvec(&i, ITER_BVEC, bvec, 1, bvec->bv_len);
	return vfs_iter_read(file, &i, pos);
}

inline static int file_read_page(struct file *file, struct page *p, loff_t *pos, unsigned int length)
{
	struct bio_vec bvec = {
		.bv_page = p,
		.bv_len = length,
		.bv_offset = 0
	};

	return file_read_bio_vec(file, &bvec, pos);
}

inline static int file_read_data(struct file *file, void *data, loff_t pos, unsigned int length)
{
	int len;
	int ret = 0;
	struct page *p = alloc_page(GFP_KERNEL);
//printk(KERN_DEBUG "tDisk: New request: Pos: %llu, Len: %u\n", pos, length);
	do
	{
		len = (length > PAGE_SIZE) ? PAGE_SIZE : length;
		//printk(KERN_DEBUG "tDisk: Pos: %llu, Len: %u, %u\n", pos, len, length);
		len = file_read_page(file, p, &pos, len);
		//printk(KERN_DEBUG "tDisk: After read: Pos %llu, Len: %u, %u\n", pos, len, length);

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
//printk(KERN_DEBUG "tDisk: Request result %d\n", ret);
	__free_page(p);
	return ret;
}

#endif //TDISK_FILE_H

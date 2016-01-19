/**
  *
  * tDisk Driver
  * @author Thomas Sparber (2015)
  *
 **/

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/fs.h>
#include <linux/file.h>
#include <linux/stat.h>
#include <linux/errno.h>
#include <linux/major.h>
#include <linux/blkdev.h>
#include <linux/blkpg.h>
#include <linux/miscdevice.h>
#include <linux/falloc.h>
#include <linux/uio.h>
#include <linux/version.h>

#include "helpers.h"
#include "tdisk.h"
#include "tdisk_control.h"

#define COMPARE 1410

static u_int TD_MAJOR = 0;
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Thomas Sparber <thomas@sparber.eu>");
MODULE_DESCRIPTION(DRIVER_NAME " Driver");
MODULE_ALIAS_BLOCKDEV_MAJOR(TD_MAJOR);

DEFINE_IDR(td_index_idr);


static loff_t get_file_size(struct file *file)
{
	return i_size_read(file->f_mapping->host) >> 9;
}

static int td_discard(struct tdisk *td, struct request *rq, loff_t pos)
{
	/*
	 * We use punch hole to reclaim the free space used by the
	 * image a.k.a. discard.
	 */
	unsigned int i;
	int ret = 0;
	int internal_ret = 0;
	for(i = 0; i < td->internal_devices_count; ++i)
	{
		struct file *file = td->internal_devices[i].backing_file;
		if(file)
		{
			int mode = FALLOC_FL_PUNCH_HOLE | FALLOC_FL_KEEP_SIZE;

			if((!file->f_op->fallocate))
			{
				ret = -EOPNOTSUPP;
				continue;
			}

			internal_ret = file->f_op->fallocate(file, mode, pos, blk_rq_bytes(rq));
			if(internal_ret)ret = internal_ret;

			if(unlikely(ret && ret != -EINVAL && ret != -EOPNOTSUPP))
				ret = -EIO;
		}
	}

	return ret;
}

/*
 * Flush td device
 */
static int td_flush(struct tdisk *td)
{
	//freeze queue and wait for completion of scheduled requests
	blk_mq_freeze_queue(td->queue);

	//unfreeze
	blk_mq_unfreeze_queue(td->queue);

	return 0;
}

static int td_req_flush(struct tdisk *td)
{
	unsigned int i;
	int ret = 0;
	for(i = 0; i < td->internal_devices_count; ++i)
	{
		struct file *file = td->internal_devices[i].backing_file;
		if(file)
		{
			int internal_ret = vfs_fsync(file, 0);

			if(unlikely(internal_ret && internal_ret != -EINVAL))
				ret = -EIO;
		}
	}

	return ret;
}

int perform_index_operation(struct tdisk *td_dev, int direction, sector_t logical_sector, struct sector_index *physical_sector)
{
	loff_t position = td_dev->index_offset_byte + logical_sector * sizeof(struct sector_index);
	unsigned int length = sizeof(struct sector_index);

	if(position + length > td_dev->header_size * td_dev->blocksize)return 1;

	//Increment access count
	((struct sector_index*)td_dev->indices+position)->access_count++;

	//index operation
	if(direction == READ)
		memcpy(physical_sector, td_dev->indices+position, length);
	else if(direction == COMPARE)
	{
		struct sector_index *cmp = (struct sector_index*)td_dev->indices+position;
		if(physical_sector->disk != cmp->disk || physical_sector->sector != cmp->sector)return -1;
	}
	else if(direction == WRITE)
	{
		int ret = 0;
		int len;
		unsigned int j;
		struct iov_iter i;
		struct page *p = alloc_page(GFP_KERNEL);
		struct bio_vec bvec = {
			.bv_page = p,
			.bv_len = PAGE_SIZE,
			.bv_offset = 0
		};

		(*(struct sector_index*)page_address(p)) = (*physical_sector);

		//Memory operation
		memcpy(td_dev->indices+position, physical_sector, length);

		//Disk operations
		for(j = 0; j < td_dev->internal_devices_count; ++j)
		{
			struct file *file = td_dev->internal_devices[j].backing_file;
			//printk(KERN_DEBUG "tDisk: File: %p\n", file);

			if(file)
			{
				loff_t pos = position;
				iov_iter_bvec(&i, ITER_BVEC, &bvec, 1, sizeof(struct sector_index));

				file_start_write(file);
				len = vfs_iter_write(file, &i, &pos);
				file_end_write(file);

				if(len < 0)ret = len;
				if(len < sizeof(struct sector_index))ret = -EIO;
			}
		}

		__free_page(p);
		if(ret)return ret;
	}

	return 0;
}

static int read_header(struct file *file, struct tdisk_header *out)
{
	int ret = 0;
	int len;
	loff_t pos = 0;
	struct iov_iter i;
	struct page *p = alloc_page(GFP_KERNEL);
	struct bio_vec bvec = {
		.bv_page = p,
		.bv_len = PAGE_SIZE,
		.bv_offset = 0
	};

	iov_iter_bvec(&i, ITER_BVEC, &bvec, 1, sizeof(struct tdisk_header));
	len = vfs_iter_read(file, &i, &pos);

	if(len < 0)
	{
		ret = len;
		goto out;
	}

	if(len < sizeof(*out))
	{
		ret = -EIO;
		goto out;
	}

	(*out) = *(struct tdisk_header*)page_address(p);

 out:
	__free_page(p);
	return ret;
}

static int write_header(struct file *file, struct tdisk_header *header)
{
	int ret = 0;
	int len;
	loff_t pos = 0;
	struct iov_iter i;
	struct page *p = alloc_page(GFP_KERNEL);
	struct bio_vec bvec = {
		.bv_page = p,
		.bv_len = PAGE_SIZE,
		.bv_offset = 0
	};

	memset(header->driver_name, 0, sizeof(header->driver_name));
	strcpy(header->driver_name, DRIVER_NAME);
	header->driver_name[sizeof(header->driver_name)-1] = 0;
	header->major_version = DRIVER_MAJOR_VERSION;
	header->minor_version = DRIVER_MINOR_VERSION;
	(*(struct tdisk_header*)page_address(p)) = (*header);

	iov_iter_bvec(&i, ITER_BVEC, &bvec, 1, sizeof(struct tdisk_header));
	file_start_write(file);
	len = vfs_iter_write(file, &i, &pos);
	file_end_write(file);

	if(len < 0)
	{
		ret = len;
		goto out;
	}

	if(len < sizeof(struct tdisk_header))
	{
		ret = -EIO;
		goto out;
	}

 out:
	__free_page(p);
	return ret;
}

static int read_all_indices(struct tdisk *td, struct file *file)
{
	int ret = 0;
	int len;
	loff_t pos = sizeof(struct tdisk_header);	//Skip header
	loff_t old_pos;
	struct iov_iter i;
	struct page *p = alloc_page(GFP_KERNEL);
	struct bio_vec bvec = {
		.bv_page = p,
		.bv_len = PAGE_SIZE,
		.bv_offset = 0
	};

	do
	{
		old_pos = pos;
		iov_iter_bvec(&i, ITER_BVEC, &bvec, 1, PAGE_SIZE);
		len = vfs_iter_read(file, &i, &pos);

		if(len < 0)
		{
			ret = len;
			break;
		}

		if(old_pos+len > td->header_size*td->blocksize)
			len = td->header_size*td->blocksize - old_pos;

		printk(KERN_DEBUG "tDisk: reading indices %llu - %llu", old_pos, old_pos+len);
		memcpy(td->indices+old_pos, page_address(p), len);
	}
	while(pos < td->header_size*td->blocksize);

	if(ret)printk(KERN_ERR "tDisk: Error reading all disk indices: %d\n", ret);
	else printk(KERN_DEBUG "tDisk: Success reading all disk indices\n");

	__free_page(p);
	return ret;
}

static int write_all_indices(struct tdisk *td, struct file *file)
{
	int ret = 0;
	int len;
	loff_t pos = sizeof(struct tdisk_header);	//Skip header
	struct iov_iter i;
	struct page *p = alloc_page(GFP_KERNEL);
	struct bio_vec bvec = {
		.bv_page = p,
		.bv_len = PAGE_SIZE,
		.bv_offset = 0
	};

	do
	{
		len = PAGE_SIZE;
		if(pos+len > td->header_size*td->blocksize)len = td->header_size*td->blocksize - pos;

		printk(KERN_DEBUG "tDisk: writing indices %llu - %llu", pos, pos+len);
		memcpy(page_address(p), td->indices+pos, len);

		iov_iter_bvec(&i, ITER_BVEC, &bvec, 1, len);
		file_start_write(file);
		len = vfs_iter_write(file, &i, &pos);
		file_end_write(file);

		if(len < 0)
		{
			ret = len;
			break;
		}
	}
	while(pos < td->header_size*td->blocksize);

	if(ret)printk(KERN_ERR "tDisk: Error reading all disk indices: %d\n", ret);
	else printk(KERN_DEBUG "tDisk: Success writing all disk indices\n");

	__free_page(p);
	return ret;
}

static int is_compatible_header(struct tdisk *td, struct tdisk_header *header)
{
	if(strcmp(header->driver_name, DRIVER_NAME) != 0)return 1;

	if(header->major_version == DRIVER_MAJOR_VERSION)
	{
		if(header->minor_version > DRIVER_MINOR_VERSION)return -1;
	}
	else if(header->major_version > DRIVER_MAJOR_VERSION)return -2;

	return 0;
}

static int do_req_filebacked(struct tdisk *td, struct request *rq)
{
	struct bio_vec bvec;
	struct req_iterator iter;
	struct iov_iter i;
	ssize_t len;
	loff_t pos_byte;
	int ret = 0;
	struct sector_index physical_sector;

	pos_byte = (loff_t)blk_rq_pos(rq) << 9;

	//Handle flush operations
	if((rq->cmd_flags & REQ_WRITE) && (rq->cmd_flags & REQ_FLUSH))
	{
		ret = td_req_flush(td);
		goto finished;
	}

	//Handle discard operations
	if((rq->cmd_flags & REQ_WRITE) && (rq->cmd_flags & REQ_DISCARD))
	{
		ret = td_discard(td, rq, pos_byte);
		goto finished;
	}

	//Normal file operations
	rq_for_each_segment(bvec, rq, iter)
	{
		struct file *file;
		loff_t sector = pos_byte;
		loff_t offset = __div64_32(&sector, td->blocksize);
		sector_t actual_pos_byte;
		sector += td->header_size;

		//index.offset = offset;
		//index.logical_sector = sector;
		perform_index_operation(td, READ, sector, &physical_sector);

		actual_pos_byte = physical_sector.sector*td->blocksize + offset;

		if(physical_sector.disk == 0 || physical_sector.disk > td->internal_devices_count)
		{
			printk_ratelimited(KERN_ERR "tDisk: found invalid disk index for reading logical sector %llu: %u\n", sector, physical_sector.disk);
			ret = -EIO;
			break;
		}

		file = td->internal_devices[physical_sector.disk - 1].backing_file;	//-1 because 0 means unused
		if(!file)
		{
			printk_ratelimited(KERN_DEBUG "tDisk: backing_file is NULL, Disk probably not yet loaded...\n");
			ret = -EIO;
			continue; //TODO continue or break??
		}

		/*printk(KERN_DEBUG "tDisk: Logical sector:%llu (%llu), disk:%u, physical sector:%llu, offset: %llu (=%llu)\n",
								sector,
								pos_byte,
								physical_sector.disk,
								physical_sector.sector,
								offset,
								actual_pos_byte);*/

		iov_iter_bvec(&i, ITER_BVEC, &bvec, 1, bvec.bv_len);

		if(rq->cmd_flags & REQ_WRITE)
		{
			file_start_write(file);
			len = vfs_iter_write(file, &i, &actual_pos_byte);
			file_end_write(file);

			if(unlikely(len != bvec.bv_len))
			{
				printk(KERN_ERR "tDisk: Write error at byte offset %llu, length %i.\n", (unsigned long long)pos_byte, bvec.bv_len);

				if(len >= 0)ret = -EIO;
				else ret = len;
				break;
			}
		}
		else
		{
			len = vfs_iter_read(file, &i, &actual_pos_byte);

			if(len < 0)
			{
				ret = len;
				break;
			}

			flush_dcache_page(bvec.bv_page);

			if(len != bvec.bv_len)
			{
				struct bio *bio;
				__rq_for_each_bio(bio, rq)zero_fill_bio(bio);
				break;
			}
		}

		pos_byte += len;
		cond_resched();
	}

 finished:
	return ret;
}

static inline int is_tdisk(struct file *file)
{
	struct inode *i = file->f_mapping->host;

	return i && S_ISBLK(i->i_mode) && MAJOR(i->i_rdev) == TD_MAJOR;
}

static void reread_partitions(struct tdisk *td, struct block_device *bdev)
{
#if 0
	int rc;

	/**
	  * bd_mutex has been held already in release path, so don't
	  * acquire it if this function is called in such case.
	  *
	  * If the reread partition isn't from release path, lo_refcnt
	  * must be at least one and it can only become zero when the
	  * current holder is released.
	 **/
	if(!atomic_read(&td->refcount))
		rc = __blkdev_reread_part(bdev);
	else
		rc = blkdev_reread_part(bdev);

	if(rc)pr_warn("tDisk: partition scan of loop%d failed (rc=%d)\n", td->number, rc);
#endif
	ioctl_by_bdev(bdev, BLKRRPART, 0);
}

static void unprepare_queue(struct tdisk *td)
{
	printk(KERN_DEBUG "tDisk: Unpreparing\n");
	flush_kthread_worker(&td->worker);
	kthread_stop(td->worker_task);
	printk(KERN_DEBUG "tDisk: Unprepared\n");
}

static int prepare_queue(struct tdisk *td)
{
	init_kthread_worker(&td->worker);
	td->worker_task = kthread_run(kthread_worker_fn, &td->worker, "td%d", td->number);
	if(IS_ERR(td->worker_task))
		return -ENOMEM;
	set_user_nice(td->worker_task, MIN_NICE);
	return 0;
}

static int tdisk_set_fd(struct tdisk *td, fmode_t mode, struct block_device *bdev, unsigned int arg)
{
	struct file	*file;
	struct inode *inode;
	struct address_space *mapping;
	struct tdisk_header header;
	int flags = 0;
	int error;
	loff_t size;
	loff_t size_counter = 0;
	sector_t sector = 0;
	struct sector_index physical_sector;
	struct td_internal_device *new_device = NULL;
	int index_operation_to_do;
	int first_device = (td->internal_devices_count == 0);

	//This is safe, since we have a reference from open().
	if(first_device)
		__module_get(THIS_MODULE);

	error = -EBADF;
	file = fget(arg);
	if(!file)
		goto out;

	//Don't add current tDisk as backend file
	if(is_tdisk(file) && file->f_mapping->host->i_bdev == bdev)
	{
		printk(KERN_WARNING "tDisk: Can't add myself as a backend file\n");
		goto out_putf;
	}

	mapping = file->f_mapping;
	inode = mapping->host;

	error = -EINVAL;
	if(!S_ISREG(inode->i_mode) && !S_ISBLK(inode->i_mode))
	{
		printk(KERN_WARNING "tDisk: Given file is not a regular file and not a blockdevice\n");
		goto out_putf;
	}

	if(!(file->f_mode & FMODE_WRITE) || !(mode & FMODE_WRITE) || !file->f_op->write_iter)
	{
		if(td->internal_devices_count && !(td->flags & TD_FLAGS_READ_ONLY))
		{
			printk(KERN_WARNING "Can't add a readonly file to a non read only device!\n");
			error = -EPERM;
			goto out_putf;
		}

		flags |= TD_FLAGS_READ_ONLY;
	}

	//File size in 512 byte blocks
	size = get_file_size(file);

	error = -EINVAL;
	if(size < (((td->header_size+1)*td->blocksize) >> 9))	//Disk too small
	{
		printk(KERN_WARNING "tDisk: Can't add disk, too small: %llu. Should be at least %u\n", size, (((td->header_size+1)*td->blocksize) >> 9));
		goto out_putf;
	}

	size -= (td->header_size*td->blocksize) >> 9;

	error = -EFBIG;
	if((loff_t)(sector_t)size != size)
		goto out_putf;

	if(first_device)
	{
		error = prepare_queue(td);
		if(error)
		{
			printk(KERN_WARNING "tDisk: Error setting up queue\n");
			goto out_putf;
		}
	}

	if(td->internal_devices_count == TDISK_MAX_PHYSICAL_DISKS)
	{
		printk(KERN_ERR "tDisk: Limit (255) of devices reached.\n");
		error = -ENODEV;
		goto out_putf;
	}

	read_header(file, &header);
	printk(KERN_DEBUG "tDisk: File header: driver: %s, minor: %u, major: %u\n", header.driver_name, header.major_version, header.minor_version);
	switch(is_compatible_header(td, &header))
	{
	case 0:
		//OK, compatible header.
		//Either same or lower (compatible) driver version
		//Disk was already part of a tDisk.
		//So doing nothing to preserve indices etc.
		index_operation_to_do = (first_device) ? READ : COMPARE;
		break;
	case 1:
		//Entirely new disk.
		header.disk_index = td->internal_devices_count;
		index_operation_to_do = WRITE;
		break;
	case -1:
		//Oops, the disk was part of a tdisk
		//But using a driver with a higher minor number
		//Assuming that user space knows if it's compatible...
		//Doing nothing
		index_operation_to_do = (first_device) ? READ : COMPARE;
		break;
	case -2:
		//Oops, the disk was part of a tdisk
		//But using a driver with a higher major number
		//Assuming that user space knows if it's compatible...
		//Doing nothing
		index_operation_to_do = (first_device) ? READ : COMPARE;
		break;
	default:
		//Bug
		printk(KERN_ERR "tDisk: Bug in is_compatible_header function\n");
		goto out_putf;
	}
	if(header.disk_index >= td->internal_devices_count)td->internal_devices_count = header.disk_index+1;

	new_device = &td->internal_devices[header.disk_index];
	new_device->old_gfp_mask = mapping_gfp_mask(mapping);
	mapping_set_gfp_mask(mapping, new_device->old_gfp_mask & ~(__GFP_IO|__GFP_FS));
	new_device->speed = 0;	//TODO
	new_device->backing_file = file;

	//printk(KERN_DEBUG "tDisk: %u devices, index operation: %s\n", td->internal_devices_count, index_operation_to_do == WRITE ? "write" : index_operation_to_do == READ ? "read" : "compare");

	error = 0;

	set_device_ro(bdev, (flags & TD_FLAGS_READ_ONLY) != 0);

	td->block_device = bdev;
	td->flags = flags;

	switch(index_operation_to_do)
	{
	case WRITE:
		//Writing header to disk
		write_header(file, &header);
		write_all_indices(td, file);
		//IMPORTANT: continuing and writing indices as well...
	case COMPARE:
		//Save sector indices
		while((size_counter+td->blocksize) <= (size << 9))
		{
			int internal_ret;
			sector_t logical_sector = td->header_size + td->size_blocks++;
			size_counter += td->blocksize;
			//printk(KERN_DEBUG "tDisk: adding logical sector: %llu (%llu/%llu)\n", td->header_size + td->size_blocks, size_counter, (size << 9));
			physical_sector.disk = header.disk_index + 1;	//+1 because 0 means unused
			physical_sector.sector = td->header_size + sector++;
			internal_ret = perform_index_operation(td, index_operation_to_do, logical_sector, &physical_sector);
			if(internal_ret == 1)
			{
				printk(KERN_WARNING "tDisk: Additional disk doesn't fit in index. Shrinking to fit.\n");
				break;
			}
			else if(internal_ret == -1)
			{
				printk(KERN_WARNING "tDisk: Disk index doesn't match. Probably wrong or corrupt disk attached. Pay attention before you write to disk!\n");
			}
		}
		break;
	case READ:
		read_all_indices(td, file);
		while((size_counter+td->blocksize) <= (size << 9))
		{
			size_counter += td->blocksize;
			td->size_blocks++;
		}
		break;
	}
	printk(KERN_DEBUG "tDisk: new physical disk %u: size: %llu bytes. Logical size(%llu)\n", header.disk_index, size << 9, td->size_blocks*td->blocksize);

	if(!(flags & TD_FLAGS_READ_ONLY) && file->f_op->fsync)
		blk_queue_flush(td->queue, REQ_FLUSH);

	set_capacity(td->kernel_disk, (td->size_blocks*td->blocksize) >> 9);
	bd_set_size(bdev, td->size_blocks*td->blocksize);
	//loop_sysfs_init(lo);
	//let user-space know about this change
	kobject_uevent(&disk_to_dev(bdev->bd_disk)->kobj, KOBJ_CHANGE);

	//set_blocksize(bdev, blocksize);

	td->state = state_bound;
	//ioctl_by_bdev(bdev, BLKRRPART, 0);
	reread_partitions(td, bdev);

	//Grab the block_device to prevent its destruction after we
	//put /dev/tdX inode. Later in tdisk_clr_fd() we bdput(bdev).
	if(first_device)bdgrab(bdev);
	return 0;

 out_putf:
	fput(file);
 out:
	//This is safe: open() is still holding a reference.
	module_put(THIS_MODULE);
	return error;
}

static int tdisk_clr_fd(struct tdisk *td)
{
	unsigned int i;
	struct block_device *bdev = td->block_device;

	if(td->state != state_bound)
		return -ENXIO;

	/*
	 * If we've explicitly asked to tear down the tdisk device,
	 * and it has an elevated reference count, set it for auto-teardown when
	 * the last reference goes away. This stops $!~#$@ udev from
	 * preventing teardown because it decided that it needs to run blkid on
	 * the tdisk device whenever they appear. xfstests is notorious for
	 * failing tests because blkid via udev races with a losetup
	 * <dev>/do something like mkfs/losetup -d <dev> causing the losetup -d
	 * command to fail with EBUSY.
	 */
	if(atomic_read(&td->refcount) > 1)
	{
		td->flags |= TD_FLAGS_AUTOCLEAR;
		mutex_unlock(&td->ctl_mutex);
		return 0;
	}

	//freeze request queue during the transition
	printk(KERN_DEBUG "tDisk: Before freeze\n");
	blk_mq_freeze_queue(td->queue);
	printk(KERN_DEBUG "tDisk: After freeze\n");

	//spin_lock_irq(&td->lock);
	td->state = state_cleared;
	//td->lo_backing_file = NULL;
	//spin_unlock_irq(&td->lock);

	td->block_device = NULL;

	if(bdev)
	{
		bdput(bdev);
		invalidate_bdev(bdev);
	}

	set_capacity(td->kernel_disk, 0);
	//loop_sysfs_exit(lo);

	if(bdev)
	{
		bd_set_size(bdev, 0);
		//let user-space know about this change
		kobject_uevent(&disk_to_dev(bdev->bd_disk)->kobj, KOBJ_CHANGE);
	}

	td->state = state_unbound;
	//This is safe: open() is still holding a reference.
	module_put(THIS_MODULE);
	printk(KERN_DEBUG "tDisk: Before unfreeze\n");
	blk_mq_unfreeze_queue(td->queue);
	printk(KERN_DEBUG "tDisk: After unfreeze\n");

	if(bdev)reread_partitions(td, bdev);//ioctl_by_bdev(bdev, BLKRRPART, 0);
	td->flags = 0;
	unprepare_queue(td);
	mutex_unlock(&td->ctl_mutex);
	/*
	 * Need not hold ctl_mutex to fput backing file.
	 * Calling fput holding ctl_mutex triggers a circular
	 * lock dependency possibility warning as fput can take
	 * bd_mutex which is usually taken before ctl_mutex.
	 */

	for(i = 0; i < td->internal_devices_count; ++i)
	{
		struct file *filp = td->internal_devices[i].backing_file;
		td->internal_devices[i].backing_file = NULL;

		if(filp)
		{
			gfp_t gfp = td->internal_devices[i].old_gfp_mask;

			mapping_set_gfp_mask(filp->f_mapping, gfp);

			fput(filp);
		}
	}
	td->internal_devices_count = 0;
	//kfree(td->internal_devices);

	printk(KERN_DEBUG "tDisk: disk %d deleted\n", td->number);

	return 0;
}

static int tdisk_set_status64(struct tdisk *td, const struct tdisk_info __user *arg)
{
	struct tdisk_info info;

	if(copy_from_user(&info, arg, sizeof(struct tdisk_info)) != 0)
		return -EFAULT;

	if(td->state != state_bound)
		return -ENXIO;

	if((td->flags & TD_FLAGS_AUTOCLEAR) != (info.flags & TD_FLAGS_AUTOCLEAR))
		td->flags ^= TD_FLAGS_AUTOCLEAR;

	return 0;
}

static int tdisk_get_status64(struct tdisk *td, struct tdisk_info __user *arg)
{
	struct tdisk_info info;

	if(td->state != state_bound)
		return -ENXIO;

	memset(&info, 0, sizeof(info));
	info.number = td->number;
	//info->block_device = huge_encode_dev(stat.dev);
	info.flags = td->flags;

	if(copy_to_user(arg, &info, sizeof(info)) != 0)
		return -EFAULT;

	return 0;
}

static int tdisk_get_sector_index(struct tdisk *td, struct sector_index __user *arg)
{
	struct sector_index index;

	if(copy_from_user(&index, arg, sizeof(struct sector_index)) != 0)
		return -EFAULT;

	if(index.sector >= td->max_sectors)return -EINVAL;

	if(copy_to_user(arg, td->sorted_sectors[index.sector].physical_sector, sizeof(struct sector_index)) != 0)
		return -EFAULT;

	return 0;
}

static int td_ioctl(struct block_device *bdev, fmode_t mode, unsigned int cmd, unsigned long arg)
{
	struct tdisk *td = bdev->bd_disk->private_data;
	int err;

	mutex_lock_nested(&td->ctl_mutex, 1);
	switch(cmd)
	{
	case TDISK_SET_FD:
		err = tdisk_set_fd(td, mode, bdev, arg);
		break;
	case TDISK_CLR_FD:
		//tdisk_clr_fd would have unlocked ctl_mutex on success
		err = tdisk_clr_fd(td);
		if(!err)goto out_unlocked;
		break;
	case TDISK_SET_STATUS:
		err = -EPERM;
		if((mode & FMODE_WRITE) || capable(CAP_SYS_ADMIN))
			err = tdisk_set_status64(td, (struct tdisk_info __user *) arg);
		break;
	case TDISK_GET_STATUS:
		err = tdisk_get_status64(td, (struct tdisk_info __user *) arg);
		break;
	case TDISK_GET_MAX_SECTORS:
		if(copy_to_user((__u64 __user *)arg, &td->max_sectors, sizeof(__u64)) != 0)
			err = -EFAULT;
		else err = 0;
		break;
	case TDISK_GET_SECTOR_INDEX:
		err = tdisk_get_sector_index(td, (struct sector_index __user *) arg);
		break;
	default:
		err = -EINVAL;
	}
	mutex_unlock(&td->ctl_mutex);

out_unlocked:
	return err;
}

//For older kernels
#ifdef CONFIG_COMPAT
static int td_compat_ioctl(struct block_device *bdev, fmode_t mode, unsigned int cmd, unsigned long arg)
{
	int err;

	switch(cmd)
	{
	case TDISK_CLR_FD:
	case TDISK_GET_STATUS:
	case TDISK_SET_STATUS:
		arg = (unsigned long)compat_ptr(arg);
	case TDISK_SET_FD:
		err = td_ioctl(bdev, mode, cmd, arg);
		break;
	default:
		err = -ENOIOCTLCMD;
		break;
	}
	return err;
}
#endif

static int td_open(struct block_device *bdev, fmode_t mode)
{
	struct tdisk *td = bdev->bd_disk->private_data;
	if(!td)return -ENXIO;

	atomic_inc(&td->refcount);

	return 0;
}

static void td_release(struct gendisk *disk, fmode_t mode)
{
	struct tdisk *td = disk->private_data;
	int err;

	if(atomic_dec_return(&td->refcount))
		return;

	mutex_lock(&td->ctl_mutex);

	if(td->flags & TD_FLAGS_AUTOCLEAR)
	{
		/*
		 * In autoclear mode, stop the tdisk thread
		 * and remove configuration after last close.
		 */
		err = tdisk_clr_fd(td);
		if(!err)return;
	}
	else
	{
		/*
		 * Otherwise keep thread (if running) and config,
		 * but flush possible ongoing bios in thread.
		 */
		td_flush(td);
	}

	mutex_unlock(&td->ctl_mutex);
}

static const struct block_device_operations td_fops = {
	.owner =	THIS_MODULE,
	.open =		td_open,
	.release =	td_release,
	.ioctl =	td_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl =	td_compat_ioctl,
#endif
};

static int tdisk_queue_rq(struct blk_mq_hw_ctx *hctx, const struct blk_mq_queue_data *bd)
{
	struct td_command *cmd = blk_mq_rq_to_pdu(bd->rq);
	struct tdisk *td = cmd->rq->q->queuedata;

	blk_mq_start_request(bd->rq);

	if(td->state != state_bound)
		return -EIO;

	queue_kthread_work(&td->worker, &cmd->td_work);

	return BLK_MQ_RQ_QUEUE_OK;
}

static void tdisk_handle_cmd(struct td_command *cmd)
{
	const bool write = cmd->rq->cmd_flags & REQ_WRITE;
	struct tdisk *td = cmd->rq->q->queuedata;
	int ret = 0;

	if(write && (td->flags & TD_FLAGS_READ_ONLY))
	{
		ret = -EIO;
		goto failed;
	}

	ret = do_req_filebacked(td, cmd->rq);

 failed:
#if LINUX_VERSION_CODE <= KERNEL_VERSION(4,1,14)
	if(ret)cmd->rq->errors = -EIO;
	blk_mq_complete_request(cmd->rq);
#else
	blk_mq_complete_request(cmd->rq, ret ? -EIO : 0);
#endif
}

static void tdisk_queue_work(struct kthread_work *work)
{
	struct td_command *cmd = container_of(work, struct td_command, td_work);
	tdisk_handle_cmd(cmd);
}

static int tdisk_init_request(void *data, struct request *rq, unsigned int hctx_idx, unsigned int request_idx, unsigned int numa_node)
{
	struct td_command *cmd = blk_mq_rq_to_pdu(rq);

	cmd->rq = rq;
	init_kthread_work(&cmd->td_work, tdisk_queue_work);

	return 0;
}

static struct blk_mq_ops tdisk_mq_ops = {
	.queue_rq       = tdisk_queue_rq,
	.map_queue      = blk_mq_map_queue,
	.init_request	= tdisk_init_request,
};

int tdisk_add(struct tdisk **t, int i, unsigned int blocksize, sector_t max_sectors)
{
	struct tdisk *td;
	struct gendisk *disk;
	int err;
	sector_t j;
	unsigned int header_size_byte;
	unsigned int index_offset_byte;

	//Calculate header size which consists
	//of a header, disk mappings and indices
	header_size_byte = sizeof(struct tdisk_header);
	index_offset_byte = header_size_byte;
	header_size_byte += max_sectors * sizeof(struct sector_index);

	err = -ENOMEM;
	td = kzalloc(sizeof(struct tdisk), GFP_KERNEL);
	if(!td)goto out;

	td->state = state_unbound;
	td->size_blocks = 0;

	//allocate id, if @id >= 0, we're requesting that specific id
	if(i >= 0)
	{
		err = idr_alloc(&td_index_idr, td, i, i + 1, GFP_KERNEL);
		if(err == -ENOSPC)err = -EEXIST;
	}
	else
	{
		err = idr_alloc(&td_index_idr, td, 0, 0, GFP_KERNEL);
	}
	if(err < 0)goto out_free_dev;
	i = err;

	err = -ENOMEM;
	td->tag_set.ops = &tdisk_mq_ops;
	td->tag_set.nr_hw_queues = 1;
	td->tag_set.queue_depth = 128;
	td->tag_set.numa_node = NUMA_NO_NODE;
	td->tag_set.cmd_size = sizeof(struct td_command);
	td->tag_set.flags = BLK_MQ_F_SHOULD_MERGE | BLK_MQ_F_SG_MERGE;
	td->tag_set.driver_data = td;
	td->blocksize = blocksize;

	err = blk_mq_alloc_tag_set(&td->tag_set);
	if(err)goto out_free_idr;

	td->queue = blk_mq_init_queue(&td->tag_set);
	if(IS_ERR_OR_NULL(td->queue))
	{
		err = PTR_ERR(td->queue);
		goto out_cleanup_tags;
	}
	td->queue->queuedata = td;

	/**
	  * It doesn't make sense to enable merge because the I/O
	  * submitted to backing file is handled page by page.
	 **/
	queue_flag_set_unlocked(QUEUE_FLAG_NOMERGES, td->queue);

	disk = td->kernel_disk = alloc_disk(1);
	if(!disk)goto out_free_queue;

	//Allocate disk indices
	err = -ENOMEM;
	td->index_offset_byte = index_offset_byte;
	td->header_size = header_size_byte/blocksize + ((header_size_byte%blocksize == 0) ? 0 : 1);
	td->indices = kzalloc(td->header_size*td->blocksize, GFP_KERNEL);
	if(!td->indices)goto out_free_queue;

	//Allocate sorted disk indices
	td->max_sectors = td->header_size*td->blocksize - td->index_offset_byte;
	__div64_32(&td->max_sectors, sizeof(struct sector_index));
	td->sorted_sectors = kzalloc(sizeof(struct sorted_sector_index) * td->max_sectors, GFP_KERNEL);
	if(!td->sorted_sectors)goto out_free_indices;
	for(j = 0; j < td->max_sectors; ++j)
	{
		INIT_LIST_HEAD(&td->sorted_sectors[j].list);
		td->sorted_sectors[j].physical_sector = (struct sector_index*)(td->indices + sizeof(struct sector_index)*j + td->index_offset_byte);

		if(j != 0)list_add(&td->sorted_sectors[j].list, &td->sorted_sectors[j-1].list);
	}

	disk->flags |= GENHD_FL_EXT_DEVT;
	mutex_init(&td->ctl_mutex);
	atomic_set(&td->refcount, 0); 
	td->number		= i;
	spin_lock_init(&td->lock);
	disk->major		= TD_MAJOR;
	disk->first_minor	= i;
	disk->fops		= &td_fops;
	disk->private_data	= td;
	disk->queue		= td->queue;
	set_blocksize(td->block_device, blocksize);
	sprintf(disk->disk_name, "td%d", i);
	add_disk(disk);
	*t = td;

	printk(KERN_DEBUG "tDisk: new disk %s: max sectors: %llu, blocksize: %u, header size: %u sec\n", disk->disk_name, max_sectors, blocksize, td->header_size);

	return td->number;

out_free_indices:
	kfree(td->indices);
out_free_queue:
	blk_cleanup_queue(td->queue);
out_cleanup_tags:
	blk_mq_free_tag_set(&td->tag_set);
out_free_idr:
	idr_remove(&td_index_idr, i);
out_free_dev:
	kfree(td);
out:
	return err;
}

void tdisk_remove(struct tdisk *td)
{
	blk_cleanup_queue(td->queue);
	del_gendisk(td->kernel_disk);
	blk_mq_free_tag_set(&td->tag_set);
	put_disk(td->kernel_disk);
	kfree(td->sorted_sectors);
	kfree(td->indices);
	kfree(td);
}

static int find_free_cb(int id, void *ptr, void *data)
{
	struct tdisk *td = ptr;
	struct tdisk **t = data;

	if(td->state == state_unbound)
	{
		*t = td;
		return 1;
	}
	return 0;
}

int tdisk_lookup(struct tdisk **t, int i)
{
	struct tdisk *td;
	int ret = -ENODEV;

	if(i < 0)
	{
		int err = idr_for_each(&td_index_idr, &find_free_cb, &td);
		if(err == 1)
		{
			*t = td;
			ret = td->number;
		}
		goto out;
	}

	//lookup and return a specific i
	td = idr_find(&td_index_idr, i);
	if(td)
	{
		*t = td;
		ret = td->number;
	}
out:
	return ret;
}

static int __init tdisk_init(void)
{
	int err;

	err = register_tdisk_control();
	if(err < 0)return err;

	TD_MAJOR = register_blkdev(TD_MAJOR, "td");
	if(TD_MAJOR <= 0)
	{
		err = -EBUSY;
		goto misc_out;
	}

	printk(KERN_INFO "tDisk: driver loaded\n");
	return 0;

misc_out:
	unregister_tdisk_control();
	return err;
}

static int tdisk_exit_cb(int id, void *ptr, void *data)
{
	tdisk_remove((struct tdisk*)ptr);
	return 0;
}

static void __exit tdisk_exit(void)
{
	idr_for_each(&td_index_idr, &tdisk_exit_cb, NULL);
	idr_destroy(&td_index_idr);

	unregister_blkdev(TD_MAJOR, "td");

	unregister_tdisk_control();
}

module_init(tdisk_init);
module_exit(tdisk_exit);

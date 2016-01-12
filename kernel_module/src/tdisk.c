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
#include <linux/miscdevice.h>
#include <linux/falloc.h>
#include <linux/uio.h>
 
#include "tier_disk.h"
#include "helpers.h"
#include "tdisk.h"
#include "tdisk_control.h"
#include "mbds/deque.h"


static u_int TD_MAJOR = 0;
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Thomas Sparber <thomas@sparber.eu>");
MODULE_DESCRIPTION("tDisk Driver");
MODULE_ALIAS_BLOCKDEV_MAJOR(TD_MAJOR);

static int part_shift;

DEFINE_IDR(td_index_idr);
DEFINE_MUTEX(td_index_mutex);

static loff_t get_size(loff_t offset, loff_t sizelimit, struct file *file)
{
	//Size in byte
	loff_t size_byte = i_size_read(file->f_mapping->host);

	if(offset > 0)
		size_byte -= offset;

	//offset beyond i_size, weird but possible
	if(size_byte < 0)
		return 0;

	if(sizelimit > 0 && sizelimit < size_byte)
		size_byte = sizelimit;

	//Convert to sectors
	return size_byte >> 9;
}

static loff_t get_tdisk_size(struct tdisk *td, struct file *file)
{
	return get_size(0, td->sizelimit, file);
}

static int figure_tdisk_size(struct tdisk *td, loff_t sizelimit)
{
	loff_t size = get_size(0, sizelimit, td->lo_backing_file);
	sector_t x = (sector_t)size;
	struct block_device *bdev = td->block_device;

	if(unlikely((loff_t)x != size))
		return -EFBIG;

	if(td->sizelimit != sizelimit)
		td->sizelimit = sizelimit;

	set_capacity(td->kernel_disk, x);
	bd_set_size(bdev, (loff_t)get_capacity(bdev->bd_disk) << 9);

	//Inform user space
	kobject_uevent(&disk_to_dev(bdev->bd_disk)->kobj, KOBJ_CHANGE);
	return 0;
}

static int td_discard(struct tdisk *td, struct request *rq, loff_t pos)
{
	/*
	 * We use punch hole to reclaim the free space used by the
	 * image a.k.a. discard.
	 */
	struct file *file = td->lo_backing_file;
	int mode = FALLOC_FL_PUNCH_HOLE | FALLOC_FL_KEEP_SIZE;
	int ret;

	if((!file->f_op->fallocate))
	{
		ret = -EOPNOTSUPP;
		goto out;
	}

	ret = file->f_op->fallocate(file, mode, pos, blk_rq_bytes(rq));

	if(unlikely(ret && ret != -EINVAL && ret != -EOPNOTSUPP))
		ret = -EIO;

 out:
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

static int td_req_flush(struct tdisk *td, struct request *rq)
{
	struct file *file = td->lo_backing_file;
	int ret = vfs_fsync(file, 0);

	if(unlikely(ret && ret != -EINVAL))
		ret = -EIO;

	return ret;
}

static int do_req_filebacked(struct tdisk *td, struct request *rq)
{
	struct bio_vec bvec;
	struct req_iterator iter;
	struct iov_iter i;
	ssize_t len;
	loff_t pos;
	int ret = 0;
	MBdeque *requests;
	int direction = READ;
	unsigned int k = 0;
	unsigned int j = 0;
	MBdeque *current_requests = NULL;
	struct mapped_sector_index *index;

	pos = (loff_t)blk_rq_pos(rq) << 9;

	//Handle flush operations
	if((rq->cmd_flags & REQ_WRITE) && (rq->cmd_flags & REQ_FLUSH))
	{
		ret = td_req_flush(td, rq);
		goto finished;
	}

	//Handle discard operations
	if((rq->cmd_flags & REQ_WRITE) && (rq->cmd_flags & REQ_DISCARD))
	{
		ret = td_discard(td, rq, pos);
		goto finished;
	}

	requests = MBdeque_create();

	//Normal file operation
	rq_for_each_segment(bvec, rq, iter)
	{
		direction = (rq->cmd_flags & REQ_WRITE) ? WRITE : READ;
		tdisk_operation_internal(td, requests, direction, pos, bvec.bv_len);

		if(rq->cmd_flags & REQ_WRITE)
		{
			//Write operation
			struct iov_iter i;
			ssize_t bw;

			iov_iter_bvec(&i, ITER_BVEC, &bvec, 1, bvec.bv_len);

			file_start_write(td->lo_backing_file);
			bw = vfs_iter_write(td->lo_backing_file, &i, &pos);
			file_end_write(td->lo_backing_file);

			if(unlikely(bw != bvec.bv_len))
			{
				printk_ratelimited(KERN_ERR "tDisk: Write error at byte offset %llu, length %i.\n", (unsigned long long)pos, bvec.bv_len);

				if(bw >= 0)
					ret = -EIO;
				else
					ret = bw;
				break;
			}
		}
		else
		{
			//Read operation
			iov_iter_bvec(&i, ITER_BVEC, &bvec, 1, bvec.bv_len);
			len = vfs_iter_read(td->lo_backing_file, &i, &pos);

			if(len < 0)
			{
				ret = len;
				break;
			}

			flush_dcache_page(bvec.bv_page);

			if(len != bvec.bv_len)
			{
				struct bio *bio;

				__rq_for_each_bio(bio, rq)
					zero_fill_bio(bio);

				break;
			}

		}

		cond_resched();
	}

	//Create struct bio's
	for(k = 0; k < requests->count; ++k)
	{
		u8 physical_disk;
		sector_t first_sector;

		current_requests = MBdeque_get_at(requests, k);
		if(current_requests == NULL)
		{
			//Request was splitted TODO
			continue;
		}

		//Check for empty request list
		if(current_requests->count <= 0)
		{
			printk(KERN_ERR "tDisk: Skipping empty request list\n");

			MBdeque_delete(current_requests);

			continue;
		}

		//Check if disk is full
		index = MBdeque_get_at(current_requests, 0);
		if(index->physical_sector.disk <= 0)
		{
			printk(KERN_ERR "tDisk: disk full! More space was allocated than disks attached!\n");

			//Free resources
			for(j = 0; j < current_requests->count; ++j)
			{
				index = MBdeque_get_at(current_requests, j);
				vfree(index);
			}
			MBdeque_delete(current_requests);

			continue;
		}
		physical_disk = index->physical_sector.disk - 1;	//-1 because 0 means unused
		first_sector = index->offset + index->physical_sector.sector;

		//Process requests
		for(j = 0; j < current_requests->count; ++j)
		{
			index = MBdeque_get_at(current_requests, j);

			printk(KERN_DEBUG "tDisk: Physical disk request: Physical-Start: %llu, Physical-End: %llu, Actual length (%u bytes) Request: %u/%u\n",
													first_sector,
													first_sector+1,
													512U,
													j, current_requests->count);

			//if(!bio_add_page(bio_req, virt_to_page(buffer), td_dev->blocksize, (index->logical_sector-first) * td_dev->blocksize))
			//{
			//	printk(KERN_ERR "tDisk: bio_add_page failed!\n");
			//}

			if(direction == WRITE)
			{
				//Save index
				int internal_ret = perform_index_operation(td, WRITE, index);
				if(internal_ret != 0)ret = internal_ret;
			}

			vfree(index);
		}

		MBdeque_delete(current_requests);
	}

	MBdeque_delete(requests);

 finished:
	return ret;
}

static inline int is_tdisk(struct file *file)
{
	struct inode *i = file->f_mapping->host;

	return i && S_ISBLK(i->i_mode) && MAJOR(i->i_rdev) == TD_MAJOR;
}

static void tdisk_config_discard(struct tdisk *td)
{
	struct file *file = td->lo_backing_file;
	struct inode *inode = file->f_mapping->host;
	struct request_queue *q = td->queue;

	//We use punch hole to reclaim the free space used by the image a.k.a. discard.
	if((!file->f_op->fallocate)) {
		q->limits.discard_granularity = 0;
		q->limits.discard_alignment = 0;
		q->limits.max_discard_sectors = 0;
		q->limits.discard_zeroes_data = 0;
		queue_flag_clear_unlocked(QUEUE_FLAG_DISCARD, q);
		return;
	}

	q->limits.discard_granularity = inode->i_sb->s_blocksize;
	q->limits.discard_alignment = 0;
	q->limits.max_discard_sectors = UINT_MAX >> 9;
	q->limits.discard_zeroes_data = 1;
	queue_flag_set_unlocked(QUEUE_FLAG_DISCARD, q);
}

static int tdisk_set_fd(struct tdisk *td, fmode_t mode, struct block_device *bdev, unsigned int arg)
{
	struct file	*file, *f;
	struct inode *inode;
	struct address_space *mapping;
	unsigned int blocksize;
	int flags = 0;
	int error;
	loff_t size;
	sector_t sector;

	//This is safe, since we have a reference from open().
	__module_get(THIS_MODULE);

	error = -EBADF;
	file = fget(arg);
	if(!file)
		goto out;

	error = -EBUSY;
	if(td->state != state_unbound)
		goto out_putf;

	//Avoid recursion
	f = file;
	while(is_tdisk(f))
	{
		struct tdisk *l;

		if(f->f_mapping->host->i_bdev == bdev)
			goto out_putf;

		l = f->f_mapping->host->i_bdev->bd_disk->private_data;

		if(l->state == state_unbound)
		{
			error = -EINVAL;
			goto out_putf;
		}

		f = l->lo_backing_file;
	}

	mapping = file->f_mapping;
	inode = mapping->host;

	error = -EINVAL;
	if(!S_ISREG(inode->i_mode) && !S_ISBLK(inode->i_mode))
		goto out_putf;

	if(!(file->f_mode & FMODE_WRITE) || !(mode & FMODE_WRITE) || !file->f_op->write_iter)
		flags |= TD_FLAGS_READ_ONLY;

	blocksize = S_ISBLK(inode->i_mode) ? inode->i_bdev->bd_block_size : PAGE_SIZE;

	error = -EFBIG;
	size = get_tdisk_size(td, file) - ((td->index_size*td->blocksize) >> 9);
	if((loff_t)(sector_t)size != size)
		goto out_putf;

	error = -ENOMEM;
	td->wq = alloc_workqueue("tdq%d", WQ_MEM_RECLAIM | WQ_HIGHPRI | WQ_UNBOUND, 16, td->number);
	if(!td->wq)
		goto out_putf;

	//Add free sectors ll
	error = -ENOMEM;
	td->free_sectors = MBdeque_create();
	for(sector = 0; sector < size; ++sector)
	{
		//sector_t *temp = vmalloc(sizeof(sector_t));
		//(*temp) = sector;
		//MBdeque_push_back(td->free_sectors, temp);
		struct mapped_sector_index index = {
			.offset = 0,
			.logical_sector = td->size_blocks++,
			.physical_sector = {
				.disk = 0,	//TODO
				.sector = sector
			}
		};
		perform_index_operation(td, WRITE, &index);
	}

	error = 0;

	set_device_ro(bdev, (flags & TD_FLAGS_READ_ONLY) != 0);

	td->blocksize = blocksize;
	td->block_device = bdev;
	td->flags = flags;
	td->lo_backing_file = file;
	td->sizelimit = 0;
	td->old_gfp_mask = mapping_gfp_mask(mapping);
	mapping_set_gfp_mask(mapping, td->old_gfp_mask & ~(__GFP_IO|__GFP_FS));

	if(!(flags & TD_FLAGS_READ_ONLY) && file->f_op->fsync)
		blk_queue_flush(td->queue, REQ_FLUSH);

	set_capacity(td->kernel_disk, size);
	bd_set_size(bdev, size << 9);
	//loop_sysfs_init(lo);
	//let user-space know about the new size
	kobject_uevent(&disk_to_dev(bdev->bd_disk)->kobj, KOBJ_CHANGE);

	set_blocksize(bdev, blocksize);

	td->state = state_bound;
	ioctl_by_bdev(bdev, BLKRRPART, 0);

	//Grab the block_device to prevent its destruction after we
	//put /dev/tdX inode. Later in tdisk_clr_fd() we bdput(bdev).
	bdgrab(bdev);
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
	struct file *filp = td->lo_backing_file;
	gfp_t gfp = td->old_gfp_mask;
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
	if(td->refcount > 1)
	{
		td->flags |= TD_FLAGS_AUTOCLEAR;
		mutex_unlock(&td->ctl_mutex);
		return 0;
	}

	if(filp == NULL)
		return -EINVAL;

	spin_lock_irq(&td->lock);
	td->state = state_cleared;
	td->lo_backing_file = NULL;
	spin_unlock_irq(&td->lock);

	td->block_device = NULL;
	td->sizelimit = 0;
	memset(td->file_name, 0, TD_NAME_SIZE);

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

	//Free free_sectors
	for(i = 0; i < td->free_sectors->count; ++i)
		vfree((sector_t*)MBdeque_get_at(td->free_sectors, i));
	MBdeque_delete(td->free_sectors);

	mapping_set_gfp_mask(filp->f_mapping, gfp);
	td->state = state_unbound;
	//This is safe: open() is still holding a reference.
	module_put(THIS_MODULE);
	if(bdev)ioctl_by_bdev(bdev, BLKRRPART, 0);
	td->flags = 0;
	destroy_workqueue(td->wq);
	td->wq = NULL;
	mutex_unlock(&td->ctl_mutex);
	/*
	 * Need not hold ctl_mutex to fput backing file.
	 * Calling fput holding ctl_mutex triggers a circular
	 * lock dependency possibility warning as fput can take
	 * bd_mutex which is usually taken before ctl_mutex.
	 */
	fput(filp);
	return 0;
}

static int tdisk_set_status(struct tdisk *td, const struct tdisk_info *info)
{
	if(td->state != state_bound)
		return -ENXIO;

	if(td->sizelimit != info->sizelimit)
		if(figure_tdisk_size(td, info->sizelimit))
			return -EFBIG;

	tdisk_config_discard(td);

	memcpy(td->file_name, info->file_name, TD_NAME_SIZE);
	td->file_name[TD_NAME_SIZE-1] = 0;

	if((td->flags & TD_FLAGS_AUTOCLEAR) != (info->flags & TD_FLAGS_AUTOCLEAR))
		td->flags ^= TD_FLAGS_AUTOCLEAR;

	return 0;
}

static int tdisk_get_status(struct tdisk *td, struct tdisk_info *info)
{
	struct file *file = td->lo_backing_file;
	struct kstat stat;
	int error;

	if(td->state != state_bound)
		return -ENXIO;

	error = vfs_getattr(&file->f_path, &stat);
	if(error)
		return error;

	memset(info, 0, sizeof(*info));
	info->number = td->number;
	info->block_device = huge_encode_dev(stat.dev);
	info->sizelimit = td->sizelimit;
	info->flags = td->flags;
	memcpy(info->file_name, td->file_name, TD_NAME_SIZE);

	return 0;
}

static int tdisk_set_status64(struct tdisk *td, const struct tdisk_info __user *arg)
{
	struct tdisk_info info64;

	if(copy_from_user(&info64, arg, sizeof(struct tdisk_info)))
		return -EFAULT;

	return tdisk_set_status(td, &info64);
}

static int tdisk_get_status64(struct tdisk *td, struct tdisk_info __user *arg)
{
	struct tdisk_info info64;
	int err = 0;

	if(!arg)
		err = -EINVAL;
	if(!err)
		err = tdisk_get_status(td, &info64);
	if(!err && copy_to_user(arg, &info64, sizeof(info64)))
		err = -EFAULT;

	return err;
}

static int tdisk_set_capacity(struct tdisk *td, struct block_device *bdev)
{
	if(unlikely(td->state != state_bound))
		return -ENXIO;

	return figure_tdisk_size(td, td->sizelimit);
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
		if(!err)
			goto out_unlocked;
		break;
	case TDISK_SET_STATUS:
		err = -EPERM;
		if((mode & FMODE_WRITE) || capable(CAP_SYS_ADMIN))
			err = tdisk_set_status64(td, (struct tdisk_info __user *) arg);
		break;
	case TDISK_GET_STATUS:
		err = tdisk_get_status64(td, (struct tdisk_info __user *) arg);
		break;
	case TDISK_SET_CAPACITY:
		err = -EPERM;
		if((mode & FMODE_WRITE) || capable(CAP_SYS_ADMIN))
			err = tdisk_set_capacity(td, bdev);
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
	case TDISK_SET_CAPACITY:
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
	struct tdisk *td;
	int err = 0;

	mutex_lock(&td_index_mutex);
	td = bdev->bd_disk->private_data;

	if(!td)
	{
		err = -ENXIO;
		goto out;
	}

	mutex_lock(&td->ctl_mutex);
	td->refcount++;
	mutex_unlock(&td->ctl_mutex);
out:
	mutex_unlock(&td_index_mutex);
	return err;
}

static void td_release(struct gendisk *disk, fmode_t mode)
{
	struct tdisk *td = disk->private_data;
	int err;

	mutex_lock(&td->ctl_mutex);

	if(--td->refcount)
		goto out;

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

out:
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

	if(cmd->rq->cmd_flags & REQ_WRITE)
	{
		struct tdisk *td = cmd->rq->q->queuedata;
		bool need_sched = true;

		spin_lock_irq(&td->lock);
		if(td->write_started)
			need_sched = false;
		else
			td->write_started = true;
		list_add_tail(&cmd->list, &td->write_cmd_head);
		spin_unlock_irq(&td->lock);

		if(need_sched)
			queue_work(td->wq, &td->write_work);
	}
	else
	{
		queue_work(td->wq, &cmd->read_work);
	}

	return BLK_MQ_RQ_QUEUE_OK;
}

static void tdisk_handle_cmd(struct td_command *cmd)
{
	const bool write = cmd->rq->cmd_flags & REQ_WRITE;
	struct tdisk *td = cmd->rq->q->queuedata;
	int ret = -EIO;

	if(write && (td->flags & TD_FLAGS_READ_ONLY))
		goto failed;

	ret = do_req_filebacked(td, cmd->rq);

 failed:
	if(ret)cmd->rq->errors = -EIO;
	blk_mq_complete_request(cmd->rq);
}

static void tdisk_queue_write_work(struct work_struct *work)
{
	struct tdisk *td = container_of(work, struct tdisk, write_work);
	LIST_HEAD(cmd_list);

	spin_lock_irq(&td->lock);
 repeat:
	list_splice_init(&td->write_cmd_head, &cmd_list);
	spin_unlock_irq(&td->lock);

	while(!list_empty(&cmd_list))
	{
		struct td_command *cmd = list_first_entry(&cmd_list, struct td_command, list);
		list_del_init(&cmd->list);
		tdisk_handle_cmd(cmd);
	}

	spin_lock_irq(&td->lock);
	if(!list_empty(&td->write_cmd_head))
		goto repeat;

	td->write_started = false;
	spin_unlock_irq(&td->lock);
}

static void tdisk_queue_read_work(struct work_struct *work)
{
	struct td_command *cmd = container_of(work, struct td_command, read_work);
	tdisk_handle_cmd(cmd);
}

static int tdisk_init_request(void *data, struct request *rq, unsigned int hctx_idx, unsigned int request_idx, unsigned int numa_node)
{
	struct td_command *cmd = blk_mq_rq_to_pdu(rq);

	cmd->rq = rq;
	INIT_WORK(&cmd->read_work, tdisk_queue_read_work);

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
	unsigned int index_size_byte = max_sectors * sizeof(struct sector_index);	//Calculate index size

	err = -ENOMEM;
	td = kzalloc(sizeof(*td), GFP_KERNEL);
	if(!td)goto out;

	td->state = state_unbound;

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

	INIT_LIST_HEAD(&td->write_cmd_head);
	INIT_WORK(&td->write_work, tdisk_queue_write_work);

	disk = td->kernel_disk = alloc_disk(1 << part_shift);
	if(!disk)goto out_free_queue;

	//Allocate disk indices
	err = -ENOMEM;
	td->index_size = index_size_byte/blocksize + ((index_size_byte%blocksize == 0) ? 0 : 1);
	td->indices = kzalloc(td->index_size*td->blocksize, GFP_KERNEL);
	if(!td->indices)goto out_free_queue;

	//Init lock which is used to search for free sectors in
	//the internal disks
	spin_lock_init(&td->free_sectors_lock);

	disk->flags |= GENHD_FL_EXT_DEVT;
	mutex_init(&td->ctl_mutex);
	td->number		= i;
	spin_lock_init(&td->lock);
	disk->major		= TD_MAJOR;
	disk->first_minor	= i << part_shift;
	disk->fops		= &td_fops;
	disk->private_data	= td;
	disk->queue		= td->queue;
	sprintf(disk->disk_name, "td%d", i);
	add_disk(disk);
	*t = td;
	return td->number;

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

	part_shift = 0;

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

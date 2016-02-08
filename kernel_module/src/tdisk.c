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
#include <linux/vmalloc.h>
#include <linux/sort.h>

#include "helpers.h"
#include "tdisk.h"
#include "tdisk_control.h"
#include "tdisk_file.h"

#define COMPARE 1410

#define DEFAULT_WORKER_TIMEOUT (5*HZ)

static int td_start_worker_thread(struct tdisk *td);
static void td_stop_worker_thread(struct tdisk *td);
static enum worker_status td_queue_work(void *private_data, struct kthread_work *work);

static u_int TD_MAJOR = 0;
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Thomas Sparber <thomas@sparber.eu>");
MODULE_DESCRIPTION(DRIVER_NAME " Driver");
MODULE_ALIAS_BLOCKDEV_MAJOR(TD_MAJOR);

DEFINE_IDR(td_index_idr);

struct sorted_internal_device
{
	struct td_internal_device *dev;
	sector_t available_blocks;

	struct hlist_head preferred_blocks;
	sector_t amount_blocks;
}; //end struct sorted_internal_device

/*
 * Flushes the td device (not the underlying devices)
 */
static int td_flush(struct tdisk *td)
{
	//Freeze and unfreeze queue
	blk_mq_freeze_queue(td->queue);
	blk_mq_unfreeze_queue(td->queue);

	return 0;
}

/**
  * Flushes the underlying devices of the tDisk
 **/
static int td_flush_devices(struct tdisk *td)
{
	unsigned int i;
	int ret = 0;
	for(i = 0; i < td->internal_devices_count; ++i)
	{
		struct file *file = td->internal_devices[i].file;
		if(file)
		{
			int internal_ret = file_flush(file);

			if(unlikely(internal_ret && internal_ret != -EINVAL))
				ret = -EIO;
		}
	}

	return ret;
}

/**
  * Performs the given index operation. This can be:
  *  - READ: reads the physical sector index for the given logical sector
  *  - WRITE: stores the given physical sector index for the given logical
  *    sector. The data are also stored to each physical disk
  *  - COMPARE: Compares the given physical sector index with the actual one.
  *    This is useful to compare individual disks for consistency.
 **/
int td_perform_index_operation(struct tdisk *td_dev, int direction, sector_t logical_sector, struct sector_index *physical_sector)
{
	struct sector_index *actual;
	loff_t position = td_dev->index_offset_byte + logical_sector * sizeof(struct sector_index);
	unsigned int length = sizeof(struct sector_index);

	if(position + length > td_dev->header_size * td_dev->blocksize)return 1;
	actual = &td_dev->indices[logical_sector];

	//Increment access count and sort
	actual->access_count++;
	MY_BUG_ON(direction == WRITE && physical_sector->disk == 0, PRINT_INT(physical_sector->disk), PRINT_ULL(logical_sector));
	//MY_BUG_ON(direction == WRITE && physical_sector->disk > td_dev->internal_devices_count, PRINT_INT(physical_sector->disk), PRINT_ULL(logical_sector));
	MY_BUG_ON(direction == READ && actual->disk == 0, PRINT_INT(actual->disk), PRINT_ULL(logical_sector));
	//MY_BUG_ON(direction == READ && actual->disk > td_dev->internal_devices_count, PRINT_INT(physical_sector->disk), PRINT_ULL(logical_sector));
	//td_reorganize_sorted_index(td_dev, logical_sector);

	//index operation
	if(direction == READ)
		(*physical_sector) = (*actual);
	else if(direction == COMPARE)
	{
		if(physical_sector->disk != actual->disk || physical_sector->sector != actual->sector)
			return -1;
	}
	else if(direction == WRITE)
	{
		unsigned int j;

		//Memory operation
		actual->disk = physical_sector->disk;
		actual->sector = physical_sector->sector;

		//Disk operations
		for(j = 0; j < td_dev->internal_devices_count; ++j)
		{
			struct file *file = td_dev->internal_devices[j].file;

			if(file)
			{
				file_write_data(file, actual, position, length, &td_dev->internal_devices[j].performance);
			}
		}
	}

	return 0;
}

int td_sort_devices_callback(const void *a, const void *b)
{
	const struct sorted_internal_device *d_a = a;
	const struct sorted_internal_device *d_b = b;

	long performance_a = (d_a->dev->performance.avg_read_time_cycles + d_a->dev->performance.avg_write_time_cycles) >> 1;
	long performance_b = (d_b->dev->performance.avg_read_time_cycles + d_b->dev->performance.avg_write_time_cycles) >> 1;

	return performance_a - performance_b;
}

static void td_insert_sorted_internal_devices(struct tdisk *td, struct sorted_internal_device *sorted_devices)
{
	unsigned int i;

	for(i = 0; i < td->internal_devices_count; ++i)
	{
		INIT_HLIST_HEAD(&sorted_devices[i].preferred_blocks);
		sorted_devices[i].dev = &td->internal_devices[i];
		sorted_devices[i].available_blocks = td->internal_devices[i].size_blocks;
		sorted_devices[i].amount_blocks = 0;
	}

	sort(sorted_devices, td->internal_devices_count, sizeof(struct sorted_internal_device), &td_sort_devices_callback, NULL);
}

static struct sorted_internal_device* td_find_sorted_device(struct sorted_internal_device *sorted_devices, struct td_internal_device *desired_device, unsigned int devices)
{
	unsigned int i;

	for(i = 0; i < devices; ++i)
	{
		if(sorted_devices[i].dev == desired_device)
		{
			return &sorted_devices[i];
		}
	}
	return NULL;
}

static struct sorted_sector_index* td_find_sector_index(struct hlist_head *head, tdisk_index disk)
{
	struct sorted_sector_index *item;
	struct sorted_sector_index *lowest = NULL;

	hlist_for_each_entry(item, head, list)
	{
		if(item->physical_sector->disk == disk)
		{
			if(lowest == NULL || item->physical_sector->access_count < lowest->physical_sector->access_count)
				lowest = item;
		}
	}

	return lowest;
}

static struct sorted_sector_index* td_find_sector_index_acc(struct hlist_head *head, tdisk_index disk, __u16 access_count)
{
	struct sorted_sector_index *item;

	hlist_for_each_entry(item, head, list)
	{
		if(item->physical_sector->disk == disk && item->physical_sector->access_count >= access_count)
		{
			return item;
		}
	}

	return NULL;
}

static void td_assign_sectors(struct tdisk *td, struct sorted_internal_device *sorted_devices, struct sorted_sector_index *sorted_sectors)
{
	unsigned int sorted_disk = 1;
	struct sorted_sector_index *item;
	struct hlist_node *item_safe;

	hlist_for_each_entry(item, &td->sorted_sectors_head, list)
	{
		unsigned int index = item - td->sorted_sectors;
		if(item->physical_sector->disk == 0)continue;

		while(sorted_devices[sorted_disk-1].available_blocks-- == 0)
			sorted_disk++;

		INIT_HLIST_NODE(&sorted_sectors[index].list);
		hlist_add_head(&sorted_sectors[index].list, &sorted_devices[sorted_disk-1].preferred_blocks);

		if(sorted_sectors[index].physical_sector->disk == sorted_devices[sorted_disk-1].dev-td->internal_devices+1)
			sorted_devices[sorted_disk-1].amount_blocks++;
	}

	//Trying to optimize a bit...
	for(sorted_disk = 1; sorted_disk <= td->internal_devices_count; ++sorted_disk)
	{
		//Iterating over each element and looking
		//for wrong elements. If there is another
		//wrong element in the corresponding disk
		//with the same access count it is just a
		//matter of the sorting algorithm and can
		//simply be swapped.
		hlist_for_each_entry_safe(item, item_safe, &sorted_devices[sorted_disk-1].preferred_blocks, list)
		{
			tdisk_index current_disk = sorted_devices[sorted_disk-1].dev-td->internal_devices+1;

			BUG_ON(item->physical_sector->disk == 0 || item->physical_sector->disk > td->internal_devices_count);

			if(item->physical_sector->disk != current_disk)
			{
				struct sorted_internal_device *corresponding = td_find_sorted_device(sorted_devices+sorted_disk-1, &td->internal_devices[item->physical_sector->disk-1], td->internal_devices_count-sorted_disk+1);
				struct sorted_sector_index *to_swap;

				if(!corresponding)continue;

				to_swap = td_find_sector_index_acc(&corresponding->preferred_blocks, current_disk, item->physical_sector->access_count);

				if(to_swap)
				{
					BUG_ON(to_swap->physical_sector->disk != current_disk);

					hlist_del_init(&item->list);
					hlist_del_init(&to_swap->list);

					hlist_add_head(&item->list, &corresponding->preferred_blocks);
					hlist_add_head(&to_swap->list, &sorted_devices[sorted_disk-1].preferred_blocks);

					corresponding->amount_blocks++;
					sorted_devices[sorted_disk-1].amount_blocks++;
				}
			}
		}
	}
}

static void td_swap_sectors(struct tdisk *td, sector_t logical_a, struct sector_index *a, sector_t logical_b, struct sector_index *b)
{
	loff_t pos_a = a->sector * td->blocksize;
	loff_t pos_b = b->sector * td->blocksize;
	u8 *buffer_a = kmalloc(td->blocksize, GFP_KERNEL);
	u8 *buffer_b = kmalloc(td->blocksize, GFP_KERNEL);
	if(!buffer_a || !buffer_b)return;

	printk(KERN_DEBUG "tDisk: swapping logical sectors %llu (disk: %u, access: %u) and %llu (disk: %u, access: %u)\n", logical_a, a->disk, a->access_count, logical_b, b->disk, b->access_count);
	//return;

	//Reading blocks from both disks
	file_read_data(td->internal_devices[a->disk-1].file, buffer_a, pos_a, td->blocksize, &td->internal_devices[a->disk-1].performance);
	file_read_data(td->internal_devices[b->disk-1].file, buffer_b, pos_b, td->blocksize, &td->internal_devices[b->disk-1].performance);

	//Saving swapped data to both disks
	file_write_data(td->internal_devices[a->disk-1].file, buffer_b, pos_a, td->blocksize, &td->internal_devices[a->disk-1].performance);
	file_write_data(td->internal_devices[b->disk-1].file, buffer_a, pos_b, td->blocksize, &td->internal_devices[b->disk-1].performance);

	swap(a->disk, b->disk);
	swap(a->sector, b->sector);

	//Updating indices
	td_perform_index_operation(td, WRITE, logical_a, a);
	td_perform_index_operation(td, WRITE, logical_b, b);

	kfree(buffer_a);
	kfree(buffer_b);
}

static bool td_move_one_sector(struct tdisk *td)
{
	bool swapped = false;
	unsigned int sorted_disk = 1;
	unsigned int inv_disk = 1;
	struct sorted_sector_index *item;
	struct sorted_internal_device *sorted_devices = vmalloc(sizeof(struct sorted_internal_device) * td->internal_devices_count);
	struct sorted_sector_index *sorted_sectors = vmalloc(sizeof(struct sorted_sector_index) * td->max_sectors);
	if(!sorted_devices || !sorted_sectors)return false;

	td_insert_sorted_internal_devices(td, sorted_devices);
	memcpy(sorted_sectors, td->sorted_sectors, sizeof(struct sorted_sector_index) * td->max_sectors);

	//Assigning the sorted sectors to the sorted devices...
	td_assign_sectors(td, sorted_devices, sorted_sectors);

	//Moving the sector with highest access count.
	for(sorted_disk = 1; sorted_disk <= td->internal_devices_count; ++sorted_disk)
	{
		printk(KERN_DEBUG "tDisk: Internal disk %u (speed: %u rank): Capacity: %llu, Correctly stored: %llu, %u percent\n",
						sorted_devices[sorted_disk-1].dev-td->internal_devices+1,
						sorted_disk,
						sorted_devices[sorted_disk-1].dev->size_blocks,
						sorted_devices[sorted_disk-1].amount_blocks,
						(size_t)sorted_devices[sorted_disk-1].amount_blocks*100 / (size_t)sorted_devices[sorted_disk-1].dev->size_blocks);
	}

	//Moving the sector with highest access count.
	for(sorted_disk = 1; sorted_disk <= td->internal_devices_count; ++sorted_disk)
	{
		struct sorted_sector_index *highest = NULL;
		tdisk_index current_disk = sorted_devices[sorted_disk-1].dev-td->internal_devices+1;

		if(sorted_devices[sorted_disk-1].amount_blocks == sorted_devices[sorted_disk-1].dev->size_blocks)
		{
			//All blocks are correctly stored. Moving on to slower disk...
			continue;
		}

		hlist_for_each_entry(item, &sorted_devices[sorted_disk-1].preferred_blocks, list)
		{
			if(item->physical_sector->disk != current_disk)
			{
				if(highest == NULL || item->physical_sector->access_count > highest->physical_sector->access_count)
				{
					highest = item;
				}
			}
		}

		BUG_ON(!highest);

		//Now starting at the slowest disk and looking
		//for a block that belongs to the current disk
		for(inv_disk = td->internal_devices_count; inv_disk != sorted_disk; --inv_disk)
		{
			struct sorted_sector_index *to_swap = td_find_sector_index(&sorted_devices[inv_disk-1].preferred_blocks, current_disk);

			if(to_swap != NULL)
			{
				//OK, now we found a sector of the possibly fastest disk
				//which is stored on the possibly slowest disk. When we
				//swap those sectors we gain the highest possible performance.

				td_swap_sectors(td, highest-sorted_sectors, highest->physical_sector, to_swap-sorted_sectors, to_swap->physical_sector);
				td->access_count_resort = 1;
				swapped = true;
				break;
			}
		}

		if(swapped)break;
	}

	vfree(sorted_devices);
	vfree(sorted_sectors);
	return swapped;
}

/**
  * This callback is used by the insertsort to determine the
  * order of a sector index
 **/
int td_sector_index_callback(struct hlist_node *a, struct hlist_node *b)
{
	struct sorted_sector_index *i_a = hlist_entry(a, struct sorted_sector_index, list);
	struct sorted_sector_index *i_b = hlist_entry(b, struct sorted_sector_index, list);

	if(i_a->physical_sector->disk == 0)return 1;
	if(i_b->physical_sector->disk == 0)return -1;

	return i_b->physical_sector->access_count - i_a->physical_sector->access_count;
}

/**
  * This function sorts the sector index just of the given
  * logical sector. This is useful for continuously soriting
  * the indices since in only touches one sector index
 **/
void td_reorganize_sorted_index(struct tdisk *td, sector_t logical_sector)
{
	struct sorted_sector_index *index;
	struct sorted_sector_index *next = NULL;
	struct sorted_sector_index *prev = NULL;

	if(logical_sector >= td->max_sectors)
	{
		printk(KERN_ERR "tDisk: Can't sort sector %llu\n", logical_sector);
		return;
	}

	//Retrieve index of logical sector and sorrounding indices
	index = &td->sorted_sectors[logical_sector];
	if(index->list.next)next = hlist_entry(index->list.next, struct sorted_sector_index, list);
	if(index->list.pprev)prev = hlist_entry(index->list.pprev, struct sorted_sector_index, list.next);

	//Check if it doen's fit anymore
	if((next && index->physical_sector->access_count < next->physical_sector->access_count) || (prev && index->physical_sector->access_count > prev->physical_sector->access_count))
	{
		//Remove it and re-insert
		td->access_count_resort = 1;
		hlist_del_init(&index->list);
		hlist_insert_sorted(&index->list, &td->sorted_sectors_head, &td_sector_index_callback);
	}
}

/**
  * This function sorts all the sector indices
  * This is useful at the loading time
 **/
void td_reorganize_all_indices(struct tdisk *td)
{
	//Sort entire arry
	hlist_insertsort(&td->sorted_sectors_head, &td_sector_index_callback);
}

/**
  * Reads the td header from the given file
  * and measures the disk performance if perf != NULL
 **/
static int td_read_header(struct file *file, struct tdisk_header *out, struct device_performance *perf)
{
	return file_read_data(file, out, 0, sizeof(struct tdisk_header), perf);
}

/**
  * Writes the td header to the given file
  * and measures the disk performance if perf != NULL
 **/
static int td_write_header(struct file *file, struct tdisk_header *header, struct device_performance *perf)
{
	memset(header->driver_name, 0, sizeof(header->driver_name));
	strcpy(header->driver_name, DRIVER_NAME);
	header->driver_name[sizeof(header->driver_name)-1] = 0;
	header->major_version = DRIVER_MAJOR_VERSION;
	header->minor_version = DRIVER_MINOR_VERSION;

	return file_write_data(file, header, 0, sizeof(struct tdisk_header), perf);
}

/**
  * Reads all the sector indices from the file and
  * stores them in data.
 **/
static int td_read_all_indices(struct tdisk *td, struct file *file, u8 *data, struct device_performance *perf)
{
	int ret = 0;
	unsigned int skip = sizeof(struct tdisk_header);
	loff_t length = td->header_size*td->blocksize - skip;
	ret = file_read_data(file, data, skip, length, perf);

	if(ret)printk(KERN_ERR "tDisk: Error reading all disk indices: %d\n", ret);
	else printk(KERN_DEBUG "tDisk: Success reading all disk indices\n");

	return ret;
}

/**
  * Writes all the sector indices to the file.
 **/
static int td_write_all_indices(struct tdisk *td, struct file *file, struct device_performance *perf)
{
	int ret = 0;

	unsigned int skip = sizeof(struct tdisk_header);
	void *data = td->indices;
	loff_t length = td->header_size*td->blocksize - skip;
	ret = file_write_data(file, data, skip, length, perf);

	if(ret)printk(KERN_ERR "tDisk: Error reading all disk indices: %d\n", ret);
	else printk(KERN_DEBUG "tDisk: Success writing all disk indices\n");

	return ret;
}

/**
  * Checks if the given disk header is compatible with the current driver.
  * It returns one of the following values:
  *  - 0 if the header is compatible (current driver version is higher or equal)
  *  - 1 if the given header was from an entirely new disk. This means it needs to be initialized
  *  - -1 if the the header was from a driver with a greater minor version than the
  *    current driver. It may be compatible...
  *  - -2 if the the header was from a driver with a greater major version than the
  *    current driver. It may be compatible but probably not...
 **/
static int td_is_compatible_header(struct tdisk *td, struct tdisk_header *header)
{
	if(strcmp(header->driver_name, DRIVER_NAME) != 0)return 1;

	if(header->major_version == DRIVER_MAJOR_VERSION)
	{
		if(header->minor_version > DRIVER_MINOR_VERSION)return -1;
	}
	else if(header->major_version > DRIVER_MAJOR_VERSION)return -2;

	return 0;
}

/**
  * This function does the actual file operations. It extracts
  * the logical sector and the data from the request. Then it
  * searches for the corresponding disk and physical sector and
  * does the actual file operation. Disk performances are also
  * recorded.
 **/
static int td_do_disk_operation(struct tdisk *td, struct request *rq)
{
	struct bio_vec bvec;
	struct req_iterator iter;
	ssize_t len;
	loff_t pos_byte;
	int ret = 0;
	struct sector_index physical_sector;

	pos_byte = (loff_t)blk_rq_pos(rq) << 9;

	//Handle flush operations
	if((rq->cmd_flags & REQ_WRITE) && (rq->cmd_flags & REQ_FLUSH))
		return td_flush_devices(td);

	//Normal file operations
	rq_for_each_segment(bvec, rq, iter)
	{
		struct file *file;
		loff_t sector = pos_byte;
		loff_t offset = __div64_32(&sector, td->blocksize);
		sector_t actual_pos_byte;

		//Fetch physical index
		td_perform_index_operation(td, READ, sector, &physical_sector);

		//Calculate actual position in the physical disk
		actual_pos_byte = physical_sector.sector*td->blocksize + offset;
		BUG_ON(actual_pos_byte > (physical_sector.sector+1)*td->blocksize);

		if(physical_sector.disk == 0 || physical_sector.disk > td->internal_devices_count)
		{
			printk_ratelimited(KERN_ERR "tDisk: found invalid disk index for reading logical sector %llu: %u\n", sector, physical_sector.disk);
			ret = -EIO;
			continue;
		}

		file = td->internal_devices[physical_sector.disk - 1].file;	//-1 because 0 means unused
		if(!file)
		{
			printk_ratelimited(KERN_DEBUG "tDisk: file is NULL, Disk probably not yet loaded...\n");
			ret = -EIO;
			continue;
		}

		if((rq->cmd_flags & REQ_WRITE) && (rq->cmd_flags & REQ_DISCARD))
		{
			//Handle discard operations
			len = bvec.bv_len;
			ret = file_alloc(file, actual_pos_byte, bvec.bv_len);
			if(ret)break;
		}
		else if(rq->cmd_flags & REQ_WRITE)
		{
			//Do write operation
			len = file_write_bio_vec(file, &bvec, &actual_pos_byte, &td->internal_devices[physical_sector.disk - 1].performance);

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
			//Do read operation
			len = file_read_bio_vec(file, &bvec, &actual_pos_byte, &td->internal_devices[physical_sector.disk - 1].performance);

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

	return ret;
}

/**
  * Read the partition table of the tDisk
 **/
static void td_reread_partitions(struct tdisk *td, struct block_device *bdev)
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

/**
  * Adds the given file descriptor to the tDisk.
  * This function reads the disk header, performs the correct
  * index operation (WRITE, READ, COMPARE), adds it to the tDisk
  * and informs user space about any changes (size, partitions...)
 **/
static int td_add_disk(struct tdisk *td, fmode_t mode, struct block_device *bdev, unsigned int arg)
{
	struct file	*file;
	struct inode *inode;
	struct address_space *mapping;
	struct tdisk_header header;
	struct device_performance perf;
	int flags = 0;
	int error;
	loff_t size;
	loff_t size_counter = 0;
	sector_t sector = 0;
	struct sector_index *physical_sector;
	struct td_internal_device *new_device = NULL;
	int index_operation_to_do;
	int first_device = (td->internal_devices_count == 0);

	//Hold a reference of the driver to prevent unwanted rmmod's
	if(first_device)__module_get(THIS_MODULE);

	error = -EBADF;
	file = fget(arg);
	if(!file)
		goto out;

	//Don't add current tDisk as backend file
	if(file_is_tdisk(file, TD_MAJOR) && file->f_mapping->host->i_bdev == bdev)
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
	size = file_get_size(file);

	error = -EINVAL;
	if(size < (((td->header_size+1)*td->blocksize) >> 9))	//Disk too small
	{
		printk(KERN_WARNING "tDisk: Can't add disk, too small: %llu. Should be at least %u\n", size, (((td->header_size+1)*td->blocksize) >> 9));
		goto out_putf;
	}

	//Subtract header size from disk size
	size -= (td->header_size*td->blocksize) >> 9;

	error = -EFBIG;
	if((loff_t)(sector_t)size != size)	//sector_t overflow
		goto out_putf;

	//Check for disk limit
	if(td->internal_devices_count == TDISK_MAX_PHYSICAL_DISKS)
	{
		printk(KERN_ERR "tDisk: Limit (255) of devices reached.\n");
		error = -ENODEV;
		goto out_putf;
	}

	if(first_device)
	{
		//Start the worker thread which handles all the disk
		//operations such as reading, writing and all the sector
		//movements.
		error = td_start_worker_thread(td);
		if(error)
		{
			printk(KERN_WARNING "tDisk: Error setting up worker thread\n");
			goto out_putf;
		}
	}

	memset(&perf, 0, sizeof(struct device_performance));
	td_read_header(file, &header, &perf);
	printk(KERN_DEBUG "tDisk: File header: driver: %s, minor: %u, major: %u\n", header.driver_name, header.major_version, header.minor_version);
	switch(td_is_compatible_header(td, &header))
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
		printk(KERN_ERR "tDisk: Bug in td_is_compatible_header function\n");
		goto out_putf;
	}
	if(header.disk_index >= td->internal_devices_count)td->internal_devices_count = header.disk_index+1;

	//Set disk references in tDisk
	new_device = &td->internal_devices[header.disk_index];
	new_device->old_gfp_mask = mapping_gfp_mask(mapping);
	mapping_set_gfp_mask(mapping, new_device->old_gfp_mask & ~(__GFP_IO|__GFP_FS));
	new_device->file = file;
	new_device->performance = header.performance;

	new_device->size_blocks = (size << 9);
	__div64_32(&new_device->size_blocks, td->blocksize);

	error = 0;

	set_device_ro(bdev, (flags & TD_FLAGS_READ_ONLY) != 0);

	td->block_device = bdev;
	td->flags = flags;

	switch(index_operation_to_do)
	{
	case WRITE:
		//Writing header to disk
		td_write_header(file, &header, &perf);

		//Setting approximate performance values using the values
		//When reading and writing the header
		perf.stdev_read_time_cycles = perf.avg_read_time_cycles = TIME_ONE_VALUE(perf.avg_read_time_cycles, perf.mod_avg_read);
		perf.stdev_write_time_cycles = perf.avg_write_time_cycles = TIME_ONE_VALUE(perf.avg_write_time_cycles, perf.mod_avg_write);
		new_device->performance = perf;

		//Save indices from previously added disks
		td_write_all_indices(td, file, &new_device->performance);

		//Save sector indices
		physical_sector = vmalloc(sizeof(struct sector_index));
		while((size_counter+td->blocksize) <= (size << 9))
		{
			int internal_ret;
			sector_t logical_sector = td->size_blocks++;
			size_counter += td->blocksize;
			physical_sector->disk = header.disk_index + 1;	//+1 because 0 means unused
			physical_sector->sector = td->header_size + sector++;
			internal_ret = td_perform_index_operation(td, WRITE, logical_sector, physical_sector);
			if(internal_ret == 1)
			{
				printk(KERN_WARNING "tDisk: Additional disk doesn't fit in index. Shrinking to fit.\n");
				break;
			}
		}
		vfree(physical_sector);
		break;
	case COMPARE:
		//Reading all indices into temporary memory
		physical_sector = vmalloc(sizeof(struct sector_index) * td->max_sectors);
		td_read_all_indices(td, file, (u8*)physical_sector, &new_device->performance);

		//Now comparing all sector indices
		for(sector = 0; sector < td->max_sectors; ++sector)
		{
			int internal_ret = td_perform_index_operation(td, COMPARE, sector, &physical_sector[sector]);
			if(internal_ret == -1)
			{
				printk_ratelimited(KERN_WARNING "tDisk: Disk index doesn't match. Probably wrong or corrupt disk attached. Pay attention before you write to disk!\n");
			}
		}

		vfree(physical_sector);
		break;
	case READ:
		//reading all indices from disk
		td_read_all_indices(td, file, (u8*)td->indices, &new_device->performance);
		td_reorganize_all_indices(td);

		for(sector = 0; sector < td->max_sectors; ++sector)
		{
			if(td->indices[sector].disk > td->internal_devices_count)
				td->internal_devices_count = td->indices[sector].disk;

			if(td->indices[sector].disk == 0)break;
		}
		td->size_blocks = sector;
		break;
	}
	printk(KERN_DEBUG "tDisk: new physical disk %u: size: %llu bytes. Logical size(%llu)\n", header.disk_index, size << 9, td->size_blocks*td->blocksize);

	if(!(flags & TD_FLAGS_READ_ONLY) && file->f_op->fsync)
		blk_queue_flush(td->queue, REQ_FLUSH);

	//let user-space know about this change
	set_capacity(td->kernel_disk, (td->size_blocks*td->blocksize) >> 9);
	bd_set_size(bdev, td->size_blocks*td->blocksize);
	kobject_uevent(&disk_to_dev(bdev->bd_disk)->kobj, KOBJ_CHANGE);

	td->state = state_bound;
	td_reread_partitions(td, bdev);

	//Grab the block_device to prevent its destruction after we
	//put /dev/tdX inode. Later in td_clear() we bdput(bdev).
	if(first_device)bdgrab(bdev);
	return 0;

 out_putf:
	fput(file);
 out:
	if(first_device)
	{
		//Release reference to the driver
		module_put(THIS_MODULE);
	}
	return error;
}

/**
  * This function removes all the internal devices and makes
  * it ready to shut down
 **/
static int td_clear(struct tdisk *td)
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

	set_capacity(td->kernel_disk, 0);

	//freeze request queue during the transition
	td_stop_worker_thread(td);

	td->state = state_cleared;
	td->block_device = NULL;

	if(bdev)
	{
		bdput(bdev);
		invalidate_bdev(bdev);

		//let user-space know about this change
		bd_set_size(bdev, 0);
		kobject_uevent(&disk_to_dev(bdev->bd_disk)->kobj, KOBJ_CHANGE);
	}

	//Release reference to the driver to make it possible to rmmod
	module_put(THIS_MODULE);

	if(bdev)td_reread_partitions(td, bdev);
	td->flags = 0;

	/*
	 * Need not hold ctl_mutex to fput backing file.
	 * Calling fput holding ctl_mutex triggers a circular
	 * lock dependency possibility warning as fput can take
	 * bd_mutex which is usually taken before ctl_mutex.
	 */
	mutex_unlock(&td->ctl_mutex);

	for(i = 0; i < td->internal_devices_count; ++i)
	{
		struct file *file = td->internal_devices[i].file;
		td->internal_devices[i].file = NULL;

		if(file)
		{
			struct tdisk_header header = {
				.disk_index = i,
				.performance = td->internal_devices[i].performance,
			};
			gfp_t gfp = td->internal_devices[i].old_gfp_mask;

			//Write current performance and index values to file
			td_write_header(file, &header, NULL);
			td_write_all_indices(td, file, NULL);

			mapping_set_gfp_mask(file->f_mapping, gfp);

			fput(file);
		}
	}
	td->internal_devices_count = 0;
	td->state = state_unbound;

	printk(KERN_DEBUG "tDisk: disk %d cleared\n", td->number);

	return 0;
}

/**
  * This function transfers devicestatus information
  * to user space
 **/
static int td_get_status(struct tdisk *td, struct tdisk_info __user *arg)
{
	struct tdisk_info info;

	if(td->state != state_bound)
		return -ENXIO;

	memset(&info, 0, sizeof(info));
	//info.block_device = huge_encode_dev(td->block_device);
	info.max_sectors = td->max_sectors;
	info.number = td->number;
	info.flags = td->flags;
	info.internaldevices = td->internal_devices_count;

	if(copy_to_user(arg, &info, sizeof(info)) != 0)
		return -EFAULT;

	return 0;
}

/**
  * This function transfers information about the internal
  * devices to user space
 **/
static int td_get_device_info(struct tdisk *td, struct internal_device_info __user *arg)
{
	struct internal_device_info info;

	if(copy_from_user(&info, arg, sizeof(struct internal_device_info)) != 0)
		return -EFAULT;

	if(info.disk == 0 || info.disk > td->internal_devices_count)
		return -EINVAL;
	
	info.performance = td->internal_devices[info.disk-1].performance;

	if(copy_to_user(arg, &info, sizeof(info)) != 0)
		return -EFAULT;

	return 0;
}

/**
  * This function transfers information about the given
  * logical sector to user space
 **/
static int td_get_sector_index(struct tdisk *td, struct sector_index __user *arg)
{
	struct sector_index index;

	if(copy_from_user(&index, arg, sizeof(struct sector_index)) != 0)
		return -EFAULT;

	if(index.sector >= td->max_sectors)return -EINVAL;

	if(copy_to_user(arg, td->sorted_sectors[index.sector].physical_sector, sizeof(struct sector_index)) != 0)
		return -EFAULT;

	return 0;
}

/**
  * This function transfers information about all
  * logical sectors to user space
 **/
static int td_get_all_sector_indices(struct tdisk *td, struct sector_info __user *arg)
{
	struct sorted_sector_index *pos;
	struct sector_info info;
	sector_t sorted_index = 0;

	hlist_for_each_entry(pos, &td->sorted_sectors_head, list)
	{
		info.physical_sector = (*pos->physical_sector);
		info.access_sorted_index = sorted_index;
		info.logical_sector = pos - td->sorted_sectors;

		if(copy_to_user(&arg[sorted_index], &info, sizeof(struct sector_info)) != 0)
			return -EFAULT;

		sorted_index++;
	}

	return 0;
}

/**
  * This function clears the access count of all
  * sectors. This is just for debugging purposes
 **/
static int td_clear_access_count(struct tdisk *td)
{
	sector_t i;

	for(i = 0; i < td->max_sectors; ++i)
	{
		td->sorted_sectors[i].physical_sector->access_count = 0;
	}

	return 0;
}

/**
  * This function handles all ioctl coming from userspace
 **/
static int td_ioctl(struct block_device *bdev, fmode_t mode, unsigned int cmd, unsigned long arg)
{
	struct tdisk *td = bdev->bd_disk->private_data;
	int err;

	mutex_lock_nested(&td->ctl_mutex, 1);
	switch(cmd)
	{
	case TDISK_ADD_DISK:
		err = td_add_disk(td, mode, bdev, arg);
		break;
	case TDISK_CLEAR:
		//td_clear would have unlocked ctl_mutex on success
		err = td_clear(td);
		if(!err)goto out_unlocked;
		break;
	case TDISK_GET_STATUS:
		err = td_get_status(td, (struct tdisk_info __user *) arg);
		break;
	case TDISK_GET_DEVICE_INFO:
		err = td_get_device_info(td, (struct internal_device_info __user *) arg);
		break;
	case TDISK_GET_SECTOR_INDEX:
		err = td_get_sector_index(td, (struct sector_index __user *) arg);
		break;
	case TDISK_GET_ALL_SECTOR_INDICES:
		err = td_get_all_sector_indices(td, (struct sector_info __user *) arg);
		break;
	case TDISK_CLEAR_ACCESS_COUNT:
		err = td_clear_access_count(td);
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
/**
  * This function handles all ioctl coming from userspace.
  * Compatibility version.
 **/
static int td_compat_ioctl(struct block_device *bdev, fmode_t mode, unsigned int cmd, unsigned long arg)
{
	int err;

	switch(cmd)
	{
	case TDISK_CLEAR:
	case TDISK_GET_STATUS:
		arg = (unsigned long)compat_ptr(arg);
	case TDISK_ADD_DISK:
		err = td_ioctl(bdev, mode, cmd, arg);
		break;
	default:
		err = -ENOIOCTLCMD;
		break;
	}
	return err;

	//Added TODO so that it doesn't compile
	TODO
}
#endif

/**
  * This function is called by the kernel whenever it
  * it needs the device. This is useful for us to count the references
 **/
static int td_open(struct block_device *bdev, fmode_t mode)
{
	struct tdisk *td = bdev->bd_disk->private_data;
	if(!td)return -ENXIO;

	atomic_inc(&td->refcount);

	return 0;
}

/**
  * This function is called by the kernel whenever it doesn't
  * need the device anymore. If we tried to clear the tDisk
  * with an elevated reference count, we will clear it here
  * once the reference count reaches 0.
 **/
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
		err = td_clear(td);
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

/**
  * The block device operations which are presented to the kernel
 **/
static const struct block_device_operations td_fops = {
	.owner =	THIS_MODULE,
	.open =		td_open,
	.release =	td_release,
	.ioctl =	td_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl =	td_compat_ioctl,
#endif
};

/**
  * This function starts the worker thread to handle
  * the actual device operations.
 **/
static int td_start_worker_thread(struct tdisk *td)
{
	init_kthread_worker(&td->worker_timeout.worker);
	td->worker_timeout.private_data = td;
	td->worker_timeout.timeout = DEFAULT_WORKER_TIMEOUT;
	td->worker_timeout.work_func = &td_queue_work;
	td->worker_task = kthread_run(kthread_worker_fn_timeout, &td->worker_timeout, "td%d", td->number);

	if(IS_ERR(td->worker_task))
		return -ENOMEM;

	set_user_nice(td->worker_task, MIN_NICE);
	return 0;
}

/**
  * Stops the worker thread
 **/
static void td_stop_worker_thread(struct tdisk *td)
{
	flush_kthread_worker_timeout(&td->worker_timeout.worker);
	kthread_stop(td->worker_task);
}

/**
  * This function is called for every request before it is
  * given to the workr thread. This makes it possible to initialize it
  * and add information
 **/
static int td_init_request(void *data, struct request *rq, unsigned int hctx_idx, unsigned int request_idx, unsigned int numa_node)
{
	struct td_command *cmd = blk_mq_rq_to_pdu(rq);

	cmd->rq = rq;

	init_thread_work_timeout(&cmd->td_work);

	return 0;
}

/**
  * This function is called by the kernel for each request
  * it reqeives. This function hands it over to the worker
  * thread.
 **/
static int td_queue_rq(struct blk_mq_hw_ctx *hctx, const struct blk_mq_queue_data *bd)
{
	struct td_command *cmd = blk_mq_rq_to_pdu(bd->rq);
	struct tdisk *td = cmd->rq->q->queuedata;

	blk_mq_start_request(bd->rq);

	if(td->state != state_bound)
		return -EIO;

	queue_kthread_work(&td->worker_timeout.worker, &cmd->td_work);

	return BLK_MQ_RQ_QUEUE_OK;
}

/**
  * This is the actual worker function which is called by
  * the worker thread. It does the file operations and moves
  * the sectors
 **/
static enum worker_status td_queue_work(void *private_data, struct kthread_work *work)
{
	struct tdisk *td = private_data;

	//Since we are not using the standard kthread_work_fn
	//It is possible that this funtion is called without work.
	//The reason behind this is that we can reorganize the indices
	//and sector operations when there is nothing to do
	if(work)
	{
		struct td_command *cmd = container_of(work, struct td_command, td_work);
		struct tdisk *td = cmd->rq->q->queuedata;
		int ret = 0;

		if(cmd->rq->cmd_flags & REQ_WRITE && (td->flags & TD_FLAGS_READ_ONLY))
			ret = -EIO;
		else
			ret = td_do_disk_operation(td, cmd->rq);

#if LINUX_VERSION_CODE <= KERNEL_VERSION(4,1,14)
		if(ret)cmd->rq->errors = -EIO;
		blk_mq_complete_request(cmd->rq);
#else
		blk_mq_complete_request(cmd->rq, ret ? -EIO : 0);
#endif //LINUX_VERSION_CODE <= KERNEL_VERSION(4,1,14)

		return next_primary_work;
	}
	else
	{
		//No work to do. This means we have reached the timeout
		//and have now the opportunity to organize the sectors.

		if(td->access_count_resort == 0)
		{
			sector_t i;
			printk(KERN_DEBUG "tDisk: nothing to do anymore. Sorting sectors\n");
			for(i = 0; i < td->size_blocks; ++i)
			{
				td_reorganize_sorted_index(td, i);
			}

			//Setting to true so that we will always do move operations
			td->access_count_resort = 1;
		}
		else
		{
			printk(KERN_DEBUG "tDisk: nothing to do anymore. Moving sectors\n");

			td->access_count_resort = 0;
			td_move_one_sector(td);
		}

		if(td->access_count_resort)return secondary_work_to_do;
		else return secondary_work_finished;
	}
}

/**
  * This struct defines the functions to dispatch requests
 **/
static struct blk_mq_ops tdisk_mq_ops = {
	.queue_rq       = td_queue_rq,
	.map_queue      = blk_mq_map_queue,
	.init_request	= td_init_request,
};

/**
  * This function creates a new tDisk with the given
  * minor number, and blocksize. max_sectors is used
  * to specify the maximum capacity of the new device
 **/
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

	//Set queue data
	err = -ENOMEM;
	td->tag_set.ops = &tdisk_mq_ops;
	td->tag_set.nr_hw_queues = 1;
	td->tag_set.queue_depth = 128;
	td->tag_set.numa_node = NUMA_NO_NODE;
	td->tag_set.cmd_size = sizeof(struct td_command);
	td->tag_set.flags = BLK_MQ_F_SHOULD_MERGE | BLK_MQ_F_SG_MERGE;
	td->tag_set.driver_data = td;

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
	td->blocksize = blocksize;
	td->index_offset_byte = index_offset_byte;
	td->header_size = header_size_byte/blocksize + ((header_size_byte%blocksize == 0) ? 0 : 1);
	td->indices = vmalloc(td->header_size*td->blocksize - index_offset_byte);
	if(!td->indices)goto out_free_queue;
	memset(td->indices, 0, td->header_size*td->blocksize - index_offset_byte);
	//printk(KERN_DEBUG "tDisk: indices created: %p - %p\n", td->indices, td->indices+td->header_size*td->blocksize);

	//Allocate sorted disk indices
	td->max_sectors = td->header_size*td->blocksize - td->index_offset_byte;
	__div64_32(&td->max_sectors, sizeof(struct sector_index));
	td->sorted_sectors = vmalloc(sizeof(struct sorted_sector_index) * td->max_sectors);
	if(!td->sorted_sectors)goto out_free_indices;
	memset(td->sorted_sectors, 0, sizeof(struct sorted_sector_index) * td->max_sectors);

	//Insert sorted indices
	INIT_HLIST_HEAD(&td->sorted_sectors_head);
	for(j = 0; j < td->max_sectors; ++j)
	{
		INIT_HLIST_NODE(&td->sorted_sectors[j].list);
		td->indices[j].access_count = 0;
		td->sorted_sectors[j].physical_sector = &td->indices[j];

		if(j == 0)hlist_add_head(&td->sorted_sectors[j].list, &td->sorted_sectors_head);
		else hlist_add_behind(&td->sorted_sectors[j].list, &td->sorted_sectors[j-1].list);
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
	vfree(td->indices);
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

/**
  * This function is called to remove a tDisk.
  * this frees all the resources
 **/
void tdisk_remove(struct tdisk *td)
{
	idr_remove(&td_index_idr, td->number);
	blk_cleanup_queue(td->queue);
	del_gendisk(td->kernel_disk);
	blk_mq_free_tag_set(&td->tag_set);
	put_disk(td->kernel_disk);
	vfree(td->sorted_sectors);
	vfree(td->indices);
	kfree(td);
}

/**
  * This function is used to find the next free minor
  * number in the idr
 **/
static int tdisk_find_free_device_callback(int id, void *ptr, void *data)
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

/**
  * This function returns the tDisk for the given
  * minor number. If the given minor number is < 0,
  * the minor number of an unbound tDisk is returned
  * or -ENODEV if there is no unbound tDisk
 **/
int tdisk_lookup(struct tdisk **t, int i)
{
	struct tdisk *td;
	int ret = -ENODEV;

	if(i < 0)
	{
		if(idr_for_each(&td_index_idr, &tdisk_find_free_device_callback, &td))
		{
			*t = td;
			ret = td->number;
		}
	}
	else
	{
		//lookup and return a specific i
		td = idr_find(&td_index_idr, i);
		if(td)
		{
			*t = td;
			ret = td->number;
		}
	}

	return ret;
}

/**
  * This funtction is the main entry point of
  * the driver. It registers the major number
  * and the control "td-control" device
 **/
static int __init tdisk_init(void)
{
	int err;

	err = register_tdisk_control();
	if(err < 0)return err;

	TD_MAJOR = register_blkdev(TD_MAJOR, "td");
	if(TD_MAJOR <= 0)
	{
		unregister_tdisk_control();
		return -EBUSY;
	}

	printk(KERN_INFO "tDisk: driver loaded\n");
	return 0;
}

/**
  * This callback function is called for each tDisk
  * at driver exit time to clean up all remaining devices.
 **/
static int tdisk_exit_callback(int id, void *ptr, void *data)
{
	tdisk_remove((struct tdisk*)ptr);
	return 0;
}

/**
  * This is the exit function for the tDisk driver. It
  * cleans up all remaining tDisks, unregisters the major
  * number and unregisters the control device "td-control"
 **/
static void __exit tdisk_exit(void)
{
	idr_for_each(&td_index_idr, &tdisk_exit_callback, NULL);
	idr_destroy(&td_index_idr);

	unregister_blkdev(TD_MAJOR, "td");

	unregister_tdisk_control();
}

module_init(tdisk_init);
module_exit(tdisk_exit);

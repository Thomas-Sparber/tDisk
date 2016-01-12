/**
  *
  * tDisk Driver
  * @author Thomas Sparber (2015)
  *
 **/

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/types.h>
#include <linux/genhd.h>
#include <linux/blkdev.h>
#include <linux/errno.h>
 
#include "tier_disk.h"

/**
  * TODO's
  *
  * - blk_noretry_request(struct request * req): indicates if a retry for a request should be made or if it should fail
  * - error checking for each vmalloc
  *
 **/
 
#define TD_FIRST_MINOR 0
#define TD_MAX_PARTITIONS 16

static int __init td_init(void);
static void __exit td_cleanup(void);
static int td_open(struct block_device *bdev, fmode_t mode);
static void td_close(struct gendisk *disk, fmode_t mode);
static void td_request_chunk_finished(void *data, unsigned int bytes);
static void td_request(struct request_queue *q);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Thomas Sparber <thomas@sparber.eu>");
MODULE_DESCRIPTION("tDisk Driver");
MODULE_ALIAS_BLOCKDEV_MAJOR(td_major);

module_init(td_init);
module_exit(td_cleanup);
 
static u_int td_major = 0;
struct td_device *td_dev;	//The disk device

//File operations for tDisk
static struct block_device_operations td_fops =
{
    .owner = THIS_MODULE,
    .open = td_open,
    .release = td_close,
};

/* 
 * Registers and initializes the driver
 */
static int __init td_init(void)
{
	printk(KERN_INFO "tDisk: Initializing driver\n");

	td_dev = tdisk_allocate(16, 512, 4);
	if(td_dev == NULL)
	{
		return -ENOMEM;
	}
 
    //Register
    td_major = register_blkdev(td_major, "td");
    if(td_major <= 0)
    {
        printk(KERN_ERR "tDisk: Unable to get Major Number\n");
        tdisk_cleanup(td_dev);
        return -EBUSY;
    }
 
    //Create request queue
    spin_lock_init(&td_dev->lock);
    td_dev->td_queue = blk_init_queue(td_request, &td_dev->lock);
    if(td_dev->td_queue == NULL)
    {
        printk(KERN_ERR "tDisk: blk_init_queue failure\n");
        unregister_blkdev(td_major, "td");
        tdisk_cleanup(td_dev);
        return -ENOMEM;
    }
	blk_queue_logical_block_size(td_dev->td_queue, td_dev->sector_size);
	blk_queue_physical_block_size(td_dev->td_queue, td_dev->sector_size);

    //Add gendisk structure
    td_dev->td_disk = alloc_disk(TD_MAX_PARTITIONS);
    if (!td_dev->td_disk)
    {
        printk(KERN_ERR "tDisk: alloc_disk failure\n");
        blk_cleanup_queue(td_dev->td_queue);
        unregister_blkdev(td_major, "td");
        tdisk_cleanup(td_dev);
        return -ENOMEM;
    }

    td_dev->td_disk->major = td_major;
    td_dev->td_disk->first_minor = TD_FIRST_MINOR;
    td_dev->td_disk->fops = &td_fops;		//Device operations
    td_dev->td_disk->private_data = &td_dev;	//Private driver data
    td_dev->td_disk->queue = td_dev->td_queue;

    sprintf(td_dev->td_disk->disk_name, "td");	//Max 32 chars
    set_capacity(td_dev->td_disk, td_dev->sectors);
 
	tdisk_add_disk(td_dev, "/dev/mapper/loop0p1", 65536, 524288 / td_dev->sector_size);

	//Add disk to system
    add_disk(td_dev->td_disk);

    printk(KERN_INFO "tDisk: driver initialised (%llu sectors; %llu bytes)\n", td_dev->sectors, td_dev->sectors * td_dev->sector_size);
 
    return 0;
}

static void __exit td_cleanup(void)
{
    del_gendisk(td_dev->td_disk);
    put_disk(td_dev->td_disk);
    blk_cleanup_queue(td_dev->td_queue);
    unregister_blkdev(td_major, "td");
    tdisk_cleanup(td_dev);
}

static int td_open(struct block_device *bdev, fmode_t mode)
{
    unsigned new_minor = iminor(bdev->bd_inode);
 
    printk(KERN_INFO "tDisk: Device %d opened\n", new_minor);
 
    if(new_minor > TD_MAX_PARTITIONS)
        return -ENODEV;

    return 0;
}

static void td_close(struct gendisk *disk, fmode_t mode)
{
    printk(KERN_INFO "tDisk: Device closed\n");
}

static void td_request_chunk_finished(void *data, unsigned int bytes)
{
	struct request *req = (struct request*)data;
	if(!__blk_end_request_cur(req, 0))
	{
		//printk(KERN_DEBUG "tDisk: Disk request finished for all chunks.\n");
	}
}
     
/*
 * Represents a block I/O request for us to execute
 */
static void td_request(struct request_queue *q)
{
    struct request *req;
 
    //Get current requests from queue
    while ((req = blk_fetch_request(q)) != NULL)
    {
#if 0
        /*
         * This function tells us whether we are looking at a filesystem request
         * - one that moves block of data
         */
        if (!blk_fs_request(req))
        {
            printk(KERN_NOTICE "tDisk: Skip non-fs request\n");
            /* We pass 0 to indicate that we successfully completed the request */
            __blk_end_request_all(req, 0);
            //__blk_end_request(req, 0, blk_rq_bytes(req));
            continue;
        }
#endif

		//struct td_device *dev = (struct td_device *)(req->rq_disk->private_data);

		int direction = rq_data_dir(req);
		sector_t start_sector = blk_rq_pos(req);
		unsigned int sector_count = blk_rq_sectors(req);
	 
		struct bio_vec bv;
		struct req_iterator iter;
	 
		sector_t sector_offset;
		unsigned int sectors;
		u8 *buffer;
	 
		int ret = 0;
		int internal_ret = 0;
	 
		//printk(KERN_DEBUG "tDisk: Direction:%d; Sec:%lld; Count:%d\n", direction, start_sector, sector_count);
	 
		//TODO Check if I really need to process bio_vec by bio_vec or if I can process all at once
		sector_offset = 0;
		rq_for_each_segment(bv, req, iter)
		{
		    if(bv.bv_len % td_dev->sector_size != 0)
		    {
		        printk(KERN_ERR "tDisk: bio size (%d) is not a multiple of TD_SECTOR_SIZE (%d).\n", bv.bv_len, td_dev->sector_size);
		        ret = -EIO;
		    }

		    buffer = page_address(bv.bv_page) + bv.bv_offset;
		    sectors = bv.bv_len / td_dev->sector_size;
		    //printk(KERN_DEBUG "tDisk: Sector Offset: %lld; Buffer: %p; Length: %d sectors\n", sector_offset, buffer, sectors);

		    internal_ret = tdisk_operation(td_dev, direction, start_sector + sector_offset, buffer, sectors, td_request_chunk_finished, req);
			if(internal_ret != 0)
			{
				printk(KERN_ERR "tDisk: Error during disk operation. See above for details\n");
				ret = internal_ret;
				break;
			}

		    sector_offset += sectors;
		}

		if(ret != 0)
		{
			__blk_end_request_all(req, ret);
		}

		if(sector_offset != sector_count)
		{
		    printk(KERN_ERR "tDisk: bio info doesn't match request info");
		    ret = -EIO;
		}
    }
}

#ifndef TDISK_H
#define TDISK_H

#include <linux/bio.h>
#include <linux/blkdev.h>
#include <linux/blk-mq.h>
#include <linux/spinlock.h>
#include <linux/mutex.h>
#include <linux/types.h>
#include <linux/atomic.h>
#include <linux/kthread.h>

#include "../include/tdisk/interface.h"

#define DRIVER_NAME "tDisk"
#define DRIVER_MAJOR_VERSION 1
#define DRIVER_MINOR_VERSION 0

typedef u8 tdisk_index;
#define TDISK_MAX_PHYSICAL_DISKS ((tdisk_index)-1 -1) /*-1 because 0 means unused*/

/**
  * Describes the header (first bytes) of a physical
  * disk. This makes it possible to identify it as a
  * member of a tDisk
 **/
struct tdisk_header
{
	char driver_name[10];	//tdisk
	__u32 major_version;
	__u32 minor_version;
	tdisk_index disk_index;	//disk index in the tdisk
	__u64 flags;			//For future releases
}; //end struct tdisk_header

/**
  * A index represents the physical location of a logical sector
 **/
struct sector_index
{
	//The disk where the logical sector is stored
	tdisk_index disk;

	//The physical sector on the disk where the logica sector is stored
	sector_t sector;
}; //end struct sector_index;

/**
  * A mapped_sector_index represents the mapping of a logical sector
  * to a physical (disk & sector) sector
 **/
struct mapped_sector_index
{
	loff_t offset;
	sector_t logical_sector;
	struct sector_index physical_sector;
}; //end struct mapped sector index

struct td_internal_device
{
	struct file *backing_file;
	gfp_t old_gfp_mask;

	unsigned int speed;
}; //end struct td_internal_device

/* Possible states of device */
enum {
	state_unbound,
	state_bound,
	state_cleared,
};

struct tdisk {
	int			number;
	atomic_t	refcount;
	loff_t		sizelimit;
	int			flags;
	char		file_name[TD_NAME_SIZE];

	struct block_device	*block_device;
	unsigned int		blocksize;
	sector_t			size_blocks;

	//The actual disks
	tdisk_index internal_devices_count;
	struct td_internal_device *internal_devices;

	spinlock_t				lock;
	int						state;
	struct mutex			ctl_mutex;

	struct kthread_worker	worker;
	struct task_struct		*worker_task;

	struct request_queue	*queue;
	struct blk_mq_tag_set	tag_set;
	struct gendisk			*kernel_disk;

	unsigned int index_offset_byte;
	unsigned int header_size;		//Size in sectors of the index where the header and sectors are stored. Located at the beginning of the disk
	u8 *indices;					//The indices of the index needs to be stored in memory
};

struct td_command {
	struct kthread_work td_work;
	struct request *rq;
	struct list_head list;
};

int tdisk_add(struct tdisk **l, int i, unsigned int blocksize, sector_t max_sectors);
int tdisk_lookup(struct tdisk **l, int i);
void tdisk_remove(struct tdisk *lo);

extern struct idr td_index_idr;

#endif //TDISK_H

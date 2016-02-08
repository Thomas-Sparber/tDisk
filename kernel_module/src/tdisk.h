/**
  *
  * tDisk Driver
  * @author Thomas Sparber (2015)
  *
 **/

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
#include "worker_timeout.h"


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
	struct device_performance performance;
	char placeholder[80];	//For future releases
}; //end struct tdisk_header

/**
  * A sorted_sector_index represents a physical sector
  * sorted according to the access_count
 **/
struct sorted_sector_index
{
	struct sector_index *physical_sector;
	struct hlist_node list;
}; //end struct mapped sector index

/**
  * A td_internal_device represents an underlying
  * physical device of a tDisk.
 **/
struct td_internal_device
{
	struct file *file;
	gfp_t old_gfp_mask;

	struct device_performance performance;

	sector_t size_blocks;
}; //end struct td_internal_device

/**
  * Possible states of a device
 **/
enum {
	state_unbound,
	state_bound,
	state_cleared,
};

/**
  * This struct represents a tDisk. It contains
  * the internal devices, the sorted sectors and
  * various other driver and kernel specific fields.
 **/
struct tdisk {
	int			number;
	atomic_t	refcount;
	int			flags;
	sector_t	max_sectors;

	struct block_device	*block_device;
	unsigned int		blocksize;
	sector_t			size_blocks;

	//The actual disks
	tdisk_index internal_devices_count;
	struct td_internal_device internal_devices[TDISK_MAX_PHYSICAL_DISKS];

	spinlock_t				lock;
	int						state;
	struct mutex			ctl_mutex;

	struct worker_timeout_data	worker_timeout;
	struct task_struct		*worker_task;

	struct request_queue	*queue;
	struct blk_mq_tag_set	tag_set;
	struct gendisk			*kernel_disk;

	unsigned int index_offset_byte;
	unsigned int header_size;		//Size in sectors of the index where the header and sectors are stored. Located at the beginning of the disk
	struct sector_index *indices;	//The indices need to be stored in memory

	struct hlist_head sorted_sectors_head;
	struct sorted_sector_index *sorted_sectors;	//The sectors sorted according to their access count;

	int access_count_resort;		//Keeps track if the access_count was updated during a file request and needs to be resorted
};

/**
  * A td_command is used by the worker thread to
  * process a request.
 **/
struct td_command {
	struct kthread_work td_work;
	struct request *rq;
	struct list_head list;
};

int tdisk_add(struct tdisk **l, int i, unsigned int blocksize, sector_t max_sectors);
int tdisk_lookup(struct tdisk **l, int i);
int tdisk_remove(struct tdisk *lo);

#endif //TDISK_H

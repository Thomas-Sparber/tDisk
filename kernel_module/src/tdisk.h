/**
  *
  * tDisk Driver
  * @author Thomas Sparber (2015-2016)
  *
 **/

#ifndef TDISK_H
#define TDISK_H

#include <tdisk/config.h>
#include <tdisk/interface.h>
#include "worker_timeout.h"
#include "tdisk_debug.h"

#pragma GCC system_header
#include <linux/atomic.h>
#include <linux/bio.h>
#include <linux/blkdev.h>
#include <linux/cdrom.h>
#include <linux/delay.h>
#include <linux/kthread.h>
#include <linux/list.h>
#include <linux/list_sort.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/sort.h>
#include <linux/spinlock.h>
#include <linux/types.h>
#include <linux/version.h>
#include <linux/vmalloc.h>

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,16,0)
#include <linux/blk-mq.h>
#endif //LINUX_VERSION_CODE >= KERNEL_VERSION(3,16,0)

/**
  * Describes the header (first bytes) of a physical
  * disk. This makes it possible to identify it as a
  * member of a tDisk
 **/
struct __attribute__((packed)) tdisk_header
{
	char driver_name[10];	//tdisk
	__u32 major_version;
	__u32 minor_version;
	struct device_performance performance;
	__u32 blocksize;
	__u64 size_blocks;
	__u64 current_max_sectors;
	tdisk_index disk_index;	//disk index in the tdisk
	char placeholder[41];	//For future releases (total 128 Byte)
}; //end struct tdisk_header

/**
  * A index represents the physical location of a logical sector
 **/
struct __attribute__((packed)) sector_index
{
	//The physical sector on the disk where the logic sector is stored
	__u64 sector;

	//This variable stores the access count of the physical sector
	__u16 access_count;

	//The disk where the logical sector is stored
	tdisk_index disk;
}; //end struct sector_index;

/**
  * A sorted_sector_index represents a physical sector
  * sorted according to the access_count.
  * This struct is used for two purposes:
  *  - total_sorted: represents the physical sector
  *    sorted by all all physical sectors of th tDisk
  *  - device_assigned: is used to assign the
  *    sorted sectors to the sorted devices. This way
  *    one object can be used for both purposes
 **/
struct sorted_sector_index
{
	struct sector_index *physical_sector;
	struct list_head total_sorted;
	struct list_head device_assigned;
}; //end struct mapped sector index

/**
  * A td_internal_device represents an underlying
  * physical device of a tDisk.
 **/
struct td_internal_device
{
	enum internal_device_type type;
	char name[TDISK_MAX_INTERNAL_DEVICE_NAME];
	char path[TDISK_MAX_INTERNAL_DEVICE_NAME];
	struct file *file;
	gfp_t old_gfp_mask;

	struct device_performance performance;

	sector_t size_blocks;

	/**
	  * The sector which is used for sector move operations. To guarantee no data loss
	 **/
	sector_t move_help_sector;
}; //end struct td_internal_device

/**
  * This struct represents an internal device but sorted
  * accorting to its performance
 **/
struct sorted_internal_device
{
	/**
	  * The blocks which should be - according to performance
	  * and block access count - stored on the device
	 **/
	struct list_head preferred_blocks;

	/**
	  * The actual device
	 **/
	struct td_internal_device *dev;

	/**
	  * This variable is just internally to count the blocks
	 **/
	sector_t available_blocks;

	/**
	  * Also used internally to count the currently correct blocks
	 **/
	sector_t amount_blocks;

}; //end struct sorted_internal_device

/**
  * This struct represents a tDisk. It contains
  * the internal devices, the sorted sectors and
  * various other driver and kernel specific fields.
 **/
struct tdisk {
	int				number;
	atomic_t		refcount;
	int				flags;
	unsigned int	blocksize;
	sector_t		max_sectors;
	sector_t		size_blocks;

	//The amount in percentage of the total storage of cache buffer
	unsigned int	percent_cache;
	sector_t		cache_sectors;

	//The internal devices
	tdisk_index						internal_devices_count;
	struct td_internal_device		internal_devices[TDISK_MAX_PHYSICAL_DISKS];
	struct sorted_internal_device	*sorted_devices;

	spinlock_t				tdisk_lock;
	struct mutex			ctl_mutex;

	bool					optimizing;
	bool					modifying;

	struct worker_timeout_data	worker_timeout;
	struct task_struct		*worker_task;

#if LINUX_VERSION_CODE < KERNEL_VERSION(3,16,0)
	spinlock_t				queue_lock;
#else
	struct blk_mq_tag_set	tag_set;
#endif //LINUX_VERSION_CODE < KERNEL_VERSION(3,16,0)

	struct request_queue	*queue;
	struct gendisk			*kernel_disk;
	struct block_device		*block_device;

	unsigned int index_offset_byte;
	unsigned int header_size;		//Size in sectors of the index where the header and sectors are stored. Located at the beginning of the disk
	struct sector_index *indices;	//The indices need to be stored in memory

	struct list_head sorted_sectors_head;
	struct sorted_sector_index *sorted_sectors;	//The sectors sorted according to their access count;

	int access_count_resort;		//Keeps track if the access_count was updated during a file request and needs to be resorted

	struct debug_struct debug;	//Used to save debugging info
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

/**
  * Adds a tDisk with the given minronumber and
  * blocksize to the system.
 **/
int tdisk_add(struct tdisk **td, struct tdisk_add_parameters *params);

/**
  * Finds the tDisk with the given minornumber
 **/
int tdisk_lookup(struct tdisk **td, int i);

/**
  * Removes the given tDisk from the system
 **/
int tdisk_remove(struct tdisk *td);

#endif //TDISK_H

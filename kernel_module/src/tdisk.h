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

	struct file			*lo_backing_file;
	struct block_device	*block_device;
	unsigned int		blocksize;
	sector_t			size_blocks;

	gfp_t		old_gfp_mask;

	spinlock_t				lock;
	int						state;
	struct mutex			ctl_mutex;

	struct kthread_worker	worker;
	struct task_struct	*worker_task;

	struct request_queue	*queue;
	struct blk_mq_tag_set	tag_set;
	struct gendisk			*kernel_disk;

	unsigned int index_size;		//Size in sectors of the index where the sectors are stored. Located at the beginning of the disk
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
extern struct mutex td_index_mutex;

#endif //TDISK_H

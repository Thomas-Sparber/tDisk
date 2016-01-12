#ifndef TDISK_H
#define TDISK_H

#include <linux/bio.h>
#include <linux/blkdev.h>
#include <linux/blk-mq.h>
#include <linux/spinlock.h>
#include <linux/mutex.h>
#include <linux/types.h>

#include "../include/tdisk/interface.h"

/* Possible states of device */
enum {
	state_unbound,
	state_bound,
	state_cleared,
};

struct tdisk {
	int			number;
	int			refcount;
	loff_t		sizelimit;
	int			flags;
	char		file_name[TD_NAME_SIZE];

	struct file			*lo_backing_file;
	struct block_device	*block_device;
	unsigned int		blocksize;

	gfp_t		old_gfp_mask;

	spinlock_t				lock;
	struct workqueue_struct *wq;
	struct list_head		write_cmd_head;
	struct work_struct		write_work;
	bool					write_started;
	int						state;
	struct mutex			ctl_mutex;

	struct request_queue	*queue;
	struct blk_mq_tag_set	tag_set;
	struct gendisk			*kernel_disk;

	unsigned int index_size;		//Size in sectors of the index where the sectors are stored. Located at the beginning of the disk
	u8 *indices;					//The indices of the index needs to be stored in memory
	spinlock_t free_sectors_lock;
	struct MBdeque *free_sectors;	//TODO
};

struct td_command {
	struct work_struct read_work;
	struct request *rq;
	struct list_head list;
};

int tdisk_add(struct tdisk **l, int i, unsigned int blocksize, sector_t max_sectors);
int tdisk_lookup(struct tdisk **l, int i);
void tdisk_remove(struct tdisk *lo);

extern struct idr td_index_idr;
extern struct mutex td_index_mutex;

#endif //TDISK_H

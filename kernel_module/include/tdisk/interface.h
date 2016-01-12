#ifndef TDISK_INTERFACE_H
#define TDISK_INTERFACE_H

#include <linux/types.h>

#define TD_NAME_SIZE	64

/*
 * tDisk flags
 */
enum {
	TD_FLAGS_READ_ONLY	= 1,
	TD_FLAGS_AUTOCLEAR	= 4
};


struct tdisk_info {
	__u64		   block_device;		/* ioctl r/o */
	__u64		   sizelimit;			/* bytes, 0 == max available */
	__u32		   number;				/* ioctl r/o */
	__u32		   flags;				/* ioctl r/o */
	__u8		   file_name[TD_NAME_SIZE];
};

/*
 * IOCTL commands --- we will commandeer 0x4C ('L')
 */

#define TDISK_SET_FD		0x4C00
#define TDISK_CLR_FD		0x4C01
#define TDISK_SET_STATUS	0x4C04
#define TDISK_GET_STATUS	0x4C05
//#define TDISK_CHANGE_FD	0x4C06
#define TDISK_SET_CAPACITY	0x4C07

/* /dev/loop-control interface */
#define TDISK_CTL_ADD		0x4C80
#define TDISK_CTL_REMOVE	0x4C81
#define TDISK_CTL_GET_FREE	0x4C82

#endif //TDISK_INTERFACE_H

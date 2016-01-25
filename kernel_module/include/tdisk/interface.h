#ifndef TDISK_INTERFACE_H
#define TDISK_INTERFACE_H

#include <linux/types.h>

#define DRIVER_NAME "tDisk"
#define DRIVER_MAJOR_VERSION 1
#define DRIVER_MINOR_VERSION 0

typedef __u8 tdisk_index;

/*
 * tDisk flags
 */
enum {
	TD_FLAGS_READ_ONLY	= 1,
	TD_FLAGS_AUTOCLEAR	= 4
};

/**
  * A index represents the physical location of a logical sector
 **/
struct sector_index
{
	//The disk where the logical sector is stored
	tdisk_index disk;

	//The physical sector on the disk where the logic sector is stored
	__u64 sector;

	//This variable stores the access count of the physical sector
	__u16 access_count;
}; //end struct sector_index;

struct tdisk_info {
	__u64		   block_device;		/* ioctl r/o */
	__u64		   sizelimit;			/* bytes, 0 == max available */
	__u32		   number;				/* ioctl r/o */
	__u32		   flags;				/* ioctl r/o */
};

/*
 * IOCTL commands --- we will commandeer 0x4C ('L')
 */

#define TDISK_SET_FD			0x4C00
#define TDISK_CLR_FD			0x4C01
//#define TDISK_SET_STATUS		0x4C04
#define TDISK_GET_STATUS		0x4C05
#define TDISK_GET_MAX_SECTORS	0x4C06
#define TDISK_GET_SECTOR_INDEX	0x4C07
#define TDISK_GET_ALL_SECTOR_INDICES 0x4C08
#define TDISK_CLEAR_ACCESS_COUNT 0x4C09
//#define TDISK_SET_CAPACITY		0x4C09

/* /dev/loop-control interface */
#define TDISK_CTL_ADD			0x4C80
#define TDISK_CTL_REMOVE		0x4C81
#define TDISK_CTL_GET_FREE		0x4C82

#endif //TDISK_INTERFACE_H

/**
  *
  * tDisk Driver
  * @author Thomas Sparber (2015)
  *
 **/

#ifndef TDISK_INTERFACE_H
#define TDISK_INTERFACE_H

#include <linux/types.h>

#define DRIVER_NAME "tDisk"
#define DRIVER_MAJOR_VERSION 1
#define DRIVER_MINOR_VERSION 0

/**
  * Amount of past operations which are averaged
  * over the time to measure the performance of a device
  * this value must be seen as a bit shift value. e.g
  * (1 << MEASUE_RECORDS_SHIFT) --> (1 << 16) = 65536
 **/
#define MEASUE_RECORDS_SHIFT 16

#define TDISK_BLOCKSIZE_MOD 4096

/**
  * The data type which is used to store the disk indices.
  * This is very important because it is also stored on the
  * disks itself (@see struct sector_index)
  * A tDisk can only have as much disks as this data type
  * supports. BUT larger data type means more overhead.
 **/
typedef __u8 tdisk_index;

#define TDISK_MAX_PHYSICAL_DISKS ((tdisk_index)-1 -1) /*-1 because 0 means unused*/

/*
 * tDisk flags
 */
enum {
	TD_FLAGS_READ_ONLY	= 1,
	TD_FLAGS_AUTOCLEAR	= 4
};

/**
  * This struct is used when a tDisk with a specific
  * minornumber should be added. It just bundles two variables
 **/
struct tdisk_add_parameters
{
	int minornumber;
	unsigned int blocksize;
}; //end tdisk_add_parameters

/**
  * This struct represents performance indicators of a
  * physical disk. It contains the average and standard
  * deviation values in processor cycles of read and write
  * operations.
  * Additionally, the residue of the averaging operations
  * is stored to be able to use fixed point numbers instead
  * of floating point.
 **/
struct device_performance
{
	__u64 avg_read_time_cycles;
	__u64 stdev_read_time_cycles;

	__u64 avg_write_time_cycles;
	__u64 stdev_write_time_cycles;


	__u32 mod_avg_read;
	__u32 mod_stdev_read;

	__u32 mod_avg_write;
	__u32 mod_stdev_write;
}; //end struct device_performance

/**
  * This struct is used to transfer internal device
  * specific information to user space
 **/
struct internal_device_info
{
	tdisk_index disk;
	struct device_performance performance;
}; //end struct internal_device_info

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

/**
  * Thsi struct is used to transfer sector specific information
  * to user space.
 **/
struct sector_info
{
	__u64 logical_sector;
	__u64 access_sorted_index;
	struct sector_index physical_sector;
}; //end struct sector_info

/**
  * Thsi struct is used to transfer tDisk information
  * to user space.
 **/
struct tdisk_info {
	//__u64			block_device;		/* ioctl r/o */
	__u64			max_sectors;		/* ioctl r/o */
	__u32			number;				/* ioctl r/o */
	__u32			flags;				/* ioctl r/o */
	tdisk_index		internaldevices;	/* ioctl r/o */
};

/*
 * IOCTL commands --- we will commandeer 0x4C ('L')
 */

#define TDISK_ADD_DISK			0x4C00
//#define TDISK_SET_STATUS		0x4C04
#define TDISK_GET_STATUS		0x4C05
#define TDISK_GET_DEVICE_INFO	0x4C06
#define TDISK_GET_SECTOR_INDEX	0x4C07
#define TDISK_GET_ALL_SECTOR_INDICES 0x4C08
#define TDISK_CLEAR_ACCESS_COUNT 0x4C09
//#define TDISK_SET_CAPACITY		0x4C09

// /dev/td-control interface
#define TDISK_CTL_ADD			0x4C80
#define TDISK_CTL_REMOVE		0x4C81
#define TDISK_CTL_GET_FREE		0x4C82

#endif //TDISK_INTERFACE_H

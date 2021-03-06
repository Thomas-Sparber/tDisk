/**
  *
  * tDisk Driver
  * @author Thomas Sparber (2015)
  *
 **/

#ifndef TDISK_INTERFACE_H
#define TDISK_INTERFACE_H

#include <tdisk/config.h>

#pragma GCC system_header
#include <linux/types.h>

#define DRIVER_NAME "tDisk"
#define DRIVER_MAJOR_VERSION 1
#define DRIVER_MINOR_VERSION 0

#define TO_XSTRING(s) TO_STRING(s)
#define TO_STRING(s) #s
#define DRIVER_VERSION_STRING DRIVER_NAME " " TO_XSTRING(DRIVER_MAJOR_VERSION) "." TO_XSTRING(DRIVER_MINOR_VERSION)

/**
  * Amount of past operations which are averaged
  * over the time to measure the performance of a device
  * this value must be seen as a bit shift value. e.g
  * (1 << MEASUE_RECORDS_SHIFT) --> (1 << 16) = 65536
 **/
#define MEASURE_RECORDS_SHIFT 12

#define TDISK_BLOCKSIZE_MOD 4096

#define TDISK_MAX_INTERNAL_DEVICE_NAME 256

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
	unsigned int percent_cache;
}; //end tdisk_add_parameters

/**
  * Defines the type on an internal device
 **/
enum internal_device_type
{
	internal_device_type_file = 0,
	internal_device_type_plugin = 1,
}; //end enum internal_device_type

/**
  * This struct is used for the "ADD_DISK"
  * ioctl to add a specific device to a tDisk
 **/
struct internal_device_add_parameters
{
	char name[TDISK_MAX_INTERNAL_DEVICE_NAME];
	char path[TDISK_MAX_INTERNAL_DEVICE_NAME];
	unsigned int fd;
	int format;
	enum internal_device_type type;
}; //end struct internal_device_add_parameters

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
	char name[TDISK_MAX_INTERNAL_DEVICE_NAME];
	char path[TDISK_MAX_INTERNAL_DEVICE_NAME];
	struct device_performance performance;
	__u64 size;
	enum internal_device_type type;
	tdisk_index disk;
	__u64 bytes_read;
	__u64 bytes_written;
}; //end struct internal_device_info

/**
  * A index represents the physical location of a logical sector
 **/
struct physical_sector_index
{
	//The physical sector on the disk where the logic sector is stored
	__u64 sector;

	//This variable stores the access count of the physical sector
	__u16 access_count;

	//The disk where the logical sector is stored
	tdisk_index disk;

	//A Flag which determines whether the sector is used or not
	__u8 used;
}; //end struct sector_index;

/**
  * Thsi struct is used to transfer sector specific information
  * to user space.
 **/
struct sector_info
{
	struct physical_sector_index physical_sector;
	__u64 logical_sector;
	__u64 access_sorted_index;
}; //end struct sector_info

/**
  * Thsi struct is used to transfer tDisk information
  * to user space.
 **/
struct tdisk_info {
	__u64			max_sectors;
	__u64			size_blocks;
	__u64			bytes_optimized;
	__u32			blocksize;
	__u32			number;
	__u32			flags;
	tdisk_index		internaldevices;
};


/****************** tDisk debugging *****************/

/**
  * This struct is used to store information about the current
  * operations of a tdisk
 **/
struct tdisk_debug_info
{
	/** The id of the debug info **/
	__u64 id;

	/** The current source file **/
	char file[256];

	/** The current line in the file **/
	int line;

	/** The current function **/
	char function[256];

	/** A custom message **/
	char message[256];

	/** The time of the debugging info in jiffies **/
	__u64 time;

	/** The value of HZ (x HZ = 1 sec) **/
	int hz;

}; //end struct tdisk_debug_info


/****************** IOCTL commands ******************/

#define TDISK_ADD_DISK					0x4C00
#define TDISK_GET_STATUS				0x4C01
#define TDISK_GET_DEVICE_INFO			0x4C02
#define TDISK_GET_SECTOR_INDEX			0x4C03
#define TDISK_GET_ALL_SECTOR_INDICES	0x4C04
#define TDISK_CLEAR_ACCESS_COUNT		0x4C05
#define TDISK_GET_DEBUG_INFO			0x4C06
#define TDISK_REMOVE_DISK				0x4C07

// /dev/td-control interface
#define TDISK_CTL_ADD			0x4C80
#define TDISK_CTL_REMOVE		0x4C81
#define TDISK_CTL_GET_FREE		0x4C82


/****************** Netlink interfaces **************/

enum nl_tdisk_msg_types {
	NLTD_CMD_READ = 0,
	NLTD_CMD_WRITE,
	NLTD_CMD_FINISHED,
	NLTD_CMD_REGISTER,
	NLTD_CMD_UNREGISTER,
	NLTD_CMD_SIZE,
	NLTD_CMD_MAX
};

enum nl_tdisk_attr {
	NLTD_UNSPEC,
	NLTD_PLUGIN_NAME,
	NLTD_REQ_NUMBER,
	NLTD_REQ_OFFSET,
	NLTD_REQ_LENGTH,
	NLTD_REQ_RET,
	NLTD_REQ_BUFFER,
	__NLTD_MAX,
};
#define NLTD_MAX (__NLTD_MAX - 1)

#define NLTD_NAME DRIVER_NAME
#define NLTD_HEADER_SIZE 0
#define NLTD_VERSION 1

#define NLTD_MAX_NAME TDISK_MAX_INTERNAL_DEVICE_NAME

#endif //TDISK_INTERFACE_H

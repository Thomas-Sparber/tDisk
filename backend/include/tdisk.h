/**
  *
  * tDisk backend
  * @author Thomas Sparber (2015-2016)
  * 
  * This file is used to directly communicate with the kernel module.
  *
 **/

#ifndef TDISK_H
#define TDISK_H

#include <stdint.h>

/**
  * Defines the mex length of the name of an internal device
 **/
#define F_TDISK_MAX_INTERNAL_DEVICE_NAME 256

/**
  * Frontend version
  * Defines the type on an internal device
 **/
enum f_internal_device_type
{

	/** The internal device is a file. Can also be a blockdevice **/
	f_internal_device_type_file,
	
	/** The internal device is a plugin. **/
	f_internal_device_type_plugin
	
}; //end enum internal_device_type

/**
  * Frontend version
  * This struct represents performance indicators of a
  * physical disk. It contains the average and standard
  * deviation values in processor cycles of read and write
  * operations.
  * Additionally, the residue of the averaging operations
  * is stored to be able to use fixed point numbers instead
  * of floating point.
 **/
struct f_device_performance
{
	uint64_t avg_read_time_cycles;
	uint64_t stdev_read_time_cycles;

	uint64_t avg_write_time_cycles;
	uint64_t stdev_write_time_cycles;


	uint32_t mod_avg_read;
	uint32_t mod_stdev_read;

	uint32_t mod_avg_write;
	uint32_t mod_stdev_write;
}; //end struct f_device_performance

/**
  * Frontend version
  * This struct is used to transfer internal device
  * specific information to user space
 **/
struct f_internal_device_info
{
	unsigned int disk;
	enum f_internal_device_type type;
	char name[F_TDISK_MAX_INTERNAL_DEVICE_NAME];
	char path[F_TDISK_MAX_INTERNAL_DEVICE_NAME];
	uint64_t size;
	struct f_device_performance performance;
}; //end struct f_internal_device_info

/**
  * Frontend version
  * A index represents the physical location of a logical sector
 **/
struct f_sector_index
{
	//The disk where the logical sector is stored
	unsigned int disk;

	//The physical sector on the disk where the logic sector is stored
	uint64_t sector;

	//This variable stores the access count of the physical sector
	uint16_t access_count;

	//Whether the sector is used or not
	uint8_t used;
}; //end struct f_sector_index;

/**
  * Frontend version
  * This struct is used to transfer sector specific information
  * to user space.
 **/
struct f_sector_info
{
	uint64_t logical_sector;
	uint64_t access_sorted_index;
	struct f_sector_index physical_sector;
}; //end struct f_sector_info

/**
  * Frontend version
  * This struct is used to retrieve debug information from a tdisk
 **/
struct f_tdisk_debug_info
{
	/** The id of the debug info **/
	uint64_t id;

	/** The current source file **/
	char file[256];

	/** The current line in the file **/
	int line;

	/** The current function **/
	char function[256];

	/** A custom message **/
	char message[256];

	/** The time of the debugging info **/
	double time;

}; //end struct tdisk_debug_info


/**
  * Returns the current amount of registered tDisks
 **/
int tdisk_get_devices_count();

/**
  * Gets the names of the current tDisks
  * @param devices The outpug buffer where the names should be stored
  * @param size The size of each member in the output buffer
  * @param length The length of the output buffer
  * @returns The amount of devices or a negative number when an error
  * happened
 **/
int tdisk_get_devices(char **devices, int size, int length);

/**
  * Adds a new device with the next free minor number to the system.
  * @param out_name The name of the new device will be stored here
  * @param blocksize The blocksize of the new device
  * @returns The minor number of the new device or a negative error code
 **/
int tdisk_add(char *out_name, unsigned int blocksize);

/**
  * Adds a new device with the given minor number to the system.
  * @param out_name The name of the new device will be stored here
  * @param minor The minor number of the new device
  * @param blocksize The blocksize of the new device
  * @returns The minor number of the new device or a negative error code
 **/
int tdisk_add_specific(char *out_name, int minor, unsigned int blocksize);

/**
  * Removes the device with the given minor number from the system.
 **/
int tdisk_remove(int device);

/**
  * Adds a new internal device to the tDisk
  * @param device The tDisk where to add the new internal device
  * @param new_disk The new internal device to be added to the tDisk.
  * @param format Defines whether the disk should be formatted
  * If it is a file path it is added as file. If not it is added as a
  * plugin.
 **/
int tdisk_add_disk(const char *device, const char *new_disk, int format);

/**
  * Removes the internal device with the given id
  * from the tDisk
 **/
int tdisk_remove_disk(const char *device, unsigned int disk);

/**
  * Gets the current maximum amount of sectors for the given tDisk
 **/
int tdisk_get_max_sectors(const char *device, uint64_t *out);

/**
  * Gets the current size in bytes for the given tDisk
 **/
int tdisk_get_size_bytes(const char *device, uint64_t *out);

/**
  * Gets the blocksize for the given tDisk
 **/
int tdisk_get_blocksize(const char *device, uint32_t *out);

/**
  * Gets information about the given sector index
 **/
int tdisk_get_sector_index(const char *device, uint64_t logical_sector, struct f_sector_index *out);

/**
  * Returns infomration about all sector indices
  * @param device The tDisk to get all sector incides from
  * @param out An array of f_sector_info to store the infomration
  * @param size The size of the array
 **/
int tdisk_get_all_sector_indices(const char *device, struct f_sector_info *out, uint64_t size);

/**
  * Resets the access count of all sectors
 **/
int tdisk_clear_access_count(const char *device);

/**
  * Gets the amount of internal devices for the given tDisk
 **/
int tdisk_get_internal_devices_count(const char *device, unsigned int *out);

/**
  * Gets device information of the device with the given id
 **/
int tdisk_get_device_info(const char *device, unsigned int disk, struct f_internal_device_info *out);

/**
  * Gets debug information about the given tDisk
 **/
int tdisk_get_debug_info(const char *device, uint64_t current_id, struct f_tdisk_debug_info *out);

/**
  * Gets the measure shift.
  * (1 << measure_shift) = number ov averaged performance measures
 **/
unsigned int get_measure_records_shift();

/**
  * Gets the number by which the blocksize must be dividable
 **/
unsigned int get_tdisk_blocksize_mod();

/**
  * Gets the number of max supported internal disks
 **/
unsigned int get_tdisk_max_physical_disks();

#endif //TDISK_H

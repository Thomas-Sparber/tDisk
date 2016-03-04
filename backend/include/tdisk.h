/**
  *
  * tDisk Driver
  * @author Thomas Sparber (2015)
  *
 **/

#ifndef TDISK_H
#define TDISK_H

#include <stdint.h>

#define EDRVNTLD 1234
#define ENOPERM 1235

#define F_TDISK_MAX_INTERNAL_DEVICE_NAME 256

/**
  * Frontend version
  * Defines the type on an internal device
 **/
enum f_internal_device_type
{
	f_internal_device_type_file,
	f_internal_device_type_plugin
}; //end enum internal_device_type

/**
  * Frontend version
  * This struct is used for the "ADD_DISK"
  * ioctl to add a specific device to a tDisk
 **/
/*struct f_internal_device_add_parameters
{
	enum f_internal_device_type type;
	char name[F_TDISK_MAX_INTERNAL_DEVICE_NAME];
	unsigned int fd;
}; //end struct internal_device_add_parameters*/

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



int tdisk_get_devices_count();
int tdisk_get_devices(char **devices, int size, int length);

int tdisk_add(char *out_name, unsigned int blocksize);
int tdisk_add_specific(char *out_name, int minor, unsigned int blocksize);
int tdisk_remove(int device);

int tdisk_add_disk(const char *device, const char *new_disk);

int tdisk_get_max_sectors(const char *device, uint64_t *out);
int tdisk_get_sector_index(const char *device, uint64_t logical_sector, struct f_sector_index *out);
int tdisk_get_all_sector_indices(const char *device, struct f_sector_info *out, uint64_t size);
int tdisk_clear_access_count(const char *device);

int tdisk_get_internal_devices_count(const char *device, unsigned int *out);
int tdisk_get_device_info(const char *device, unsigned int disk, struct f_internal_device_info *out);

unsigned int get_measure_records_shift();
unsigned int get_tdisk_blocksize_mod();
unsigned int get_tdisk_max_physical_disks();

#endif //TDISK_H

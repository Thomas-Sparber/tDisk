/**
  *
  * tDisk Driver
  * @author Thomas Sparber (2015)
  *
 **/

#ifndef __linux__

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
 
#include <tdisk.h>

#define UNUSED(x) x=x

int tdisk_get_devices_count()
{
	return 2;
}

int tdisk_get_devices(char **devices, int size, int length)
{
	if(length < 1)return 0;
	snprintf(devices[0], (size_t)size, "td0");
	if(length < 2)return 1;
	snprintf(devices[1], (size_t)size, "td1");
	return 2;
}

int tdisk_add(char *out_name, unsigned int blocksize)
{
	UNUSED(blocksize);

	sprintf(out_name, "/dev/td0");
	return 0;
}

int tdisk_add_specific(char *out_name, int minor, unsigned int blocksize)
{
	UNUSED(blocksize);

	sprintf(out_name, "/dev/td%d", minor);
	return minor;
}

int tdisk_remove(int device)
{
	UNUSED(device);
	
	return 0;
}

int tdisk_add_disk(const char *device, const char *new_disk)
{
	UNUSED(device);
	UNUSED(new_disk);

	return 0;
}

int tdisk_get_max_sectors(const char *device, uint64_t *out)
{
	UNUSED(device);

	(*out) = 1024;
	return 0;
}

int tdisk_get_sector_index(const char *device, uint64_t logical_sector, struct f_sector_index *out)
{
	UNUSED(device);
	UNUSED(logical_sector);

	out->disk = (unsigned int) (rand() % 3);
	out->sector = (uint64_t) (rand() % 1024);
	out->access_count = (uint16_t) (rand() % 32768);
	return 0;
}

int tdisk_get_all_sector_indices(const char *device, struct f_sector_info *out, uint64_t size)
{
	uint64_t i;

	for(i = 0; i < size; ++i)
	{
		out->logical_sector = i;
		out->access_sorted_index = size-i-1;
		tdisk_get_sector_index(device, i, &out[i].physical_sector);
	}

	return 0;
}

int tdisk_clear_access_count(const char *device)
{
	UNUSED(device);

	return 0;
}

int tdisk_get_internal_devices_count(const char *device, unsigned int *out)
{
	UNUSED(device);

	(*out) = 3;
	return 0;
}

int tdisk_get_device_info(const char *device, unsigned int disk, struct f_internal_device_info *out)
{
	UNUSED(device);

	out->disk = disk;
	
	switch(disk)
	{
	case 1:
		out->type = f_internal_device_type_file;
		snprintf(out->name, F_TDISK_MAX_INTERNAL_DEVICE_NAME, "disk1");
		break;
	case 2:
		out->type = f_internal_device_type_file;
		snprintf(out->name, F_TDISK_MAX_INTERNAL_DEVICE_NAME, "disk2");
		break;
	case 3:
		out->type = f_internal_device_type_plugin;
		snprintf(out->name, F_TDISK_MAX_INTERNAL_DEVICE_NAME, "dropbox_user@domain.com_disk1");
		break;
	}
	
	out->performance.avg_read_time_cycles = (uint64_t) (rand() % 1000000);
	out->performance.stdev_read_time_cycles = (uint64_t) (rand() % 1000000);
	out->performance.avg_write_time_cycles = (uint64_t) (rand() % 1000000);
	out->performance.stdev_write_time_cycles = (uint64_t) (rand() % 1000000);
	out->performance.mod_avg_read = (uint32_t) (rand() % 16384);
	out->performance.mod_stdev_read = (uint32_t) (rand() % 16384);
	out->performance.mod_avg_write = (uint32_t) (rand() % 16384);
	out->performance.mod_stdev_write = (uint32_t) (rand() % 16384);

	return 0;
}

unsigned int get_measure_records_shift()
{
	return 14;
}

unsigned int get_tdisk_blocksize_mod()
{
	return 4096;
}

unsigned int get_tdisk_max_physical_disks()
{
	return 255;
}

#endif //__linux__

/**
  *
  * tDisk backend
  * @author Thomas Sparber (2015-2016)
  *
  * This file contains basic communication functions to the linux kernel
  * and is only available in linux
 **/

#ifdef __linux__
#define _GNU_SOURCE

#include <tdisk.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <errno.h>
#include <dirent.h>
#include <string.h>
#include <stdlib.h>

#include <tdisk/interface.h>

#if TDISK_MAX_INTERNAL_DEVICE_NAME != F_TDISK_MAX_INTERNAL_DEVICE_NAME
	#warning Interface changed: TDISK_MAX_INTERNAL_DEVICE_NAME != F_TDISK_MAX_INTERNAL_DEVICE_NAME
#endif

#define CONTROL_FILE "/dev/td-control"

inline static void set_sector_index(struct f_sector_index *target, const struct sector_index *source)
{
	target->disk = source->disk;
	target->sector = source->sector;
	target->access_count = source->access_count;
}

inline static void set_sector_info(struct f_sector_info *target, const struct sector_info *source)
{
	target->logical_sector = source->logical_sector;
	target->access_sorted_index = source->access_sorted_index;
	set_sector_index(&target->physical_sector, &source->physical_sector);
}

inline static void set_device_performance(struct f_device_performance *target, const struct device_performance *source)
{
	target->avg_read_time_cycles = source->avg_read_time_cycles;
	target->stdev_read_time_cycles = source->stdev_read_time_cycles;
	target->avg_write_time_cycles = source->avg_write_time_cycles;
	target->stdev_write_time_cycles = source->stdev_write_time_cycles;

	target->mod_avg_read = source->mod_avg_read;
	target->mod_stdev_read = source->mod_stdev_read;
	target->mod_avg_write = source->mod_avg_write;
	target->mod_stdev_write = source->mod_stdev_write;
}

inline static void set_internal_device_info(struct f_internal_device_info *target, const struct internal_device_info *source)
{
	target->disk = source->disk;
	strncpy(target->name, source->name, TDISK_MAX_INTERNAL_DEVICE_NAME);
	strncpy(target->path, source->path, TDISK_MAX_INTERNAL_DEVICE_NAME);
	target->type = source->type;
	target->size = source->size;
	set_device_performance(&target->performance, &source->performance);
}

int check_td_control()
{
	return (access(CONTROL_FILE, F_OK) != -1);
}

void build_device_name(int device, char *out_name)
{
	sprintf(out_name, "/dev/td%d", device);
}

int tdisk_get_devices_count()
{
	DIR *dir;
	struct dirent *ent;
	int counted = 0;
	char *end;

	if((dir = opendir("/dev")) != NULL)
	{
		while((ent = readdir (dir)) != NULL)
		{
			size_t len = strlen(ent->d_name);
			if(len < 3)continue;
			if(strncmp(ent->d_name, "td", 2) != 0)continue;
			strtol(ent->d_name+2, &end, 10);
			if(end != ent->d_name + len)continue;
			counted++;
		}
		closedir(dir);

		return counted;
	}
	else
	{
		return -1;
	}
}

int tdisk_get_devices(char **devices, int size, int length)
{
	DIR *dir;
	struct dirent *ent;
	int counter = 0;
	char *end;

	if((dir = opendir("/dev")) != NULL)
	{
		while((ent = readdir (dir)) != NULL)
		{
			size_t len = strlen(ent->d_name);
			if(len < 3)continue;
			if(strncmp(ent->d_name, "td", 2) != 0)continue;
			strtol(ent->d_name+2, &end, 10);
			if(end != ent->d_name + len)continue;
			if(counter == length)return -2;
			strncpy(devices[counter++], ent->d_name, (size_t)size);
		}
		closedir(dir);

		return counter;
	}
	else
	{
		return -1;
	}
}

int tdisk_add(char *out_name, unsigned int blocksize)
{
	int controller;
	int device;

	if(!check_td_control())return -ENODEV;

	controller = open(CONTROL_FILE, O_RDWR);
	if(controller < 0)return -EACCES;

	device = ioctl(controller, TDISK_CTL_GET_FREE, blocksize);

	if(device >= 0)
		build_device_name(device, out_name);

	close(controller);

	return device;
}

int tdisk_add_specific(char *out_name, int minor, unsigned int blocksize)
{
	int controller;
	int device;

	if(!check_td_control())return -ENODEV;

	controller = open(CONTROL_FILE, O_RDWR);
	if(controller < 0)return -EACCES;

	struct tdisk_add_parameters params = {
		.minornumber = minor,
		.blocksize = blocksize
	};

	device = ioctl(controller, TDISK_CTL_ADD, &params);

	if(device >= 0)
		build_device_name(device, out_name);

	close(controller);

	return device;
}

int tdisk_remove(int device)
{
	int controller;
	int ret;

	if(!check_td_control())return -ENODEV;

	controller = open(CONTROL_FILE, O_RDWR);
	if(controller < 0)return -EACCES;

	ret = ioctl(controller, TDISK_CTL_REMOVE, device);

	close(controller);

	return ret;
}

int tdisk_add_disk(const char *device, const char *new_disk)
{
	int dev;
	int ret;
	int file;
	struct internal_device_add_parameters parameters;

	if(!check_td_control())return -ENODEV;

	file = open(new_disk, O_RDWR/* | O_SYNC | O_DIRECT*/);
	if(file > 0)
	{
		parameters.type = internal_device_type_file;
		parameters.fd = (unsigned int)file;
	}
	else
	{
		parameters.type = internal_device_type_plugin;
	}
	strncpy(parameters.name, new_disk, TDISK_MAX_INTERNAL_DEVICE_NAME);	//TODO??
	strncpy(parameters.path, new_disk, TDISK_MAX_INTERNAL_DEVICE_NAME);

	dev = open(device, O_RDWR);
	if(dev < 0)
	{
		close(file);
		return -EACCES;
	}

	ret = ioctl(dev, TDISK_ADD_DISK, &parameters);

	close(dev);
	close(file);

	return ret;
}

int tdisk_get_max_sectors(const char *device, uint64_t *out)
{
	int dev;
	int ret;

	if(!check_td_control())return -ENODEV;

	dev = open(device, O_RDWR);
	if(dev < 0)return -EACCES;

	struct tdisk_info info;
	ret = ioctl(dev, TDISK_GET_STATUS, &info);
	(*out) = info.max_sectors;

	close(dev);

	return ret;
}

int tdisk_get_size_bytes(const char *device, uint64_t *out)
{
	int dev;
	int ret;

	if(!check_td_control())return -ENODEV;

	dev = open(device, O_RDWR);
	if(dev < 0)return -EACCES;

	struct tdisk_info info;
	ret = ioctl(dev, TDISK_GET_STATUS, &info);
	(*out) = info.size_blocks * info.blocksize;

	close(dev);

	return ret;
}

int tdisk_get_sector_index(const char *device, uint64_t logical_sector, struct f_sector_index *out)
{
	int dev;
	int ret;
	struct sector_index temp;

	if(!check_td_control())return -ENODEV;

	dev = open(device, O_RDWR);
	if(dev < 0)return -EACCES;

	temp.sector = logical_sector;
	ret = ioctl(dev, TDISK_GET_SECTOR_INDEX, &temp);
	set_sector_index(out, &temp);

	close(dev);

	return ret;
}

int tdisk_get_all_sector_indices(const char *device, struct f_sector_info *out, uint64_t size)
{
	uint64_t i;
	int dev;
	int ret;
	struct sector_info *temp = malloc((size_t)(sizeof(struct sector_info) * size));
	if(!temp)return -ENOMEM;

	if(!check_td_control())
	{
		free(temp);
		return -ENODEV;
	}

	dev = open(device, O_RDWR);
	if(dev < 0)
	{
		free(temp);
		return -EACCES;
	}

	ret = ioctl(dev, TDISK_GET_ALL_SECTOR_INDICES, temp);
	
	for(i = 0; i < size; ++i)
		set_sector_info(&out[i], &temp[i]);

	free(temp);

	close(dev);

	return ret;
}

int tdisk_clear_access_count(const char *device)
{
	int dev;
	int ret;

	if(!check_td_control())return -ENODEV;

	dev = open(device, O_RDWR);
	if(dev < 0)return -EACCES;

	ret = ioctl(dev, TDISK_CLEAR_ACCESS_COUNT);

	close(dev);

	return ret;
}

int tdisk_get_internal_devices_count(const char *device, unsigned int *out)
{
	int dev;
	int ret;

	if(!check_td_control())return -ENODEV;

	dev = open(device, O_RDWR);
	if(dev < 0)return -EACCES;

	struct tdisk_info info;
	ret = ioctl(dev, TDISK_GET_STATUS, &info);
	(*out) = info.internaldevices;

	close(dev);

	return ret;
}

int tdisk_get_device_info(const char *device, unsigned int disk, struct f_internal_device_info *out)
{
	int dev;
	int ret;
	struct internal_device_info temp;

	if(!check_td_control())return -ENODEV;

	dev = open(device, O_RDWR);
	if(dev < 0)return -EACCES;

	temp.disk = (tdisk_index)disk;
	ret = ioctl(dev, TDISK_GET_DEVICE_INFO, &temp);
	set_internal_device_info(out, &temp);

	close(dev);

	return ret;
}

unsigned int get_measure_records_shift()
{
	return MEASURE_RECORDS_SHIFT;
}

unsigned int get_tdisk_blocksize_mod()
{
	return TDISK_BLOCKSIZE_MOD;
}

unsigned int get_tdisk_max_physical_disks()
{
	return TDISK_MAX_PHYSICAL_DISKS;
}

#endif //__linux__

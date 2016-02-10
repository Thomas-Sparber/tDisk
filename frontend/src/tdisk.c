/**
  *
  * tDisk Driver
  * @author Thomas Sparber (2015)
  *
 **/

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

#define CONTROL_FILE "/dev/td-control"

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

	if(!check_td_control())return -EDRVNTLD;

	controller = open(CONTROL_FILE, O_RDWR);
	if(controller < 0)return -ENOPERM;

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

	if(!check_td_control())return -EDRVNTLD;

	controller = open(CONTROL_FILE, O_RDWR);
	if(controller < 0)return -ENOPERM;

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

	if(!check_td_control())return -EDRVNTLD;

	controller = open(CONTROL_FILE, O_RDWR);
	if(controller < 0)return -ENOPERM;

	ret = ioctl(controller, TDISK_CTL_REMOVE, device);

	close(controller);

	return ret;
}

int tdisk_add_disk(const char *device, const char *new_disk)
{
	int dev;
	int ret;
	int file;

	if(!check_td_control())return -EDRVNTLD;

	file = open(new_disk, O_RDWR/* | O_SYNC | O_DIRECT*/);
	if(!file)return -EIO;

	dev = open(device, O_RDWR);
	if(dev < 0)
	{
		close(file);
		return -ENOPERM;
	}

	ret = ioctl(dev, TDISK_ADD_DISK, file);

	close(dev);
	close(file);

	return ret;
}

int tdisk_get_max_sectors(const char *device, __u64 *out)
{
	int dev;
	int ret;

	if(!check_td_control())return -EDRVNTLD;

	dev = open(device, O_RDWR);
	if(dev < 0)return -ENOPERM;

	struct tdisk_info info;
	ret = ioctl(dev, TDISK_GET_STATUS, &info);
	(*out) = info.max_sectors;

	close(dev);

	return ret;
}

int tdisk_get_sector_index(const char *device, __u64 logical_sector, struct sector_index *out)
{
	int dev;
	int ret;

	if(!check_td_control())return -EDRVNTLD;

	dev = open(device, O_RDWR);
	if(dev < 0)return -ENOPERM;

	out->sector = logical_sector;
	ret = ioctl(dev, TDISK_GET_SECTOR_INDEX, out);

	close(dev);

	return ret;
}

int tdisk_get_all_sector_indices(const char *device, struct sector_info *out)
{
	int dev;
	int ret;

	if(!check_td_control())return -EDRVNTLD;

	dev = open(device, O_RDWR);
	if(dev < 0)return -ENOPERM;

	ret = ioctl(dev, TDISK_GET_ALL_SECTOR_INDICES, out);

	close(dev);

	return ret;
}

int tdisk_clear_access_count(const char *device)
{
	int dev;
	int ret;

	if(!check_td_control())return -EDRVNTLD;

	dev = open(device, O_RDWR);
	if(dev < 0)return -ENOPERM;

	ret = ioctl(dev, TDISK_CLEAR_ACCESS_COUNT);

	close(dev);

	return ret;
}

int tdisk_get_internal_devices_count(const char *device, tdisk_index *out)
{
	int dev;
	int ret;

	if(!check_td_control())return -EDRVNTLD;

	dev = open(device, O_RDWR);
	if(dev < 0)return -ENOPERM;

	struct tdisk_info info;
	ret = ioctl(dev, TDISK_GET_STATUS, &info);
	(*out) = info.internaldevices;

	close(dev);

	return ret;
}

int tdisk_get_device_info(const char *device, tdisk_index disk, struct internal_device_info *out)
{
	int dev;
	int ret;

	if(!check_td_control())return -EDRVNTLD;

	dev = open(device, O_RDWR);
	if(dev < 0)return -ENOPERM;

	out->disk = disk;
	ret = ioctl(dev, TDISK_GET_DEVICE_INFO, out);

	close(dev);

	return ret;
}

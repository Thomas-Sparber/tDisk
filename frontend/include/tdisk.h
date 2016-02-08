/**
  *
  * tDisk Driver
  * @author Thomas Sparber (2015)
  *
 **/

#ifndef TDISK_H
#define TDISK_H

#include <tdisk/interface.h>

#define EDRVNTLD 1234
#define ENOPERM 1235

int tdisk_get_devices_count();
int tdisk_get_devices(char **devices, int size, int length);

int tdisk_add(char *out_name);
int tdisk_remove(int device);
int tdisk_add_specific(char *out_name, int minor);

int tdisk_add_disk(const char *device, const char *new_disk);

int tdisk_get_max_sectors(const char *device, __u64 *out);
int tdisk_get_sector_index(const char *device, __u64 logical_sector, struct sector_index *out);
int tdisk_get_all_sector_indices(const char *device, struct sector_info *out);
int tdisk_clear_access_count(const char *device);

int tdisk_get_internal_devices_count(const char *device, tdisk_index *out);
int tdisk_get_device_info(const char *device, tdisk_index disk, struct internal_device_info *out);

#endif //TDISK_H

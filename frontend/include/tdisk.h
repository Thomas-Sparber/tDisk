#ifndef TDISK_H
#define TDISK_H

#include <tdisk/interface.h>

#define EDRVNTLD 1234
#define ENOPERM 1235

int tdisk_add(char *out_name);
int tdisk_remove(int device);
int tdisk_add_specific(char *out_name, int minor);

int tdisk_add_disk(const char *device, const char *new_disk);
int tdisk_clear(const char *device);

int tdisk_get_max_sectors(const char *device, __u64 *out);
int tdisk_get_sector_index(const char *device, __u64 logical_sector, struct sector_index *out);

#endif //TDISK_H

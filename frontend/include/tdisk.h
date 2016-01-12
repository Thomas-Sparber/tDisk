#ifndef TDISK_H
#define TDISK_H

#define EDRVNTLD 1234
#define ENOPERM 1235

int tdisk_add(char *out_name);
int tdisk_remove(int device);
int tdisk_add_specific(char *out_name, int minor);

int tdisk_add_disk(const char *device, const char *new_disk);
int tdisk_clear(const char *device);

#endif //TDISK_H

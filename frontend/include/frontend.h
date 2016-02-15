#ifndef FRONTEND_H
#define FRONTEND_H

int add_tDisk(int argc, char *args[], char *format, char *out, int out_length);
int remove_tDisk(int argc, char *args[], char *format, char *out, int out_length);
int add_disk(int argc, char *args[], char *format, char *out, int out_length);
int get_max_sectors(int argc, char *args[], char *format, char *out, int out_length);
int get_sector_index(int argc, char *args[], char *format, char *out, int out_length);
int get_all_sector_indices(int argc, char *args[], char *format, char *out, int out_length);
int clear_access_count(int argc, char *args[], char *format, char *out, int out_length);
int get_internal_devices_count(int argc, char *args[], char *format, char *out, int out_length);
int get_device_info(int argc, char *args[], char *format, char *out, int out_length);
int load_config_file(int argc, char *args[], char *format, char *out, int out_length);

#endif //FRONTEND_H

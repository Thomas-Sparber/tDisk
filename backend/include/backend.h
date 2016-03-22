/**
  *
  * tDisk Driver
  * @author Thomas Sparber (2015)
  *
  * This file contains the C interface definitions for the
  * tDisk backend. It is possible to perform all major tDisk
  * tasks using the following functions
  *
 **/

#ifndef BACKEND_H
#define BACKEND_H

struct Options;

/**
  * C version of get_tDisks. Look at the C++ version for more details.
 **/
int get_tDisks(int argc, char *args[], struct Options *options, char *out, int out_length);

/**
  * C version of get_tDisk. Look at the C++ version for more details.
 **/
int get_tDisk(int argc, char *args[], struct Options *options, char *out, int out_length);

/**
  * C version of add_tDisk. Look at the C++ version for more details.
 **/
int add_tDisk(int argc, char *args[], struct Options *options, char *out, int out_length);

/**
  * C version of remove_tDisk. Look at the C++ version for more details.
 **/
int remove_tDisk(int argc, char *args[], struct Options *options, char *out, int out_length);

/**
  * C version of add_disk. Look at the C++ version for more details.
 **/
int add_disk(int argc, char *args[], struct Options *options, char *out, int out_length);

/**
  * C version of get_max_sectors. Look at the C++ version for more details.
 **/
int get_max_sectors(int argc, char *args[], struct Options *options, char *out, int out_length);

/**
  * C version of get_size_bytes. Look at the C++ version for more details.
 **/
int get_size_bytes(int argc, char *args[], struct Options *options, char *out, int out_length);

/**
  * C version of get_sector_index. Look at the C++ version for more details.
 **/
int get_sector_index(int argc, char *args[], struct Options *options, char *out, int out_length);

/**
  * C version of get_all_sector_indices. Look at the C++ version for more details.
 **/
int get_all_sector_indices(int argc, char *args[], struct Options *options, char *out, int out_length);

/**
  * C version of clear_access_count. Look at the C++ version for more details.
 **/
int clear_access_count(int argc, char *args[], struct Options *options, char *out, int out_length);

/**
  * C version of get_internal_devices_count. Look at the C++ version for more details.
 **/
int get_internal_devices_count(int argc, char *args[], struct Options *options, char *out, int out_length);

/**
  * C version of get_device_info. Look at the C++ version for more details.
 **/
int get_device_info(int argc, char *args[], struct Options *options, char *out, int out_length);

/**
  * C version of load_config_file. Look at the C++ version for more details.
 **/
int load_config_file(int argc, char *args[], struct Options *options, char *out, int out_length);

/**
  * C version of get_device_advice. Look at the C++ version for more details.
 **/
int get_device_advice(int argc, char *args[], struct Options *options, char *out, int out_length);

/************************* Options ****************************/

/**
  * This function creates an empty options object
  * which can be passed to the backend functions
 **/
struct Options* create_options(void);

/**
  * This function created default options for the
  * backend functions
 **/
struct Options* create_default_options(void);

/**
  * This function frees the previously created
  * options
 **/
void free_options(struct Options *o);

#endif //BACKEND_H

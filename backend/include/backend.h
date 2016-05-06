/**
  *
  * tDisk backend
  * @author Thomas Sparber (2015-2016)
  *
  * This file contains the C interface definitions for the
  * tDisk backend. It is possible to perform all major tDisk
  * tasks using the following functions
  *
 **/

#ifndef BACKEND_H
#define BACKEND_H

struct Options;
struct BackendResult;

/**
  * C version of get_tDisks. Look at the C++ version for more details.
 **/
struct BackendResult* get_tDisks(int argc, char *args[], struct Options *options);

/**
  * C version of get_tDisk. Look at the C++ version for more details.
 **/
struct BackendResult* get_tDisk(int argc, char *args[], struct Options *options);

/**
  * C version of get_devices. Look at the C++ version for more details.
 **/
struct BackendResult* get_devices(int argc, char *args[], struct Options *options);

/**
  * C version of create_tDisk. Look at the C++ version for more details.
 **/
struct BackendResult* create_tDisk(int argc, char *args[], struct Options *options);

/**
  * C version of add_tDisk. Look at the C++ version for more details.
 **/
struct BackendResult* add_tDisk(int argc, char *args[], struct Options *options);

/**
  * C version of remove_tDisk. Look at the C++ version for more details.
 **/
struct BackendResult* remove_tDisk(int argc, char *args[], struct Options *options);

/**
  * C version of add_disk. Look at the C++ version for more details.
 **/
struct BackendResult* add_disk(int argc, char *args[], struct Options *options);

/**
  * C version of get_max_sectors. Look at the C++ version for more details.
 **/
struct BackendResult* get_max_sectors(int argc, char *args[], struct Options *options);

/**
  * C version of get_size_bytes. Look at the C++ version for more details.
 **/
struct BackendResult* get_size_bytes(int argc, char *args[], struct Options *options);

/**
  * C version of get_sector_index. Look at the C++ version for more details.
 **/
struct BackendResult* get_sector_index(int argc, char *args[], struct Options *options);

/**
  * C version of get_all_sector_indices. Look at the C++ version for more details.
 **/
struct BackendResult* get_all_sector_indices(int argc, char *args[], struct Options *options);

/**
  * C version of clear_access_count. Look at the C++ version for more details.
 **/
struct BackendResult* clear_access_count(int argc, char *args[], struct Options *options);

/**
  * C version of get_internal_devices_count. Look at the C++ version for more details.
 **/
struct BackendResult* get_internal_devices_count(int argc, char *args[], struct Options *options);

/**
  * C version of get_device_info. Look at the C++ version for more details.
 **/
struct BackendResult* get_device_info(int argc, char *args[], struct Options *options);

/**
  * C version of load_config_file. Look at the C++ version for more details.
 **/
struct BackendResult* load_config_file(int argc, char *args[], struct Options *options);

/**
  * C version of get_device_advice. Look at the C++ version for more details.
 **/
struct BackendResult* get_device_advice(int argc, char *args[], struct Options *options);

/**
  * C version of get_files_at. Look at the C++ version for more details.
 **/
struct BackendResult* get_files_at(int argc, char *args[], struct Options *options);

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

/*********************** BackendResult ************************/

/**
  * Deallocates the memory used by the given backendResult
 **/
void BackendResult_delete(struct BackendResult *r);

/**
  * Returns the result string of the Backendresult
 **/
const char* BackendResult_result(struct BackendResult *r);

/**
  * Saves the message string of the Backendresult in the buffer
 **/
void BackendResult_messages(struct BackendResult *r, char *out, size_t length);

/**
  * Saves the warning string of the Backendresult in the buffer
 **/
void BackendResult_warnings(struct BackendResult *r, char *out, size_t length);

/**
  * Saves the error string of the Backendresult in the buffer
 **/
void BackendResult_errors(struct BackendResult *r, char *out, size_t length);

#endif //BACKEND_H

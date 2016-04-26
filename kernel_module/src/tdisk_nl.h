/**
  *
  * tDisk Driver
  * @author Thomas Sparber (2015-2016)
  *
 **/

#ifndef TDISK_NL
#define TDISK_NL

#include <tdisk/interface.h>

/**
  * Defines the timeout of a plugin request.
  * After this time period the request is finished
  * with an error
 **/
#define NLTD_TIMEOUT_SECS 30

/**
  * The callback type which is used for asynchronous
  * plugin requests
 **/
typedef void (*plugin_callback)(void*,int);

/**
  * Registers the netlink family and starts the timeout thread
 **/
int nltd_register(void);

/**
  * Finishes all pending requests, stops the timeout
  * thread and unregisters the netlink family
 **/
void nltd_unregister(void);

/**
  * Checks whether the plugin with the given name
  * is registered
 **/
int nltd_is_registered(const char *plugin);

/**
  * Returns the size of the plugin disk.
  * This function actually sends a SIZE request
  * to the plugin synchronously.
 **/
loff_t nltd_get_size(const char *plugin);

/**
  * Asynchronously read data from the plugin
 **/
void nltd_read_async(const char *plugin, loff_t offset, char *buffer, int length, plugin_callback callback, void *userobject);

/**
  * Asynchronously write data to the plugin
 **/
void nltd_write_async(const char *plugin, loff_t offset, char *buffer, int length, plugin_callback callback, void *userobject);

/**
  * Synchronously read data from the plugin.
  * The struct sync_request is used to wait for
  * completion of the request
 **/
int nltd_read_sync(const char *plugin, loff_t offset, char *buffer, int length);

/**
  * Synchronously write data to the plugin.
  * The struct sync_request is used to wait for
  * completion of the request
 **/
int nltd_write_sync(const char *plugin, loff_t offset, char *buffer, int length);

#endif //TDISK_NL

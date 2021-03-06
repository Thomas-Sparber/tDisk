/**
  *
  * tDisk Driver
  * @author Thomas Sparber (2015-2016)
  *
 **/

#ifndef TDISK_PLUGIN_H
#define TDISK_PLUGIN_H

#include <tdisk/config.h>
#include "tdisk_nl.h"

#ifdef USE_PLUGINS

/**
  * Checks whether a plugin with the given name is
  * available
 **/
inline static bool plugin_is_loaded(const char *plugin)
{
	return nltd_is_registered(plugin);
}

/**
  * Returns the size of a plugin 
 **/
inline static loff_t plugin_get_size(const char *plugin)
{
	return nltd_get_size(plugin);
}

/**
  * This function writes the given bytes to
  * plugin at the given position.
 **/
inline static int plugin_write_data(const char *plugin, void *data, loff_t pos, unsigned int length)
{
	return nltd_write_sync(plugin, pos, data, length);
}

/**
  * This function reads the given bytes from
  * plugin at the given position.
 **/
inline static int plugin_read_data(const char *plugin, void *data, loff_t pos, unsigned int length)
{
	return nltd_read_sync(plugin, pos, data, length);
}

/**
  * This function writes the given bio_vec to
  * plugin at the given position.
 **/
inline static int plugin_write_bio_vec(const char *plugin, struct bio_vec *bvec, loff_t *pos)
{
	char *data = page_address(bvec->bv_page) + bvec->bv_offset;
	int ret = plugin_write_data(plugin, data, (*pos), bvec->bv_len);
	if(ret == 0)
	{
		(*pos) += bvec->bv_len;
		return (int)bvec->bv_len;
	}
	return ret;
}

/**
  * This function reads the given bio_vec from
  * plugin at the given position.
 **/
inline static int plugin_read_bio_vec(const char *plugin, struct bio_vec *bvec, loff_t *pos)
{
	char *data = page_address(bvec->bv_page) + bvec->bv_offset;
	int ret = plugin_read_data(plugin, data, (*pos), bvec->bv_len);
	if(ret == 0)
	{
		(*pos) += bvec->bv_len;
		return (int)bvec->bv_len;
	}
	return ret;
}

struct aio_plugin_data
{
	void *private_data;
	void (*callback)(void*,long);
}; //end struct aio_plugin_data

inline static void plugin_aio_complete(void *private_data, long ret)
{
	struct aio_plugin_data *data = private_data;
	if(data->callback)data->callback(data->private_data, ret);
	kfree(data);
}

/**
  * This function writes the given bytes to
  * plugin at the given position.
 **/
inline static void plugin_write_data_async(const char *plugin, void *data, loff_t pos, unsigned int length, void *private_data, void (*callback)(void*,long))
{
	struct aio_plugin_data *aio_data = kmalloc(sizeof(struct aio_plugin_data), GFP_KERNEL);

	aio_data->callback = callback;
	aio_data->private_data = private_data;

	nltd_write_async(plugin, pos, data, length, plugin_aio_complete, aio_data);
}

/**
  * This function reads the given bytes from
  * plugin at the given position.
 **/
inline static void plugin_read_data_async(const char *plugin, void *data, loff_t pos, unsigned int length, void *private_data, void (*callback)(void*,long))
{
	struct aio_plugin_data *aio_data = kmalloc(sizeof(struct aio_plugin_data), GFP_KERNEL);

	aio_data->callback = callback;
	aio_data->private_data = private_data;

	nltd_read_async(plugin, pos, data, length, plugin_aio_complete, aio_data);
}

/**
  * This function writes the given bio_vec to
  * plugin at the given position.
 **/
inline static void plugin_write_bio_vec_async(const char *plugin, struct bio_vec *bvec, loff_t pos, void *private_data, void (*callback)(void*,long))
{
	char *data = page_address(bvec->bv_page) + bvec->bv_offset;
	plugin_write_data_async(plugin, data, pos, bvec->bv_len, private_data, callback);
}

/**
  * This function reads the given bio_vec from
  * plugin at the given position.
 **/
inline static void plugin_read_bio_vec_async(const char *plugin, struct bio_vec *bvec, loff_t pos, void *private_data, void (*callback)(void*,long))
{
	char *data = page_address(bvec->bv_page) + bvec->bv_offset;
	plugin_read_data_async(plugin, data, pos, bvec->bv_len, private_data, callback);
}

/**
  * Flushes the given plugin
 **/
inline static int plugin_flush(const char *plugin)
{
	//TODO
	plugin = plugin;
	return 0;
}

#else
#pragma message "Plugins are disabled"
#endif //USE_PLUGINS

#endif //TDISK_PLUGIN_H

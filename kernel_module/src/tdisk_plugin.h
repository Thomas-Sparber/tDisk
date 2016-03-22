/**
  *
  * tDisk Driver
  * @author Thomas Sparber (2015-2016)
  *
 **/

#ifndef TDISK_PLUGIN_H
#define TDISK_PLUGIN_H

#include "tdisk_nl.h"
#include "tdisk_performance.h"

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
  * plugin at the given position. It also measures the performance
 **/
inline static int plugin_write_data(const char *plugin, void *data, loff_t pos, unsigned int length, struct device_performance *perf)
{
	int ret;
	cycles_t time;

	time = get_cycles();
	ret = nltd_write_sync(plugin, pos, data, length);
	update_performance(WRITE, get_cycles()-time, perf);

	return ret;
}

/**
  * This function reads the given bytes from
  * plugin at the given position. It also measures the performance
 **/
inline static int plugin_read_data(const char *plugin, void *data, loff_t pos, unsigned int length, struct device_performance *perf)
{
	int ret;
	cycles_t time;

	time = get_cycles();
	ret = nltd_read_sync(plugin, pos, data, length);
	update_performance(READ, get_cycles()-time, perf);

	return ret;
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

/**
  * This function writes the given bio_vec to
  * plugin at the given position. It also measures the performance
 **/
inline static int plugin_write_bio_vec(const char *plugin, struct bio_vec *bvec, loff_t *pos, struct device_performance *perf)
{
	char *data = page_address(bvec->bv_page) + bvec->bv_offset;
	int ret = plugin_write_data(plugin, data, (*pos), bvec->bv_len, perf);
	if(ret == 0)
	{
		(*pos) += bvec->bv_len;
		return bvec->bv_len;
	}
	return ret;
}

/**
  * This function reads the given bio_vec from
  * plugin at the given position. It also measures the performance
 **/
inline static int plugin_read_bio_vec(const char *plugin, struct bio_vec *bvec, loff_t *pos, struct device_performance *perf)
{
	char *data = page_address(bvec->bv_page) + bvec->bv_offset;
	int ret = plugin_read_data(plugin, data, (*pos), bvec->bv_len, perf);
	if(ret == 0)
	{
		(*pos) += bvec->bv_len;
		return bvec->bv_len;
	}
	return ret;
}

#endif //TDISK_PLUGIN_H

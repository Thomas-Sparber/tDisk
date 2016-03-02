#ifndef TDISK_PLUGIN_H
#define TDISK_PLUGIN_H

#include "tdisk_nl.h"
#include "tdisk_performance.h"

inline static bool plugin_is_loaded(const char *plugin)
{
	return nltd_is_registered(plugin);
}

inline static loff_t plugin_get_size(const char *plugin)
{
	return nltd_get_size(plugin);
}

inline static int plugin_write_data(const char *plugin, void *data, loff_t pos, unsigned int length, struct device_performance *perf)
{
	int ret;
	cycles_t time;

	time = get_cycles();
	ret = nltd_write_sync(plugin, pos, data, length);
	update_performance(WRITE, get_cycles()-time, perf);

	return ret;
}

inline static int plugin_read_data(const char *plugin, void *data, loff_t pos, unsigned int length, struct device_performance *perf)
{
	int ret;
	cycles_t time;

	time = get_cycles();
	ret = nltd_read_sync(plugin, pos, data, length);
	update_performance(WRITE, get_cycles()-time, perf);

	return ret;
}

inline static int plugin_flush(const char *plugin)
{
	//TODO
	plugin = plugin;
	return 0;
}

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

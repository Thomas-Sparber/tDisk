/**
  *
  * tDisk Driver
  * @author Thomas Sparber (2015-2016)
  *
 **/

#ifndef TDSIK_DEVICE_OPERATIONS_H
#define TDSIK_DEVICE_OPERATIONS_H

#include <tdisk/config.h>
#include "helpers.h"
#include "tdisk_file.h"
#include "tdisk_plugin.h"

/**
  * Generic function that writes data to a device.
  * The device can be a file or a plugin.
 **/
inline static int write_data(struct td_internal_device *device, void *data, loff_t position, unsigned int length)
{
	int ret;

#ifdef MEASURE_PERFORMANCE
	struct timespec startTime;
	struct timespec endTime;

	getnstimeofday(&startTime);
#else
//#pragma message "Performance measurement is disabled"
#endif //MEASURE_PERFORMANCE

	switch(device->type)
	{
#ifdef USE_FILES
	case internal_device_type_file:
		if(unlikely(!device->file))return -ENODEV;
		ret = file_write_data(device->file, data, position, length);
		break;
#else
#pragma message "Files are disabled"
#endif //USE_FILES

#ifdef USE_PLUGINS
	case internal_device_type_plugin:
		ret = plugin_write_data(device->name, data, position, length);
		break;
#else
#pragma message "Plugins are disabled"
#endif //USE_PLUGINS

	default:
		printk(KERN_ERR "tDisk: Invalid internal device type: %d\n", device->type);
		MY_BUG_ON(true, PRINT_INT(device->type));
		ret = -EINVAL;
		break;
	}

#ifdef MEASURE_PERFORMANCE
	getnstimeofday(&endTime);
	update_performance(WRITE, &startTime, &endTime, length, &device->performance);
#else
//#pragma message "Performance measurement is disabled"
#endif //MEASURE_PERFORMANCE

	return ret;
}

/**
  * Generic function that reads data from a device.
  * The device can be a file or a plugin.
 **/
inline static int read_data(struct td_internal_device *device, void *data, loff_t position, unsigned int length)
{
	int ret;

#ifdef MEASURE_PERFORMANCE
	struct timespec startTime;
	struct timespec endTime;

	getnstimeofday(&startTime);
#else
//#pragma message "Performance measurement is disabled"
#endif //MEASURE_PERFORMANCE

	switch(device->type)
	{
#ifdef USE_FILES
	case internal_device_type_file:
		if(unlikely(!device->file))return -ENODEV;
		ret = file_read_data(device->file, data, position, length);
		break;
#else
#pragma message "Files are disabled"
#endif //USE_FILES

#ifdef USE_PLUGINS
	case internal_device_type_plugin:
		ret = plugin_read_data(device->name, data, position, length);
		break;
#else
#pragma message "Plugins are disabled"
#endif //USE_PLUGINS

	default:
		printk(KERN_ERR "tDisk: Invalid internal device type: %d\n", device->type);
		MY_BUG_ON(true, PRINT_INT(device->type));
		ret = -EINVAL;
		break;
	}

#ifdef MEASURE_PERFORMANCE
	getnstimeofday(&endTime);
	update_performance(READ, &startTime, &endTime, length, &device->performance);
#else
//#pragma message "Performance measurement is disabled"
#endif //MEASURE_PERFORMANCE

	return ret;
}

/**
  * Generic function that writes a bio_vec to a device.
  * The device can be a file or a plugin.
 **/
inline static int write_bio_vec(struct td_internal_device *device, struct bio_vec *bvec, loff_t *position)
{
	int ret;

#ifdef MEASURE_PERFORMANCE
	struct timespec startTime;
	struct timespec endTime;

	getnstimeofday(&startTime);
#else
//#pragma message "Performance measurement is disabled"
#endif //MEASURE_PERFORMANCE

	switch(device->type)
	{
#ifdef USE_FILES
	case internal_device_type_file:
		if(unlikely(!device->file))return -ENODEV;
		ret = file_write_bio_vec(device->file, bvec, position);
		break;
#else
#pragma message "Files are disabled"
#endif //USE_FILES

#ifdef USE_PLUGINS
	case internal_device_type_plugin:
		ret = plugin_write_bio_vec(device->name, bvec, position);
		break;
#else
#pragma message "Plugins are disabled"
#endif //USE_PLUGINS

	default:
		printk(KERN_ERR "tDisk: Invalid internal device type: %d\n", device->type);
		MY_BUG_ON(true, PRINT_INT(device->type));
		ret = -EINVAL;
		break;
	}

#ifdef MEASURE_PERFORMANCE
	getnstimeofday(&endTime);
	update_performance(WRITE, &startTime, &endTime, bvec->bv_len, &device->performance);
#else
//#pragma message "Performance measurement is disabled"
#endif //MEASURE_PERFORMANCE

	return ret;
}

/**
  * Generic function that reads a bio_vec from a device.
  * The device can be a file or a plugin.
 **/
inline static int read_bio_vec(struct td_internal_device *device, struct bio_vec *bvec, loff_t *position)
{
	int ret;

#ifdef MEASURE_PERFORMANCE
	struct timespec startTime;
	struct timespec endTime;

	getnstimeofday(&startTime);
#else
//#pragma message "Performance measurement is disabled"
#endif //MEASURE_PERFORMANCE

	switch(device->type)
	{
#ifdef USE_FILES
	case internal_device_type_file:
		if(unlikely(!device->file))return -ENODEV;
		ret = file_read_bio_vec(device->file, bvec, position);
		break;
#else
#pragma message "Files are disabled"
#endif //USE_FILES

#ifdef USE_PLUGINS
	case internal_device_type_plugin:
		ret = plugin_read_bio_vec(device->name, bvec, position);
		break;
#else
#pragma message "Plugins are disabled"
#endif //USE_PLUGINS

	default:
		printk(KERN_ERR "tDisk: Invalid internal device type: %d\n", device->type);
		MY_BUG_ON(true, PRINT_INT(device->type));
		ret = -EINVAL;
		break;
	}

#ifdef MEASURE_PERFORMANCE
	getnstimeofday(&endTime);
	update_performance(READ, &startTime, &endTime, bvec->bv_len, &device->performance);
#else
//#pragma message "Performance measurement is disabled"
#endif //MEASURE_PERFORMANCE

	return ret;
}

#ifdef ASYNC_OPERATIONS

/**
  * Generic function that writes data to a device.
  * The device can be a file or a plugin.
 **/
inline static void write_data_async(struct td_internal_device *device, void *data, loff_t position, unsigned int length, void *private_data, void (*callback)(void*,long))
{
	switch(device->type)
	{
#ifdef USE_FILES
	case internal_device_type_file:
		if(unlikely(!device->file))
		{
			if(callback)callback(private_data, -ENODEV);
			break;
		}
		file_write_data_async(device->file, data, position, length, private_data, callback);
		break;
#else
#pragma message "Files are disabled"
#endif //USE_FILES

#ifdef USE_PLUGINS
	case internal_device_type_plugin:
		plugin_write_data_async(device->name, data, position, length, private_data, callback);
		break;
#else
#pragma message "Plugins are disabled"
#endif //USE_PLUGINS

	default:
		printk(KERN_ERR "tDisk: Invalid internal device type: %d\n", device->type);
		MY_BUG_ON(true, PRINT_INT(device->type));
		if(callback)callback(private_data, -EINVAL);
		break;
	}
}

/**
  * Generic function that reads data from a device.
  * The device can be a file or a plugin.
 **/
inline static void read_data_async(struct td_internal_device *device, void *data, loff_t position, unsigned int length, void *private_data, void (*callback)(void*,long))
{
	switch(device->type)
	{
#ifdef USE_FILES
	case internal_device_type_file:
		if(unlikely(!device->file))
		{
			if(callback)callback(private_data, -ENODEV);
			break;
		}
		file_read_data_async(device->file, data, position, length, private_data, callback);
		break;
#else
#pragma message "Files are disabled"
#endif //USE_FILES

#ifdef USE_PLUGINS
	case internal_device_type_plugin:
		plugin_read_data_async(device->name, data, position, length, private_data, callback);
		break;
#else
#pragma message "Plugins are disabled"
#endif //USE_PLUGINS

	default:
		printk(KERN_ERR "tDisk: Invalid internal device type: %d\n", device->type);
		MY_BUG_ON(true, PRINT_INT(device->type));
		if(callback)callback(private_data, -EINVAL);
		break;
	}
}

/**
  * Generic function that writes a bio_vec to a device.
  * The device can be a file or a plugin.
 **/
inline static void write_bio_vec_async(struct td_internal_device *device, struct bio_vec *bvec, loff_t position, void *private_data, void (*callback)(void*,long))
{
	switch(device->type)
	{
#ifdef USE_FILES
	case internal_device_type_file:
		if(unlikely(!device->file))
		{
			if(callback)callback(private_data, -ENODEV);
			break;
		}
		file_write_bio_vec_async(device->file, bvec, position, private_data, callback);
		break;
#else
#pragma message "Files are disabled"
#endif //USE_FILES

#ifdef USE_PLUGINS
	case internal_device_type_plugin:
		plugin_write_bio_vec_async(device->name, bvec, position, private_data, callback);
		break;
#else
#pragma message "Plugins are disabled"
#endif //USE_PLUGINS

	default:
		printk(KERN_ERR "tDisk: Invalid internal device type: %d\n", device->type);
		MY_BUG_ON(true, PRINT_INT(device->type));
		if(callback)callback(private_data, -EINVAL);
		break;
	}
}

/**
  * Generic function that reads a bio_vec from a device.
  * The device can be a file or a plugin.
 **/
inline static void read_bio_vec_async(struct td_internal_device *device, struct bio_vec *bvec, loff_t position, void *private_data, void (*callback)(void*,long))
{
	switch(device->type)
	{
#ifdef USE_FILES
	case internal_device_type_file:
		if(unlikely(!device->file))
		{
			if(callback)callback(private_data, -ENODEV);
			break;
		}		
		file_read_bio_vec_async(device->file, bvec, position, private_data, callback);
		break;
#else
#pragma message "Files are disabled"
#endif //USE_FILES

#ifdef USE_PLUGINS
	case internal_device_type_plugin:
		plugin_read_bio_vec_async(device->name, bvec, position, private_data, callback);
		break;
#else
#pragma message "Plugins are disabled"
#endif //USE_PLUGINS

	default:
		printk(KERN_ERR "tDisk: Invalid internal device type: %d\n", device->type);
		MY_BUG_ON(true, PRINT_INT(device->type));
		if(callback)callback(private_data, -EINVAL);
		break;
	}
}

#endif //ASYNC_OPERATIONS

/**
  * Generic function that flushes a device.
  * The device can be a file or a plugin.
 **/
inline static int flush_device(struct td_internal_device *device)
{
	switch(device->type)
	{
#ifdef USE_FILES
	case internal_device_type_file:
		if(unlikely(!device->file))return -ENODEV;
		return file_flush(device->file);
#else
#pragma message "Files are disabled"
#endif //USE_FILES

#ifdef USE_PLUGINS
	case internal_device_type_plugin:
		return plugin_flush(device->name);
#else
#pragma message "Plugins are disabled"
#endif //USE_PLUGINS

	default:
		printk(KERN_ERR "tDisk: Invalid internal device type: %d\n", device->type);
		MY_BUG_ON(true, PRINT_INT(device->type));
		return -EINVAL;
	}
}

/**
  * Generic function that allocs space on a device.
  * The device can be a file or a plugin.
 **/
inline static int device_alloc(struct td_internal_device *device, loff_t position, unsigned int length)
{
	switch(device->type)
	{
#ifdef USE_FILES
	case internal_device_type_file:
		if(unlikely(!device->file))return -ENODEV;
		return file_alloc(device->file, position, length);
#else
#pragma message "Files are disabled"
#endif //USE_FILES

#ifdef USE_PLUGINS
	case internal_device_type_plugin:
		return 0; //TODO
#else
#pragma message "Plugins are disabled"
#endif //USE_PLUGINS

	default:
		printk(KERN_ERR "tDisk: Invalid internal device type: %d\n", device->type);
		MY_BUG_ON(true, PRINT_INT(device->type));
		return -EINVAL;
	}
}

/**
  * Generic function that checks if a device is ready or not.
  * The device can be a file or a plugin.
 **/
inline static bool device_is_ready(struct td_internal_device *device)
{
	switch(device->type)
	{
#ifdef USE_FILES
	case internal_device_type_file:
		return (device->file != NULL);
#else
#pragma message "Files are disabled"
#endif //USE_FILES

#ifdef USE_PLUGINS
	case internal_device_type_plugin:
		return plugin_is_loaded(device->name);
#else
#pragma message "Plugins are disabled"
#endif //USE_PLUGINS

	default:
		printk(KERN_ERR "tDisk: Invalid internal device type: %d\n", device->type);
		MY_BUG_ON(true, PRINT_INT(device->type));
		return -EINVAL;
	}
}

#endif //TDSIK_DEVICE_OPERATIONS_H

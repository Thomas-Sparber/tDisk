/**
  *
  * tDisk Driver
  * @author Thomas Sparber (2015-2016)
  *
 **/

#ifndef TDISK_PERFORMANCE_H
#define TDISK_PERFORMANCE_H

#include "helpers.h"
#include <tdisk/config.h>

#pragma GCC system_header
#include <linux/timex.h>

/**
  * Calculates the amount of measured records-1
  * @see file_update_performance
 **/
#define PREVIOUS_RECORDS ((1 << MEASURE_RECORDS_SHIFT) - 1)

/**
  * This function updates the performance data
  * of the given performacen data. Actually it
  * calculates the average and standard deviation
  * over the last (1 << MEASURE_RECORDS_SHIFT)
  * requests
 **/
inline static void update_performance(int direction, struct timespec *startTime, struct timespec *endTime, unsigned int length, struct device_performance *perf)
{
	unsigned long diff;
	unsigned long long time = (endTime->tv_sec-startTime->tv_sec) * 1000000000 + (endTime->tv_nsec-startTime->tv_nsec);
	//time *= 1024;
	__div64_32(&time, length);

	if(perf == NULL)return;

	switch(direction)
	{
	case READ:
		if(unlikely(perf->avg_read_time_cycles == 0))
		{
			//Means it was the first read operation of this file.
			//Assuming that this is the average read performance
			printk(KERN_DEBUG "tDisk: measuring ping read-performance for device: %llu\n", time);
			perf->avg_read_time_cycles = time;
			break;
		}

		//Avg difference
		if(perf->avg_read_time_cycles > time)
			diff = perf->avg_read_time_cycles-time;
		else diff = time-perf->avg_read_time_cycles;

		if(unlikely(diff > perf->avg_read_time_cycles))
			printk_ratelimited(KERN_WARNING "tDisk: High read difference: avg=%llu current=%llu\n", perf->avg_read_time_cycles, time);

		//The following lines calculate the following
		//equation using bitshift for better performance
		//avg = (avg*(records_count-1) + current_time) / (records_count)
		perf->avg_read_time_cycles *= PREVIOUS_RECORDS;
		perf->avg_read_time_cycles += time + perf->mod_avg_read;
		perf->stdev_read_time_cycles *= PREVIOUS_RECORDS;
		perf->stdev_read_time_cycles += diff + perf->mod_stdev_read;

		perf->mod_avg_read = perf->avg_read_time_cycles & PREVIOUS_RECORDS;
		perf->avg_read_time_cycles = perf->avg_read_time_cycles >> MEASURE_RECORDS_SHIFT;
		perf->mod_stdev_read = perf->stdev_read_time_cycles & PREVIOUS_RECORDS;
		perf->stdev_read_time_cycles = perf->stdev_read_time_cycles >> MEASURE_RECORDS_SHIFT;
		break;
	case WRITE:
		if(unlikely(perf->avg_write_time_cycles == 0))
		{
			//Means it was the first write operation of this file.
			//Assuming that this is the average write performance
			printk(KERN_DEBUG "tDisk: measuring ping write-performance for device: %llu\n", time);
			perf->avg_write_time_cycles = time;
			break;
		}

		//Avg difference
		if(perf->avg_write_time_cycles > time)
			diff = perf->avg_write_time_cycles-time;
		else diff = time-perf->avg_write_time_cycles;

		if(unlikely(diff > perf->avg_write_time_cycles))
			printk_ratelimited(KERN_WARNING "tDisk: High write difference: avg=%llu current=%llu\n", perf->avg_write_time_cycles, time);

		//The following lines calculate the following
		//equation using bitshift for better performance
		//avg = (avg*(records_count-1) + current_time) / (records_count)
		perf->avg_write_time_cycles *= PREVIOUS_RECORDS;
		perf->avg_write_time_cycles += time + perf->mod_avg_write;
		perf->stdev_write_time_cycles *= PREVIOUS_RECORDS;
		perf->stdev_write_time_cycles += diff + perf->mod_stdev_write;

		perf->mod_avg_write = perf->avg_write_time_cycles & PREVIOUS_RECORDS;
		perf->avg_write_time_cycles = perf->avg_write_time_cycles >> MEASURE_RECORDS_SHIFT;
		perf->mod_stdev_write = perf->stdev_write_time_cycles & PREVIOUS_RECORDS;
		perf->stdev_write_time_cycles = perf->stdev_write_time_cycles >> MEASURE_RECORDS_SHIFT;
		break;
	}
}

#endif //TDISK_PERFORMANCE_H

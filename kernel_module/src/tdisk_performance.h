/**
  *
  * tDisk Driver
  * @author Thomas Sparber (2015-2016)
  *
 **/

#ifndef TDISK_PERFORMANCE_H
#define TDISK_PERFORMANCE_H

#include <tdisk/config.h>

#pragma GCC system_header
#include <linux/timex.h>

#ifdef MEASURE_PERFORMANCE

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
inline static void update_performance(int direction, cycles_t time, struct device_performance *perf)
{
	unsigned long diff;

	//time is 0 on platforms which have no cycles measure
	WARN_ONCE(time == 0, "Your processor doesn't count cycles. Performances values may be wrong!");
	if(perf == NULL || time == 0)return;

	switch(direction)
	{
	case READ:
		if(unlikely(perf->avg_read_time_cycles == 0))
		{
			//Means it was the first read operation of this file.
			//Assuming that this is the averyge read performance
			printk(KERN_DEBUG "tDisk: measuring ping read-performance for device: %llu\n", time);
			perf->avg_read_time_cycles = time;
			break;
		}

		//Avg difference
		if(perf->avg_read_time_cycles > time)
			diff = perf->avg_read_time_cycles-time;
		else diff = time-perf->avg_read_time_cycles;

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
			//Means it was the first read operation of this file.
			//Assuming that this is the averyge read performance
			printk(KERN_DEBUG "tDisk: measuring ping write-performance for device: %llu\n", time);
			perf->avg_write_time_cycles = time;
			break;
		}

		//Avg difference
		if(perf->avg_write_time_cycles > time)
			diff = perf->avg_write_time_cycles-time;
		else diff = time-perf->avg_write_time_cycles;

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

#else
#pragma message "Performance measurement is disabled"
#endif //MEASURE_PERFORMANCE

#endif //TDISK_PERFORMANCE_H

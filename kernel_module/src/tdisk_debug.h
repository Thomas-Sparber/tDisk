/**
  *
  * tDisk Driver
  * @author Thomas Sparber (2015-2016)
  *
 **/

#ifndef TDISK_DEBUG_H
#define TDISK_DEBUG_H

#include <tdisk/config.h>
#include <tdisk/interface.h>

#pragma GCC system_header
#include <linux/jiffies.h>
#include <linux/spinlock.h>
#include <linux/types.h>

#define AMOUNT_MEASURE_POINTS 10

#define DEBUG_POINTS_ENABLED

/**
  * This struct is used to store information about the current
  * operations of a program
 **/
struct debug_info
{
	/** The id of the debug info **/
	u64 id;

	/** The current source file **/
	const char *file;

	/** The current line in the file **/
	int line;

	/** The current function **/
	const char *function;

	/** A custom message **/
	char message[256];

	/** The time of the debugging info **/
	u64 time_jiffies;

}; //end struct debug_info

/**
  * This struct holds the data for saving debug info
 **/
struct debug_struct
{
	/** The atomic id counter **/
	u64 id_counter;

	/** This spinlock is used to synchronize access to the counter and info **/
	spinlock_t counter_lock;

	/** The stored debug info **/
	struct debug_info info[AMOUNT_MEASURE_POINTS];

	/** The current index of the debug info **/
	int current_debug_info;

}; //end struct debug_struct

#ifdef DEBUG_POINTS_ENABLED
/**
  * This macro can be used to set debug points.
  * These debug points are stored in the debug info.
  * The file, line and function are stored and an
  * additional custom text.
 **/
#define DEBUG_POINT(ds, msg, ...) debug_point(ds, __FILE__, __LINE__, __FUNCTION__, msg, __VA_ARGS__)
#else
/** Disables debug points **/
#define DEBUG_POINT(...)
#endif //DEBUG_POINTS_ENABLED

/**
  * Initializes a debug struct
 **/
inline static void init_debug_struct(struct debug_struct *ds)
{
	ds->id_counter = 0;
	spin_lock_init(&ds->counter_lock);
	memset(ds->info, 0, sizeof(ds->info));
	ds->current_debug_info = 0;
}

/**
  * This function is called by the DEBUG_POINT macro.
 **/
void debug_point(struct debug_struct *ds, const char *file, int line, const char *function, const char *message, ...);

/**
  * Retrieves the next debug info
 **/
void get_next_debug_point(struct debug_struct *ds, u64 current_id, struct tdisk_debug_info *out);

#endif //TDISK_DEBUG_H

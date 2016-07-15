/**
  *
  * tDisk Driver
  * @author Thomas Sparber (2015-2016)
  *
 **/

#include <tdisk/config.h>
#include "tdisk_debug.h"

void debug_point(struct debug_struct *ds, const char *file, int line, const char *function, const char *message, ...)
{
	va_list args;
	struct debug_info *info;

	//Find new debug_info
	spin_lock(&ds->counter_lock);
	if(ds->current_debug_info == AMOUNT_MEASURE_POINTS)
		ds->current_debug_info = 0;
	info = &ds->info[ds->current_debug_info];
	info->id = ds->id_counter++;
	ds->current_debug_info++;
	spin_unlock(&ds->counter_lock);

	//Save debug info
	info->file = file;
	info->line = line;
	info->function = function;
	info->time_jiffies = get_jiffies_64();

	//Save custom message
	va_start(args, message);
	snprintf(info->message, sizeof(info->message), message, args);
	va_end(args);
}

void get_next_debug_point(struct debug_struct *ds, u64 current_id, struct tdisk_debug_info *out)
{
	int i;
	int converted_i;
	struct debug_info info;

	current_id++;

	//Find new debug_info
	spin_lock(&ds->counter_lock);
	for(i = 0; i < AMOUNT_MEASURE_POINTS; ++i)
	{
		converted_i = i + ds->current_debug_info;
		if(converted_i >= AMOUNT_MEASURE_POINTS)converted_i -= AMOUNT_MEASURE_POINTS;

		if(ds->info[converted_i].id >= current_id)break;
	}
	info = ds->info[converted_i];
	spin_unlock(&ds->counter_lock);

	out->id = info.id;
	strncpy(out->file, info.file, sizeof(out->file));
	out->line = info.line;
	strncpy(out->function, info.file, sizeof(out->function));
	strncpy(out->message, info.file, sizeof(out->message));
	out->time = info.time_jiffies;
}

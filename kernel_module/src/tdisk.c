/**
  *
  * tDisk Driver
  * @author Thomas Sparber (2015-2016)
  *
 **/

#include <tdisk/config.h>
#include "helpers.h"
#include "tdisk.h"
#include "tdisk_control.h"
#include "tdisk_device_operations.h"
#include "tdisk_performance.h"

#define COMPARE 1410

#define DEFAULT_WORKER_TIMEOUT (20*HZ)
#define DEFAULT_SECONDARY_WORK_DELAY 2

/**
  * Actual internal device calculated using memory offset
 **/
#define DEVICE_INDEX(desired, all_devices) ((tdisk_index)((desired)-(all_devices)+1))

/**
  * Checks whether the sector is used given the access count
 **/
#define SECTOR_USED(sector) ((sector) & 1)

/**
  * Increments the access count
 **/
#define INC_ACCESS_COUNT(sector) (sector) = (typeof(sector))(((((sector)>>1)+1)<<1) | 1)

/**
  * Gets the access count of the sector
 **/
#define ACCESS_COUNT(sector) ((sector)>>1)

/**
  * Resets the access count of the sector
 **/
#define RESET_ACCESS_COUNT(sector) sector = (sector) & 1

/**
  * Sets the access count of the given sector
 **/
#define SET_ACCESS_COUNT(sector, count) sector = (typeof(sector))(((sector) & 1) | ((count)<<1))

static int td_start_worker_thread(struct tdisk *td);
static void td_stop_worker_thread(struct tdisk *td);
static enum worker_status td_queue_work(void *private_data, struct kthread_work *work);

static int TD_MAJOR = 0;
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Thomas Sparber <thomas@sparber.eu>");
MODULE_DESCRIPTION(DRIVER_NAME " Driver");
MODULE_ALIAS_BLOCKDEV_MAJOR(TD_MAJOR);

DEFINE_IDR(td_index_idr);

/*
 * Flushes the td device (not the underlying devices)
 */
static int td_flush(struct tdisk *td)
{
	//Freeze and unfreeze queue
	blk_mq_freeze_queue(td->queue);
	blk_mq_unfreeze_queue(td->queue);

	return 0;
}

/**
  * Flushes the underlying devices of the tDisk
 **/
static int td_flush_devices(struct tdisk *td)
{
	unsigned int i;
	int ret = 0;
	for(i = 0; i < td->internal_devices_count; ++i)
	{
		int internal_ret = flush_device(&td->internal_devices[i]);

		if(unlikely(internal_ret && internal_ret != -EINVAL))
			ret = -EIO;
	}

	return ret;
}

/**
  * Writes all the sector indices to the device.
 **/
static int td_write_all_indices(struct tdisk *td, struct td_internal_device *device)
{
	int ret = 0;

	loff_t skip = sizeof(struct tdisk_header);
	void *data = td->indices;
	loff_t length = td->header_size*td->blocksize - skip;
	unsigned int u_length = (unsigned int)length;

	//Header too big
	//BUG_ON(length != u_length);

	ret = write_data(device, data, skip, u_length);

	if(ret)printk(KERN_ERR "tDisk: Error writing all disk indices: %d. Offset: %llu, length: %llu\n", ret, skip, length);
	else printk(KERN_DEBUG "tDisk: Success writing all disk indices\n");

	return ret;
}

#ifdef AUTO_RESET_ACCESS_COUNT

/**
  * Divides the access count of all sectors by the lowest
  * access count. Optionally, it's also written to disks
 **/
static void reset_access_count(struct tdisk *td, bool do_disk_operation)
{
	tdisk_index disk;
	sector_t sector;
	__u16 min_access_count = (__u16)-1;

	//Find lowest access count
	for(sector = 0; sector < td->size_blocks; ++sector)
	{
		if(ACCESS_COUNT(td->indices[sector].access_count) < min_access_count)
			min_access_count = ACCESS_COUNT(td->indices[sector].access_count);
	}

	//Needs to be at least 2
	if(min_access_count < 2)min_access_count = 2;

	//Divide all sector access count by lowest access count
	for(sector = 0; sector < td->size_blocks; ++sector)
	{
		SET_ACCESS_COUNT(td->indices[sector].access_count, ACCESS_COUNT(td->indices[sector].access_count) / min_access_count);
	}

	if(do_disk_operation)
	{
		//Save sector indices
		for(disk = 1; disk < td->internal_devices_count; ++disk)
		{
			td_write_all_indices(td, &td->internal_devices[disk-1]);
		}
	}
}

#else
#pragma message "Reset auto access count is disabled"
#endif //AUTO_RESET_ACCESS_COUNT

/**
  * Writes the given sector index to the given internal device
 **/
int td_write_index_to_disk(struct tdisk *td, sector_t logical_sector, tdisk_index disk)
{
	struct sector_index *actual;
	loff_t position = td->index_offset_byte + (loff_t)logical_sector * (loff_t)sizeof(struct sector_index);
	unsigned int length = sizeof(struct sector_index);

	if(position + length > td->header_size * td->blocksize)return 1;
	actual = &td->indices[logical_sector];

	return write_data(&td->internal_devices[disk-1], actual, position, length);
}

/**
  * Writes the given sector index to the given internal device aysnchronously
 **/
void td_write_index_to_disk_async(struct tdisk *td, sector_t logical_sector, tdisk_index disk, void *private_data, void (*callback)(void*,long))
{
	struct sector_index *actual;
	loff_t position = td->index_offset_byte + (loff_t)logical_sector * (loff_t)sizeof(struct sector_index);
	unsigned int length = sizeof(struct sector_index);

	if(position + length > td->header_size * td->blocksize)
	{
		if(callback)callback(private_data, -ENOMEM);
	}
	actual = &td->indices[logical_sector];

	write_data_async(&td->internal_devices[disk-1], actual, position, length, private_data, callback);
}

/**
  * Performs the given index operation. This can be:
  *  - READ: reads the physical sector index for the given logical sector
  *  - WRITE: stores the given physical sector index for the given logical
  *    sector. The data are also stored to each physical disk
  *  - COMPARE: Compares the given physical sector index with the actual one.
  *    This is useful to compare individual disks for consistency.
  * The flag do_disk_operation can be used to define whether the data
  * should be written or not. Usually it is a good idea to write the data
  * immediately to disk to prevent data loss, but in case there are multiple
  * index operations to write it is better to write them all at once.
  * The flag update_access_count can be used to define whether the access
  * count of the specific sector should be updated. This is useful e.g. when
  * sectors are moved - this shouldn't influence the access count variable.
 **/
int td_perform_index_operation(struct tdisk *td, int direction, sector_t logical_sector, struct sector_index *physical_sector, bool do_disk_operation, bool update_access_count)
{
	struct sector_index *actual;
	loff_t position = td->index_offset_byte + (loff_t)logical_sector * (loff_t)sizeof(struct sector_index);
	unsigned int length = sizeof(struct sector_index);

	if(position + length > td->header_size * td->blocksize)return 1;
	actual = &td->indices[logical_sector];

	//Increment access count
	if(update_access_count)
	{
		INC_ACCESS_COUNT(actual->access_count);

#ifdef AUTO_RESET_ACCESS_COUNT
		//printk_ratelimited(KERN_DEBUG "tDisk: access count: %u max: %u\n", actual->access_count, (typeof(actual->access_count))-1);
		if(ACCESS_COUNT(actual->access_count) == (((typeof(actual->access_count))-1)>>1))
		{
			printk(KERN_DEBUG "tDisk: Resetting tDisk access count\n");
			reset_access_count(td, do_disk_operation);
		}
#else
#pragma message "Reset auto access count is disabled"
#endif //AUTO_RESET_ACCESS_COUNT
	}


	//MY_BUG_ON(direction == WRITE && physical_sector->disk == 0, PRINT_INT(physical_sector->disk), PRINT_ULL(logical_sector));
	MY_BUG_ON(direction == READ && actual->disk == 0, PRINT_INT(actual->disk), PRINT_ULL(logical_sector));

	//index operation
	if(direction == READ)
		(*physical_sector) = (*actual);
	else if(direction == COMPARE)
	{
		if(physical_sector->disk != actual->disk || physical_sector->sector != actual->sector)
			return -1;
	}
	else if(direction == WRITE)
	{
		tdisk_index disk;

		//Memory operation
		actual->disk = physical_sector->disk;
		actual->sector = physical_sector->sector;

		//Disk operations
		if(do_disk_operation)
		{
			for(disk = 1; disk <= td->internal_devices_count; ++disk)
			{
				td_write_index_to_disk(td, logical_sector, disk);
			}
		}
	}

	return 0;
}

/**
  * This is the heuristic function that calculates the
  * speed of a device
 **/
inline static unsigned long long td_get_device_performance(const struct td_internal_device *d)
{
	return (d->performance.avg_read_time_cycles + d->performance.avg_write_time_cycles) >> 1;
}

/**
  * This function physically swaps the two given sectors.
  * This means it reads the data of both sectors, stores
  * sector a in sector b and vice versa and updates the
  * indices
 **/
static bool td_swap_sectors(struct tdisk *td, sector_t logical_a, struct sector_index *a, sector_t logical_b, struct sector_index *b, bool do_disk_operation)
{
	int ret;
	loff_t pos_a;
	loff_t pos_b;
	loff_t pos_help_a;
	loff_t pos_help_b;
	tdisk_index disk_a;
	tdisk_index disk_b;
	sector_t sector_b;
	sector_t sector_a;
	u16 access_count_a;
	u16 access_count_b;
	u8 *buffer_a;
	u8 *buffer_b;

	//Swap sectors in case disk b is better. This speeds up the swapping process
	if(td_get_device_performance(&td->internal_devices[a->disk-1]) > td_get_device_performance(&td->internal_devices[b->disk-1]))
	{
		swap(a, b);
		swap(logical_a, logical_b);
	}

	disk_a = a->disk;
	disk_b = b->disk;
	sector_b = b->sector;
	sector_a = a->sector;
	access_count_a = a->access_count;
	access_count_b = b->access_count;
	pos_a = (loff_t)a->sector * td->blocksize;
	pos_b = (loff_t)b->sector * td->blocksize;
	pos_help_a = (loff_t)td->internal_devices[disk_a-1].move_help_sector * td->blocksize;
	pos_help_b = (loff_t)td->internal_devices[disk_b-1].move_help_sector * td->blocksize;

	buffer_a = vmalloc(td->blocksize);
	if(!buffer_a)return false;

	buffer_b = vmalloc(td->blocksize);
	if(!buffer_b)
	{
		vfree(buffer_a);
		return false;
	}


	//Reading blocks from both disks
	ret = read_data(&td->internal_devices[disk_a-1], buffer_a, pos_a, td->blocksize);		//a read op1
	if(ret != 0)
	{
		printk(KERN_WARNING "tDisk: Swap error: reading %lu, disk: %u, ret: %d\n", logical_a, disk_a, ret);
		goto out;
	}

	ret = read_data(&td->internal_devices[disk_b-1], buffer_b, pos_b, td->blocksize);		//b read op1
	if(ret != 0)
	{
		printk(KERN_WARNING "tDisk: Swap error: reading %lu, disk: %u, ret: %d\n", logical_b, disk_b, ret);
		goto out;
	}



	ret = write_data(&td->internal_devices[disk_a-1], buffer_a, pos_help_a, td->blocksize);
	if(ret != 0)
	{
		printk(KERN_WARNING "tDisk: Swap-help error: writing %lu, disk: %u, ret: %d\n", logical_a, disk_a, ret);
		goto out;
	}

	a->disk = disk_a;
	a->access_count = access_count_a;
	a->sector = td->internal_devices[disk_a-1].move_help_sector;
	td_perform_index_operation(td, WRITE, logical_a, a, do_disk_operation, false);

	ret = write_data(&td->internal_devices[disk_a-1], buffer_b, pos_a, td->blocksize);
	if(ret != 0)
	{
		printk(KERN_WARNING "tDisk: Swap error: writing %lu, disk: %u, ret: %d\n", logical_a, disk_a, ret);
		goto out;
	}

	b->disk = disk_a;
	b->access_count = access_count_b;
	b->sector = sector_a;
	td_perform_index_operation(td, WRITE, logical_b, b, do_disk_operation, false);

	ret = write_data(&td->internal_devices[disk_b-1], buffer_a, pos_b, td->blocksize);
	if(ret != 0)
	{
		printk(KERN_WARNING "tDisk: Swap error: writing %lu, disk: %u, ret: %d\n", logical_b, disk_b, ret);
		goto out;
	}

	a->disk = disk_b;
	a->access_count = access_count_a;
	a->sector = sector_b;
	td_perform_index_operation(td, WRITE, logical_a, a, do_disk_operation, false);

 out:
	vfree(buffer_a);
	vfree(buffer_b);

	return (ret != 0);
}

#ifdef MOVE_SECTORS

/**
  * This is the callback which is used to sort the
  * devices according to its performance
 **/
static int td_sort_devices_callback(const void *a, const void *b)
{
	const struct sorted_internal_device *d_a = a;
	const struct sorted_internal_device *d_b = b;

	unsigned long long performance_a = td_get_device_performance(d_a->dev);
	unsigned long long performance_b = td_get_device_performance(d_b->dev);

	if(performance_a == performance_b)return 0;
	else if(performance_a > performance_b)return 1;
	else return -1;
}

/**
  * Inserts the internal devices in the given array
  * in an ordered manner
 **/
static void td_insert_sorted_internal_devices(struct tdisk *td)
{
	unsigned int i;

	for(i = 0; i < td->internal_devices_count; ++i)
	{
		INIT_LIST_HEAD(&td->sorted_devices[i].preferred_blocks);
		td->sorted_devices[i].dev = &td->internal_devices[i];
		td->sorted_devices[i].available_blocks = td->internal_devices[i].size_blocks;
		td->sorted_devices[i].amount_blocks = 0;
	}

	//Sort array
	sort(td->sorted_devices, td->internal_devices_count, sizeof(struct sorted_internal_device), &td_sort_devices_callback, NULL);
}

/**
  * Finds the corresponding sorted device of the given
  * td_internal_device
 **/
static struct sorted_internal_device* td_find_sorted_device(struct sorted_internal_device *sorted_devices, struct td_internal_device *desired_device, unsigned int devices)
{
	unsigned int i;

	for(i = 0; i < devices; ++i)
	{
		if(sorted_devices[i].dev == desired_device)
		{
			return &sorted_devices[i];
		}
	}
	return NULL;
}

/**
  * This function returns the sector_index in the given
  * disk with the lowest access count.
  * This is used when swapping a (high access) sector from a slower
  * disk with a (lowest possible) sector from a faster disk.
  * A sector with a lower access count has a lower probability
  * of being moved to a faster disk in the near future.
 **/
static struct sorted_sector_index* td_find_sector_index(struct sorted_internal_device *device, tdisk_index disk)
{
	struct sorted_sector_index *item;
	struct sorted_sector_index *lowest = NULL;

	//TODO Actually these sectors should be sorted
	//so no need to access all of them
	list_for_each_entry(item, &device->preferred_blocks, device_assigned)
	{
		if(item->physical_sector->disk == disk)
		{
			if(lowest == NULL || ACCESS_COUNT(item->physical_sector->access_count) < ACCESS_COUNT(lowest->physical_sector->access_count))
				lowest = item;
		}
	}

	return lowest;
}

/**
  * This is an optimizing function for assigning sectors
  * properly to their devices. Obviously it is possible that
  * two or more sectors have the same access count (or
  * acceptable variation). So it is also possible that sector
  * a and sector b have the same access count but are stored on
  * the wrong disks (according to the sorting algorithm). So if
  * they have the same access count they can simply be ignored.
 **/
static struct sorted_sector_index* td_find_sector_index_acc(struct sorted_internal_device *device, tdisk_index disk, __u16 access_count, bool is_faster)
{
	struct sorted_sector_index *item;
	struct sorted_sector_index *ret = NULL;

	//TODO Actually these sectors should be sorted
	//so no need to access all of them
	list_for_each_entry(item, &device->preferred_blocks, device_assigned)
	{
		if(is_faster)
		{
			if(item->physical_sector->disk == disk && ACCESS_COUNT(item->physical_sector->access_count) <= access_count)
			{
				if(ret == NULL || ACCESS_COUNT(item->physical_sector->access_count) > ACCESS_COUNT(ret->physical_sector->access_count))
					ret = item;
			}
		}
		else
		{
			if(item->physical_sector->disk == disk && ACCESS_COUNT(item->physical_sector->access_count) >= access_count)
			{
				if(ret == NULL || ACCESS_COUNT(item->physical_sector->access_count) < ACCESS_COUNT(ret->physical_sector->access_count))
					ret = item;
			}
		}
	}

	return ret;
}

/**
  * This function assigns the sorted sectors to the sorted devices
  * and tries to optimize it using the function td_find_sector_index_acc
  * The result of this function is the base of the sectors to be swapped
 **/
static void td_assign_sectors(struct tdisk *td)
{
	sector_t missing = td->size_blocks;
	unsigned int sorted_disk;
	struct sorted_sector_index *sector;
	struct sorted_sector_index *item_safe;

	for(sorted_disk = 1; sorted_disk <= td->internal_devices_count; ++sorted_disk)
		INIT_LIST_HEAD(&td->sorted_devices[sorted_disk-1].preferred_blocks);

	sorted_disk = 1;

	//Iterating over all sorted sectors in the tDisk (!!!)
	//and assigning them to the corresponding internal device
	//The sectors are processed according to access count
	//and assigned to devices according to performance
	list_for_each_entry(sector, &td->sorted_sectors_head, total_sorted)
	{
		//Not processing unused sectors
		if(sector->physical_sector->disk == 0)continue;

		//Count missing sectors
		missing--;

		//Here the counter available_blocks of the sorted_internal_device
		//is used to assign the sectors. If there are no more free sectors
		//the next sorted device is used
		while(td->sorted_devices[sorted_disk-1].available_blocks == 0)
		{
			sorted_disk++;
			MY_BUG_ON(sorted_disk > td->internal_devices_count, PRINT_UINT(sorted_disk), PRINT_ULL(missing));
		}

		td->sorted_devices[sorted_disk-1].available_blocks--;

		//Adding sector to device's preferred blocks
		INIT_LIST_HEAD(&sector->device_assigned);
		list_add(&sector->device_assigned, &td->sorted_devices[sorted_disk-1].preferred_blocks);

		//Here, the amount of correctly assigned blocks is counted.
		//The memory offset is used to convert from sorted device
		//to actual device
		if(sector->physical_sector->disk == td->sorted_devices[sorted_disk-1].dev-td->internal_devices+1)
			td->sorted_devices[sorted_disk-1].amount_blocks++;
	}

	//All blocks must be used
	for(sorted_disk = 1; sorted_disk <= td->internal_devices_count; ++sorted_disk)
	{
		//MY_BUG_ON(td->sorted_devices[sorted_disk-1].available_blocks != 0,
		//	PRINT_ULL(td->sorted_devices[sorted_disk-1].available_blocks),
		//	PRINT_UINT(DEVICE_INDEX(td->sorted_devices[sorted_disk-1].dev, td->internal_devices)));

		if(td->sorted_devices[sorted_disk-1].available_blocks != 0)
		{
			printk(KERN_ERR "tDisk: While assigning sectors: available_blocks (%lu) of disk %u is not 0 as it should be\n", td->sorted_devices[sorted_disk-1].available_blocks, DEVICE_INDEX(td->sorted_devices[sorted_disk-1].dev, td->internal_devices)+1);
		}
	}

	//Trying to optimize a bit...
	for(sorted_disk = 1; sorted_disk <= td->internal_devices_count; ++sorted_disk)
	{
		//Iterating over each element and looking
		//for wrong elements. If there is another
		//wrong element in the corresponding disk
		//with the same access count it is just a
		//matter of the sorting algorithm and can
		//simply be swapped.
		list_for_each_entry_safe(sector, item_safe, &td->sorted_devices[sorted_disk-1].preferred_blocks, device_assigned)
		{
			//Actual internal device calculated using memory offset
			tdisk_index current_disk = DEVICE_INDEX(td->sorted_devices[sorted_disk-1].dev, td->internal_devices);

			MY_BUG_ON(sector->physical_sector->disk == 0 || sector->physical_sector->disk > td->internal_devices_count, PRINT_INT(sector->physical_sector->disk));

			if(sector->physical_sector->disk != current_disk)
			{
				//The sector to swap is only searched in the SLOWER
				//devices BUT with an equal or HIGHER access count.

				//The device offset which is used to only search in slower
				//devices than the current
				//unsigned int dev_off = sorted_disk-1+1;

				//unsigned int dev_length = td->internal_devices_count-dev_off;

				//This is the internal device where the current sector
				//Should be stored according to its access count.
				//Since we are just looking just for devices which
				//Are slower than the current it can also be null
				struct sorted_internal_device *corresponding = td_find_sorted_device(td->sorted_devices, &td->internal_devices[sector->physical_sector->disk-1], td->internal_devices_count);
				struct sorted_sector_index *to_swap;

				bool isFaster = corresponding < &td->sorted_devices[sorted_disk-1];

				if(!corresponding)continue;

				//Finds a sector with an equal or higher access count
				//for the current disk inside the "corresponding"
				to_swap = td_find_sector_index_acc(corresponding, current_disk, ACCESS_COUNT(sector->physical_sector->access_count), isFaster);

				if(to_swap != NULL)
				{
					//printk(KERN_DEBUG "tDisk: %u: pre-swapping %u (%u - %u) with %u (%u - %u)\n", current_disk, sector-td->sorted_sectors, sector->physical_sector->disk, sector->physical_sector->access_count, to_swap-td->sorted_sectors, to_swap->physical_sector->disk, to_swap->physical_sector->access_count);

					//Simply swap those sectors
					BUG_ON(to_swap->physical_sector->disk != current_disk);

					list_del_init(&sector->device_assigned);
					list_del_init(&to_swap->device_assigned);

					//TODO insert sorted
					list_add(&sector->device_assigned, &corresponding->preferred_blocks);
					list_add(&to_swap->device_assigned, &td->sorted_devices[sorted_disk-1].preferred_blocks);

					corresponding->amount_blocks++;
					td->sorted_devices[sorted_disk-1].amount_blocks++;
				}
				//else printk(KERN_DEBUG "tDisk: %u: Didn't find a sector to swap %u (%u - %u) \n", current_disk, sector-td->sorted_sectors, sector->physical_sector->disk, sector->physical_sector->access_count);
			}
		}
	}
}

/**
  * This function moves the sector with the
  * highest access count to the disk with the
  * best performance.
  * The function returns true and sets td->access_count_resort
  * when a sector could be moved.
 **/
static bool td_move_one_sector(struct tdisk *td)
{
	bool swapped = false;
	unsigned int sorted_disk;
	struct sorted_sector_index *item;
	struct sorted_sector_index *to_swap;
	sector_t correctly_stored;

	//Check if all devices are loaded
	for(sorted_disk = 1; sorted_disk <= td->internal_devices_count; ++sorted_disk)
	{
		if(!device_is_ready(&td->internal_devices[sorted_disk-1]))
		{
			//Not all disks are loaded yet
			printk(KERN_DEBUG "tDisk: Not all disks are ready. Not moving sectors\n");
			return false;
		}
	}

	if(td->sorted_devices == NULL)
	{
		//Create sorted devices
		td->sorted_devices = vmalloc(sizeof(struct sorted_internal_device) * td->internal_devices_count);
		if(!td->sorted_devices)
		{
			printk(KERN_WARNING "tDisk: Error allocating sorted_devices memory\n");
			return false;
		}

		td_insert_sorted_internal_devices(td);

		//Assigning the sorted sectors to the sorted devices...
		td_assign_sectors(td);
	}

	correctly_stored = 0;
	for(sorted_disk = 1; sorted_disk <= td->internal_devices_count; ++sorted_disk)
	{
		correctly_stored += td->sorted_devices[sorted_disk-1].amount_blocks;
		/*printk(KERN_DEBUG "tDisk: Internal disk %u (speed: %u rank --> %llu): Capacity: %llu, Correctly stored: %llu, %u percent\n",
						td->sorted_devices[sorted_disk-1].dev-td->internal_devices+1,
						sorted_disk,
						td_get_device_performance(td->sorted_devices[sorted_disk-1].dev),
						td->sorted_devices[sorted_disk-1].dev->size_blocks,
						td->sorted_devices[sorted_disk-1].amount_blocks,
						(size_t)td->sorted_devices[sorted_disk-1].amount_blocks*100 / (size_t)td->sorted_devices[sorted_disk-1].dev->size_blocks);*/
	}

	//Moving the sector with highest access count to
	//the disk with the best performance
	for(sorted_disk = 1; sorted_disk <= td->internal_devices_count; ++sorted_disk)
	{
		struct sorted_sector_index *highest = NULL;
		tdisk_index current_disk_index = DEVICE_INDEX(td->sorted_devices[sorted_disk-1].dev, td->internal_devices);
		struct sorted_internal_device *other_disk;
		tdisk_index other_disk_sorted_index;

		if(td->sorted_devices[sorted_disk-1].amount_blocks == td->sorted_devices[sorted_disk-1].dev->size_blocks)
		{
			//All blocks are correctly stored. Moving on to slower disk...
			continue;
		}

		//If we reach this point there is at least one
		//sector stored on a wrong disk which should be
		//stored on this disk.
		//So searching the sector with the highest access
		//count which gains the best performance.
		list_for_each_entry(item, &td->sorted_devices[sorted_disk-1].preferred_blocks, device_assigned)
		{
			if(item->physical_sector->disk != current_disk_index)
			{
				if(highest == NULL || ACCESS_COUNT(item->physical_sector->access_count) > ACCESS_COUNT(highest->physical_sector->access_count))
					highest = item;
			}
		}

		BUG_ON(!highest);
		other_disk = td_find_sorted_device(td->sorted_devices, &td->internal_devices[highest->physical_sector->disk-1], td->internal_devices_count);

		BUG_ON(!other_disk);
		other_disk_sorted_index = DEVICE_INDEX(other_disk, td->sorted_devices);

		//Above there is a continue flag so there must
		//be a wrong sector. Otherwise it is a bug
		BUG_ON(!highest);

		//Now looking at the disk where the current highest
		//sector is stored for a block that belongs to
		//the current disk
		to_swap = td_find_sector_index(&td->sorted_devices[other_disk_sorted_index-1], current_disk_index);

		//If no sector was found it means we have a circual dependency
		//So we just skip to the next disk and proceed
		while(to_swap == NULL && other_disk_sorted_index < td->internal_devices_count)
		{
			if(++other_disk_sorted_index != sorted_disk)
				to_swap = td_find_sector_index(&td->sorted_devices[other_disk_sorted_index-1], current_disk_index);
		}

		if(to_swap != NULL)
		{
			//OK, now we found a sector of the possibly fastest disk
			//which is stored on the possibly slowest disk. When we
			//swap those sectors we gain the highest possible performance.

			sector_t logical_a = (sector_t)(highest-td->sorted_sectors);
			sector_t logical_b = (sector_t)(to_swap-td->sorted_sectors);
			struct sector_index *a = highest->physical_sector;
			struct sector_index *b = to_swap->physical_sector;

			//printk(KERN_DEBUG "tDisk: swapping logical sectors %llu (disk: %u, access: %u) and %llu (disk: %u, access: %u): %llu/%llu\n",
			//		logical_a, a->disk, a->access_count, logical_b, b->disk, b->access_count, correctly_stored, td->size_blocks);

			td_swap_sectors(td, logical_a, a, logical_b, b, true);

			if(td->sorted_devices[sorted_disk-1].dev == &td->internal_devices[a->disk-1])
				td->sorted_devices[sorted_disk-1].amount_blocks++;
			if(td->sorted_devices[other_disk_sorted_index-1].dev == &td->internal_devices[b->disk-1])
				td->sorted_devices[other_disk_sorted_index-1].amount_blocks++;

			td->access_count_resort = 1;
			swapped = true;
			break;
		}

		if(swapped)break;
	}

	return swapped;
}

#else
#pragma message "Moving sectors is disabled"
#endif //MOVE_SECTORS

#ifdef MEASURE_PING_PERFORMANCE

void td_measure_device_performance(struct td_internal_device *device, loff_t size)
{
	struct timespec startTime;
	struct timespec endTime;
	unsigned long time = jiffies;
	char *buffer = vmalloc(1048576);
	unsigned int counter = 0;
	unsigned int elapsed;

	getnstimeofday(&startTime);

	while(jiffies-time < HZ*5)
	{
		read_data(device, buffer, counter*1048576, 1048576);
		counter++;
		if(counter*1048576 > size)break;
	}

	getnstimeofday(&endTime);
	update_performance(READ, &startTime, &endTime, counter, &device->performance);

	elapsed = (unsigned)(endTime.tv_sec-startTime.tv_sec);
	if(elapsed == 0)elapsed = 1;
	printk(KERN_DEBUG "tDisk: read %u MiB --> %u MiB/s\n", counter, counter/elapsed);
}

#else
#pragma message "Ping performance measurement is disabled"
#endif //MEASURE_PING_PERFORMANCE

/**
  * This callback is used by the list_sort to determine the
  * order of a sector index
 **/
int td_sector_index_callback_total(void *priv, struct list_head *a, struct list_head *b)
{
	struct sorted_sector_index *i_a = list_entry(a, struct sorted_sector_index, total_sorted);
	struct sorted_sector_index *i_b = list_entry(b, struct sorted_sector_index, total_sorted);

	if(i_a->physical_sector->disk == 0 && i_b->physical_sector->disk == 0)return 0;
	if(i_a->physical_sector->disk == 0)return 1;
	if(i_b->physical_sector->disk == 0)return -1;

	return ACCESS_COUNT(i_b->physical_sector->access_count) - ACCESS_COUNT(i_a->physical_sector->access_count);
}

/**
  * This function sorts all the sector indices
  * This is useful at the loading time
 **/
bool td_reorganize_all_indices(struct tdisk *td)
{
	//Sort entire arry
	//list_sort(NULL, &td->sorted_sectors_head, &td_sector_index_callback_total);

	struct sorted_sector_index *item_safe;
	struct sorted_sector_index *item;
	struct sorted_sector_index *prev;

	unsigned long start_time = jiffies;

	bool did_something = false;
	bool moved = true;
	while(moved)
	{
		moved = false;
		list_for_each_entry_safe(item, item_safe, &td->sorted_sectors_head, total_sorted)
		{
			//printk_ratelimited(KERN_DEBUG "tDisk sorting logical sector %u\n", item-td->sorted_sectors);
			if(unlikely(item->total_sorted.prev != &td->sorted_sectors_head))
			{
				bool move = false;
				prev = list_entry(item->total_sorted.prev, struct sorted_sector_index, total_sorted);

				while(ACCESS_COUNT(item->physical_sector->access_count) > ACCESS_COUNT(prev->physical_sector->access_count))
				{
					move = true;
					if(unlikely(prev->total_sorted.prev == &td->sorted_sectors_head))break;
					prev = list_entry(prev->total_sorted.prev, struct sorted_sector_index, total_sorted);
				}

				if(move)
				{
					did_something = true;
					moved = true;
					list_del_init(&item->total_sorted);

					if(unlikely(prev->total_sorted.prev == &td->sorted_sectors_head))
					{
						list_add(&item->total_sorted, &td->sorted_sectors_head);
					}
					else
					{
						list_add(&item->total_sorted, &prev->total_sorted);
					}
				}
			}

			if((jiffies - start_time) >= HZ && did_something)return did_something;
		}
	}

	return did_something;
}

/**
  * Checks if the given disk header is compatible with the current driver.
  * It returns one of the following values:
  *  - 0 if the header is compatible (current driver version is higher or equal)
  *  - 1 if the given header was from an entirely new disk. This means it needs to be initialized
  *  - -1 if the the header was from a driver with a greater minor version than the
  *    current driver. It may be compatible...
  *  - -2 if the the header was from a driver with a greater major version than the
  *    current driver. It may be compatible but probably not...
 **/
static int td_is_compatible_header(struct tdisk *td, struct tdisk_header *header)
{
	if(strcmp(header->driver_name, DRIVER_NAME) != 0)return 1;

	if(header->major_version == DRIVER_MAJOR_VERSION)
	{
		if(header->minor_version > DRIVER_MINOR_VERSION)return -1;
	}
	else if(header->major_version > DRIVER_MAJOR_VERSION)return -2;

	return 0;
}

/**
  * Reads the td header from the given device
  * and measures the disk performance if perf != NULL
 **/
static int td_read_header(struct tdisk *td, struct td_internal_device *device, struct tdisk_header *header, bool first_device, int *index_operation_to_do, bool format)
{
	int error;

	error = read_data(device, header, 0, sizeof(struct tdisk_header));
	printk(KERN_DEBUG "tDisk: Finished reading header: %d\n", error);
	if(error)return error;

	printk(KERN_DEBUG "tDisk: Header: driver: %s, minor: %u, major: %u\n", header->driver_name, header->major_version, header->minor_version);
	switch(td_is_compatible_header(td, header))
	{
	case 1:
		//Entirely new disk.
		header->disk_index = (tdisk_index)(td->internal_devices_count + 1);	//+1 because 0 means unused
		(*index_operation_to_do) = WRITE;
		break;
	case 0:
		//OK, compatible header.
		//Either same or lower (compatible) driver version
		//Disk was already part of a tDisk.
		//So doing nothing to preserve indices etc.
	case -1:
		//Oops, the disk was part of a tdisk
		//But using a driver with a higher minor number
		//Assuming that user space knows if it's compatible...
		//Doing nothing
	case -2:
		//Oops, the disk was part of a tdisk
		//But using a driver with a higher major number
		//Assuming that user space knows if it's compatible...
		//Doing nothing

		if(first_device)
			(*index_operation_to_do) = READ;
		else
			(*index_operation_to_do) = COMPARE;

#ifdef MEASURE_PERFORMANCE
		device->performance = header->performance;
#else
//#pragma message "Performance measurement is disabled"
#endif //MEASURE_PERFORMANCE
		break;
	default:
		//Bug
		printk(KERN_ERR "tDisk: Bug in td_is_compatible_header function\n");
		BUG_ON(td_is_compatible_header(td, header));
		(*index_operation_to_do) = 123456;
		break;
	}

	if((*index_operation_to_do) != WRITE && format == true)
	{
		printk(KERN_INFO "tDisk: device was part of a tDisk but formatting as you requested\n");
		(*index_operation_to_do) = WRITE;
		header->disk_index = (tdisk_index)(td->internal_devices_count + 1);
	}

	return 0;
}

/**
  * Writes the td header to the given device
  * and measures the disk performance if perf != NULL
 **/
static int td_write_header(struct td_internal_device *device, struct tdisk_header *header)
{
	int ret;

	memset(header->driver_name, 0, sizeof(header->driver_name));
	strcpy(header->driver_name, DRIVER_NAME);
	header->driver_name[sizeof(header->driver_name)-1] = 0;
	header->major_version = DRIVER_MAJOR_VERSION;
	header->minor_version = DRIVER_MINOR_VERSION;

	ret = write_data(device, header, 0, sizeof(struct tdisk_header));

	if(ret)printk(KERN_ERR "tDisk: Error writing header: %d\n", ret);

	return ret;
}

/**
  * Reads all the sector indices from the device and
  * stores them in data.
 **/
static int td_read_all_indices(struct tdisk *td, struct td_internal_device *device, u8 *data)
{
	int ret = 0;
	loff_t skip = sizeof(struct tdisk_header);
	loff_t length = td->header_size * td->blocksize - skip;
	unsigned int u_length = (unsigned int)length;

	BUG_ON(length != u_length);

	ret = read_data(device, data, skip, u_length);

	if(ret)printk(KERN_ERR "tDisk: Error reading all disk indices: %d\n", ret);
	else printk(KERN_DEBUG "tDisk: Success reading all disk indices\n");

	return ret;
}

#ifdef USE_INITIAL_OPTIMIZATION
/**
  * This function finds a sector which is unused and
  * has the better performance than the given one.
 **/
sector_t td_find_sector_for_better_performance(struct tdisk *td, sector_t sector)
{
	tdisk_index i;
	tdisk_index j;
	tdisk_index disk = td->indices[sector].disk;
	unsigned long long original_device_performance = td_get_device_performance(&td->internal_devices[disk-1]);
	tdisk_index better_devices[TDISK_MAX_PHYSICAL_DISKS];
	sector_t current_sector;

	memset(better_devices, 0, sizeof(tdisk_index)*TDISK_MAX_PHYSICAL_DISKS);

	//Assigning devices with better performance to better_devices
	//in an ordered mode
	for(i = 1; i <= td->internal_devices_count; ++i)
	{
		unsigned long long current_device_performance;

		if(i == disk)continue;
		current_device_performance = td_get_device_performance(&td->internal_devices[i-1]);

		//The current device is slower than the original
		//device. It doesn't make sense to use it
		if(current_device_performance > original_device_performance)
			continue;

		for(j = td->internal_devices_count; j > 0; --j)
		{
			if(better_devices[j-1] != 0)
			{
				if(td_get_device_performance(&td->internal_devices[better_devices[j-1] - 1]) > current_device_performance)
					swap(better_devices[j-1], better_devices[j]);
				else break;
			}
		}

		better_devices[j] = i;
	}

	//No faster device found
	if(better_devices[0] == 0)return sector;

	//Try to find a free sector from a bett erdevice
	for(current_sector = 0; current_sector < td->size_blocks; ++current_sector)
	{
		if(td->indices[current_sector].disk == disk ||
			td->indices[current_sector].disk == 0 ||
			SECTOR_USED(td->indices[current_sector].access_count))continue;

		for(j = 0; j < td->internal_devices_count && better_devices[j] != 0; ++j)
		{
			//Original sector is better than current sector
			if(td->indices[sector].disk == better_devices[j])break;

			//Current sector is better. Swapping
			if(td->indices[current_sector].disk == better_devices[j])
			{
				//Better sector found
				sector = current_sector;

				//Sector from best device found
				if(j == 0)current_sector = td->size_blocks;

				break;
			}
		}
	}

	return sector;
}
#else
#pragma message "Initial optimization is disabled"
#endif //USE_INITIAL_OPTIMIZATION

/**
  * This function does the actual device operations. It extracts
  * the logical sector and the data from the request. Then it
  * searches for the corresponding disk and physical sector and
  * does the actual device operation. Disk performances are also
  * recorded.
 **/
static int td_do_disk_operation(struct tdisk *td, struct request *rq)
{
	struct bio_vec bvec;
	struct req_iterator iter;
	ssize_t len;
	loff_t pos_byte;
	int ret = 0;
	struct sector_index physical_sector;

	pos_byte = (loff_t)blk_rq_pos(rq) << 9;

	//Handle flush operations
	if((rq->cmd_flags & REQ_WRITE) && (rq->cmd_flags & REQ_FLUSH))
		return td_flush_devices(td);

	//Normal file operations
	rq_for_each_segment(bvec, rq, iter)
	{
		struct td_internal_device *device;
		loff_t sector_div = pos_byte;
		loff_t offset = __div64_32(&sector_div, td->blocksize);
		sector_t sector = (sector_t)sector_div;
		loff_t actual_pos_byte;

#ifdef USE_INITIAL_OPTIMIZATION
		if(!SECTOR_USED(td->indices[sector].access_count))
		{
			//If the sector is not yet used we can try to find a
			//faster disk to gain some performance

			sector_t better_sector = td_find_sector_for_better_performance(td, sector);

			if(sector != better_sector)
			{
				physical_sector.disk = td->indices[better_sector].disk;
				physical_sector.sector = td->indices[better_sector].sector;

				td->indices[better_sector].disk = td->indices[sector].disk;
				td->indices[better_sector].sector = td->indices[sector].sector;

				td->indices[sector].disk = physical_sector.disk;
				td->indices[sector].sector = physical_sector.sector;

				td_write_index_to_disk(td, sector, td->indices[sector].disk);
				td_write_index_to_disk(td, better_sector, td->indices[sector].disk);
				td_write_index_to_disk(td, sector, td->indices[better_sector].disk);
				td_write_index_to_disk(td, better_sector, td->indices[better_sector].disk);
			}
		}
#else
#pragma message "Initial optimization is disabled"
#endif //USE_INITIAL_OPTIMIZATION

		//Fetch physical index
		ret = td_perform_index_operation(td, READ, sector, &physical_sector, true, true);
		if(ret != 0)
		{
			printk_ratelimited(KERN_ERR "tDisk: Error reading logical sector index: %llu\n", sector);
			ret = -EIO;
			continue;
		}

		//Calculate actual position in the physical disk
		actual_pos_byte = (loff_t)physical_sector.sector*td->blocksize + offset;

		if(physical_sector.disk == 0 || physical_sector.disk > td->internal_devices_count)
		{
			printk_ratelimited(KERN_ERR "tDisk: found invalid disk index for reading logical sector %llu: %u\n", sector, physical_sector.disk);
			ret = -EIO;
			continue;
		}

		device = &td->internal_devices[physical_sector.disk - 1];	//-1 because 0 means unused
		if(!device_is_ready(device))
		{
			printk_ratelimited(KERN_DEBUG "tDisk: Device %u is not ready. Probably not yet loaded...\n", physical_sector.disk);
			ret = -EIO;
			continue;
		}

		if((rq->cmd_flags & REQ_WRITE) && (rq->cmd_flags & REQ_DISCARD))
		{
			//Handle discard operations
			len = (ssize_t)bvec.bv_len;
			ret = device_alloc(device, actual_pos_byte, bvec.bv_len);
			if(ret)break;
		}
		else if(rq->cmd_flags & REQ_WRITE)
		{
			//Do write operation
			len = write_bio_vec(device, &bvec, &actual_pos_byte);

			if(unlikely((size_t)len != bvec.bv_len))
			{
				printk(KERN_ERR "tDisk: Write error at byte offset %llu, length %li/%u.\n", pos_byte, len, bvec.bv_len);

				if(len >= 0)ret = -EIO;
				else ret = (int)len;
				break;
			}
		}
		else
		{
			//Do read operation
			len = read_bio_vec(device, &bvec, &actual_pos_byte);

			if(len < 0)
			{
				ret = (int)len;
				break;
			}

			flush_dcache_page(bvec.bv_page);

			if((size_t)len != bvec.bv_len)
			{
				struct bio *bio;
				__rq_for_each_bio(bio, rq)zero_fill_bio(bio);
				break;
			}
		}

		pos_byte += len;
		cond_resched();
	}

	return ret;
}

#ifdef ASYNC_OPERATIONS

/**
  * This callback is used for async file operations
 **/
static void async_request_callback(void *private_data, long ret)
{
	struct request *rq = private_data;
	printk(KERN_DEBUG "tDisk: request finished, ret: %ld\n", ret);
	if(ret > 0)
	{
		//Handle partial reads
		if(!(rq->cmd_flags & REQ_WRITE))
		{
			if(unlikely((size_t)ret < blk_rq_bytes(rq)))
			{
				struct bio *bio = rq->bio;

				bio_advance(bio, (unsigned int)ret);
				zero_fill_bio(bio);
			}
		}

		ret = 0;
	}
	else
	{
		printk(KERN_ERR "tDisk: Async disk error\n");
		ret = -EIO;
	}
	

#if LINUX_VERSION_CODE <= KERNEL_VERSION(4,1,14)
	rq->errors = ret;
	blk_mq_complete_request(rq);
#else
	blk_mq_complete_request(rq, ret);
#endif //LINUX_VERSION_CODE <= KERNEL_VERSION(4,1,14)
}

/**
  * This function does the actual device operations. It extracts
  * the logical sector and the data from the request. Then it
  * searches for the corresponding disk and physical sector and
  * does the actual device operation. Disk performances are also
  * recorded.
 **/
static int td_do_disk_operation_async(struct tdisk *td, struct request *rq)
{
	struct bio_vec bvec;
	struct req_iterator iter;
	loff_t pos_byte;
	int ret = -EIOCBQUEUED;
	struct sector_index physical_sector;

	struct multi_aio_data *multi_data = kmalloc(sizeof(struct multi_aio_data), GFP_KERNEL);

	atomic_set(&multi_data->ret, 0);
	atomic_set(&multi_data->remaining, 1);
	multi_data->length = (long)blk_rq_bytes(rq);
	multi_data->private_data = rq;
	multi_data->callback = async_request_callback;


	pos_byte = (loff_t)blk_rq_pos(rq) << 9;

	//Handle flush operations
	if((rq->cmd_flags & REQ_WRITE) && (rq->cmd_flags & REQ_FLUSH))
		return td_flush_devices(td);

	//Normal file operations
	rq_for_each_segment(bvec, rq, iter)
	{
		struct td_internal_device *device;
		loff_t sector_div = pos_byte;
		loff_t offset = __div64_32(&sector_div, td->blocksize);
		sector_t sector = (sector_t)sector_div;
		loff_t actual_pos_byte;

#ifdef USE_INITIAL_OPTIMIZATION
		if(!SECTOR_USED(td->indices[sector].access_count))
		{
			//If the sector is not yet used we can try to find a
			//faster disk to gain some performance

			sector_t better_sector = td_find_sector_for_better_performance(td, sector);

			if(sector != better_sector)
			{
				physical_sector.disk = td->indices[better_sector].disk;
				physical_sector.sector = td->indices[better_sector].sector;

				td->indices[better_sector].disk = td->indices[sector].disk;
				td->indices[better_sector].sector = td->indices[sector].sector;

				td->indices[sector].disk = physical_sector.disk;
				td->indices[sector].sector = physical_sector.sector;

				//atomic_inc(&multi_data->remaining);
				//atomic_inc(&multi_data->remaining);
				//atomic_inc(&multi_data->remaining);
				//atomic_inc(&multi_data->remaining);
				//td_write_index_to_disk_async(td, sector, td->indices[sector].disk, multi_data, &file_multi_aio_complete);
				//td_write_index_to_disk_async(td, better_sector, td->indices[sector].disk, multi_data, &file_multi_aio_complete);
				//td_write_index_to_disk_async(td, sector, td->indices[better_sector].disk, multi_data, &file_multi_aio_complete);
				//td_write_index_to_disk_async(td, better_sector, td->indices[better_sector].disk, multi_data, &file_multi_aio_complete);
				td_write_index_to_disk_async(td, sector, td->indices[sector].disk, multi_data, &file_multi_aio_complete);
				td_write_index_to_disk_async(td, better_sector, td->indices[sector].disk, multi_data, &file_multi_aio_complete);
				td_write_index_to_disk_async(td, sector, td->indices[better_sector].disk, multi_data, &file_multi_aio_complete);
				td_write_index_to_disk_async(td, better_sector, td->indices[better_sector].disk, multi_data, &file_multi_aio_complete);
			}
		}
#else
#pragma message "Initial optimization is disabled"
#endif //USE_INITIAL_OPTIMIZATION

		//Fetch physical index
		td_perform_index_operation(td, READ, sector, &physical_sector, true, true);

		//Calculate actual position in the physical disk
		actual_pos_byte = (loff_t)physical_sector.sector*td->blocksize + offset;

		if(physical_sector.disk == 0 || physical_sector.disk > td->internal_devices_count)
		{
			printk_ratelimited(KERN_ERR "tDisk: found invalid disk index for reading logical sector %llu: %u\n", sector, physical_sector.disk);
			ret = -EIO;
			continue;
		}

		device = &td->internal_devices[physical_sector.disk - 1];	//-1 because 0 means unused
		if(!device_is_ready(device))
		{
			printk_ratelimited(KERN_DEBUG "tDisk: Device %u is not ready. Probably not yet loaded...\n", physical_sector.disk);
			ret = -EIO;
			continue;
		}

		if((rq->cmd_flags & REQ_WRITE) && (rq->cmd_flags & REQ_DISCARD))
		{
			//Handle discard operations
			ret = device_alloc(device, actual_pos_byte, bvec.bv_len);
			if(ret)break;
		}
		else if(rq->cmd_flags & REQ_WRITE)
		{
			//Do write operation
			printk(KERN_DEBUG "tDisk: Async write operation\n");
			atomic_inc(&multi_data->remaining);
			write_bio_vec_async(device, &bvec, actual_pos_byte, multi_data, &file_multi_aio_complete);
		}
		else
		{
			//Do read operation
			printk(KERN_DEBUG "tDisk: Async read operation\n");
			atomic_inc(&multi_data->remaining);
			read_bio_vec_async(device, &bvec, actual_pos_byte, multi_data, &file_multi_aio_complete);
		}

		pos_byte += bvec.bv_len;
		cond_resched();
	}

	file_multi_aio_complete(multi_data, 0);
	return ret;
}
#endif //ASYNC_OPERATIONS

/**
  * Read the partition table of the tDisk
 **/
static void td_reread_partitions(struct tdisk *td, struct block_device *bdev)
{
#if LINUX_VERSION_CODE > KERNEL_VERSION(4,1,14)
	/**
	  * bd_mutex has been held already in release path, so don't
	  * acquire it if this function is called in such case.
	  *
	  * If the reread partition isn't from release path, lo_refcnt
	  * must be at least one and it can only become zero when the
	  * current holder is released.
	 **/
	if(!atomic_read(&td->refcount))
		__blkdev_reread_part(bdev);
	else
		blkdev_reread_part(bdev);

#else
	ioctl_by_bdev(bdev, BLKRRPART, 0);
#endif
}

/**
  * Returns how much MORE sectors would be needed
  * to store the sector indices if the tDisk would
  * be resized to the given size
 **/
int td_get_max_sectors_header_increase(struct tdisk *td, sector_t max_sectors)
{
	sector_t header_size_byte_help = td->index_offset_byte + max_sectors * sizeof(struct sector_index);
	size_t header_size_byte = (size_t)header_size_byte_help;
	size_t new_header_size;

	//Check if we can actually hold the index in memory...
	if(unlikely(header_size_byte_help != header_size_byte))
	{
		printk(KERN_WARNING "tDisk: can't hold index memory of %llu bytes\n", header_size_byte_help);
		return -1;
	}
	

	new_header_size = header_size_byte/td->blocksize + ((header_size_byte%td->blocksize == 0) ? 0 : 1);

	return (int)(new_header_size - td->header_size);
}

/**
  * This function resets the already resized sector
  * indices and sorted sectors if an error occurred
  * while adding a new internal device
 **/
void td_reset_sectors(struct tdisk *td)
{
	if(td->indices != NULL)vfree(td->indices);
	if(td->sorted_sectors != NULL)vfree(td->sorted_sectors);
	td->max_sectors = 0;
	td->header_size = 0;
}

/**
  * This function is used to resize the sector indices.
  * It also sets all the required parameters in the tDisk.
  * The function returns the amount of additional blocks
  * which are used to store the increased amount of sectors
  * (can be 0) or a negative value when an error happened.
 **/
int td_set_max_sectors(struct tdisk *td, sector_t max_sectors)
{
	int ret;
	sector_t j;

	//Just casting and hoping that it was previously checked using td_get_max_sectors_header_increase
	size_t header_size_byte = (size_t)(td->index_offset_byte + max_sectors * sizeof(struct sector_index));

	size_t new_header_size = header_size_byte/td->blocksize + ((header_size_byte%td->blocksize == 0) ? 0 : 1);

	struct sector_index *new_indices;
	struct sorted_sector_index *new_sorted_sectors;

	//Simply casting. If header_size_byte didn't overflow, this shouln'd overflow as well
	sector_t new_max_sectors = __div64_32_nomod(new_header_size*td->blocksize - td->index_offset_byte, sizeof(struct sector_index));

	if(td->header_size == new_header_size)return 0;

	//New max sectors must be greater or equal than before
	MY_BUG_ON(td->max_sectors > new_max_sectors, PRINT_ULL(td->max_sectors), PRINT_ULL(new_max_sectors));

	//Allocate disk indices
	ret = -ENOMEM;
	new_indices = vmalloc((size_t)(sizeof(struct sector_index) * new_max_sectors));
	if(!new_indices)goto out;

	//Allocate sorted disk indices
	ret = -ENOMEM;
	new_sorted_sectors = vmalloc((size_t)(sizeof(struct sorted_sector_index) * new_max_sectors));
	if(!new_sorted_sectors)goto out_free_indices;

	memset(new_indices, 0, (size_t)(sizeof(struct sector_index) * new_max_sectors));
	memset(new_sorted_sectors, 0, (size_t)(sizeof(struct sorted_sector_index) * new_max_sectors));

	//Copy old indices
	memcpy(new_indices, td->indices, (size_t)(td->max_sectors * sizeof(struct sector_index)));
	memcpy(new_sorted_sectors, td->sorted_sectors, (size_t)(td->max_sectors * sizeof(struct sorted_sector_index)));

	//TODO lock index spinlock
	swap(new_indices, td->indices);
	swap(new_sorted_sectors, td->sorted_sectors);
	swap(new_max_sectors, td->max_sectors);
	swap(new_header_size, td->header_size);

	//Insert sorted indices
	INIT_LIST_HEAD(&td->sorted_sectors_head);
	for(j = 0; j < td->max_sectors; ++j)
	{
		INIT_LIST_HEAD(&td->sorted_sectors[j].total_sorted);
		td->sorted_sectors[j].physical_sector = &td->indices[j];

		list_add(&td->sorted_sectors[j].total_sorted, &td->sorted_sectors_head);
	}
	//TODO unlock index spinlock

	ret = (int)(td->header_size - new_header_size);

	vfree(new_sorted_sectors);
 out_free_indices:
	vfree(new_indices);
 out:
	return ret;
}

#ifdef USE_FILES

/**
  * This function checks if the given file can be
  * added as internal device to the tDisk
 **/
static int td_add_check_file(struct tdisk *td, fmode_t mode, struct block_device *bdev, struct file *file)
{
	struct inode *inode;
	struct address_space *mapping;

	//Don't add current tDisk as backend file
	if(file_is_tdisk(file, TD_MAJOR) && file->f_mapping->host->i_bdev == bdev)
	{
		printk(KERN_WARNING "tDisk: Can't add myself as a backend file\n");
		return -EINVAL;
	}

	mapping = file->f_mapping;
	inode = mapping->host;

	//Check file type
	if(!S_ISREG(inode->i_mode) && !S_ISBLK(inode->i_mode))
	{
		printk(KERN_WARNING "tDisk: Given file is not a regular file and not a blockdevice\n");
		return -EINVAL;
	}

	//Check rw permissions
	if(!(file->f_mode & FMODE_WRITE) || !(mode & FMODE_WRITE) || !file->f_op->write_iter)
	{
		if(td->internal_devices_count && !(td->flags & TD_FLAGS_READ_ONLY))
		{
			printk(KERN_WARNING "Can't add a readonly file to a non read only device!\n");
			return -EPERM;
		}

		//flags |= TD_FLAGS_READ_ONLY;
	}

	return 0;
}

#else
#pragma message "Files are disabled"
#endif //USE_FILES

/**
  * Applies the internal_device_add_parameters to the actual new
  * internal device
 **/
static int set_device_parameters(struct td_internal_device *new_device, struct internal_device_add_parameters __user *arg)
{
	struct internal_device_add_parameters parameters;

	if(copy_from_user(&parameters, arg, sizeof(struct internal_device_add_parameters)) != 0)
		return -EFAULT;

	new_device->type = parameters.type;
	memcpy(new_device->name, parameters.name, TDISK_MAX_INTERNAL_DEVICE_NAME);
	memcpy(new_device->path, parameters.path, TDISK_MAX_INTERNAL_DEVICE_NAME);

	new_device->file = fget(parameters.fd);

	return 0;
}

/**
  * Applies the internal_device_add_parameters to the actual new
  * internal device
 **/
static int device_should_format(struct internal_device_add_parameters __user *arg)
{
	struct internal_device_add_parameters parameters;

	if(copy_from_user(&parameters, arg, sizeof(struct internal_device_add_parameters)) != 0)
		return -EFAULT;

	return parameters.format;
}

static sector_t find_move_help_sector(struct tdisk *td, tdisk_index disk, sector_t max_sector)
{
	sector_t sector;
	sector_t current_sector = 0;

	for(current_sector = td->header_size; current_sector <= max_sector; ++current_sector)
	{
		bool found = false;

		for(sector = 0; sector < td->max_sectors; ++sector)
		{
			if(td->indices[sector].disk == disk && td->indices[sector].sector == current_sector)
			{
				found = true;
				break;
			}
		}

		if(!found)return current_sector;
	}

	printk(KERN_WARNING "tDisk: No move help sector found for disk %u! Using last: %llu\n", disk, max_sector+1);
	return max_sector+1;
}

/**
  * Adds the given internal device to the tDisk.
  * This function reads the disk header, performs the correct
  * index operation (WRITE, READ, COMPARE), adds it to the tDisk
  * and informs user space about any changes (size, partitions...)
 **/
static int td_add_disk(struct tdisk *td, fmode_t mode, struct block_device *bdev, struct internal_device_add_parameters __user *arg)
{
	struct address_space *mapping;
	struct tdisk_header header;
	int error;
	loff_t device_size;
	sector_t sector = 0;
	sector_t new_max_sectors;
	struct sector_index *physical_sector;
	struct td_internal_device new_device;
	int index_operation_to_do;
	int first_device = (td->internal_devices_count == 0);
	int additional_sectors;
	bool format;

	//Check for disk limit
	if(td->internal_devices_count == TDISK_MAX_PHYSICAL_DISKS)
	{
		printk(KERN_ERR "tDisk: Limit (255) of devices reached.\n");
		error = -ENODEV;
		goto out;
	}

	spin_lock(&td->tdisk_lock);
	td->modifying = true;
	while(td->optimizing)
	{
		//We set the modifying flag to true so that the thread
		//stops optimizing, but we still need to wait until the
		//thread has finished its current work
		printk_ratelimited(KERN_DEBUG "tDisk: Waiting for optimizer to finish before adding new device\n");

		spin_unlock(&td->tdisk_lock);
		msleep(1000);
		spin_lock(&td->tdisk_lock);
	}
	spin_unlock(&td->tdisk_lock);

	error = -EFAULT;
	memset(&new_device, 0, sizeof(struct td_internal_device));
	if(set_device_parameters(&new_device, arg) != 0)
		goto out;

	format = device_should_format(arg);

	switch(new_device.type)
	{
#ifdef USE_FILES
	case internal_device_type_file:
		DEBUG_POINT(&td->debug, "tDisk: Adding new internal device file: %s (Path: %s)\n", new_device.name, new_device.path);
		printk(KERN_INFO "tDisk: Adding new internal device file: %s (Path: %s)\n", new_device.name, new_device.path);
		error = -EBADF;
		if(!new_device.file)goto out;

		error = td_add_check_file(td, mode, bdev, new_device.file);
		if(error)goto out_putf;

		mapping = new_device.file->f_mapping;
		new_device.old_gfp_mask = mapping_gfp_mask(mapping);
		mapping_set_gfp_mask(mapping, new_device.old_gfp_mask & ~(__GFP_IO|__GFP_FS));

		//File size in bytes
		device_size = file_get_size(new_device.file);
		break;
#else
#pragma message "Files are disabled"
#endif //USE_FILES

#ifdef USE_PLUGINS
	case internal_device_type_plugin:
		DEBUG_POINT(&td->debug, "tDisk: Adding new internal device plugin: %s (Path: %s)\n", new_device.name, new_device.path);
		printk(KERN_INFO "tDisk: Adding new internal device plugin: %s (Path: %s)\n", new_device.name, new_device.path);

		//Plugin size in bytes
		device_size = plugin_get_size(new_device.name);
		break;
#else
#pragma message "Plugins are disabled"
#endif //USE_PLUGINS

	default:
		printk(KERN_WARNING "tDisk: Invalid device type %d\n", new_device.type);
		error = -EINVAL;
		goto out;
	}

#ifdef MEASURE_PING_PERFORMANCE
	td_measure_device_performance(&new_device, device_size);
#else
#pragma message "Ping performance measurement is disabled"
#endif //MEASURE_PING_PERFORMANCE

	error = td_read_header(td, &new_device, &header, first_device, &index_operation_to_do, format);
	if(error)goto out_putf;

	//Calculate new max_sectors of tDisk
	if(index_operation_to_do == WRITE)
	{
		loff_t device_size_blocks = (loff_t)td->size_blocks * td->blocksize + device_size;
		if(__div64_32(&device_size_blocks, td->blocksize))device_size_blocks++;
		new_max_sectors = (sector_t)device_size_blocks;
	}
	else new_max_sectors = td->max_sectors;

	//Set max_sectors if it's a known device
	if(index_operation_to_do == READ && header.current_max_sectors > new_max_sectors)new_max_sectors = header.current_max_sectors;

	error = -ENOMEM;
	additional_sectors = td_get_max_sectors_header_increase(td, new_max_sectors);
	if(additional_sectors < 0)goto out_putf;

	error = -EINVAL;
	if(additional_sectors != 0)printk(KERN_DEBUG "tDisk: %d additional sectors needed\n", additional_sectors);
	if(device_size < ((loff_t)td->header_size+additional_sectors+1)*td->blocksize)	//Disk too small
	{
		printk(KERN_WARNING "tDisk: Can't add disk, too small: %llu. Should be at least %llu\n", device_size, ((loff_t)td->header_size+additional_sectors+1)*td->blocksize);
		goto out_putf;
	}

	//Subtract (new) header size from disk size
	device_size -= ((loff_t)td->header_size+additional_sectors)*td->blocksize;

	//resize tDisk indices
	if(additional_sectors != 0)
	{
		tdisk_index disk;

		if(index_operation_to_do != WRITE && index_operation_to_do != READ)
		{
			printk(KERN_WARNING "tDisk: Can't add new disk because it would increase the tDisk index\n");
			error = -EINVAL;
			goto out_putf;
		}

		for(disk = 1; disk <= td->internal_devices_count; ++disk)
		{
			if(!device_is_ready(&td->internal_devices[disk-1]))
			{
				printk(KERN_WARNING "tDisk: This disk would increase the tDisk index size but not all disks are loaded yet.\n");
				printk(KERN_WARNING "tDisk: ...can't continue, please add all disks first.\n");
				error = -EINVAL;
				goto out_putf;
			}
		}
	}

	//Resize sector indices an sorted sectors
	additional_sectors = td_set_max_sectors(td, new_max_sectors);
	if(additional_sectors < 0)
	{
		error = additional_sectors;
		goto out_reset_sectors;
	}

	//Set disk references in tDisk
	if(header.disk_index > td->internal_devices_count)td->internal_devices_count = (tdisk_index)(header.disk_index);

	//Calculate actual device size
	new_device.size_blocks = __div64_32_nomod((uint64_t)device_size, td->blocksize) - 1;	//-1 to leave one sector for movement

	error = 0;

	//set_device_ro(bdev, (td->flags & TD_FLAGS_READ_ONLY) != 0); //TODO move to function where tdisk is created

	td->block_device = bdev;

	//Perform index operations
	switch(index_operation_to_do)
	{
	case WRITE:
		//Writing header to disk
		header.current_max_sectors = td->max_sectors;
		td_write_header(&new_device, &header);

		//Save sector indices
		physical_sector = vmalloc(sizeof(struct sector_index));
		for(sector = 0; sector < new_device.size_blocks; ++sector, ++td->size_blocks)
		{
			int internal_ret;
			physical_sector->disk = (tdisk_index)(header.disk_index);
			physical_sector->sector = td->header_size + sector;
			internal_ret = td_perform_index_operation(td, WRITE, td->size_blocks, physical_sector, false, false);
			if(internal_ret == 1)
			{
				//This should be impossible since we increase the index everytime it is necessary
				printk(KERN_ERR "tDisk: Additional disk doesn't fit in index. Shrinking to fit.\n");
				break;
			}
		}
		vfree(physical_sector);

		new_device.move_help_sector = td->header_size + sector;

		//Write indices
		td_write_all_indices(td, &new_device);
		{
			int disk;
			for(disk = 1; disk <= td->internal_devices_count; ++disk)
				if(device_is_ready(&td->internal_devices[disk-1]))
					td_write_all_indices(td, &td->internal_devices[disk-1]);
		}

		break;
	case COMPARE:
		//Reading all indices into temporary memory
		physical_sector = vmalloc((size_t)(sizeof(struct sector_index) * td->max_sectors));
		td_read_all_indices(td, &new_device, (u8*)physical_sector);

		//Now comparing all sector indices
		for(sector = 0; sector < td->max_sectors; ++sector)
		{
			int internal_ret = td_perform_index_operation(td, COMPARE, sector, &physical_sector[sector], false, false);
			if(internal_ret == -1)
			{
				//We have the rule that if the index doesn't match, each disk
				//has priority of it's own index
				if(physical_sector[sector].disk == header.disk_index)
				{
					//Replace index value
					td_perform_index_operation(td, WRITE, sector, &physical_sector[sector], false, false);
				}
				else printk_ratelimited(KERN_WARNING "tDisk: Disk index doesn't match. Probably wrong or corrupt disk attached. Pay attention before you write to disk!\n");
			}
		}

		new_device.move_help_sector = find_move_help_sector(td, header.disk_index, new_device.size_blocks+1);

		vfree(physical_sector);
		break;
	case READ:
		//reading all indices from disk
		td_read_all_indices(td, &new_device, (u8*)td->indices);

		for(sector = 0; sector < td->max_sectors; ++sector)
		{
			if(td->indices[sector].disk > td->internal_devices_count)
				td->internal_devices_count = td->indices[sector].disk;

			if(td->indices[sector].disk == 0)break;
		}
		td->size_blocks = sector;
		new_device.move_help_sector = find_move_help_sector(td, header.disk_index, new_device.size_blocks+1);
		if(td->size_blocks == 0)
		{
			error = -EINVAL;
			td->internal_devices_count = 0;
			printk(KERN_WARNING "tDisk: blocksize is 0. This probably means that the system crashed.\n");
			printk(KERN_WARNING "tDisk: Sorry but the disk is broken. Maybe it can be reconstructed if you use another disk from this tDisk\n");
			goto out_reset_sectors;
		}
		break;
	default:
		printk(KERN_ERR "tDisk: Invalid index_operation_to_do: %d\n", index_operation_to_do);
		BUG_ON(true);
	}

	printk(KERN_DEBUG "tDisk: move_help_sector is %llu\n", new_device.move_help_sector);

	if(additional_sectors != 0 && !first_device)
	{
		//Header size increased. Let's create available sectors
		//At the beginning of each disk by moving them to the new disk.
		//Since this operation is very critical and wee need to use the
		//newly inserted device before it is actually ready, we stop the
		//queue to prevent any data corruption

		sector_t search;

		//Stopping queue
		td_stop_worker_thread(td);

		//Now we need to set the partially finished device
		//to make sector movement possible
		td->internal_devices[header.disk_index-1] = new_device;

		printk(KERN_DEBUG "tDisk: Need to move sectors because index size increased by %d\n", additional_sectors);

		for(sector = 0; sector < td->max_sectors; ++sector)
		{
			if(td->indices[sector].disk != header.disk_index && td->indices[sector].disk != 0)
			{
				//sector is not a sector of the current disk (=header.disk_index) but it's used (!= 0)

				if(td->indices[sector].sector < td->header_size)
				{
					//This sector needs to be moved

					tdisk_index disk = td->indices[sector].disk;
					bool moved = false;

					for(search = td->size_blocks; search > sector; --search)
					{
						if(td->indices[search].disk == header.disk_index)
						{
							//Appropriate block found
							printk(KERN_DEBUG "tDisk: Moved header block %lu (disk: %u, sector: %llu) to %lu (disk: %u, sector: %llu)",
									sector, td->indices[sector].disk, td->indices[sector].sector, search, td->indices[search].disk, td->indices[search].sector);

							td_swap_sectors(td, sector, &td->indices[sector], search, &td->indices[search], true);

							//Resetting moved-block values
							td->indices[search].disk = 0;
							td->indices[search].sector = 0;
							td->indices[search].access_count = 0;

							//Set disks size
							td->size_blocks--;
							td->internal_devices[disk].size_blocks--;

							moved = true;
							break;
						}
					}
					printk(KERN_DEBUG "tDisk: finished sector %lu: %s", sector, moved ? "true" : "false");
					MY_BUG_ON(!moved, PRINT_INT(td->indices[sector].disk), PRINT_ULL(td->indices[sector].sector), PRINT_ULL(search));
				}
			}
		}

		//Starting again the queue
		td_start_worker_thread(td);
	}

	if(first_device)
	{
		//Start the worker thread which handles all the disk
		//operations such as reading, writing and all the sector
		//movements.
		error = td_start_worker_thread(td);
		if(error)
		{
			printk(KERN_WARNING "tDisk: Error setting up worker thread\n");
			goto out_reset_sectors;
		}
	}

	td->internal_devices[header.disk_index-1] = new_device;
	printk(KERN_DEBUG "tDisk: new physical disk %u: size: %llu bytes. Logical size(%lu)\n", header.disk_index, device_size, td->size_blocks*td->blocksize);

	//let user-space know about this change
	set_capacity(td->kernel_disk, (td->size_blocks*td->blocksize) >> 9);
	bd_set_size(bdev, (loff_t)td->size_blocks*td->blocksize);
	kobject_uevent(&disk_to_dev(bdev->bd_disk)->kobj, KOBJ_CHANGE);

	td->state = state_bound;
	td_reread_partitions(td, bdev);

	//Grab the block_device to prevent its destruction
	if(first_device)bdgrab(bdev);
	spin_lock(&td->tdisk_lock);
	td->modifying = false;
	spin_unlock(&td->tdisk_lock);
	return 0;

 out_reset_sectors:
	td_reset_sectors(td);
 out_putf:
	if(new_device.file)fput(new_device.file);
 out:
	spin_lock(&td->tdisk_lock);
	td->modifying = false;
	spin_unlock(&td->tdisk_lock);
	return error;
}

/**
  * This function removes the disk with the given index
  * from the tdisk. It does so by moving the disk to be
  * removed to the end and then shrinking the tDisk. If
  * the file system was previously shrinked then there will
  * be no data loss
 **/
static int td_remove_disk(struct tdisk *td, tdisk_index disk)
{
	int error = 0;
	sector_t sector;
	sector_t sector_rev = td->max_sectors - 1;
	sector_t amount_sectors_removed = 0;
	tdisk_index i;
	struct tdisk_header invalid_header;

	spin_lock(&td->tdisk_lock);
	td->modifying = true;
	while(td->optimizing)
	{
		//We set the modifying flag to true so that the thread
		//stops optimizing, but we still need to wait until the
		//thread has finished its current work
		printk_ratelimited(KERN_DEBUG "tDisk: Waiting for optimizer to finish before removing new device\n");

		spin_unlock(&td->tdisk_lock);
		msleep(1000);
		spin_lock(&td->tdisk_lock);
	}
	spin_unlock(&td->tdisk_lock);

	//For such a critical operation we need to stop the
	//queue to prevent any data loss
	td_stop_worker_thread(td);

	if(disk == 0 || !device_is_ready(&td->internal_devices[disk-1]))
	{
		printk(KERN_WARNING, "tDisk: Can't remove device %u because it is not ready\n", disk);
		error = -EINVAL;
		goto out;
	}

	//Move all sectors which belong to the internal device to the end of the tDisk
	for(sector = 0; sector < sector_rev; ++sector)
	{
		if(td->indices[sector].disk == disk)
		{
			//This sector needs to be moved to the end

			//Find sector at the end which can be swapped
			while(td->indices[sector_rev].disk == 0 || td->indices[sector_rev].disk == disk)
			{
				if(unlikely(sector_rev == 0))break;
				sector_rev--;
			}

			if(td->indices[sector_rev].disk == 0 || td->indices[sector_rev].disk == disk)
			{
				//No sector found
				printk(KERN_WARNING "tDisk: No sector found at the end while removing disk %d\n", disk);
				error = -ENODEV;
				goto out;
			}

			if(!SECTOR_USED(td->indices[sector].access_count) && !SECTOR_USED(td->indices[sector_rev].access_count))
			{
				//Both sectors are unused. We can simply swap
				//the indices instead of copying data
				swap(td->indices[sector].disk, td->indices[sector_rev].disk);
				swap(td->indices[sector].access_count, td->indices[sector_rev].access_count);
				swap(td->indices[sector].sector, td->indices[sector_rev].sector);
				td_perform_index_operation(td, WRITE, sector, &td->indices[sector], false, false);
				td_perform_index_operation(td, WRITE, sector_rev, &td->indices[sector_rev], false, false);
			}
			else
			{
				error = td_swap_sectors(td, sector, &td->indices[sector], sector_rev, &td->indices[sector_rev], false);
				if(error)
				{
					printk(KERN_WARNING "tDisk: Error swapping sectors %llu and %llu when removing disk %u\n", sector, sector_rev, disk);
					goto out;
				}
			}

			//Now we make the sector invalid by setting the disk to 0
			//Then it is not usable anymore. If there was any data
			//beyond the size limit after the disk is removed then
			//this will be the first point where data is lost. But
			//we assume that the user either modified the partition table
			//or resized the filesystem before.
			td->indices[sector_rev].disk = 0;
			td->indices[sector_rev].access_count = 0;
			td->indices[sector_rev].sector = 0;

			amount_sectors_removed++;
		}
	}

	MY_BUG_ON(td->internal_devices[disk-1].size_blocks != amount_sectors_removed, PRINT_ULL(td->internal_devices[disk-1].size_blocks), PRINT_ULL(amount_sectors_removed));

	//All the sectors are now moved at the end of the tDisk and the 
	//disk is set to 0 which makes them "empty".
	//Now we decrement all disk indices with an id higher than the
	//disk which was removed
	for(sector = 0; sector < td->max_sectors; ++sector)
	{
		if(td->indices[sector].disk > disk)
			td->indices[sector].disk--;
	}

	//Invalidate header
	memset(&invalid_header, 0, sizeof(struct tdisk_header));
	write_data(&td->internal_devices[disk-1], &invalid_header, 0, sizeof(struct tdisk_header));

	//Release file if any
	if(td->internal_devices[disk-1].file)
	{
		gfp_t gfp = td->internal_devices[disk-1].old_gfp_mask;
		mapping_set_gfp_mask(td->internal_devices[disk-1].file->f_mapping, gfp);
		fput(td->internal_devices[disk-1].file);
		td->internal_devices[disk-1].file = NULL;
	}

	//Now we remove the internal device from the array
	for(i = disk; i < td->internal_devices_count; ++i)
		td->internal_devices[i-1] = td->internal_devices[i];
	td->internal_devices_count--;

	//Write all disk indices
	for(i = 1; i <= td->internal_devices_count; ++i)
		td_write_all_indices(td, &td->internal_devices[i-1]);

	//Set new size
	td->size_blocks -= amount_sectors_removed;

	//let user-space know about this change
	set_capacity(td->kernel_disk, (td->size_blocks*td->blocksize) >> 9);
	bd_set_size(td->block_device, (loff_t)td->size_blocks*td->blocksize);
	kobject_uevent(&disk_to_dev(td->block_device->bd_disk)->kobj, KOBJ_CHANGE);

 out:
	spin_lock(&td->tdisk_lock);
	td->modifying = false;
	spin_unlock(&td->tdisk_lock);

	//Starting queue again
	td_start_worker_thread(td);
	td_reread_partitions(td, td->block_device);

	return error;
}

/**
  * This function transfers device status information
  * to user space
 **/
static int td_get_status(struct tdisk *td, struct tdisk_info __user *arg)
{
	struct tdisk_info info;

	if(td->state != state_bound)
		return -ENXIO;

	memset(&info, 0, sizeof(info));
	//info.block_device = huge_encode_dev(td->block_device);
	info.max_sectors = td->max_sectors;
	info.size_blocks = td->size_blocks;
	info.blocksize = td->blocksize;
	info.number = (__u32)td->number;
	info.flags = (__u32)td->flags;
	info.internaldevices = td->internal_devices_count;

	if(copy_to_user(arg, &info, sizeof(info)) != 0)
		return -EFAULT;

	return 0;
}

/**
  * This function transfers information about the internal
  * devices to user space
 **/
static int td_get_device_info(struct tdisk *td, struct internal_device_info __user *arg)
{
	struct internal_device_info info;

	if(copy_from_user(&info, arg, sizeof(struct internal_device_info)) != 0)
		return -EFAULT;

	if(info.disk == 0 || info.disk > td->internal_devices_count)
		return -EINVAL;

	info.type = td->internal_devices[info.disk-1].type;
	memcpy(info.name, td->internal_devices[info.disk-1].name, TDISK_MAX_INTERNAL_DEVICE_NAME);
	memcpy(info.path, td->internal_devices[info.disk-1].path, TDISK_MAX_INTERNAL_DEVICE_NAME);
	info.performance = td->internal_devices[info.disk-1].performance;
	info.size = td->internal_devices[info.disk-1].size_blocks * td->blocksize;

	if(copy_to_user(arg, &info, sizeof(info)) != 0)
		return -EFAULT;

	return 0;
}

/**
  * This function transfers information about the given
  * logical sector to user space
 **/
static int td_get_sector_index(struct tdisk *td, struct physical_sector_index __user *arg)
{
	struct physical_sector_index index;

	if(copy_from_user(&index, arg, sizeof(struct physical_sector_index)) != 0)
		return -EFAULT;

	if(index.sector >= td->max_sectors)return -EINVAL;

	index.disk = td->sorted_sectors[index.sector].physical_sector->disk;
	index.sector = td->sorted_sectors[index.sector].physical_sector->sector;
	index.access_count = ACCESS_COUNT(td->sorted_sectors[index.sector].physical_sector->access_count);
	index.used = SECTOR_USED(td->sorted_sectors[index.sector].physical_sector->access_count);

	if(copy_to_user(arg, &index, sizeof(struct physical_sector_index)) != 0)
		return -EFAULT;

	return 0;
}

/**
  * This function transfers information about all
  * logical sectors to user space
 **/
static int td_get_all_sector_indices(struct tdisk *td, struct sector_info __user *arg)
{
	struct sorted_sector_index *pos;
	struct sector_info info;
	sector_t sorted_index = 0;

	list_for_each_entry(pos, &td->sorted_sectors_head, total_sorted)
	{
		info.physical_sector.disk = pos->physical_sector->disk;
		info.physical_sector.sector = pos->physical_sector->sector;
		info.physical_sector.access_count = ACCESS_COUNT(pos->physical_sector->access_count);
		info.physical_sector.used = SECTOR_USED(pos->physical_sector->access_count);
		info.access_sorted_index = sorted_index;
		info.logical_sector = (__u64)(pos - td->sorted_sectors);

		if(copy_to_user(&arg[sorted_index], &info, sizeof(struct sector_info)) != 0)
			return -EFAULT;

		sorted_index++;
	}

	return 0;
}

/**
  * This function clears the access count of all
  * sectors. This is just for debugging purposes.
 **/
static int td_clear_access_count(struct tdisk *td)
{
	sector_t i;

	for(i = 0; i < td->max_sectors; ++i)
	{
		RESET_ACCESS_COUNT(td->sorted_sectors[i].physical_sector->access_count);
	}

	return 0;
}

/**
  * This function retrieves the next debug info
 **/
static int td_get_debug(struct tdisk *td, struct tdisk_debug_info __user *arg)
{
	struct tdisk_debug_info info;

	if(copy_from_user(&info, arg, sizeof(struct tdisk_debug_info)) != 0)
		return -EFAULT;

	get_next_debug_point(&td->debug, info.id, &info);
	info.hz = HZ;

	if(copy_to_user(arg, &info, sizeof(struct tdisk_debug_info)) != 0)
		return -EFAULT;

	return 0;
}

/**
  * This function handles all ioctl coming from userspace
 **/
static int td_ioctl(struct block_device *bdev, fmode_t mode, unsigned int cmd, unsigned long arg)
{
	struct tdisk *td = bdev->bd_disk->private_data;
	int err;

	mutex_lock_nested(&td->ctl_mutex, 1);
	switch(cmd)
	{
	case TDISK_ADD_DISK:
		err = td_add_disk(td, mode, bdev, (struct internal_device_add_parameters __user *) arg);
		break;
	case TDISK_GET_STATUS:
		err = td_get_status(td, (struct tdisk_info __user *) arg);
		break;
	case TDISK_GET_DEVICE_INFO:
		err = td_get_device_info(td, (struct internal_device_info __user *) arg);
		break;
	case TDISK_GET_SECTOR_INDEX:
		err = td_get_sector_index(td, (struct physical_sector_index __user *) arg);
		break;
	case TDISK_GET_ALL_SECTOR_INDICES:
		err = td_get_all_sector_indices(td, (struct sector_info __user *) arg);
		break;
	case TDISK_CLEAR_ACCESS_COUNT:
		err = td_clear_access_count(td);
		break;
	case TDISK_GET_DEBUG_INFO:
		err = td_get_debug(td, (struct tdisk_debug_info __user *) arg);
		break;
	case TDISK_REMOVE_DISK:
		err = td_remove_disk(td, (tdisk_index)arg);
		break;
	case CDROM_GET_CAPABILITY:
		//Udev sends it and we don't want to spam dmesg
		err = -ENOIOCTLCMD;
		break;
	default:
		printk(KERN_WARNING "tDisk: Invalid IOCTL: %d\n", cmd);
		err = -ENOIOCTLCMD;
		break;
	}
	mutex_unlock(&td->ctl_mutex);

	return err;
}

//For older kernels
#ifdef CONFIG_COMPAT
/**
  * This function handles all ioctl coming from userspace.
  * Compatibility version.
 **/
static int td_compat_ioctl(struct block_device *bdev, fmode_t mode, unsigned int cmd, unsigned long arg)
{
	int err;

	switch(cmd)
	{
	case TDISK_ADD_DISK:
	case TDISK_GET_STATUS:
	case TDISK_GET_DEVICE_INFO:
	case TDISK_GET_SECTOR_INDEX:
	case TDISK_GET_ALL_SECTOR_INDICES:
	case TDISK_CLEAR_ACCESS_COUNT:
	case TDISK_GET_DEBUG_INFO:
	case CDROM_GET_CAPABILITY:
		arg = (unsigned long)compat_ptr((compat_uptr_t)arg);
		err = td_ioctl(bdev, mode, cmd, arg);
		break;
	case TDISK_REMOVE_DISK:
		err = td_ioctl(bdev, mode, cmd, arg);
		break;
	default:
		err = -ENOIOCTLCMD;
		break;
	}
	return err;
}
#endif

/**
  * This function is called by the kernel whenever it
  * it needs the device. This is useful for us to count the references
 **/
static int td_open(struct block_device *bdev, fmode_t mode)
{
	struct tdisk *td = bdev->bd_disk->private_data;
	if(!td)return -ENXIO;

	atomic_inc(&td->refcount);

	return 0;
}

/**
  * This function is called by the kernel whenever it doesn't
  * need the device anymore. If we tried to clear the tDisk
  * with an elevated reference count, we will clear it here
  * once the reference count reaches 0.
 **/
static void td_release(struct gendisk *disk, fmode_t mode)
{
	struct tdisk *td = disk->private_data;

	if(atomic_dec_return(&td->refcount))
		return;

	td_flush(td);
}

/**
  * The block device operations which are presented to the kernel
 **/
static const struct block_device_operations td_fops = {
	.owner =	THIS_MODULE,
	.open =		td_open,
	.release =	td_release,
	.ioctl =	td_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl =	td_compat_ioctl,
#endif
};

/**
  * This function starts the worker thread to handle
  * the actual device operations.
 **/
static int td_start_worker_thread(struct tdisk *td)
{
	init_kthread_worker(&td->worker_timeout.worker);
	td->worker_timeout.private_data = td;
	td->worker_timeout.timeout = DEFAULT_WORKER_TIMEOUT;
	td->worker_timeout.secondary_work_delay = DEFAULT_SECONDARY_WORK_DELAY;
	td->worker_timeout.work_func = &td_queue_work;
	td->worker_task = kthread_run(kthread_worker_fn_timeout, &td->worker_timeout, "td%d", td->number);

	if(IS_ERR(td->worker_task))
		return -ENOMEM;

	set_user_nice(td->worker_task, MIN_NICE);
	return 0;
}

/**
  * Stops the worker thread
 **/
static void td_stop_worker_thread(struct tdisk *td)
{
	flush_kthread_worker_timeout(&td->worker_timeout.worker);
	kthread_stop(td->worker_task);
}

/**
  * This function is called for every request before it is
  * given to the workr thread. This makes it possible to initialize it
  * and add information
 **/
static int td_init_request(void *data, struct request *rq, unsigned int hctx_idx, unsigned int request_idx, unsigned int numa_node)
{
	struct td_command *cmd = blk_mq_rq_to_pdu(rq);

	cmd->rq = rq;

	init_thread_work_timeout(&cmd->td_work);

	return 0;
}

/**
  * This function is called by the kernel for each request
  * it reqeives. This function hands it over to the worker
  * thread.
 **/
static int td_queue_rq(struct blk_mq_hw_ctx *hctx, const struct blk_mq_queue_data *bd)
{
	struct td_command *cmd = blk_mq_rq_to_pdu(bd->rq);
	struct tdisk *td = cmd->rq->q->queuedata;

	blk_mq_start_request(bd->rq);

	if(td->state != state_bound)
		return -EIO;

	queue_kthread_work(&td->worker_timeout.worker, &cmd->td_work);

	return BLK_MQ_RQ_QUEUE_OK;
}

/**
  * This is the actual worker function which is called by
  * the worker thread. It does the file operations and moves
  * the sectors
 **/
static enum worker_status td_queue_work(void *private_data, struct kthread_work *work)
{
	struct tdisk *td = private_data;

	//Since we are not using the standard kthread_work_fn
	//It is possible that this funtion is called without work.
	//The reason behind this is that we can reorganize the indices
	//and sector operations when there is nothing to do
	if(work)
	{
		struct td_command *cmd = container_of(work, struct td_command, td_work);
		int ret = 0;

		if(cmd->rq->cmd_flags & REQ_WRITE && (td->flags & TD_FLAGS_READ_ONLY))
			ret = -EIO;
		else
		{
#ifdef ASYNC_OPERATIONS
			ret = td_do_disk_operation_async(td, cmd->rq);
#else
			ret = td_do_disk_operation(td, cmd->rq);
#endif //ASYNC_OPERATIONS
		}

#ifdef ASYNC_OPERATIONS
		if(ret && ret != -EIOCBQUEUED)
		{
#if LINUX_VERSION_CODE <= KERNEL_VERSION(4,1,14)
			cmd->rq->errors = -EIO;
			blk_mq_complete_request(cmd->rq);
#else
			blk_mq_complete_request(cmd->rq, -EIO);
#endif //LINUX_VERSION_CODE <= KERNEL_VERSION(4,1,14)
		}
#else
#if LINUX_VERSION_CODE <= KERNEL_VERSION(4,1,14)
		if(ret)cmd->rq->errors = -EIO;
		blk_mq_complete_request(cmd->rq);
#else
		blk_mq_complete_request(cmd->rq, ret ? -EIO : 0);
#endif //LINUX_VERSION_CODE <= KERNEL_VERSION(4,1,14)
#endif //ASYNC_OPERATIONS

		//Setting this flag is required to force resoring the indices
		//if the worker was disturbed during sector movement
		td->access_count_resort = 0;

		return next_primary_work;
	}
	else
	{
#ifdef MOVE_SECTORS
		enum worker_status ret_val;

		spin_lock(&td->tdisk_lock);
		if(td->modifying)
		{
			printk_ratelimited(KERN_DEBUG "tDisk: Not optimizing because the tDisk is currently being modified\n");

			//We can't optimize the tDisk while it is being modified
			spin_unlock(&td->tdisk_lock);

			//Returning secondary_work_finished so the thread goes to sleep
			return secondary_work_finished;
		}
		td->optimizing = true;
		spin_unlock(&td->tdisk_lock);

		//No work to do. This means we have reached the timeout
		//and have now the opportunity to organize the sectors.

		if(td->access_count_resort == 0)
		{
			bool still_to_sort;

			printk(KERN_DEBUG "tDisk: nothing to do anymore. Sorting sectors\n");

			still_to_sort = td_reorganize_all_indices(td);

			if(still_to_sort)
			{
				printk(KERN_DEBUG "tDisk: Still sectors to sort. Doing next time...\n");
			}
			else
			{
				printk(KERN_DEBUG "tDisk: Done sorting sectors. Now moving sectors\n");

				if(td->sorted_devices != NULL)
				{
					vfree(td->sorted_devices);
					td->sorted_devices = NULL;
				}

				//Setting to true so the next time we will do move operations
				td->access_count_resort = 1;
			}

			ret_val = secondary_work_to_do;
		}
		else
		{
			//printk(KERN_DEBUG "tDisk: nothing to do anymore. Moving sectors\n");

			td->access_count_resort = 0;
			td_move_one_sector(td);

			if(td->access_count_resort)ret_val = secondary_work_to_do;
			else ret_val = secondary_work_finished;
		}

		spin_lock(&td->tdisk_lock);
		td->optimizing = false;
		spin_unlock(&td->tdisk_lock);
		return ret_val;
#else
#pragma message "Moving sectors is disabled"
		return secondary_work_finished;
#endif //MOVE_SECTORS
	}
}

/**
  * This struct defines the functions to dispatch requests
 **/
static struct blk_mq_ops tdisk_mq_ops = {
	.queue_rq       = td_queue_rq,
	.map_queue      = blk_mq_map_queue,
	.init_request	= td_init_request,
};

/**
  * This function creates a new tDisk with the given
  * minor number, and blocksize.
 **/
int tdisk_add(struct tdisk **t, int i, unsigned int blocksize)
{
	struct tdisk *td;
	struct gendisk *disk;
	int err;
	unsigned int header_size_byte;

	if(blocksize == 0 || blocksize % TDISK_BLOCKSIZE_MOD)
	{
		printk(KERN_WARNING"tDisk: Failed to add tDisk. blocksize must be a multiple of 4096 but is %u\n", blocksize);
		return -EINVAL;
	}

	//Calculate header size which consists
	//of a header, disk mappings and indices
	header_size_byte = sizeof(struct tdisk_header);

	err = -ENOMEM;
	td = kzalloc(sizeof(struct tdisk), GFP_KERNEL);
	if(!td)goto out;

	td->state = state_unbound;
	td->size_blocks = 0;

	//allocate id, if id >= 0, we're requesting that specific id
	if(i >= 0)
	{
		err = idr_alloc(&td_index_idr, td, i, i + 1, GFP_KERNEL);
		if(err == -ENOSPC)err = -EEXIST;
	}
	else
	{
		err = idr_alloc(&td_index_idr, td, 0, 0, GFP_KERNEL);
	}
	if(err < 0)goto out_free_dev;
	i = err;

	//Set queue data
	err = -ENOMEM;
	td->tag_set.ops = &tdisk_mq_ops;
	td->tag_set.nr_hw_queues = 1;
	td->tag_set.queue_depth = 128;
	td->tag_set.numa_node = NUMA_NO_NODE;
	td->tag_set.cmd_size = sizeof(struct td_command);
	td->tag_set.flags = BLK_MQ_F_SHOULD_MERGE | BLK_MQ_F_SG_MERGE;
	td->tag_set.driver_data = td;

	err = blk_mq_alloc_tag_set(&td->tag_set);
	if(err)goto out_free_idr;

	td->queue = blk_mq_init_queue(&td->tag_set);
	if(IS_ERR_OR_NULL(td->queue))
	{
		err = (int)PTR_ERR(td->queue);
		goto out_cleanup_tags;
	}
	td->queue->queuedata = td;

	queue_flag_set_unlocked(QUEUE_FLAG_NOMERGES, td->queue);

	disk = td->kernel_disk = alloc_disk(1);
	if(!disk)goto out_free_queue;

	td->blocksize = blocksize;
	td->index_offset_byte = header_size_byte;
	err = td_set_max_sectors(td, 0);
	if(err < 0)goto out_free_queue;

	init_debug_struct(&td->debug);

	disk->flags |= GENHD_FL_EXT_DEVT;
	mutex_init(&td->ctl_mutex);
	atomic_set(&td->refcount, 0); 
	td->number		= i;
	spin_lock_init(&td->tdisk_lock);
	disk->major		= TD_MAJOR;
	disk->first_minor	= i;
	disk->fops		= &td_fops;
	disk->private_data	= td;
	disk->queue		= td->queue;
	//set_blocksize(td->block_device, blocksize);	TODO?
	sprintf(disk->disk_name, "td%d", i);
	add_disk(disk);
	*t = td;

	DEBUG_POINT(&td->debug, "tDisk: new disk %s: blocksize: %u, header size: %u sec\n", disk->disk_name, blocksize, td->header_size);
	printk(KERN_DEBUG "tDisk: new disk %s: blocksize: %u, header size: %u sec\n", disk->disk_name, blocksize, td->header_size);

	return td->number;

out_free_queue:
	blk_cleanup_queue(td->queue);
out_cleanup_tags:
	blk_mq_free_tag_set(&td->tag_set);
out_free_idr:
	idr_remove(&td_index_idr, i);
out_free_dev:
	kfree(td);
out:
	return err;
}

/**
  * This function is called to remove a tDisk.
  * this frees all the resources
 **/
int tdisk_remove(struct tdisk *td)
{
	tdisk_index i;

	if(atomic_read(&td->refcount) > 0)
		return -EBUSY;

	if(td->block_device)
	{
		bdput(td->block_device);
		invalidate_bdev(td->block_device);
	}

	if(td->internal_devices_count)
		td_stop_worker_thread(td);

	//Write header and indices to all files
	for(i = 1; i <= td->internal_devices_count; ++i)
	{
		struct file *file = td->internal_devices[i-1].file;

		struct tdisk_header header = {
			.disk_index = i,
			.performance = td->internal_devices[i-1].performance,
			.current_max_sectors = td->max_sectors
		};
		gfp_t gfp = td->internal_devices[i-1].old_gfp_mask;

		//Write current performance and index values to file
		td_write_header(&td->internal_devices[i-1], &header);
		td_write_all_indices(td, &td->internal_devices[i-1]);

		if(file)
		{
			mapping_set_gfp_mask(file->f_mapping, gfp);

			fput(file);
		}
	}

	printk(KERN_DEBUG "tDisk: disk %d removed\n", td->number);

	idr_remove(&td_index_idr, td->number);
	blk_cleanup_queue(td->queue);
	del_gendisk(td->kernel_disk);
	blk_mq_free_tag_set(&td->tag_set);
	put_disk(td->kernel_disk);
	vfree(td->sorted_sectors);
	vfree(td->indices);
	kfree(td);

	return 0;
}

/**
  * This function is used to find the next free minor
  * number in the idr
 **/
static int tdisk_find_free_device_callback(int id, void *ptr, void *data)
{
	struct tdisk *td = ptr;
	struct tdisk **t = data;

	if(td->state == state_unbound)
	{
		*t = td;
		return 1;
	}
	return 0;
}

/**
  * This function returns the tDisk for the given
  * minor number. If the given minor number is < 0,
  * the minor number of an unbound tDisk is returned
  * or -ENODEV if there is no unbound tDisk
 **/
int tdisk_lookup(struct tdisk **t, int i)
{
	struct tdisk *td;
	int ret = -ENODEV;

	if(i < 0)
	{
		if(idr_for_each(&td_index_idr, &tdisk_find_free_device_callback, &td))
		{
			*t = td;
			ret = td->number;
		}
	}
	else
	{
		//lookup and return a specific i
		td = idr_find(&td_index_idr, i);
		if(td)
		{
			*t = td;
			ret = td->number;
		}
	}

	return ret;
}

/**
  * This funtction is the main entry point of
  * the driver. It registers the major number
  * and the control "td-control" device
 **/
static int __init tdisk_init(void)
{
	int err;

	err = register_tdisk_control();
	if(err < 0)goto error;

#ifdef USE_NETLINK
	err = nltd_register();
	if(err < 0)goto error_tdisk_control;
#else
#pragma message "Netlink is disabled"
#endif //USE_NETLINK

	err = -EBUSY;
	TD_MAJOR = register_blkdev((unsigned)TD_MAJOR, "td");
	if(TD_MAJOR <= 0)goto error_nltd;

	printk(KERN_INFO "tDisk: driver loaded\n");
	return 0;

 error_nltd:
#ifdef USE_NETLINK
	nltd_unregister();
 error_tdisk_control:
#else
#pragma message "Netlink is disabled"
#endif //USE_NETLINK
	unregister_tdisk_control();
 error:
	return err;
}

/**
  * This callback function is called for each tDisk
  * at driver exit time to clean up all remaining devices.
 **/
static int tdisk_exit_callback(int id, void *ptr, void *data)
{
	return tdisk_remove((struct tdisk*)ptr);
}

/**
  * This is the exit function for the tDisk driver. It
  * cleans up all remaining tDisks, unregisters the major
  * number and unregisters the control device "td-control"
 **/
static void __exit tdisk_exit(void)
{
	idr_for_each(&td_index_idr, &tdisk_exit_callback, NULL);
	idr_destroy(&td_index_idr);

	unregister_blkdev((unsigned)TD_MAJOR, "td");

#ifdef USE_NETLINK
	nltd_unregister();
#else
#pragma message "Netlink is disabled"
#endif //USE_NETLINK

	unregister_tdisk_control();
}

module_init(tdisk_init);
module_exit(tdisk_exit);

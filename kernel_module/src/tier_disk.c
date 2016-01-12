/**
  *
  * tDisk Driver
  * @author Thomas Sparber (2015)
  *
 **/

#include <linux/types.h>
#include <linux/vmalloc.h>
#include <linux/string.h>
#include <linux/errno.h>
#include <linux/fs.h>
#include <linux/bio.h>
#include <linux/atomic.h>
#include <linux/blkdev.h>

#include "tier_disk.h"
#include "helpers.h"
#include "mbds/deque.h"

void tdisk_free_internal_device(struct td_internal_device *internal_device);
int get_disk_and_sector(struct tdisk *td_dev, sector_t s, u8 *disk_out, sector_t *sector_out);

/*struct td_device* tdisk_allocate(sector_t sectors, unsigned int sector_size, unsigned int amount_disks)
{
	//Allocate memory
	struct td_device *td_dev = vmalloc(sizeof(struct td_device));
	unsigned int index_size_byte = sectors * sizeof(struct sector_index);	//Calculate index size

	if(td_dev == NULL)
	{
		return NULL;
	}

	if(amount_disks > 256)
	{
		printk(KERN_ERR "tDisk: Invalid number of disks: %d\n", amount_disks);
		vfree(td_dev);
		return NULL;
	}

	//Set disk values
	td_dev->index_size = index_size_byte/sector_size + ((index_size_byte%sector_size == 0) ? 0 : 1);
    td_dev->sectors = sectors - td_dev->index_size;
    td_dev->sector_size = sector_size;
	td_dev->amount_disks = amount_disks;
	printk(KERN_INFO "tDisk: Disk index size: %d sectors (%d bytes)\n", td_dev->index_size, index_size_byte);

	//Check disk size
	if(sectors <= td_dev->index_size)
	{
		printk(KERN_ERR "tDisk: Disk is too small. It can't hold index: %llu sectors\n", sectors);
		vfree(td_dev);
		return NULL;
	}

	//Set up tDisk
	td_dev->data = vmalloc(td_dev->sectors * td_dev->sector_size);
    if(td_dev->data == NULL)
	{
		vfree(td_dev);
        return NULL;
	}

	//Set up memory index
	td_dev->indices = vmalloc(td_dev->index_size*td_dev->sctor_size);
    if(td_dev->indices == NULL)
	{
		vfree(td_dev->data);
		vfree(td_dev);
        return NULL;
	}
	memset(td_dev->indices, 0, td_dev->index_size*td_dev->sctor_size);

	//Init lock which is used to search for free sectors in
	//the internal disks
	spin_lock_init(&td_dev->free_sectors_lock);

	//Set up internal disks
	td_dev->internal_devices_count = 0;
	td_dev->internal_devices = NULL;

	return td_dev;
}

void tdisk_cleanup(struct td_device *td_dev)
{
	//Clean up devices
	unsigned int i;
	for(i = 0; i < td_dev->internal_devices_count; ++i)
		tdisk_free_internal_device(&td_dev->internal_devices[i]);

	vfree(td_dev->internal_devices);
	vfree(td_dev->indices);
	vfree(td_dev->data);
	vfree(td_dev);
}*/

/*int tdisk_add_disk(struct td_device *td_dev, const char *path, unsigned int speed, sector_t capacity_sectors)
{
	sector_t sector;
	struct td_internal_device *new_devices = NULL;
	struct td_internal_device *new_device = NULL;

	if(td_dev->internal_devices_count == 254)
	{
		printk(KERN_ERR "tDisk: Limit (255) of devices reached.\n");
		return -ENODEV;
	}

	new_devices = vmalloc(sizeof(struct td_internal_device) * (td_dev->internal_devices_count+1));
	if(new_devices == NULL)
	{
		printk(KERN_ERR "tDisk: No memory to add disk device. Leaving disk unchanged.\n");
		return -ENOMEM;
	}

	memcpy(new_devices, td_dev->internal_devices, sizeof(struct td_internal_device) * td_dev->internal_devices_count);
	new_device = &new_devices[td_dev->internal_devices_count];

	//Exclusively open kernel block device
	new_device->bdev = blkdev_get_by_path(path, FMODE_READ|FMODE_WRITE|FMODE_EXCL, new_device);
	if(IS_ERR(new_device->bdev))
	{
		long error = PTR_ERR(new_device->bdev);
		printk(KERN_ERR "tDisk: Error opening device \"%s\": %ld.\n", path, error);
		vfree(new_devices);
		return error;
	}

	//Add free sectors ll
	new_device->free_sectors = MBdeque_create();
	for(sector = 0; sector < capacity_sectors; ++sector)
	{
		sector_t *temp = vmalloc(sizeof(sector_t));
		(*temp) = sector;
		MBdeque_push_back(new_device->free_sectors, temp);
	}

	//Set device values
	new_device->speed = speed;
	new_device->capacity_sectors = capacity_sectors;
	new_device->path = vmalloc(strlen(path) + 1);
	strncpy(new_device->path, path, strlen(path));
	new_device->path[strlen(path)] = '\0';

	//Replace old devices
	vfree(td_dev->internal_devices);
	td_dev->internal_devices = new_devices;
	++td_dev->internal_devices_count;

	printk(KERN_INFO "tDisk: Disk \"%s\" added to device. (Capacity: %llu sectors, Speed: %u)\n", path, capacity_sectors, speed);

	return 0;
}*/

/*void tdisk_free_internal_device(struct td_internal_device *internal_device)
{
	//Free free-sector-deque
	unsigned int i;
	for(i = 0; i < internal_device->free_sectors->count; ++i)
		vfree((sector_t*)MBdeque_get_at(internal_device->free_sectors, i));
	MBdeque_delete(internal_device->free_sectors);

	blkdev_put(internal_device->bdev, FMODE_READ|FMODE_WRITE|FMODE_EXCL);

	vfree(internal_device->path);
}*/

void insert_and_merge_request(MBdeque *requests, struct mapped_sector_index *index)
{
	unsigned int i = 0;
	int disk_found = 0;
	MBdeque *current_requests = NULL;
	struct mapped_sector_index *last_index = NULL;

	//printk(KERN_DEBUG "tDisk: Physical disk request of sector %llu on disk %u (%llu)\n", current_sector, index->disk, index->sector);

	//Trying to merge requests
	for(i = 0; i < requests->count; ++i)
	{
		current_requests = (MBdeque*)MBdeque_get_at(requests, i);

		if(current_requests->count <= 0)
		{
			printk(KERN_ERR "tDisk: driver bug detected: current_requests->count <= 0\n");
			continue;
		}

		last_index = MBdeque_get_at(current_requests, current_requests->count-1);

		//current_requests is not empty. Checking disk in first element
		if(last_index->physical_sector.disk == index->physical_sector.disk)
		{
			//Checking if blocks are contiguous
			if(last_index->logical_sector +1 == index->logical_sector)
			{
				//Request for same disk --> merging
				MBdeque_push_back(current_requests, index);
				disk_found = 1;
				break;
			}
			else
			{
				printk(KERN_DEBUG "tDisk: Non contiguous disk request for disk %u. Creating new\n", index->physical_sector.disk);
			}
		}
	}

	if(!disk_found)
	{
		//Create new deque
		current_requests = MBdeque_create();

		//Add index to new deque
		MBdeque_push_back(current_requests, index);

		//Adding new deque to request list
		MBdeque_push_back(requests, current_requests);
	}
}

int perform_index_operation(struct tdisk *td_dev, int direction, struct mapped_sector_index *index)
{
	loff_t position = index->logical_sector * sizeof(struct sector_index);
	unsigned int length = sizeof(struct sector_index);

	//Read/Write index operation
	if(direction == READ)
		memcpy(index, td_dev->indices+position, length);
	else
		memcpy(td_dev->indices+position, index, length);

	return 0;
}

int create_requests(struct tdisk *td_dev, int direction, loff_t offset, sector_t first, sector_t last, MBdeque *requests)
{
	int ret = 0;
	sector_t current_sector;

	for(current_sector = first; current_sector <= last; ++current_sector)
	{
		if(current_sector >= td_dev->index_size)
		{
			int internal_ret;
			struct mapped_sector_index *index = vmalloc(sizeof(struct mapped_sector_index));
			index->offset = offset;
			index->logical_sector = current_sector;

			//printk(KERN_DEBUG "tDisk: Requested data is on disk (%llu)\n", current_sector);

			//Retrieve index
			internal_ret = perform_index_operation(td_dev, READ, index);
			if(internal_ret != 0)ret = internal_ret;

			if(direction == READ && index->physical_sector.disk == 0)
			{
				//Reading sectors which are not yet used is not possible
				//printk(KERN_DEBUG "tDisk: Sector %llu not yet used. Not reading...\n", current_sector);

				//Free index since no disk request is needed
				vfree(index);
			}
			else
			{
				insert_and_merge_request(requests, index);
			}

			//Offset only needed the first time
			offset = 0;
		}
	}

	return ret;
}

u8 find_suitable_device(struct tdisk *td_dev, unsigned int sectors_needed)
{
	//u8 disk_index = 0;

	//Iterate over all disks and find suitable disk
	//with enough free space to store all blocks
	/*for(disk_index = 0; disk_index < td_dev->internal_devices_count; ++disk_index)
	{
		if(td_dev->internal_devices[disk_index].free_sectors->count >= sectors_needed)
		{
			printk(KERN_DEBUG "tDisk: Suitable disk found: %u. Saving %u unused sectors\n", disk_index, sectors_needed);

			return disk_index + 1;
		}
	} TODO*/
return 1;

	return 0;
}

void split_requests(struct tdisk *td_dev, MBdeque *requests, MBdeque *current_requests)
{
	/*u8 disk_index = 0;

	//Split data across all disks
	for(disk_index = 0; disk_index < td_dev->internal_devices_count && current_requests->count > 0; ++disk_index)
	{
		struct MBdeque *free_sectors = td_dev->internal_devices[disk_index].free_sectors;

		if(free_sectors->count > 0)
		{
			//Create new request
			struct MBdeque *new_request = MBdeque_create();

			while(current_requests->count > 0 && free_sectors->count > 0)
			{
				sector_t *free_sector = MBdeque_pop_front(free_sectors);
				struct mapped_sector_index *index = MBdeque_pop_front(current_requests);

				//Assign physical disk and sector
				index->physical_sector.disk = disk_index + 1;	//+1 because 0 means unused
				index->physical_sector.sector = (*free_sector);
				MBdeque_push_back(new_request, index);

				vfree(free_sector);
			}

			MBdeque_push_back(requests, new_request);
		}
	} TODO*/
}

int distribute_unused_sectors(struct tdisk *td_dev, MBdeque *requests)
{
	int ret = 0;
	unsigned int i = 0;
	MBdeque *current_requests = NULL;
	struct mapped_sector_index *index;

	for(i = 0; i < requests->count; ++i)
	{
		current_requests = (MBdeque*)MBdeque_get_at(requests, i);

		if(current_requests == NULL)
		{
			printk(KERN_ERR "tDisk: Current requests is NULL!\n");
			continue;
		}

		if(current_requests->count <= 0)continue;	//Error is printed later, so just skipping

		//Assign unused blocks to device
		index = MBdeque_get_at(current_requests, 0);
		if(index->physical_sector.disk == 0)		//disk index 0 means unused
		{
			u8 disk_index = 0;

			//Lock the spinlock to avoid data races of
			//the free_sectors deques
			spin_lock(&td_dev->free_sectors_lock);	//TODO try to use less code between the lock

			disk_index = find_suitable_device(td_dev, current_requests->count);

			if(disk_index != 0)
			{
				unsigned int j = 0;
				MBdeque *free_sectors = td_dev->/*internal_devices[disk_index - 1].*/free_sectors;	//TODO

				for(j = 0; j < current_requests->count; ++j)
				{
					sector_t *free_sector = MBdeque_pop_front(free_sectors);
					if(j != 0)index = MBdeque_get_at(current_requests, j);	//index at j=0 is already retrieved

					//Assign physical disk and sector
					index->physical_sector.disk = disk_index;	//+1 because 0 means unused
					index->physical_sector.sector = (*free_sector);

					vfree(free_sector);
				}
			}
			else
			{
				printk(KERN_DEBUG "tDisk: No physical disk present to store entire data. Splitting.\n");

				//Distribute request across free devices
				split_requests(td_dev, requests, current_requests);

				if(current_requests->count > 0)
				{
					printk(KERN_ERR "tDisk: No space left on device!\n");
					ret = 1;
				}

				//Remove old requests
				MBdeque_set_at(requests, i, NULL);
				MBdeque_delete(current_requests);
			}

			//Release lock
			spin_unlock(&td_dev->free_sectors_lock);
		}
	}

	return ret;
}

int make_disk_requests(struct tdisk *td_dev, MBdeque *requests, int direction, sector_t first, sector_t last, u8 *buffer, unsigned int length)
{
	int ret = 0;
	unsigned int i = 0;
	unsigned int j = 0;
	MBdeque *current_requests = NULL;
	struct mapped_sector_index *index;

	//Create struct bio's
	for(i = 0; i < requests->count; ++i)
	{
		struct bio *bio_req = NULL;
		u8 physical_disk;
		sector_t first_sector;

		current_requests = MBdeque_get_at(requests, i);
		if(current_requests == NULL)
		{
			//Request was splitted TODO
			continue;
		}

		//Check for empty request list
		if(current_requests->count <= 0)
		{
			printk(KERN_ERR "tDisk: Skipping empty request list\n");

			MBdeque_delete(current_requests);

			continue;
		}

		//Check if disk is full
		index = MBdeque_get_at(current_requests, 0);
		if(index->physical_sector.disk <= 0)
		{
			printk(KERN_ERR "tDisk: disk full! More space was allocated than disks attached!\n");

			//Free resources
			for(j = 0; j < current_requests->count; ++j)
			{
				index = MBdeque_get_at(current_requests, j);
				vfree(index);
			}
			MBdeque_delete(current_requests);

			continue;
		}
		physical_disk = index->physical_sector.disk - 1;	//-1 because 0 means unused
		first_sector = index->physical_sector.sector;	//-1 because 0 means unused

		//Allocate struct bio with enough struct bio_vec's to store the requests
		bio_req = bio_kmalloc(__GFP_WAIT, current_requests->count);	//TODO: __GFP_WAIT --> deadlock?
		if(bio_req == NULL)
		{
			printk(KERN_ERR "tDisk: can't allocate struct bio\n");

			//Free resources
			for(j = 0; j < current_requests->count; ++j)
			{
				index = MBdeque_get_at(current_requests, j);
				vfree(index);
			}
			MBdeque_delete(current_requests);

			//Callback TODO

			continue;
		}

		//Set block device and first sector
		//bio_req->bi_bdev = td_dev->internal_devices[physical_disk].bdev;
		bio_req->bi_iter.bi_sector = first_sector * (td_dev->blocksize / 512);
		bio_req->bi_rw = direction;

		//Process requests
		printk(KERN_DEBUG "tDisk: %u requests for physical disk (%u bytes)\n", current_requests->count, current_requests->count * td_dev->blocksize);
		for(j = 0; j < current_requests->count; ++j)
		{
			index = MBdeque_get_at(current_requests, j);

			if(!bio_add_page(bio_req, virt_to_page(buffer), td_dev->blocksize, (index->logical_sector-first) * td_dev->blocksize))
			{
				printk(KERN_ERR "tDisk: bio_add_page failed!\n");
			}

			if(direction == WRITE)
			{
				//TODO: create cache to store data until it's physically saved

				//Save index
				int internal_ret = perform_index_operation(td_dev, WRITE, index);
				if(internal_ret != 0)ret = internal_ret;
			}

			vfree(index);
		}

		//Actually make request
		printk(KERN_DEBUG "tDisk: Physical disk request: Physical-Start: %llu, Physical-End: %llu, Actual length (%u bytes) Request: %u/%llu\n",
													bio_req->bi_iter.bi_sector,
													bio_end_sector(bio_req),
													length,
													requests->count, last-first+1);

		//generic_make_request(bio_req);
		//tdisk_worker_thread_add_task(bio_req);
		//internal_callback(bio_req, 0);
		bio_put(bio_req);

		MBdeque_delete(current_requests);
	}

	return ret;
}

int tdisk_operation_internal(struct tdisk *td_dev, MBdeque *requests, int direction, loff_t position, unsigned int length)
{
	int ret = 0;
	int internal_ret = 0;
	loff_t offset;
	sector_t first = position + td_dev->index_size*td_dev->blocksize;
	sector_t last = first + length;

	//Deque to store requests to be made
	//MBdeque *requests = MBdeque_create();

	//Calculate first and last sector
	offset = __div64_32(&first, td_dev->blocksize);
	if(__div64_32(&last, td_dev->blocksize) == 0)
		--last;

	//printk(KERN_DEBUG "tDisk: Request: first=%llu last=%llu incl.\n", first, last);

	//Create requests queue for disk operations
	internal_ret = create_requests(td_dev, direction, offset, first, last, requests);
	if(internal_ret != 0)ret = internal_ret;

	if(requests->count > 0)
	{
		//Handle and distribute unused sectors
		internal_ret = distribute_unused_sectors(td_dev, requests);
		if(internal_ret != 0)ret = internal_ret;

		if(direction == READ)
			printk(KERN_DEBUG "tDisk: Need to make read requests for %u different disks\n", requests->count);
		else
			printk(KERN_DEBUG "tDisk: Need to make write requests for %u different disks\n", requests->count);

		//internal_ret = make_disk_requests(td_dev, requests, direction, first, last, buffer, length);
		//if(internal_ret != 0)ret = internal_ret;
	}

	//MBdeque_delete(requests);

	return ret;
}

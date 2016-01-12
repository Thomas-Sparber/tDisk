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

#include "tier_disk.h"
#include "helpers.h"
#include "mbds/deque.h"

void tdisk_free_internal_device(struct td_internal_device *internal_device);
int get_disk_and_sector(struct td_device *td_dev, sector_t s, u8 *disk_out, sector_t *sector_out);
int tdisk_operation_internal(struct td_device *td_dev, int direction, sector_t start, u8 *buffer, unsigned int length, void(*callback)(void*,unsigned int), void *callback_data);

/**
  * A index represents the physical location of a logical sector
 **/
struct sector_index
{
	//The disk where the logical sector is stored
	u8 disk;

	//The physical sector on the disk where the logica sector is stored
	sector_t sector;
}; //end struct sector_index;

/**
  * A mapped_sector_index represents the mapping of a logical sector
  * to a physical (disk & sector) sector
 **/
struct mapped_sector_index
{
	sector_t logical_sector;
	struct sector_index physical_sector;
}; //end struct mapped sector index

struct td_device* tdisk_allocate(sector_t sectors, unsigned int sector_size, unsigned int amount_disks)
{
	//Allocate memory
	struct td_device *td_dev = vmalloc(sizeof(struct td_device));

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
	td_dev->index_size_byte = sectors * sizeof(struct sector_index);	//Calculate index size
	td_dev->index_size = td_dev->index_size_byte/sector_size + ((td_dev->index_size_byte%sector_size == 0) ? 0 : 1);
    td_dev->sectors = sectors - td_dev->index_size;
    td_dev->sector_size = sector_size;
	td_dev->amount_disks = amount_disks;
	printk(KERN_INFO "tDisk: Disk index size: %d sectors (%d bytes)\n", td_dev->index_size, td_dev->index_size_byte);

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
	td_dev->indices = vmalloc(td_dev->index_size_byte);
    if(td_dev->indices == NULL)
	{
		vfree(td_dev->data);
		vfree(td_dev);
        return NULL;
	}
	memset(td_dev->indices, 0, td_dev->index_size_byte);

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
}

int tdisk_add_disk(struct td_device *td_dev, const char *path, unsigned int speed, sector_t capacity_sectors)
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
}

void tdisk_free_internal_device(struct td_internal_device *internal_device)
{
	//Free free-sector-deque
	unsigned int i;
	for(i = 0; i < internal_device->free_sectors->count; ++i)
		vfree((sector_t*)MBdeque_get_at(internal_device->free_sectors, i));
	MBdeque_delete(internal_device->free_sectors);

	blkdev_put(internal_device->bdev, FMODE_READ|FMODE_WRITE|FMODE_EXCL);

	vfree(internal_device->path);
}

int tdisk_operation_internal(struct td_device *td_dev, int direction, sector_t position, u8 *buffer, unsigned int length, void(*callback)(void*,unsigned int), void *callback_data)
{
	int ret = 0;
	sector_t current_sector;
	sector_t first = position;
	sector_t last = position + length;

	//Deque to store requests to be made
	MBdeque *requests = MBdeque_create();
	unsigned int i;
	unsigned int j;

	//Calculate first and last sector
	__div64_32(&first, td_dev->sector_size);
	if(__div64_32(&last, td_dev->sector_size) == 0)
		--last;

	//printk(KERN_DEBUG "tDisk: Request: first=%llu last=%llu incl.\n", first, last);

	for(current_sector = first; current_sector <= last; ++current_sector)
	{
		const unsigned int current_length = ((length > td_dev->sector_size) ? td_dev->sector_size : length);

		//Error handling
		if(current_sector >= td_dev->sectors+td_dev->index_size)
		{
			printk(KERN_ERR "tDisk: Someone wants to make a request of sector %llu/%llu! Skipping...\n", current_sector, td_dev->sectors+td_dev->index_size);
			ret = 1;
			continue;
		}

		if(current_sector < td_dev->index_size)
		{
			//printk(KERN_DEBUG "tDisk: Requested data is index of index (%llu)\n", current_sector);
			//printk(KERN_DEBUG "tDisk: Index request: %llu - %llu (size=%llu)\n", position, position+current_length, td_dev->index_size_byte);

			//Error handling
			if(position+current_length > td_dev->index_size_byte)
			{
				printk(KERN_ERR "tDisk: Internal bug detected: reading index at byte %llu/%u! Skipping...\n", position+current_length, td_dev->index_size_byte);
				ret = 1;
				continue;
			}

			//Read/Write index operation
			if(direction == READ)
				memcpy(buffer, td_dev->indices+position, current_length);
			else
				memcpy(td_dev->indices+position, buffer, current_length);

			if(callback != NULL)
				printk(KERN_NOTICE "tDisk: Index location requested but callback given. Strange...\n");
		}
		else
		{
			int internal_ret;
			struct mapped_sector_index *index = vmalloc(sizeof(struct mapped_sector_index));
			index->logical_sector = current_sector;

			//printk(KERN_DEBUG "tDisk: Requested data is on disk (%llu)\n", current_sector);

			//Retrieve index
			internal_ret = tdisk_operation_internal(td_dev, READ, current_sector * sizeof(struct sector_index), (u8*)&index->physical_sector, sizeof(struct sector_index), NULL, NULL);	//MUST return immediately --> index query
			if(internal_ret != 0)
				ret = internal_ret;

			if(direction == READ && index->physical_sector.disk == 0)
			{
				//Reading sectors ehich are not yet used is not possible
				//printk(KERN_DEBUG "tDisk: Sector %llu not yet used. Not reading...\n", current_sector);

				//Free index since no disk request is needed
				vfree(index);
			}
			else
			{
				int disk_found = 0;
				MBdeque *current_requests = NULL;

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

					//current_requests is not empty. Checking disk in first element
					if(((struct mapped_sector_index*)MBdeque_get_at(current_requests, 0))->physical_sector.disk == index->physical_sector.disk)
					{
						//Checking if blocks are contiguous
						if(((struct mapped_sector_index*)MBdeque_get_at(current_requests, current_requests->count-1))->logical_sector +1 == index->logical_sector)
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
		}

		position += current_length;
		length -= current_length;
	}

	if(requests->count > 0)
	{
		MBdeque *current_requests = NULL;
		struct mapped_sector_index *index;

		if(direction == READ)
			printk(KERN_DEBUG "tDisk: Need to make read requests for %u different disks\n", requests->count);
		else
			printk(KERN_DEBUG "tDisk: Need to make write requests for %u different disks\n", requests->count);

		if(callback == NULL)
		{
			printk(KERN_NOTICE "tDisk: requests need to be done but no callback is set.\n");
			ret = 1;
		}

		//Handle and distribute unused sectors
		for(i = 0; i < requests->count; ++i)
		{
			current_requests = (MBdeque*)MBdeque_get_at(requests, i);
			if(current_requests->count <= 0)continue;	//Error is printed later, so just skipping

			//Assign unused blocks to device
			index = MBdeque_get_at(current_requests, 0);
			if(index->physical_sector.disk == 0)		//disk index 0 means unused
			{
				int disk_index;

				//Iterate over all disks and find suitable disk
				//with enough free space to store all blocks
				for(disk_index = 0; disk_index < td_dev->internal_devices_count; ++disk_index)
				{
					struct MBdeque *free_sectors = td_dev->internal_devices[disk_index].free_sectors;

					//TODO lock "free_sectors" deque
					if(free_sectors->count >= current_requests->count)
					{
						printk(KERN_DEBUG "tDisk: Suitable disk found: %u. Saving %u unused sectors\n", disk_index, current_requests->count);
						for(j = 0; j < current_requests->count; ++j)
						{
							sector_t *free_sector = MBdeque_pop_front(free_sectors);
							if(j != 0)index = MBdeque_get_at(current_requests, j);	//index at j=0 is already retrieved

							//Assign physical disk and sector
							index->physical_sector.disk = disk_index + 1;	//+1 because 0 means unused
							index->physical_sector.sector = (*free_sector);

							vfree(free_sector);
						}
					}
					//TODO unlock "free_sectors" deque

					//Break if suitable disk was found
					if(index->physical_sector.disk == 0)break;
				}

				if(index->physical_sector.disk == 0)
				{
					printk(KERN_DEBUG "tDisk: No physical disk present to store entire data. Splitting.\n");

					//Remove request from list but add new requests
					MBdeque_set_at(requests, i, NULL);

					//Split data across all disks
					for(disk_index = 0; disk_index < td_dev->internal_devices_count && current_requests->count > 0; ++disk_index)
					{
						struct MBdeque *free_sectors = td_dev->internal_devices[disk_index].free_sectors;

						//TODO lock "free_sectors" deque
						if(free_sectors->count > 0)
						{
							//Create new request
							struct MBdeque *new_request = MBdeque_create();

							while(current_requests->count > 0 && free_sectors->count > 0)
							{
								sector_t *free_sector = MBdeque_pop_front(free_sectors);
								index = MBdeque_pop_front(current_requests);

								//Assign physical disk and sector
								index->physical_sector.disk = disk_index + 1;	//+1 because 0 means unused
								index->physical_sector.sector = (*free_sector);
								MBdeque_push_back(new_request, index);

								vfree(free_sector);
							}

						MBdeque_push_back(requests, new_request);
						}
						//TODO unlock "free_sectors" deque
					}

					MBdeque_delete(current_requests);
				}
			}
		}

		//Create struct bio's
		for(i = 0; i < requests->count; ++i)
		{
			struct bio *bio_req = NULL;
			u8 physical_disk;
			sector_t first_sector;

			current_requests = (MBdeque*)MBdeque_get_at(requests, i);
			if(current_requests == NULL)continue;	//Request was splitted

			//Check for empty request list
			if(current_requests->count <= 0)
			{
				printk(KERN_ERR "tDisk: Skipping empty request list\n");

				//Free resources
				for(j = 0; j < current_requests->count; ++j)
				{
					index = MBdeque_get_at(current_requests, j);
					vfree(index);
				}
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
			physical_disk = index->physical_sector.sector;	//-1 because 0 means unused

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

				continue;
			}

			//Set block device and first sector
			bio_req->bi_bdev = td_dev->internal_devices[physical_disk].bdev;
			//bio_req->bi_bsector = first_sector;

			/*bio_req->bi_sector = 0;	//The first (512 byte) sector to be transferred for this bio
			bio_req->bi_size = 0;	//Size of data to be transferred in bytes. --> bio_sectors(bio) gives size in sectors
			bio_req->bi_flags = WRITE | READ;	//Flags describing BIO --> bio_data_dir(bio) tells if read or write
			bio_req->bio_phys_segments = 0;	//Number of physical segments in bio
			bio_req->bio_hw_segments = 0;	//Number of segments seen by the hardware after DMA mapping is done
			bio_req->bi_io_vec = NULL;		//Most important --> array of struct bio_vec. Directly working is discouraged because of possible future changes
			bio_req->bi_vcnt = 0;			//Number of bio_vec's
			bio_req->bi_end_io = NULL;		//Function which is called on completion
			bio_req->bi_private = NULL;		//Private data of the caller*/

			//Needed:
			// - bi_io_vec
			// - bi_dev
			// - bi_sector

			printk(KERN_DEBUG "tDisk: vcnt: %u\n", bio_req->bi_vcnt);
			for(j = 0; j < current_requests->count; ++j)
			{
				index = MBdeque_get_at(current_requests, j);

				printk(KERN_DEBUG "tDisk: bio_add_page(bio_req, %p, %u, %llu) %llu\n", virt_to_page(buffer), td_dev->sector_size, (index->logical_sector-first) * td_dev->sector_size, index->logical_sector);
				if(!bio_add_page(bio_req, virt_to_page(buffer), td_dev->sector_size, (index->logical_sector-first) * td_dev->sector_size))
				{
					printk(KERN_ERR "tDisk: bio_add_page failed!\n");
				}

				vfree(index);
			}

			//generic_make_request
			bio_put(bio_req);	//TODO move in callback

			MBdeque_delete(current_requests);
		}

		//TEMP
		if(callback != NULL)
			callback(callback_data, length);
	}
	else
	{
		//Call callback since the is nothing to do
		if(callback != NULL)
			callback(callback_data, length);
	}

	MBdeque_delete(requests);

	return ret;
}

int tdisk_operation(struct td_device *td_dev, int direction, sector_t start, u8 *buffer, unsigned int length, void (*callback)(void*,unsigned int), void *callback_data)
{
	//Add index offset
	start += td_dev->index_size;

	//Convert sectors to bytes
	start *= td_dev->sector_size;
	length *= td_dev->sector_size;

	return tdisk_operation_internal(td_dev, direction, start, buffer, length, callback, callback_data);
}

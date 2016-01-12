/**
  *
  * tDisk Driver
  * @author Thomas Sparber (2015)
  *
 **/

#ifndef TIER_DISK_H
#define TIER_DISK_H

#include <linux/spinlock.h>

#include "tdisk.h"

struct MBdeque;

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
	loff_t offset;
	sector_t logical_sector;
	struct sector_index physical_sector;
}; //end struct mapped sector index

struct td_internal_device
{
	//Kernel members
	struct block_device *bdev;

	char *path;
	unsigned int speed;
	sector_t capacity_sectors;

	struct MBdeque *free_sectors;
}; //end struct td_internal_device

/*struct td_device
{
	//Kernel members
    spinlock_t lock;				//Exclusive access to queue
    struct request_queue *td_queue;	//Request queue
    struct gendisk *td_disk;		//Kernel's disk representation

	//tDisk members
	sector_t sectors;				//Size in sectors
    unsigned int sector_size;
	unsigned int index_size_byte;	//Size in byte of the index where the sectors are stored. Located at the beginning of the disk
	unsigned int index_size;		//Size in sectors of the index where the sectors are stored. Located at the beginning of the disk
	u8 amount_disks;				//The amount of different disks which are used
	u8 *data;						//Space where the data is stored
	u8 *indices;					//The indices of the index needs to be stored in memory

	//The actual disks
	u8 internal_devices_count;
	struct td_internal_device *internal_devices;
	spinlock_t free_sectors_lock;
}; //end struct td_device*/

struct tdisk* tdisk_allocate(sector_t sectors, unsigned int sector_size, unsigned int amount_disks);
void tdisk_cleanup(struct tdisk *td_dev);
int tdisk_add_disk(struct tdisk *td_dev, const char *path, unsigned int speed, sector_t capacity_sectors);
int tdisk_operation_internal(struct tdisk *td_dev, struct MBdeque *requests, int direction, loff_t position, unsigned int length);
int perform_index_operation(struct tdisk *td_dev, int direction, struct mapped_sector_index *index);

#endif	//TIER_DISK_H

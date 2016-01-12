/**
  *
  * tDisk Driver
  * @author Thomas Sparber (2015)
  *
 **/

#ifndef TIER_DISK_H
#define TIER_DISK_H

struct MBdeque;

struct td_internal_device
{
	//Kernel members
	struct block_device *bdev;

	char *path;
	unsigned int speed;
	sector_t capacity_sectors;

	struct MBdeque *free_sectors;
}; //end struct td_internal_device

struct td_device
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
}; //end struct td_device

struct td_device* tdisk_allocate(sector_t sectors, unsigned int sector_size, unsigned int amount_disks);
void tdisk_cleanup(struct td_device *td_dev);
int tdisk_add_disk(struct td_device *td_dev, const char *path, unsigned int speed, sector_t capacity_sectors);
int tdisk_operation(struct td_device *device, int direction, sector_t start, u8 *buffer, unsigned int length, void (*callback)(void*,unsigned int), void *callback_data);
//void tdisk_read(struct td_device *device, sector_t start, u8 *buffer, unsigned int length);

#endif	//TIER_DISK_H

/**
  *
  * tDisk backend
  * @author Thomas Sparber (2015-2016)
  *
 **/

#include <atomic>
#include <iostream>
#include <string.h>

#ifdef __linux__
#include <fcntl.h>
#include <linux/fs.h>
#include <parted/parted.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h> 
#endif //__linux__

#include <filesystem.hpp>
#include <backendexception.hpp>

using std::cout;
using std::endl;
using std::string;

using namespace td;

int getNextNumber()
{
	static std::atomic_int i(0);
	return i++;
}

const fs::device_type fs::device_type::invalid("invalid", getNextNumber());
const fs::device_type fs::device_type::blockdevice("blockdevice", getNextNumber());
const fs::device_type fs::device_type::blockdevice_part("blockdevice_part", getNextNumber());
const fs::device_type fs::device_type::file("file", getNextNumber());
const fs::device_type fs::device_type::file_part("file_part", getNextNumber());
const fs::device_type fs::device_type::raid1("raid1", getNextNumber());
const fs::device_type fs::device_type::raid5("raid5", getNextNumber());
const fs::device_type fs::device_type::raid6("raid6", getNextNumber());

#ifdef __linux__

fs::Device fs::getDevice(const string &name)
{
	Device device;

	struct stat info;
	if(stat(name.c_str(), &info) != 0)
		throw BackendException("Can't get device info for \"", name, "\": ", strerror(errno));

	device.name = name;
	device.path = name;

	if(S_ISREG(info.st_mode))
	{
		device.type = device_type::file;
		device.size = info.st_size;
	}
	else if(S_ISBLK(info.st_mode))
	{
		device.type = device_type::blockdevice;

		int fd = open(name.c_str(), O_RDONLY);
		if(fd == -1)throw BackendException("Can't get size of block device ", name, ": ", strerror(errno));
		uint64_t size;
		if(ioctl(fd, BLKGETSIZE64, &size) == -1)
			throw BackendException("Can't get size of block device ", name, ": ", strerror(errno));
		device.size = size;
	}
	else throw BackendException("Unknown device type ", name);

	return std::move(device);
}

void fs::getDevices(std::vector<Device> &out)
{
	ped_exception_fetch_all();
	PedDevice* dev = NULL;
	ped_device_probe_all();

	while((dev = ped_device_get_next (dev)))
	{
		if(!ped_device_open(dev))break;

		Device device;
		device.name = dev->model;
		device.path = dev->path;
		device.size = dev->sector_size * dev->length;
		out.push_back(std::move(device));

		PedDisk *disk = ped_disk_new(dev);
		if(!disk)continue;

		for(PedPartition *part = ped_disk_next_partition(disk, nullptr); part; part = ped_disk_next_partition (disk, part))
		{
			if(!ped_partition_is_active(part))continue;
			const char *path = ped_partition_get_path(part);
			const char *name = ped_partition_get_name(part);

			device = Device();
			if(name)device.name = name;
			if(path)device.path = path;
			device.size = part->geom.length * dev->sector_size;
			if(device.path != dev->path)out.push_back(device);
		}

		ped_disk_destroy(disk);

		/*{
			next = 0x80947f0,
			model = 0x8094868 "ATA VBOX HARDDISK",
			path = 0x8094758 "/dev/sda",
			type = PED_DEVICE_SCSI,
			sector_size = 512,
			phys_sector_size = 512,
			length = 33554432,
			open_count = 0,
			read_only = 0,
			external_mode = 0,
			dirty = 0,
			boot_dirty = 0,
			hw_geom = {
				cylinders = 2088,
				heads = 255,
				sectors = 63
			},
			bios_geom = {
				cylinders = 2088,
				heads = 255,
				sectors = 63
			},
			host = 3,
			did = 0,
			arch_specific = 0x80945c8
		}*/
	}

	//fdisk_unref_context(cxt);
}

#else

fs::Device fs::getDevice(const string &name)
{
	fs::Device device;

	device.name = name;
	device.path = name;

	if(rand() % 2)
		device.type = device_type::file;
	else
		device.type = device_type::blockdevice;

	device.size = rand() % 1000000;

	return std::move(device);
}

#endif //__linux__

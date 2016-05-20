/**
  *
  * tDisk backend
  * @author Thomas Sparber (2015-2016)
  *
 **/

#include <atomic>
#include <functional>
#include <iostream>
#include <list>
#include <string.h>
#include <memory>

#ifdef __linux__
#include <fcntl.h>
#include <linux/fs.h>
#include <parted/parted.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h> 
#endif //__linux__

#include <backendexception.hpp>
#include <filesystem.hpp>
#include <inodescan.hpp>
#include <logger.hpp>
#include <performance.hpp>

using std::cout;
using std::endl;
using std::function;
using std::list;
using std::make_pair;
using std::max;
using std::min;
using std::pair;
using std::string;
using std::unique_ptr;
using std::vector;

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

void fs::getDevices(vector<Device> &out)
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

void fs::getDevices(vector<Device> &out)
{
	Device device;
	device.name = "USB Stick";
	device.path = "/dev/sda";
	device.size = rand() % 1000000;
	out.push_back(std::move(device));

	device.name = "USB Stick Partition 1";
	device.path = "/dev/sda1";
	device.size = rand() % 1000000;
	out.push_back(std::move(device));

	device.name = "SATA Festplatte";
	device.path = "/dev/sdb";
	device.size = rand() % 10000000;
	out.push_back(std::move(device));

	device.name = "SATA Festplatte Partition 1";
	device.path = "/dev/sdb1";
	device.size = rand() % 10000000;
	out.push_back(std::move(device));

	device.name = "SATA Festplatte Partition 2";
	device.path = "/dev/sdb2";
	device.size = rand() % 10000000;
	out.push_back(std::move(device));

	device.name = "SATA Festplatte Partition 3";
	device.path = "/dev/sdb3";
	device.size = rand() % 10000000;
	out.push_back(std::move(device));
}

#endif //__linux__

void fs::iterateFiles(const string &disk, bool filesOnly, function<bool(unsigned int, const string&, unsigned long long, const vector<unsigned long long>&)> callback)
{
	unique_ptr<InodeScan> scan(InodeScan::getInodeScan(disk));
	unique_ptr<Inode> inode(scan->getInitialInode());

	unsigned int blocksize = scan->getBlocksize();

	list<unique_ptr<Inode> > directories;

	unsigned long long inodeBlock;
	vector<unsigned long long> dataBlocks;
	while(scan->nextInode(inode.get()))
	{
		Inode *parent = nullptr;
		inodeBlock = inode->getInodeBlock();
		inode->getDataBlocks(dataBlocks);

		if(inode->isDirectory())
		{
			directories.emplace_front(inode->clone());
		}
		else
		{
			for(const auto &directory : directories)
			{
				if(directory->contains(inode.get()))
				{
					parent = directory.get();
					break;
				}
			}
		}

		if(!filesOnly || !inode->isDirectory())
		{
			bool cont = callback(blocksize, inode->getPath(parent), inodeBlock, dataBlocks);
			if(!cont)break;
		}
	}
}

vector<FileAssignment> fs::getFilesOnDisk(const string &disk, vector<pair<unsigned long long,unsigned long long> > p, bool calculatePercentage, bool filesOnly)
{
	LOG(LogLevel::debug, p.size()," positions to get files for");
	performance::start("sortSectorIndices");
	sort(p.begin(), p.end(), [](const pair<unsigned long long,unsigned long long> &a, const pair<unsigned long long,unsigned long long> &b)
	{
		return a.first < b.first;
	});

	vector<pair<unsigned long long,unsigned long long> > positions;
	for(const auto &pos : p)
	{
		if(!positions.empty() && pos.first == positions.back().second)
			positions.back().second = pos.second;
		else
			positions.push_back(pos);
	}
	performance::stop("sortSectorIndices");
	LOG(LogLevel::debug, positions.size()," positions to get files for after combining");

	vector<FileAssignment> result;

	performance::start("getFilesOnDisk");
	vector<pair<unsigned long long,unsigned long long> > helper;
	iterateFiles(disk, filesOnly, [&result,positions,calculatePercentage,&helper](unsigned int blocksize, const string &file, unsigned long long inodeBlock, const vector<unsigned long long> &dataBlocks)
	{
		//performance::start("oneFile");
		unsigned long long onDisk = 0;
		unsigned long long totalBytes = (unsigned long long)dataBlocks.size() * blocksize + blocksize;

		for(const auto &pos : positions)
		{
			if(inodeBlock*blocksize >= pos.first && (inodeBlock+1)*blocksize <= pos.second)
			{
				onDisk += blocksize;
				break;
			}
		}

		if(calculatePercentage || onDisk == 0)
		{
			helper.clear();
			for(const unsigned long long &block : dataBlocks)
			{
				if(!helper.empty() && block*blocksize == helper.back().second)
					helper.back().second = (block+1)*blocksize;
				else
					helper.push_back(make_pair(block*blocksize, (block+1)*blocksize));
			}

			for(const auto &pos : positions)
			{
				for(const auto &help : helper)
				{
					unsigned long long maxStart = max(pos.first, help.first);
					unsigned long long minEnd = min(pos.second, help.second);

					if(minEnd > maxStart)
						onDisk += (minEnd - maxStart);
				}
			}
		}

		if(onDisk != 0 || calculatePercentage)
		{
			long double percentage = calculatePercentage ? (long double)onDisk / totalBytes : 0;
			result.push_back(FileAssignment(file, (double)percentage));
		}

		//performance::stop("oneFile");
		return true;
	});
	performance::stop("getFilesOnDisk");

	return std::move(result);
}

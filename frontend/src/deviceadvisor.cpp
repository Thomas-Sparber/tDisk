#include <atomic>
#include <fcntl.h>
#include <iostream>
#include <linux/fs.h>
#include <list>
#include <queue>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h> 

#include <deviceadvisor.hpp>
#include <frontend.hpp>

using std::cout;
using std::endl;
using std::list;
using std::priority_queue;
using std::string;
using std::vector;

using namespace td;

int getNextNumber()
{
	static std::atomic_int i(0);
	return i++;
}

const advisor::device_type advisor::device_type::invalid("invalid", getNextNumber());
const advisor::device_type advisor::device_type::blockdevice("blockdevice", getNextNumber());
const advisor::device_type advisor::device_type::partition("partition", getNextNumber());
const advisor::device_type advisor::device_type::file("file", getNextNumber());
const advisor::device_type advisor::device_type::raid1("raid1", getNextNumber());
const advisor::device_type advisor::device_type::raid5("raid5", getNextNumber());
const advisor::device_type advisor::device_type::raid6("raid6", getNextNumber());

struct device_combination
{
	device_combination(const vector<advisor::device> *v_devices) :
		processed(),
		available(),
		score(),
		devices(v_devices)
	{
		for(std::size_t i = 0; i < devices->size(); ++i)
			available.push_back(i);
	}

	device_combination(const device_combination&) = default;

	device_combination& operator= (const device_combination&) = default;

	int getRecommendationSum() const
	{
		int sum = 0;
		for(std::size_t index : processed)
			sum += (*devices)[index].recommendation_score;
		return sum;
	}

	bool operator< (const device_combination &other) const
	{
		return score < other.score;
	}

	bool operator== (const device_combination &other) const
	{
		return processed == other.processed;
	}

	list<std::size_t> processed;
	list<std::size_t> available;
	int score;
	const vector<advisor::device> *devices;
}; //end struct device_combination

advisor::device getDevice(const string &name)
{
	advisor::device device;

	struct stat info;
	if(stat(name.c_str(), &info) != 0)
		throw FrontendException("Can't get device info for \"", name, "\": ", strerror(errno));

	device.name = name;

	if(S_ISREG(info.st_mode))
	{
		device.type = advisor::device_type::file;
		device.size = info.st_size;
	}
	else if(S_ISBLK(info.st_mode))
	{
		device.type = advisor::device_type::blockdevice;

		int fd = open(name.c_str(), O_RDONLY);
		if(fd == -1)throw FrontendException("Can't get size of block device ", name, ": ", strerror(errno));
		uint64_t size;
		if(ioctl(fd, BLKGETSIZE64, &size) == -1)
			throw FrontendException("Can't get size of block device ", name, ": ", strerror(errno));
		device.size = size;
	}
	else throw FrontendException("Unknown device type ", name);

	return std::move(device);
}

std::size_t addDevice(const advisor::device &d, vector<advisor::device> &devices)
{
	for(std::size_t i = 0; i < devices.size(); ++i)
	{
		if(devices[i] == d)return i;
	}
	devices.push_back(d);
	return devices.size() - 1;
}

vector<advisor::tdisk_advice> advisor::getTDiskAdvices(const vector<string> &files)
{
	vector<tdisk_advice> advices;
	vector<device> devices;

	for(const string &name : files)
	{
		devices.push_back(getDevice(name));
	}

	priority_queue<device_combination> combinations;
	combinations.emplace(&devices);

	while(!combinations.empty())
	{
		const device_combination c = combinations.top();
		combinations.pop();

		if(c.available.size() >= 4)
		{
			//Create Raid6
			for(std::size_t index : c.available)
			{
				//Not creating raid devices of raid devices
				if(devices[index].type == device_type::raid6)continue;
				if(devices[index].type == device_type::raid5)continue;
				if(devices[index].type == device_type::raid1)continue;

				device_combination new_c(c);
				new_c.processed.push_back(index);
				new_c.available.remove(index);

				device d;
				d.type = device_type::raid6;
				d.subdevices.push_back(devices[index]);
				index = addDevice(d, devices);

				new_c.available.push_back(index);
				combinations.push(new_c);
			}
		}

		if(c.available.size() >= 3)
		{
			//Create Raid5
			for(std::size_t index : c.available)
			{
				//Not creating raid devices of raid devices
				if(devices[index].type == device_type::raid6)continue;
				if(devices[index].type == device_type::raid5)continue;
				if(devices[index].type == device_type::raid1)continue;

				device_combination new_c(c);
				new_c.processed.push_back(index);
				new_c.available.remove(index);

				device d;
				d.type = device_type::raid5;
				d.subdevices.push_back(devices[index]);
				index = addDevice(d, devices);

				new_c.available.push_back(index);
				combinations.push(new_c);
			}
		}

		if(c.available.size() >= 2)
		{
			//Create Raid1
			for(std::size_t index : c.available)
			{
				//Not creating raid devices of raid devices
				if(devices[index].type == device_type::raid6)continue;
				if(devices[index].type == device_type::raid5)continue;
				if(devices[index].type == device_type::raid1)continue;

				device_combination new_c(c);
				new_c.processed.push_back(index);
				new_c.available.remove(index);

				device d;
				d.type = device_type::raid1;
				d.subdevices.push_back(devices[index]);
				index = addDevice(d, devices);

				new_c.available.push_back(index);
				combinations.push(new_c);
			}
		}

		//Finish raid devices
		for(std::size_t index : c.available)
		{
			if(devices[index].type != device_type::raid6 &&
				devices[index].type != device_type::raid5 &&
				devices[index].type != device_type::raid1)continue;

			for(std::size_t new_index : c.available)
			{
				//Not creating raid devices of raid devices
				if(devices[new_index].type == device_type::raid6)continue;
				if(devices[new_index].type == device_type::raid5)continue;
				if(devices[new_index].type == device_type::raid1)continue;

				device_combination new_c(c);
				new_c.processed.push_back(index);
				new_c.processed.push_back(new_index);
				new_c.available.remove(index);
				new_c.available.remove(new_index);

				device d = devices[index];
				d.subdevices.push_back(devices[new_index]);
				new_index = addDevice(d, devices);

				new_c.available.push_back(new_index);
				combinations.push(new_c);
			}
		}

		bool valid = true;
		tdisk_advice td;
		for(std::size_t index : c.available)
		{
			if(!devices[index].isValid())
			{
				valid = false;
				break;
			}

			td.devices.push_back(devices[index]);
		}

		if(valid)
		{
			if(std::find(advices.begin(), advices.end(), td) == advices.end())
			{
				cout<<createResultString(td, 0, "json")<<endl;
				advices.push_back(td);
			}
		}
	}

	return std::move(advices);
}

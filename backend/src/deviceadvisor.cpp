#include <iostream>
#include <list>
#include <set>
#include <string.h>

#include <deviceadvisor.hpp>
#include <backend.hpp>

using std::cout;
using std::endl;
using std::list;
using std::multiset;
using std::string;
using std::vector;

using namespace td;

struct device_combination
{
	device_combination(const vector<fs::Device> *v_devices) :
		available(),
		score(),
		redundancy_level(),
		wasted_space(),
		devices(v_devices)
	{
		for(std::size_t i = 0; i < devices->size(); ++i)
			available.push_back(i);
	}

	device_combination(const device_combination&) = default;

	device_combination& operator= (const device_combination&) = default;

	bool operator< (const device_combination &other) const
	{
		if(score == other.score)return wasted_space > other.wasted_space;
		return score < other.score;
	}

	bool operator== (const device_combination &other) const
	{
		return available == other.available;
	}

	void evaluateScore()
	{
		int splitted_count = 0;
		redundancy_level = 100000;
		wasted_space = 0;

		for(std::size_t index : available)
		{
			const fs::Device &device = (*devices)[index];

			uint64_t minSpace = uint64_t(-1);
			for(const fs::Device &d : device.subdevices)
			{
				minSpace = std::min(minSpace, d.size);
				if(d.type == fs::device_type::file_part)splitted_count++;
				if(d.type == fs::device_type::blockdevice_part)splitted_count++;
			}

			for(const fs::Device &d : device.subdevices)
				wasted_space += d.size - minSpace;

			if(device.type == fs::device_type::raid6)redundancy_level = std::min(redundancy_level, 2);
			else if(device.type == fs::device_type::raid5)redundancy_level = std::min(redundancy_level, 1);
			else if(device.type == fs::device_type::raid1)redundancy_level = std::min(redundancy_level, (int)device.subdevices.size()-1);
			else redundancy_level = 0;
		}

		int wastedInt = (int)(wasted_space/(1024*1024));

		score =
			-wastedInt						//Wasted space has negative impact
			+wastedInt/3*redundancy_level	//Redundancy level has positive impact
			-wastedInt/6*splitted_count;	//Amount of splitted disks has negative impact
	}

	list<std::size_t> available;
	int score;
	int redundancy_level;
	uint64_t wasted_space;
	const vector<fs::Device> *devices;
}; //end struct device_combination

std::size_t addDevice(const fs::Device &d, vector<fs::Device> &devices)
{
	for(std::size_t i = 0; i < devices.size(); ++i)
	{
		if(devices[i] == d)return i;
	}
	devices.push_back(d);
	return devices.size() - 1;
}

void createNewRaid(const device_combination &c, multiset<device_combination> &combinations, vector<fs::Device> &devices, fs::device_type raidType, unsigned int minDevices)
{
	if(c.available.size() >= minDevices)
	{
		for(std::size_t index : c.available)
		{
			//Not creating raid devices of raid devices
			if(devices[index].type == fs::device_type::raid6)continue;
			if(devices[index].type == fs::device_type::raid5)continue;
			if(devices[index].type == fs::device_type::raid1)continue;

			device_combination new_c(c);
			new_c.available.remove(index);

			fs::Device d;
			d.type = raidType;
			d.subdevices.push_back(devices[index]);
			index = addDevice(d, devices);

			new_c.available.push_back(index);
			combinations.insert(new_c);
		}
	}
}

vector<advisor::tdisk_advice> advisor::getTDiskAdvices(const vector<string> &files)
{
	vector<tdisk_advice> advices;
	vector<fs::Device> devices;

	for(const string &name : files)
	{
		devices.push_back(fs::getDevice(name));
	}

	multiset<device_combination> combinations;
	combinations.insert(device_combination(&devices));

	while(!combinations.empty())
	{
		const device_combination c = *combinations.begin();
		combinations.erase(combinations.begin());

		createNewRaid(c, combinations, devices, fs::device_type::raid6, 4);
		createNewRaid(c, combinations, devices, fs::device_type::raid5, 3);
		createNewRaid(c, combinations, devices, fs::device_type::raid1, 2);

		//Finish raid devices
		for(const std::size_t index : c.available)
		{
			if(devices[index].type != fs::device_type::raid6 &&
				devices[index].type != fs::device_type::raid5 &&
				devices[index].type != fs::device_type::raid1)continue;

			for(const std::size_t new_index : c.available)
			{
				//Not creating raid devices of raid devices
				if(devices[new_index].type == fs::device_type::raid6)continue;
				if(devices[new_index].type == fs::device_type::raid5)continue;
				if(devices[new_index].type == fs::device_type::raid1)continue;

				if(devices[index].containsDevice(devices[new_index].name))continue;

				device_combination new_c(c);
				new_c.available.remove(index);
				new_c.available.remove(new_index);

				fs::Device d = devices[index];
				d.subdevices.push_back(devices[new_index]);
				std::size_t new_device = addDevice(d, devices);

				new_c.available.push_back(new_device);
				new_c.evaluateScore();
				combinations.insert(new_c);

				//Try to split device
				uint64_t minSize = uint64_t(-1);
				for(const fs::Device &dev : devices[index].subdevices)
					minSize = std::min(minSize, dev.size);

				if(minSize < devices[new_index].size)
				{
					//new_index-device is larger than smallest subdevice
					// --> It makes sense to split the device
					fs::Device splitted_a = devices[new_index].getSplitted(minSize);
					fs::Device splitted_b = devices[new_index].getSplitted(devices[new_index].size - minSize);

					addDevice(splitted_a, devices);
					std::size_t splitted_index_b = addDevice(splitted_b, devices);

					device_combination splitted_c(c);
					splitted_c.available.remove(index);
					splitted_c.available.remove(new_index);
					splitted_c.available.push_back(splitted_index_b);

					fs::Device splitted_d = devices[index];
					splitted_d.subdevices.push_back(splitted_a);
					std::size_t new_splitted_device = addDevice(splitted_d, devices);

					splitted_c.available.push_back(new_splitted_device);
					splitted_c.evaluateScore();
					combinations.insert(splitted_c);
				}
			}
		}

		bool isRedundant = true;
		bool isNotRedundant = true;
		bool valid = true;
		tdisk_advice td;
		td.recommendation_score = c.score;
		td.redundancy_level = c.redundancy_level;
		td.wasted_space = c.wasted_space;
		for(std::size_t index : c.available)
		{
			if(!devices[index].isValid())
			{
				valid = false;
				break;
			}

			if(devices[index].isRedundant())isNotRedundant = false;
			else isRedundant = false;

			td.devices.push_back(devices[index]);
		}

		if(valid && (isNotRedundant || isRedundant))
		{
			if(std::find(advices.begin(), advices.end(), td) == advices.end())
			{
				advices.push_back(td);
			}
		}
	}

	std::sort(advices.begin(), advices.end());
	for(const tdisk_advice &advice : advices)
		cout<<createResultString(advice, 0, "json")<<endl;

	return std::move(advices);
}

/**
  *
  * tDisk backend
  * @author Thomas Sparber (2015-2016)
  *
 **/

#include <algorithm>
#include <iostream>
#include <typeinfo>

#include <configfile.hpp>
#include <device.hpp>
#include <deviceadvisor.hpp>
#include <backend.hpp>
#include <backendexception.hpp>
#include <resultformatter.hpp>
#include <tdisk.hpp>
#include <utils.hpp>

using std::find;
using std::string;
using std::vector;

using td::utils::concat;

td::Options td::getDefaultOptions()
{
	return Options({
		Option::output_format
	});
}

string td::get_tDisks(const vector<string> &/*args*/, Options &options)
{
	vector<tDisk> tdisks;

	if(!options.getOptionBoolValue("config-only"))
	{
		tDisk::getTDisks(tdisks);
	}

	if(!options.getOptionBoolValue("temporary"))
	{
		configuration config(options.getStringOptionValue("configfile"), options);
		for(const configuration::tdisk_config &disk : config.tdisks)
		{
			tDisk d(disk.minornumber);
			auto res = find(tdisks.begin(), tdisks.end(), d);
			if(res == tdisks.end())tdisks.push_back(std::move(d));
		}
	}

	return createResultString(tdisks, 0, options.getOptionValue("output-format"));
}

string td::get_tDisk(const vector<string> &args, Options &options)
{
	if(args.empty())throw BackendException("\"get_tDisk\" needs the device to get information for");

	tDisk d;
	if(!options.getOptionBoolValue("config-only"))
	{
		try {
			d = tDisk::get(args[0]);
			d.loadSize();
		} catch(const tDiskException &e) {
			//This is ok here... it just means it is offline
		}
	}

	if(!options.getOptionBoolValue("temporary"))
	{
		//Only look in config file
		if(!d.isValid())
		{
			tDisk temp = tDisk::get(args[0]);
			configuration config(options.getStringOptionValue("configfile"), options);
			d = tDisk(config.getTDisk(temp.getMinornumber()).minornumber);
		}
	}

	if(!d.isValid())throw BackendException("tDisk ",args[0]," does not exist");

	return createResultString(d, 0, options.getOptionValue("output-format"));
}

string td::get_devices(const vector<string> &/*args*/, Options &options)
{
	vector<fs::Device> devices;
	fs::getDevices(devices);

	vector<c::f_internal_device_info> internal_devices;
	internal_devices.reserve(devices.size());
	for(const fs::Device &device : devices)
	{
		c::f_internal_device_info d = {};
		d.type = c::f_internal_device_type_file;
		strncpy(d.name, device.name.c_str(), F_TDISK_MAX_INTERNAL_DEVICE_NAME);
		strncpy(d.path, device.path.c_str(), F_TDISK_MAX_INTERNAL_DEVICE_NAME);
		d.size = device.size;
		internal_devices.push_back(d);
	}

	return createResultString(internal_devices, 0, options.getOptionValue("output-format"));
}

string td::add_tDisk(const vector<string> &args, Options &options)
{
	if(args.empty())throw BackendException("\"add\" needs the desired blocksize (e.g. 16384)");

	char *test;
	tDisk disk;
	int number = -1;
	unsigned int blocksize = 0;
	if(args.size() > 1)
	{
		number = strtol(args[0].c_str(), &test, 10);
		if(test != args[0].c_str() + args[0].length())throw BackendException("Invalid minor number ", args[0]);

		blocksize = strtol(args[1].c_str(), &test, 10);
		if(test != args[1].c_str() + args[1].length())throw BackendException("Invalid blocksize ", args[1]);
	}
	else
	{
		blocksize = strtol(args[0].c_str(), &test, 10);
		if(test != args[0].c_str() + args[0].length())throw BackendException("Invalid blocksize ", args[0]);
	}

	if(!options.getOptionBoolValue("config-only"))
	{
		try {
			if(number >= 0)disk = tDisk::create(number, blocksize);
			else disk = tDisk::create(blocksize);

			number = disk.getMinornumber();
		} catch(const tDiskException &e) {
			//This is ok since only the driver is not loaded
		}
	}

	if(!options.getOptionBoolValue("temporary"))
	{
		configuration config(options.getStringOptionValue("configfile"), options);

		if(number == -1)
		{
			for(const configuration::tdisk_config &d : config.tdisks)
				if(d.minornumber+1 >= number)number = d.minornumber + 1;

			disk = tDisk(number);
		}

		configuration::tdisk_config newDevice;
		newDevice.minornumber = number;
		newDevice.blocksize = blocksize;
		config.addDevice(std::move(newDevice));

		config.save(options.getStringOptionValue("configfile"));
	}

	return createResultString(concat("Device ", disk.getName(), " opened"), 0, options.getOptionValue("output-format"));
}

string td::remove_tDisk(const vector<string> &args, Options &options)
{
	if(args.empty())throw BackendException("\"remove\" needs the device to be removed");

	tDisk d = tDisk::get(args[0]);

	if(!options.getOptionBoolValue("config-only"))
	{
		try {
			d.remove();
		} catch(const tDiskException &e) {
			//This is ok since only the driver is offline
		}
	}

	if(!options.getOptionBoolValue("temporary"))
	{
		configuration config(options.getStringOptionValue("configfile"), options);

		configuration::tdisk_config temp;
		temp.minornumber = d.getMinornumber();
		auto found = find(config.tdisks.begin(), config.tdisks.end(), temp);
		if(found != config.tdisks.end())
		{
			config.tdisks.erase(found);
			config.save(options.getStringOptionValue("configfile"));
		}
	}

	return createResultString(concat("tDisk ", d.getName(), " removed"), 0, options.getOptionValue("output-format"));
}

string td::add_disk(const vector<string> &args, Options &options)
{
	if(args.size() < 2)throw BackendException("\"add_disk\" needs the tDisk and at least one file to add");

	vector<string> results;
	tDisk d = tDisk::get(args[0]);

	if(!options.getOptionBoolValue("config-only"))
	{
		for(std::size_t i = 1; i < args.size(); ++i)
		{
			d.addDisk(args[i]);
			results.push_back(concat("Successfully added disk ", args[i]));
		}
	}

	if(!options.getOptionBoolValue("temporary"))
	{
		configuration config(options.getStringOptionValue("configfile"), options);

		for(std::size_t i = 1; i < args.size(); ++i)
		{

		}
	}

	return createResultString(results, 0, options.getOptionValue("output-format"));
}

string td::get_max_sectors(const vector<string> &args, Options &options)
{
	if(args.empty())throw BackendException("\"get_max_sectors\" needs the td device");

	tDisk d = tDisk::get(args[0]);
	unsigned long long maxSectors = d.getMaxSectors();
	return createResultString(maxSectors, 0, options.getOptionValue("output-format"));
}

string td::get_size_bytes(const vector<string> &args, Options &options)
{
	if(args.empty())throw BackendException("\"get_max_sectors\" needs the td device");

	tDisk d = tDisk::get(args[0]);
	unsigned long long size = d.getSize();
	return createResultString(size, 0, options.getOptionValue("output-format"));
}

string td::get_sector_index(const vector<string> &args, Options &options)
{
	if(args.size() < 2)throw BackendException("\"get_sector_index\" needs the td device and the sector index to retrieve");

	char *test;
	unsigned long long logicalSector = strtoull(args[1].c_str(), &test, 10);
	if(test != args[1].c_str() + args[1].length())throw BackendException(args[1], " is not a valid number");

	tDisk d = tDisk::get(args[0]);
	f_sector_index index = d.getSectorIndex(logicalSector);
	return createResultString(index, 0, options.getOptionValue("output-format"));
}

string td::get_all_sector_indices(const vector<string> &args, Options &options)
{
	if(args.empty())throw BackendException("\"get_all_sector_indices\" needs the td device");

	tDisk d = tDisk::get(args[0]);
	vector<f_sector_info> sectorIndices = d.getAllSectorIndices();
	return createResultString(sectorIndices, 0, options.getOptionValue("output-format"));
}

string td::clear_access_count(const vector<string> &args, Options &options)
{
	if(args.empty())throw BackendException("\"clear_access_count\" needs the td device\n");

	tDisk d = tDisk::get(args[0]);
	d.clearAccessCount();
	return createResultString(concat("Access count for tDisk ", d.getName(), " cleared"), 0, options.getOptionValue("output-format"));
}

string td::get_internal_devices_count(const vector<string> &args, Options &options)
{
	if(args.empty())throw BackendException("\"get_internal_device_count\" needs the td device\n");

	tDisk d = tDisk::get(args[0]);
	unsigned int devices = d.getInternalDevicesCount();
	return createResultString(devices, 0, options.getOptionValue("output-format"));
}

string td::get_device_info(const vector<string> &args, Options &options)
{
	if(args.size() < 2)throw BackendException("\"get_device_info\" needs the td device and the device to retrieve infos for\n");

	char *test;
	unsigned int device = strtol(args[1].c_str(), &test, 10);
	if(test != args[1].c_str() + args[1].length())
		throw BackendException("Invalid number ", args[1], " for device index");

	tDisk d = tDisk::get(args[0]);
	f_internal_device_info info = d.getDeviceInfo(device);
	return createResultString(info, 0, options.getOptionValue("output-format"));
}

string td::load_config_file(const vector<string> &args, Options &options)
{
	if(args.empty())throw BackendException("\"load_config_file\" needs at least one config file to load\n");

	configuration cfg;
	for(std::size_t i = 0; i < args.size(); ++i)
	{
		configuration config(args[i], options);
		cfg.merge(config);
	}

	for(const configuration::tdisk_global_option &option : cfg.global_options)
		options.setOptionValue(option.name, option.value);

	configuration loadedCfg;
	for(const configuration::tdisk_config &config : cfg.tdisks)
	{
		if(!config.isValid())
		{
			//std::cout<<"Config for "<<config.minornumber<<" is not valid. skipping "<<std::endl;
			continue;
		}

		tDisk disk = tDisk::create(config.minornumber, config.blocksize);

		for(const string device : config.devices)
			disk.addDisk(device);

		loadedCfg.addDevice(config);
	}

	return createResultString(loadedCfg, 0, options.getOptionValue("output-format"));
}

string td::get_device_advice(const vector<string> &args, Options &options)
{
	vector<advisor::tdisk_advice> advices = advisor::getTDiskAdvices(args);
	return createResultString(advices, 0, options.getOptionValue("output-format"));
}

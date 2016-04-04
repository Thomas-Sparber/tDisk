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
using std::swap;
using std::vector;

using namespace td;
using utils::concat;

Options td::getDefaultOptions()
{
	return Options({
		Option::output_format
	});
}

BackendResult td::get_tDisks(const vector<string> &/*args*/, Options &options)
{
	BackendResult r;
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

	r.result(tdisks, options.getOptionValue("output-format"));
	return std::move(r);
}

BackendResult td::get_tDisk(const vector<string> &args, Options &options)
{
	BackendResult r;
	if(args.empty())
	{
		r.error(BackendResultType::general, "\"get_tDisk\" needs the device to get information for");
		return std::move(r);
	}

	tDisk d;
	if(!options.getOptionBoolValue("config-only"))
	{
		try {
			d = tDisk::get(args[0]);
			d.loadSize();
		} catch(const tDiskException &e) {
			r.error(BackendResultType::driver, e.what);
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

	if(!d.isValid())
	{
		r.error(BackendResultType::general, utils::concat("tDisk ",args[0]," does not exist"));
		return std::move(r);
	}

	r.result(d, options.getOptionValue("output-format"));
	return std::move(r);
}

BackendResult td::get_devices(const vector<string> &/*args*/, Options &options)
{
	BackendResult r;
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

	r.result(internal_devices, options.getOptionValue("output-format"));
	return std::move(r);
}

BackendResult td::create_tDisk(const vector<string> &args, Options &options)
{
	BackendResult r;
	if(args.size() < 2)
	{
		r.error(BackendResultType::general, "\"create\" needs the desired blocksize (e.g. 16384) and at least one device");
		return std::move(r);
	}

	size_t devicesIndex = 1;
	int number = -1;
	char *test;
	tDisk disk;
	int blocksize = strtol(args[0].c_str(), &test, 10);
	if(test != args[0].c_str() + args[0].length())throw BackendException("Invalid blocksize ", args[0]);

	number = strtol(args[1].c_str(), &test, 10);
	if(test == args[1].c_str() + args[1].length())
	{
		//If minornumber and blocksize are given, they are given in the reverse order
		swap(number, blocksize);
		devicesIndex++;

		if(args.size() == devicesIndex)throw BackendException("\"create\" needs the minornumber, desired blocksize (e.g. 16384) and at least one device");
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
		for(size_t i = devicesIndex; i < args.size(); ++i)newDevice.devices.push_back(args[i]);
		config.addDevice(std::move(newDevice));

		config.save(options.getStringOptionValue("configfile"));
	}

	if(!options.getOptionBoolValue("config-only"))
	{
		try {
			if(number >= 0)disk = tDisk::create(number, blocksize);
			else disk = tDisk::create(blocksize);

			number = disk.getMinornumber();
			for(size_t i = devicesIndex; i < args.size(); ++i)disk.addDisk(args[i]);
		} catch(const tDiskException &e) {
			r.error(BackendResultType::driver, e.what);
		}
	}

	r.message(BackendResultType::general, utils::concat("tDisk with minornumber ",number," created successfully"));
	return std::move(r);
}

BackendResult td::add_tDisk(const vector<string> &args, Options &options)
{
	BackendResult r;
	if(args.empty())
	{
		r.error(BackendResultType::general, "\"add\" needs the desired blocksize (e.g. 16384)");
		return std::move(r);
	}

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

	if(!options.getOptionBoolValue("config-only"))
	{
		try {
			if(number >= 0)disk = tDisk::create(number, blocksize);
			else disk = tDisk::create(blocksize);

			number = disk.getMinornumber();
		} catch(const tDiskException &e) {
			r.error(BackendResultType::driver, e.what);
		}
	}

	r.message(BackendResultType::general, concat("Device ", disk.getName(), " opened"));
	return std::move(r);
}

BackendResult td::remove_tDisk(const vector<string> &args, Options &options)
{
	BackendResult r;
	if(args.empty())
	{
		r.error(BackendResultType::general, "\"remove\" needs the device to be removed");
		return std::move(r);
	}

	tDisk d = tDisk::get(args[0]);

	if(!options.getOptionBoolValue("config-only"))
	{
		try {
			d.remove();
		} catch(const tDiskException &e) {
			r.error(BackendResultType::driver, e.what);
			return std::move(r);
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

	r.message(BackendResultType::general, concat("tDisk ", d.getName(), " removed"));
	return std::move(r);
}

BackendResult td::add_disk(const vector<string> &args, Options &options)
{
	BackendResult r;
	if(args.size() < 2)
	{
		r.error(BackendResultType::general, "\"add_disk\" needs the tDisk and at least one file to add");
		return std::move(r);
	}

	vector<string> results;
	tDisk d = tDisk::get(args[0]);

	if(!options.getOptionBoolValue("config-only"))
	{
		for(std::size_t i = 1; i < args.size(); ++i)
		{
			d.addDisk(args[i]);
			r.message(BackendResultType::driver, concat("Successfully added disk ", args[i]));
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
			for(std::size_t i = 1; i < args.size(); ++i)
			{
				found->devices.push_back(args[i]);
				r.message(BackendResultType::configfile, concat("Successfully added disk ", args[i]));
			}

			config.save(options.getStringOptionValue("configfile"));
		}
	}

	return std::move(r);
}

BackendResult td::get_max_sectors(const vector<string> &args, Options &options)
{
	BackendResult r;
	if(args.empty())
	{
		r.error(BackendResultType::general, "\"get_max_sectors\" needs the td device");
		return std::move(r);
	}

	tDisk d = tDisk::get(args[0]);
	unsigned long long maxSectors = d.getMaxSectors();

	r.result(maxSectors, options.getOptionValue("output-format"));
	return std::move(r);
}

BackendResult td::get_size_bytes(const vector<string> &args, Options &options)
{
	BackendResult r;
	if(args.empty())
	{
		r.error(BackendResultType::general, "\"get_max_sectors\" needs the td device");
		return std::move(r);
	}

	tDisk d = tDisk::get(args[0]);
	unsigned long long size = d.getSize();

	r.result(size, options.getOptionValue("output-format"));
	return std::move(r);
}

BackendResult td::get_sector_index(const vector<string> &args, Options &options)
{
	BackendResult r;
	if(args.size() < 2)
	{
		r.error(BackendResultType::general, "\"get_sector_index\" needs the td device and the sector index to retrieve");
		return std::move(r);
	}

	char *test;
	unsigned long long logicalSector = strtoull(args[1].c_str(), &test, 10);
	if(test != args[1].c_str() + args[1].length())throw BackendException(args[1], " is not a valid number");

	tDisk d = tDisk::get(args[0]);
	f_sector_index index = d.getSectorIndex(logicalSector);

	r.result(index, options.getOptionValue("output-format"));
	return std::move(r);
}

BackendResult td::get_all_sector_indices(const vector<string> &args, Options &options)
{
	BackendResult r;
	if(args.empty())
	{
		r.error(BackendResultType::general, "\"get_all_sector_indices\" needs the td device");
		return std::move(r);
	}

	tDisk d = tDisk::get(args[0]);
	vector<f_sector_info> sectorIndices = d.getAllSectorIndices();

	r.result(sectorIndices, options.getOptionValue("output-format"));
	return std::move(r);
}

BackendResult td::clear_access_count(const vector<string> &args, Options &/*options*/)
{
	BackendResult r;
	if(args.empty())
	{
		r.error(BackendResultType::general, "\"clear_access_count\" needs the td device\n");
		return std::move(r);
	}

	tDisk d = tDisk::get(args[0]);
	d.clearAccessCount();

	r.message(BackendResultType::general, concat("Access count for tDisk ", d.getName(), " cleared"));
	return std::move(r);
}

BackendResult td::get_internal_devices_count(const vector<string> &args, Options &options)
{
	BackendResult r;
	if(args.empty())
	{
		r.error(BackendResultType::general, "\"get_internal_device_count\" needs the td device\n");
		return std::move(r);
	}

	tDisk d = tDisk::get(args[0]);
	unsigned int devices = d.getInternalDevicesCount();

	r.result(devices, options.getOptionValue("output-format"));
	return std::move(r);
}

BackendResult td::get_device_info(const vector<string> &args, Options &options)
{
	BackendResult r;
	if(args.size() < 2)
	{
		r.error(BackendResultType::general, "\"get_device_info\" needs the td device and the device to retrieve infos for\n");
		return std::move(r);
	}

	char *test;
	unsigned int device = strtol(args[1].c_str(), &test, 10);
	if(test != args[1].c_str() + args[1].length())
		throw BackendException("Invalid number ", args[1], " for device index");

	tDisk d = tDisk::get(args[0]);
	f_internal_device_info info = d.getDeviceInfo(device);

	r.result(info, options.getOptionValue("output-format"));
	return std::move(r);
}

BackendResult td::load_config_file(const vector<string> &args, Options &options)
{
	BackendResult r;
	if(args.empty())
	{
		r.error(BackendResultType::general, "\"load_config_file\" needs at least one config file to load\n");
		return std::move(r);
	}

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
			r.warning(BackendResultType::configfile, utils::concat("Config for ",config.minornumber," is not valid. skipping"));
			continue;
		}

		tDisk disk = tDisk::create(config.minornumber, config.blocksize);

		for(const string device : config.devices)
			disk.addDisk(device);

		loadedCfg.addDevice(config);
	}

	r.result(loadedCfg, options.getOptionValue("output-format"));
	return std::move(r);
}

BackendResult td::get_device_advice(const vector<string> &args, Options &options)
{
	BackendResult r;
	vector<advisor::tdisk_advice> advices = advisor::getTDiskAdvices(args);
	r.result(advices, options.getOptionValue("output-format"));
	return std::move(r);
}

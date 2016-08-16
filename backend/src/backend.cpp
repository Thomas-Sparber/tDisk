/**
  *
  * tDisk backend
  * @author Thomas Sparber (2015-2016)
  *
 **/

#include <algorithm>
#include <iostream>
#include <typeinfo>

#include <backend.hpp>
#include <backendexception.hpp>
#include <configfile.hpp>
#include <convert.hpp>
#include <device.hpp>
#include <deviceadvisor.hpp>
#include <fileassignment.hpp>
#include <logger.hpp>
#include <performance.hpp>
#include <resultformatter.hpp>
#include <shell.hpp>
#include <tdisk.hpp>
#include <utils.hpp>

using std::find;
using std::make_pair;
using std::pair;
using std::sort;
using std::string;
using std::swap;
using std::unique_ptr;
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

	//Perform driver operation
	if(!options.getOptionBoolValue("config-only"))
	{
		try {
			tDisk::getTDisks(tdisks);
		} catch (const tDiskOfflineException &e) {
			r.warning(BackendResultType::driver, e.what());
		} catch (const tDiskException &e) {
			r.error(BackendResultType::driver, e.what());
		}
	}

	//Perform config file operation
	if(!options.getOptionBoolValue("temporary"))
	{
		try {
			configuration config(options.getStringOptionValue("configfile"), options);
			for(const configuration::tdisk_config &disk : config.tdisks)
			{
				tDisk d(disk.minornumber);
				auto res = find(tdisks.begin(), tdisks.end(), d);
				if(res == tdisks.end())tdisks.push_back(std::move(d));
			}
		} catch (const tDiskException &e) {
			r.error(BackendResultType::configfile, e.what());
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

	//Perform driver operation
	if(!options.getOptionBoolValue("config-only"))
	{
		try {
			d = tDisk::get(args[0]);
			d.loadSize();
		} catch(const tDiskOfflineException &e) {
			r.warning(BackendResultType::driver, e.what());
		} catch(const tDiskException &e) {
			r.error(BackendResultType::driver, e.what());
		}
	}

	//Perform config file operation
	if(!options.getOptionBoolValue("temporary"))
	{
		//Only look in config file
		if(!d.isValid())
		{
			try {
				d = tDisk::get(args[0]);
				configuration config(options.getStringOptionValue("configfile"), options);
				d = tDisk(config.getTDisk(d.getMinornumber()).minornumber);
			} catch (const tDiskException &e) {
				r.error(BackendResultType::configfile, e.what());
			}
		}
	}

	if(!d.isValid())
	{
		//tDisk was neither found by driver nor by configfile
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
	//vector<c::f_internal_device_info> internal_devices;

	try {
		fs::getDevices(devices);
		/*for(const fs::Device &device : devices)
		{
			c::f_internal_device_info d;
			d.type = c::f_internal_device_type_file;
			strncpy(d.name, device.name.c_str(), F_TDISK_MAX_INTERNAL_DEVICE_NAME);
			strncpy(d.path, device.path.c_str(), F_TDISK_MAX_INTERNAL_DEVICE_NAME);
			d.size = device.size;
			internal_devices.push_back(d);
		}*/
	} catch (const tDiskException &e) {
		r.error(BackendResultType::general, e.what());
	}

	r.result(devices, options.getOptionValue("output-format"));
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

	//First, read minornumber and blocksize
	size_t devicesIndex = 1;
	tDisk disk;

	int blocksize;
	if(!utils::convertTo(args[0], blocksize))
	{
		r.error(BackendResultType::general, utils::concat("Invalid blocksize ", args[0]));
		return std::move(r);
	}

	int number = -1;
	if(utils::convertTo(args[1], number))
	{
		//If minornumber and blocksize are given, they are given in the reverse order
		swap(number, blocksize);
		devicesIndex++;

		if(args.size() == devicesIndex)
		{
			r.error(BackendResultType::general, utils::concat("\"create\" needs the minornumber, desired blocksize (e.g. 16384) and at least one device"));
			return std::move(r);
		}
	}
	else number = -1;
	
	if(number == -1)
	{
		//Looking for a suiable minornumber in the configfile
		try {
			configuration config(options.getStringOptionValue("configfile"), options);
			for(const configuration::tdisk_config &d : config.tdisks)
				if(d.minornumber+1 >= number)number = d.minornumber + 1;
		} catch (const tDiskException &e) {
			//I don't care about the error since I'm just trying
			//to find a suitable minornumber
		}

		//Asking the driver for a suitable minornumber
		try {
			vector<tDisk> tdisks;
			tDisk::getTDisks(tdisks);
			for(const tDisk &d : tdisks)
				if(d.getMinornumber()+1 >= number)number = d.getMinornumber() + 1;
		} catch (const tDiskException &e) {
			//I don't care about the error since I'm just trying
			//to find a suitable minornumber
		}

		//No tDisks are present yet
		if(number == -1)number = 0;
	}

	//Perform driver operation
	if(!options.getOptionBoolValue("config-only"))
	{
		try {
			if(number >= 0)disk = tDisk::create(number, (unsigned int)blocksize);
			else disk = tDisk::create((unsigned int)blocksize);

			for(size_t i = devicesIndex; i < args.size(); ++i)disk.addDisk(args[i], true);
		} catch(const tDiskOfflineException &e) {
			r.warning(BackendResultType::driver, e.what());
		} catch(const tDiskException &e) {
			r.error(BackendResultType::driver, e.what());
		}
	}

	//Perform config file operations
	if(!options.getOptionBoolValue("temporary"))
	{
		try {
			configuration config(options.getStringOptionValue("configfile"), options);

			configuration::tdisk_config newDevice;
			newDevice.minornumber = number;
			newDevice.blocksize = blocksize;
			for(size_t i = devicesIndex; i < args.size(); ++i)newDevice.devices.push_back(args[i]);
			config.addDevice(std::move(newDevice));

			config.save(options.getStringOptionValue("configfile"));
		} catch (const tDiskException &e) {
			r.error(BackendResultType::configfile, e.what());
		}
	}

	r.result(number, options.getOptionValue("output-format"));
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

	//Read minornumber and blocksize first
	tDisk disk;
	int number = -1;
	unsigned int blocksize = 0;
	if(args.size() > 1)
	{
		if(!utils::convertTo(args[0], number))
		{
			r.error(BackendResultType::general, utils::concat("Invalid minor number ", args[0]));
			return std::move(r);
		}

		if(!utils::convertTo(args[1], blocksize))
		{
			r.error(BackendResultType::general, utils::concat("Invalid blocksize ", args[1]));
			return std::move(r);
		}
	}
	else
	{
		if(!utils::convertTo(args[0], blocksize))
		{
			r.error(BackendResultType::general, utils::concat("Invalid blocksize ", args[0]));
			return std::move(r);
		}
	}
	
	if(number == -1)
	{
		//Looking for a suiable minornumber in the configfile
		try {
			configuration config(options.getStringOptionValue("configfile"), options);
			for(const configuration::tdisk_config &d : config.tdisks)
				if(d.minornumber+1 >= number)number = d.minornumber + 1;
		} catch (const tDiskException &e) {
			//I don't care about the error since I'm just trying
			//to find a suitable minornumber
		}

		//Asking the driver for a suitable minornumber
		try {
			vector<tDisk> tdisks;
			tDisk::getTDisks(tdisks);
			for(const tDisk &d : tdisks)
				if(d.getMinornumber()+1 >= number)number = d.getMinornumber() + 1;
		} catch (const tDiskException &e) {
			//I don't care about the error since I'm just trying
			//to find a suitable minornumber
		}

		//No tDisks are present yet
		if(number == -1)number = 0;
	}

	//Perform driver operation
	if(!options.getOptionBoolValue("config-only"))
	{
		try {
			if(number >= 0)disk = tDisk::create(number, blocksize);
			else disk = tDisk::create(blocksize);
		} catch(const tDiskOfflineException &e) {
			r.warning(BackendResultType::driver, e.what());
		} catch(const tDiskException &e) {
			r.error(BackendResultType::driver, e.what());
		}
	}

	//Perform config file operation
	if(!options.getOptionBoolValue("temporary"))
	{
		try {
			configuration config(options.getStringOptionValue("configfile"), options);

			configuration::tdisk_config newDevice;
			newDevice.minornumber = number;
			newDevice.blocksize = blocksize;
			config.addDevice(std::move(newDevice));

			config.save(options.getStringOptionValue("configfile"));
		} catch (const tDiskException &e) {
			r.error(BackendResultType::configfile, e.what());
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

	//Perform driver operation
	if(!options.getOptionBoolValue("config-only"))
	{
		try {
			d.remove();
		} catch(const tDiskOfflineException &e) {
			r.warning(BackendResultType::driver, e.what());
		} catch(const tDiskException &e) {
			r.error(BackendResultType::driver, e.what());
		}
	}

	//Perform config file operation
	if(!options.getOptionBoolValue("temporary"))
	{
		try {
			configuration config(options.getStringOptionValue("configfile"), options);

			configuration::tdisk_config temp;
			temp.minornumber = d.getMinornumber();
			auto found = find(config.tdisks.begin(), config.tdisks.end(), temp);
			if(found != config.tdisks.end())
			{
				config.tdisks.erase(found);
				config.save(options.getStringOptionValue("configfile"));
			}
		} catch (const tDiskException &e) {
			r.error(BackendResultType::configfile, e.what());
		}
	}

	r.message(BackendResultType::general, concat("tDisk ", d.getName(), " removed"));
	return std::move(r);
}

BackendResult td::remove_disk(const vector<string> &args, Options &/*options*/)
{
	BackendResult r;
	if(args.size() < 2)
	{
		r.error(BackendResultType::general, "\"remove_disk\" needs the tDisk and the device to remove");
		return std::move(r);
	}

	unsigned int disk;
	if(!utils::convertTo(args[1], disk))
	{
		r.error(BackendResultType::general, utils::concat(args[1]," is not a valid id for an internal device"));
		return std::move(r);
	}

	tDisk d = tDisk::get(args[0]);

	//Perform driver operation
	try {
		d.removeDisk(disk);
		r.message(BackendResultType::driver, concat("Successfully removed disk ", disk));

		/*configuration config(options.getStringOptionValue("configfile"), options);
TODO
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
		}*/
	} catch (const tDiskOfflineException &e) {
		r.error(BackendResultType::driver, e.what());
	} catch (const tDiskException &e) {
		r.error(BackendResultType::driver, e.what());
	}

	//Perform config file operation
	/*try {	TODO
		
	} catch (const tDiskException &e) {
		r.error(BackendResultType::configfile, e.what());
	}*/

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

	//Perform driver operation
	if(!options.getOptionBoolValue("config-only"))
	{
		try {
			for(std::size_t i = 1; i < args.size(); ++i)
			{
				d.addDisk(args[i], false);
				r.message(BackendResultType::driver, concat("Successfully added disk ", args[i]));
			}
		} catch (const tDiskOfflineException &e) {
			r.warning(BackendResultType::driver, e.what());
		} catch (const tDiskException &e) {
			r.error(BackendResultType::driver, e.what());
		}
	}

	//Perform config file operation
	if(!options.getOptionBoolValue("temporary"))
	{
		try {
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
		} catch (const tDiskException &e) {
			r.error(BackendResultType::configfile, e.what());
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

	unsigned long long maxSectors = 0;

	try {
		tDisk d = tDisk::get(args[0]);
		maxSectors = d.getMaxSectors();
		r.result(maxSectors, options.getOptionValue("output-format"));
	} catch (const tDiskException &e) {
		r.error(BackendResultType::driver, e.what());
	}

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

	unsigned long long size = 0;

	try {
		tDisk d = tDisk::get(args[0]);
		size = d.getSize();
		r.result(size, options.getOptionValue("output-format"));
	} catch (const tDiskException &e) {
		r.error(BackendResultType::driver, e.what());
	}

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

	//Read requested logical sector
	uint64_t logicalSector;
	if(!utils::convertTo(args[1], logicalSector))
	{
		r.error(BackendResultType::general, utils::concat(args[1], " is not a valid number"));
		return std::move(r);
	}
	
	try {
		tDisk d = tDisk::get(args[0]);
		f_sector_index index = d.getSectorIndex(logicalSector);
		r.result(index, options.getOptionValue("output-format"));
	} catch (const tDiskException &e) {
		r.error(BackendResultType::driver, e.what());
	}

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

	vector<f_sector_info> sectorIndices;

	try {
		tDisk d = tDisk::get(args[0]);
		sectorIndices = d.getAllSectorIndices();

		performance::start("result");
		r.result(sectorIndices, options.getOptionValue("output-format"));
		performance::stop("result");
	} catch (const tDiskException &e) {
		r.error(BackendResultType::driver, e.what());
	}
	
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

	try {
		tDisk d = tDisk::get(args[0]);
		d.clearAccessCount();

		r.message(BackendResultType::general, concat("Access count for tDisk ", d.getName(), " cleared"));
	} catch (const tDiskException &e) {
		r.error(BackendResultType::driver, e.what());
	}
	
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

	try {
		tDisk d = tDisk::get(args[0]);
		unsigned int devices = d.getInternalDevicesCount();

		r.result(devices, options.getOptionValue("output-format"));
	} catch (const tDiskException &e) {
		r.error(BackendResultType::driver, e.what());
	}
	
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

	//Read requested device id first
	unsigned int device;
	if(!utils::convertTo(args[1], device))
	{
		r.error(BackendResultType::general, utils::concat("Invalid number ", args[1], " for device index"));
		return std::move(r);
	}

	try {
		tDisk d = tDisk::get(args[0]);
		f_internal_device_info info = d.getDeviceInfo(device);

		r.result(info, options.getOptionValue("output-format"));
	} catch (const tDiskException &e) {
		r.error(BackendResultType::driver, e.what());
	}
	
	return std::move(r);
}

BackendResult td::get_debug_info(const vector<string> &args, Options &options)
{
	BackendResult r;
	if(args.size() < 1)
	{
		r.error(BackendResultType::general, "\"get_device_info\" needs the td device and and optional info id\n");
		return std::move(r);
	}

	uint64_t id = 0;
	if(args.size() > 1)
	{
		//Read debug id
		if(!utils::convertTo(args[1], id))
		{
			r.error(BackendResultType::general, utils::concat("Invalid number ", args[1], " for debug id"));
			return std::move(r);
		}
	}

	try {
		tDisk d = tDisk::get(args[0]);
		f_tdisk_debug_info info = d.getDebugInfo(id);

		r.result(info, options.getOptionValue("output-format"));
	} catch (const tDiskException &e) {
		r.error(BackendResultType::driver, e.what());
	}
	
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

	try {
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

			//Load tDisk
			try {
				tDisk disk = tDisk::create(config.minornumber, config.blocksize);

				for(const string device : config.devices)
					disk.addDisk(device, false);

				loadedCfg.addDevice(config);
			} catch (const tDiskException &e) {
				r.error(BackendResultType::driver, e.what());
			}
		}

		r.result(loadedCfg, options.getOptionValue("output-format"));
	} catch (const tDiskException &e) {
		r.error(BackendResultType::configfile, e.what());
	}
	
	return std::move(r);
}

BackendResult td::get_device_advice(const vector<string> &args, Options &options)
{
	BackendResult r;

	try {
		vector<advisor::tdisk_advice> advices = advisor::getTDiskAdvices(args);
		r.result(advices, options.getOptionValue("output-format"));
	} catch (const tDiskException &e) {
		r.error(BackendResultType::general, e.what());
	}
	
	return std::move(r);
}

BackendResult td::get_files_at(const std::vector<std::string> & args, Options &options)
{
	BackendResult r;
	if(args.size() < 3)
	{
		r.error(BackendResultType::general, "\"get_files_at\" needs the disk, start and end\n");
		return std::move(r);
	}

	const string &path = args[0];

	uint64_t start;
	if(!utils::convertTo(args[1], start))
	{
		r.error(BackendResultType::general, utils::concat(args[1]," is not a valid number for start"));
		return std::move(r);
	}

	uint64_t end;
	if(!utils::convertTo(args[2], end))
	{
		r.error(BackendResultType::general, utils::concat(args[2]," is not a valid number for end"));
		return std::move(r);
	}

	vector<FileAssignment> files = fs::getFilesOnDisk(path, { { start, end } }, false, true);

	performance::start("r.result");
	r.result(files, options.getOptionValue("output-format"));
	performance::stop("r.result");

	return std::move(r);
}

BackendResult td::get_files_on_disk(const vector<string> &args, Options &options)
{
	BackendResult r;
	if(args.empty())
	{
		r.error(BackendResultType::general, "\"get_files_on_disk\" needs the td device and the internal device");
		return std::move(r);
	}

	unsigned int device;;
	if(!utils::convertTo(args[1], device))
	{
		r.error(BackendResultType::general, utils::concat(args[1]," is not a valid number for an internal device"));
		return std::move(r);
	}

	try {
		tDisk d = tDisk::get(args[0]);
		vector<FileAssignment> files = d.getFilesOnDevice(device, true, true);
		r.result(files, options.getOptionValue("output-format"));
	} catch (const tDiskException &e) {
		r.error(BackendResultType::driver, e.what());
	}
	
	return std::move(r);
}

BackendResult td::tdisk_post_create(const vector<string> &args, Options &options)
{
	BackendResult r;
	if(args.empty())
	{
		r.error(BackendResultType::general, "\"tdisk_post_create\" needs the td device");
		return std::move(r);
	}

	tDisk d;

	try {
		d = tDisk::get(args[0]);
	} catch (const tDiskException &e) {
		r.error(BackendResultType::driver, e.what());
		return std::move(r);
	}

	shell::ShellResult result = shell::execute(shell::tDiskPostCreateCommand, d.getPath());

	if(result.empty())
	{
		r.error(BackendResultType::driver, "Shell command tDiskPostCreate didn't report any output. Unknown error");
		return std::move(r);
	}

	string path = result.get<shell::path>();
	if(!utils::folderExists(path))
	{
		r.error(BackendResultType::driver, utils::concat("Shell command tDiskPostCreate error: ",result.getMessage()));
		return std::move(r);
	}

	r.result(path, options.getOptionValue("output-format"));
	
	return std::move(r);
}

BackendResult td::tdisk_pre_remove(const vector<string> &args, Options &/*options*/)
{
	BackendResult r;
	if(args.empty())
	{
		r.error(BackendResultType::general, "\"tdisk_pre_remove\" needs the td device");
		return std::move(r);
	}

	tDisk d;

	try {
		d = tDisk::get(args[0]);
	} catch (const tDiskException &e) {
		r.error(BackendResultType::driver, e.what());
		return std::move(r);
	}

	shell::ShellResult result = shell::execute(shell::tDiskPreRemoveCommand, d.getPath());

	if(!result.empty())
	{
		r.message(BackendResultType::general, result.getMessage());
	}

	return std::move(r);
}

BackendResult td::performance_improvement(const vector<string> &args, Options &options)
{
	BackendResult r;
	if(args.empty())
	{
		r.error(BackendResultType::general, "\"performance_improvement\" needs the td device");
		return std::move(r);
	}

	try {
		tDisk d = tDisk::get(args[0]);
		const tDiskPerformanceImprovement &p = d.getPerformanceImprovement(20);
		r.result(p, options.getOptionValue("output-format"));
	} catch (const tDiskException &e) {
		r.error(BackendResultType::driver, e.what());
	}
	
	return std::move(r);
}

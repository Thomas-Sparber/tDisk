#include <typeinfo>

#include <configfile.hpp>
#include <frontend.hpp>
#include <resultformatter.hpp>
#include <tdisk.hpp>
#include <utils.hpp>

using std::string;
using std::vector;
namespace td
{

	vector<Option> options {
		Option("output-format", { "text", "json" })
	};

	void addOption(const Option &option)
	{
		if(optionExists(option.name))
			getOption(option.name) = option;
		else
			options.push_back(option);
	}
	
	bool optionExists(const ci_string &name)
	{
		for(const Option &option : options)
		{
			if(option.name == name)
				return true;
		}

		return false;
	}

	Option& getOption(const ci_string &name)
	{
		for(Option &option : options)
		{
			if(option.name == name)
				return option;
		}

		throw FrontendException("Option \"", name ,"\" is not set");
	}

	ci_string getOptionValue(const ci_string &name)
	{
		return getOption(name).value;
	}

	void setOptionValue(const ci_string &name, const ci_string &value)
	{
		Option &o = getOption(name);

		if(!o.values.empty())
		{
			bool valueFound = false;
			for(const ci_string &v : o.values)
			{
				if(v == value)
				{
					valueFound = true;
					break;
				}
			}

			if(!valueFound)
				throw FrontendException("Invalid value \"", value ,"\" for option ", name);
		}
		o.value = value;
	}

} //end namespace td

/***********************************************************************/

string td::add_tDisk(const vector<string> &args, const ci_string &outputFormat)
{
	if(args.empty())throw FrontendException("\"add\" needs the desired blocksize (e.g. 16384)");

	char *test;
	tDisk disk;
	if(args.size() > 1)
	{
		int number = strtol(args[0].c_str(), &test, 10);
		if(test != args[0].c_str() + args[0].length())throw FrontendException("Invalid minor number ", args[0]);

		unsigned int blocksize = strtol(args[1].c_str(), &test, 10);
		if(test != args[1].c_str() + args[1].length())throw FrontendException("Invalid blocksize ", args[1]);

		disk = tDisk::create(number, blocksize);
	}
	else
	{
		unsigned int blocksize = strtol(args[0].c_str(), &test, 10);
		if(test != args[0].c_str() + args[0].length())throw FrontendException("Invalid blocksize ", args[0]);

		disk = tDisk::create(blocksize);
	}
	return createResultString(concat("Device ", disk.getName(), " opened"), 0, outputFormat);
}

string td::remove_tDisk(const vector<string> &args, const ci_string &outputFormat)
{
	if(args.empty())throw FrontendException("\"remove\" needs the device to be removed");

	tDisk d = tDisk::get(args[0]);
	d.remove();
	return createResultString(concat("tDisk ", d.getName(), " removed"), 0, outputFormat);
}

string td::add_disk(const vector<string> &args, const ci_string &outputFormat)
{
	if(args.size() < 2)throw FrontendException("\"add_disk\" needs the tDisk and at least one file to add");

	vector<string> results;
	tDisk d = tDisk::get(args[0]);
	for(std::size_t i = 1; i < args.size(); ++i)
	{
		d.addDisk(args[i]);
		results.push_back(concat("Successfully added disk ", args[i]));
	}
	return createResultString(results, 0, outputFormat);
}

string td::get_max_sectors(const vector<string> &args, const ci_string &outputFormat)
{
	if(args.empty())throw FrontendException("\"get_max_sectors\" needs the td device");

	tDisk d = tDisk::get(args[0]);
	unsigned long long maxSectors = d.getMaxSectors();
	return createResultString(maxSectors, 0, outputFormat);
}

string td::get_sector_index(const vector<string> &args, const ci_string &outputFormat)
{
	if(args.size() < 2)throw FrontendException("\"get_sector_index\" needs the td device and the sector index to retrieve");

	char *test;
	unsigned long long logicalSector = strtoull(args[1].c_str(), &test, 10);
	if(test != args[1].c_str() + args[1].length())throw FrontendException(args[1], " is not a valid number");

	tDisk d = tDisk::get(args[0]);
	sector_index index = d.getSectorIndex(logicalSector);
	return createResultString(index, 0, outputFormat);
}

string td::get_all_sector_indices(const vector<string> &args, const ci_string &outputFormat)
{
	if(args.empty())throw FrontendException("\"get_all_sector_indices\" needs the td device");

	tDisk d = tDisk::get(args[0]);
	vector<sector_info> sectorIndices = d.getAllSectorIndices();
	return createResultString(sectorIndices, 0, outputFormat);
}

string td::clear_access_count(const vector<string> &args, const ci_string &outputFormat)
{
	if(args.empty())throw FrontendException("\"clear_access_count\" needs the td device\n");

	tDisk d = tDisk::get(args[0]);
	d.clearAccessCount();
	return createResultString(concat("Access count for tDisk ", d.getName(), " cleared"), 0, outputFormat);
}

string td::get_internal_devices_count(const vector<string> &args, const ci_string &outputFormat)
{
	if(args.empty())throw FrontendException("\"get_internal_device_count\" needs the td device\n");

	tDisk d = tDisk::get(args[0]);
	tdisk_index devices = d.getInternalDevicesCount();
	return createResultString(devices, 0, outputFormat);
}

string td::get_device_info(const vector<string> &args, const ci_string &outputFormat)
{
	if(args.size() < 2)throw FrontendException("\"get_device_info\" needs the td device and the device to retrieve infos for\n");

	char *test;
	long device = strtol(args[1].c_str(), &test, 10);
	if(test != args[1].c_str() + args[1].length())
		throw FrontendException("Invalid number ", args[1], " for device index");

	if(device != (tdisk_index)device)
		throw FrontendException("Device index ", device, " exceeds limit of devices");

	tDisk d = tDisk::get(args[0]);
	internal_device_info info = d.getDeviceInfo((tdisk_index)device);
	return createResultString(info, 0, outputFormat);
}

string td::load_config_file(const vector<string> &args, const ci_string &outputFormat)
{
	if(args.empty())throw FrontendException("\"load_config_file\" needs at least one donfig file to load\n");

	configuration cfg;
	for(std::size_t i = 0; i < args.size(); ++i)
	{
		struct configuration config = loadConfiguration(args[i]);
		cfg.merge(config);
	}

	for(const tdisk_global_option &option : cfg.global_options)
		setOptionValue(option.name, option.value);

	for(const tdisk_config &config : cfg.tdisks)
	{
		tDisk disk;
		if(config.minornumber < 0)disk = tDisk::create(config.blocksize);
		else disk = tDisk::create(config.minornumber, config.blocksize);

		for(const string device : config.devices)
			disk.addDisk(device);
	}

	return createResultString(cfg, 0, outputFormat);
}

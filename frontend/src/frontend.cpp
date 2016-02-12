#include <typeinfo>

#include <frontend.hpp>
#include <resultformatter.hpp>
#include <tdisk.hpp>
#include <utils.hpp>

using std::string;
using std::vector;
namespace td
{

	template <> string createResultString(const sector_index &index, unsigned int hierarchy, const ci_string &outputFormat)
	{
		if(outputFormat == "json")
			return concat(
				"{\n",
					vector<char>(hierarchy+1, '\t'), CREATE_RESULT_STRING_MEMBER_JSON(index, disk, hierarchy+1, outputFormat), ",\n",
					vector<char>(hierarchy+1, '\t'), CREATE_RESULT_STRING_MEMBER_JSON(index, sector, hierarchy+1, outputFormat), ",\n",
					vector<char>(hierarchy+1, '\t'), CREATE_RESULT_STRING_MEMBER_JSON(index, access_count, hierarchy+1, outputFormat), "\n",
				vector<char>(hierarchy, '\t'), "}"
			);
		else if(outputFormat == "text")
			return concat(
					CREATE_RESULT_STRING_MEMBER_TEXT(index, disk, hierarchy+1, outputFormat), "\n",
					CREATE_RESULT_STRING_MEMBER_TEXT(index, sector, hierarchy+1, outputFormat), "\n",
					CREATE_RESULT_STRING_MEMBER_TEXT(index, access_count, hierarchy+1, outputFormat), "\n"
			);
		else
			throw FrontendException("Invalid output-format ", outputFormat);
	}

	template <> string createResultString(const sector_info &info, unsigned int hierarchy, const ci_string &outputFormat)
	{
		if(outputFormat == "json")
			return concat(
				"{\n",
					vector<char>(hierarchy+1, '\t'), CREATE_RESULT_STRING_MEMBER_JSON(info, logical_sector, hierarchy+1, outputFormat), ",\n",
					vector<char>(hierarchy+1, '\t'), CREATE_RESULT_STRING_MEMBER_JSON(info, access_sorted_index, hierarchy+1, outputFormat), ",\n",
					vector<char>(hierarchy+1, '\t'), CREATE_RESULT_STRING_MEMBER_JSON(info, physical_sector, hierarchy+1, outputFormat), "\n",
				vector<char>(hierarchy, '\t'), "}"
			);
		else if(outputFormat == "text")
			return concat(
					CREATE_RESULT_STRING_MEMBER_TEXT(info, logical_sector, hierarchy+1, outputFormat), "\n",
					CREATE_RESULT_STRING_MEMBER_TEXT(info, access_sorted_index, hierarchy+1, outputFormat), "\n",
					createResultString(info.physical_sector, hierarchy+1, outputFormat), "\n"
			);
		else
			throw FrontendException("Invalid output-format ", outputFormat);
	}

	template <> string createResultString(const device_performance &perf, unsigned int hierarchy, const ci_string &outputFormat)
	{
		double avg_read_dec		= (double)perf.mod_avg_read / (1 << MEASUE_RECORDS_SHIFT);
		double stdev_read_dec	= (double)perf.mod_stdev_write / (1 << MEASUE_RECORDS_SHIFT);
		double avg_write_dec	= (double)perf.mod_avg_write / (1 << MEASUE_RECORDS_SHIFT);
		double stdev_write_dec	= (double)perf.mod_stdev_write / (1 << MEASUE_RECORDS_SHIFT);

		double avg_read_time_cycles = (double)perf.avg_read_time_cycles			+ avg_read_dec;
		double stdev_read_time_cycles = (double)perf.stdev_read_time_cycles		+ stdev_read_dec;
		double avg_write_time_cycles = (double)perf.avg_write_time_cycles		+ avg_write_dec;
		double stdev_write_time_cycles = (double)perf.stdev_write_time_cycles	+ stdev_write_dec;

		if(outputFormat == "json")
			return concat(
				"{\n",
					vector<char>(hierarchy+1, '\t'), CREATE_RESULT_STRING_NONMEMBER_JSON(avg_read_time_cycles, hierarchy+1, outputFormat), ",\n",
					vector<char>(hierarchy+1, '\t'), CREATE_RESULT_STRING_NONMEMBER_JSON(stdev_read_time_cycles, hierarchy+1, outputFormat), ",\n",
					vector<char>(hierarchy+1, '\t'), CREATE_RESULT_STRING_NONMEMBER_JSON(avg_write_time_cycles, hierarchy+1, outputFormat), ",\n",
					vector<char>(hierarchy+1, '\t'), CREATE_RESULT_STRING_NONMEMBER_JSON(stdev_write_time_cycles, hierarchy+1, outputFormat), "\n",
				vector<char>(hierarchy, '\t'), "}"
			);
		else if(outputFormat == "text")
			return concat(
					CREATE_RESULT_STRING_NONMEMBER_TEXT(avg_read_time_cycles, hierarchy+1, outputFormat), "\n",
					CREATE_RESULT_STRING_NONMEMBER_TEXT(stdev_read_time_cycles, hierarchy+1, outputFormat), "\n",
					CREATE_RESULT_STRING_NONMEMBER_TEXT(avg_write_time_cycles, hierarchy+1, outputFormat), "\n",
					CREATE_RESULT_STRING_NONMEMBER_TEXT(stdev_write_time_cycles, hierarchy+1, outputFormat), "\n"
			);
		else
			throw FrontendException("Invalid output-format ", outputFormat);
	}

	template <> string createResultString(const internal_device_info &info, unsigned int hierarchy, const ci_string &outputFormat)
	{
		if(outputFormat == "json")
			return concat(
				"{\n",
					vector<char>(hierarchy+1, '\t'), CREATE_RESULT_STRING_MEMBER_JSON(info, disk, hierarchy+1, outputFormat), ",\n",
					vector<char>(hierarchy+1, '\t'), CREATE_RESULT_STRING_MEMBER_JSON(info, performance, hierarchy+1, outputFormat), "\n",
				vector<char>(hierarchy, '\t'), "}"
			);
		else if(outputFormat == "text")
			return concat(
					CREATE_RESULT_STRING_MEMBER_TEXT(info, disk, hierarchy+1, outputFormat), "\n",
					createResultString(info.performance, hierarchy+1, outputFormat), "\n"
			);
		else
			throw FrontendException("Invalid output-format ", outputFormat);
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

#ifndef TDISK_HPP
#define TDISK_HPP

#include <errno.h>
#include <string>
#include <string.h>
#include <vector>

#include <utils.hpp>
#include <resultformatter.hpp>

namespace td
{

namespace c
{
	extern "C"
	{
		#include "tdisk.h"
	}
} //end namespace c

struct tDiskException
{
	template <class ...T> tDiskException(T ...t) : message(concat(t...)) {}
	std::string message;
}; //end class tDiskException

using c::f_device_performance;
using c::f_internal_device_info;
using c::f_sector_index;
using c::f_sector_info;

class tDisk
{

public:
	template <template<class ...T> class cont>
	static void getTDisks(cont<std::string> &out)
	{
		std::vector<char*> buffer(100);
		for(char* &c : buffer)c = new char[256];

		int devices = c::tdisk_get_devices(&buffer[0], 256, 100);
		if(devices >= 0)
		{
			for(int i = 0; i < devices && i < (int)buffer.size(); ++i)
				out.push_back(buffer[i]);
		}

		for(char* &c : buffer)delete [] c;

		if(devices == -1)throw tDiskException("Unable to read /dev folder");
		if(devices == -2)throw tDiskException("More than 100 tDisks present");
	}

	static int getMinorNumber(const std::string &name)
	{
		if(name.length() < 7 || name.substr(0, 7) != "/dev/td")
			throw tDiskException("Invalid tDisk path ", name);

		char *test;
		int number = strtol(name.c_str() + 7, &test, 10);

		if(test != name.c_str() + name.length())
			throw tDiskException("Invalid tDisk ", name);

		return number;
	}

	static tDisk get(const std::string &str)
	{
		char *test;
		int number = strtol(str.c_str(), &test, 10);

		if(test == str.c_str() + str.length())return tDisk(number);
		else return tDisk(str);
	}

	static tDisk create(int i_minornumber, unsigned int blocksize)
	{
		char temp[256];
		int ret = c::tdisk_add_specific(temp, i_minornumber, blocksize);
		try {
			handleError(ret);
		} catch(const tDiskException &e) {
			throw tDiskException("Unable to create new tDisk ", i_minornumber, ": ", e.message);
		}

		if(ret != i_minornumber)
			throw tDiskException("tDisk was created but not the desired one. Don't know why... Requested: ", i_minornumber, ", Created: ", ret);

		return tDisk(i_minornumber, temp);
	}

	static tDisk create(unsigned int blocksize)
	{
		char temp[256];
		int ret = c::tdisk_add(temp, blocksize);
		try {
			handleError(ret);
		} catch(const tDiskException &e) {
			throw tDiskException("Unable to create new tDisk: ", e.message);
		}

		return tDisk(ret, temp);
	}

	static void remove(int i_minornumber)
	{
		int ret = c::tdisk_remove(i_minornumber);

		try {
			handleError(ret);
		} catch(const tDiskException &e) {
			throw tDiskException("Unable to remove tDisk: ", e.message);
		}
	}

	static void remove(const std::string &name)
	{
		int number = tDisk::getMinorNumber(name);
		int ret = c::tdisk_remove(number);

		try {
			handleError(ret);
		} catch(const tDiskException &e) {
			throw tDiskException("Unable to remove tDisk: ", e.message);
		}
	}

	static void remove(const tDisk &disk)
	{
		remove(disk.minornumber);
	}

private:
	static void handleError(int ret)
	{
		if(ret == 0)return;

		switch(ret)
		{
		default:
			throw tDiskException(strerror(errno));
		}
	}

public:
	tDisk() :
		minornumber(),
		name()
	{}

	tDisk(int i_minornumber) :
		minornumber(i_minornumber),
		name(concat("/dev/td", minornumber))
	{}

	tDisk(const std::string &str_name) :
		minornumber(getMinorNumber(str_name)),
		name(str_name)
	{}

	tDisk(int i_minornumber, const std::string &str_name) :
		minornumber(i_minornumber),
		name(str_name)
	{}

	std::string getName() const
	{
		return name;
	}

	int getMinornumber() const
	{
		return minornumber;
	}

	void remove()
	{
		tDisk::remove(*this);
	}

	void addDisk(const std::string &path)
	{
		int ret = c::tdisk_add_disk(name.c_str(), path.c_str());

		try {
			handleError(ret);
		} catch (const tDiskException &e) {
			throw tDiskException("Can't add disk \"", path ,"\" to tDisk ", name, ": ", e.message);
		}
	}

	unsigned long long getMaxSectors() const
	{
		uint64_t maxSectors;
		int ret = c::tdisk_get_max_sectors(name.c_str(), &maxSectors);

		try {
			handleError(ret);
		} catch (const tDiskException &e) {
			throw tDiskException("Can't get max sectors for tDisk ", name, ": ", e.message);
		}

		return maxSectors;
	}

	f_sector_index getSectorIndex(unsigned long long logicalSector)
	{
		f_sector_index index;
		int ret = c::tdisk_get_sector_index(name.c_str(), logicalSector, &index);

		try {
			handleError(ret);
		} catch (const tDiskException &e) {
			throw tDiskException("Can't get sector index ", logicalSector, " for tDisk ", name, ": ", e.message);
		}

		return std::move(index);
	}

	std::vector<f_sector_info> getAllSectorIndices() const
	{
		unsigned long long max_sectors = getMaxSectors();

		std::vector<f_sector_info> indices((std::size_t)max_sectors);
		int ret = c::tdisk_get_all_sector_indices(name.c_str(), &indices[0], max_sectors);
		
		try {
			handleError(ret);
		} catch (const tDiskException &e) {
			throw tDiskException("Can't all sector indices for tDisk ", name, ": ", e.message);
		}

		return std::move(indices);
	}

	void clearAccessCount()
	{
		int ret = c::tdisk_clear_access_count(name.c_str());

		try {
			handleError(ret);
		} catch (const tDiskException &e) {
			throw tDiskException("Can't clear access count for tDisk ", name, ": ", e.message);
		}
	}

	unsigned int getInternalDevicesCount() const
	{
		unsigned int devices;
		int ret = c::tdisk_get_internal_devices_count(name.c_str(), &devices);

		try {
			handleError(ret);
		} catch (const tDiskException &e) {
			throw tDiskException("Can't get internal devices count for tDisk ", name, ": ", e.message);
		}

		return devices;
	}

	f_internal_device_info getDeviceInfo(unsigned int device) const
	{
		f_internal_device_info info;
		int ret = tdisk_get_device_info(name.c_str(), device, &info);

		try {
			handleError(ret);
		} catch (const tDiskException &e) {
			throw tDiskException("Can't get device info for device ", (int)device, " for tDisk ", name, ": ", e.message);
		}

		return info;
	}

private:
	int minornumber;
	std::string name;

}; //end class tDisk

template <> inline std::string createResultString(const f_sector_index &index, unsigned int hierarchy, const ci_string &outputFormat)
{
	if(outputFormat == "json")
		return concat(
			"{\n",
				std::vector<char>(hierarchy+1, '\t'), CREATE_RESULT_STRING_MEMBER_JSON(index, disk, hierarchy+1, outputFormat), ",\n",
				std::vector<char>(hierarchy+1, '\t'), CREATE_RESULT_STRING_MEMBER_JSON(index, sector, hierarchy+1, outputFormat), ",\n",
				std::vector<char>(hierarchy+1, '\t'), CREATE_RESULT_STRING_MEMBER_JSON(index, access_count, hierarchy+1, outputFormat), "\n",
			std::vector<char>(hierarchy, '\t'), "}"
		);
	else if(outputFormat == "text")
		return concat(
				CREATE_RESULT_STRING_MEMBER_TEXT(index, disk, hierarchy+1, outputFormat), "\n",
				CREATE_RESULT_STRING_MEMBER_TEXT(index, sector, hierarchy+1, outputFormat), "\n",
				CREATE_RESULT_STRING_MEMBER_TEXT(index, access_count, hierarchy+1, outputFormat), "\n"
		);
	else
		throw FormatException("Invalid output-format ", outputFormat);
}

template <> inline std::string createResultString(const f_sector_info &info, unsigned int hierarchy, const ci_string &outputFormat)
{
	if(outputFormat == "json")
		return concat(
			"{\n",
				std::vector<char>(hierarchy+1, '\t'), CREATE_RESULT_STRING_MEMBER_JSON(info, logical_sector, hierarchy+1, outputFormat), ",\n",
				std::vector<char>(hierarchy+1, '\t'), CREATE_RESULT_STRING_MEMBER_JSON(info, access_sorted_index, hierarchy+1, outputFormat), ",\n",
				std::vector<char>(hierarchy+1, '\t'), CREATE_RESULT_STRING_MEMBER_JSON(info, physical_sector, hierarchy+1, outputFormat), "\n",
			std::vector<char>(hierarchy, '\t'), "}"
		);
	else if(outputFormat == "text")
		return concat(
				CREATE_RESULT_STRING_MEMBER_TEXT(info, logical_sector, hierarchy+1, outputFormat), "\n",
				CREATE_RESULT_STRING_MEMBER_TEXT(info, access_sorted_index, hierarchy+1, outputFormat), "\n",
				createResultString(info.physical_sector, hierarchy+1, outputFormat), "\n"
		);
	else
		throw FormatException("Invalid output-format ", outputFormat);
}

template <> inline std::string createResultString(const f_device_performance &perf, unsigned int hierarchy, const ci_string &outputFormat)
{
	double avg_read_dec		= (double)perf.mod_avg_read / (1 << c::get_measure_records_shift());
	double stdev_read_dec	= (double)perf.mod_stdev_write / (1 << c::get_measure_records_shift());
	double avg_write_dec	= (double)perf.mod_avg_write / (1 << c::get_measure_records_shift());
	double stdev_write_dec	= (double)perf.mod_stdev_write / (1 << c::get_measure_records_shift());

	double avg_read_time_cycles = (double)perf.avg_read_time_cycles			+ avg_read_dec;
	double stdev_read_time_cycles = (double)perf.stdev_read_time_cycles		+ stdev_read_dec;
	double avg_write_time_cycles = (double)perf.avg_write_time_cycles		+ avg_write_dec;
	double stdev_write_time_cycles = (double)perf.stdev_write_time_cycles	+ stdev_write_dec;

	if(outputFormat == "json")
		return concat(
			"{\n",
				std::vector<char>(hierarchy+1, '\t'), CREATE_RESULT_STRING_NONMEMBER_JSON(avg_read_time_cycles, hierarchy+1, outputFormat), ",\n",
				std::vector<char>(hierarchy+1, '\t'), CREATE_RESULT_STRING_NONMEMBER_JSON(stdev_read_time_cycles, hierarchy+1, outputFormat), ",\n",
				std::vector<char>(hierarchy+1, '\t'), CREATE_RESULT_STRING_NONMEMBER_JSON(avg_write_time_cycles, hierarchy+1, outputFormat), ",\n",
				std::vector<char>(hierarchy+1, '\t'), CREATE_RESULT_STRING_NONMEMBER_JSON(stdev_write_time_cycles, hierarchy+1, outputFormat), "\n",
			std::vector<char>(hierarchy, '\t'), "}"
		);
	else if(outputFormat == "text")
		return concat(
				CREATE_RESULT_STRING_NONMEMBER_TEXT(avg_read_time_cycles, hierarchy+1, outputFormat), "\n",
				CREATE_RESULT_STRING_NONMEMBER_TEXT(stdev_read_time_cycles, hierarchy+1, outputFormat), "\n",
				CREATE_RESULT_STRING_NONMEMBER_TEXT(avg_write_time_cycles, hierarchy+1, outputFormat), "\n",
				CREATE_RESULT_STRING_NONMEMBER_TEXT(stdev_write_time_cycles, hierarchy+1, outputFormat), "\n"
		);
	else
		throw FormatException("Invalid output-format ", outputFormat);
}

template <> inline std::string createResultString(const f_internal_device_info &info, unsigned int hierarchy, const ci_string &outputFormat)
{
	if(outputFormat == "json")
		return concat(
			"{\n",
				std::vector<char>(hierarchy+1, '\t'), CREATE_RESULT_STRING_MEMBER_JSON(info, disk, hierarchy+1, outputFormat), ",\n",
				std::vector<char>(hierarchy+1, '\t'), CREATE_RESULT_STRING_MEMBER_JSON(info, performance, hierarchy+1, outputFormat), "\n",
			std::vector<char>(hierarchy, '\t'), "}"
		);
	else if(outputFormat == "text")
		return concat(
				CREATE_RESULT_STRING_MEMBER_TEXT(info, disk, hierarchy+1, outputFormat), "\n",
				createResultString(info.performance, hierarchy+1, outputFormat), "\n"
		);
	else
		throw FormatException("Invalid output-format ", outputFormat);
}

}

#endif //TDISK_HPP

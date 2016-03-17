/**
  *
  * tDisk Driver
  * @author Thomas Sparber (2015)
  *
 **/

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

/**
  * Is thrown whenever a tDisk related error happened
 **/
struct tDiskException
{

	/**
	  * Constructs a tDiskException using any number of arguments
	 **/
	template <class ...T>
	tDiskException(T ...t) :
		message(utils::concat(t...))
	{}
	
	/**
	  * The reason for the error
	 **/
	std::string message;
	
}; //end class tDiskException

using c::f_device_performance;
using c::f_internal_device_info;
using c::f_internal_device_type;
using c::f_sector_index;
using c::f_sector_info;

/**
  * Represents the C++ interface to manage tDisks.
  * It basically just calls the corresponding C functions which
  * "talk" to the kernel module
 **/
class tDisk
{

public:

	/**
	  * Gets all currently loaded tDisks
	  * @param out The container where the disks should be stored
	 **/
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

	/**
	  * Gets the minornumber of the tDisk with the given path
	  * @param name The path of the tDisk to get the minornumber 
	  * @returns The minro number of the tDisk
	 **/
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

	/**
	  * Returns the tDisk for the gven name or minronumber
	  * @param str Can be the path or minornumber
	  * @returns The matching tDisk
	 **/
	static tDisk get(const std::string &str)
	{
		char *test;
		int number = strtol(str.c_str(), &test, 10);

		if(test == str.c_str() + str.length())return tDisk(number);
		else return tDisk(str);
	}

	/**
	  * Creates a new tDIsk with the given minornumber and blocksize
	  * @param i_minornumber The minornumber for the new tDisk
	  * @param blocksize The blocksize for the new tDisk
	  * @returns The newly created tDisk
	 **/
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

	/**
	  * Creates a new tDisk with the given blocksize. The next available
	  * minornumber is taken
	  * @param blocksize The blocksize for the new tDisk
	  * @returns The new tDisk
	 **/
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

	/**
	  * Removes the tDisk with the given minornumber from the system
	  * @param i_minornumber The minornumber of thetDisk to be removed
	 **/
	static void remove(int i_minornumber)
	{
		int ret = c::tdisk_remove(i_minornumber);

		try {
			handleError(ret);
		} catch(const tDiskException &e) {
			throw tDiskException("Unable to remove tDisk: ", e.message);
		}
	}

	/**
	  * Removes the tDisk with the given path from the system
	  * @param name The path of the tDisk to be removed
	 **/
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

	/**
	  * Removes the given tDisk from the system
	  * @param disk The disk to be removed 
	 **/
	static void remove(const tDisk &disk)
	{
		remove(disk.minornumber);
	}

private:

	/**
	  * Handles the error of the C-functions and throws the
	  * corresponding tDiskException
	  * @param ret The return code of the C function
	 **/
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

	/**
	  * Default constructor
	 **/
	tDisk() :
		minornumber(),
		name()
	{}

	/**
	  * Constructs a tDisk using the given minornumber
	 **/
	tDisk(int i_minornumber) :
		minornumber(i_minornumber),
		name(utils::concat("/dev/td", minornumber))
	{}

	/**
	  * Constructs a tDisk using the given path
	 **/
	tDisk(const std::string &str_name) :
		minornumber(getMinorNumber(str_name)),
		name(str_name)
	{}

	/**
	  * Constructs a tDisk using the given path and minornumber
	 **/
	tDisk(int i_minornumber, const std::string &str_name) :
		minornumber(i_minornumber),
		name(str_name)
	{}

	/**
	  * Returns the name/path of the tDisk
	 **/
	std::string getName() const
	{
		return name;
	}

	/**
	  * Returns the minornumber of the tDisk
	 **/
	int getMinornumber() const
	{
		return minornumber;
	}

	/**
	  * Removes the tDisk from the system
	 **/
	void remove()
	{
		tDisk::remove(*this);
	}

	/**
	  * Adds the given internal disk to the tDisk
	  * @param path The disk to be added. If the file doesn't exist it
	  * is treated as a plugin name
	 **/
	void addDisk(const std::string &path)
	{
		int ret = c::tdisk_add_disk(name.c_str(), path.c_str());

		try {
			handleError(ret);
		} catch (const tDiskException &e) {
			throw tDiskException("Can't add disk \"", path ,"\" to tDisk ", name, ": ", e.message);
		}
	}

	/**
	  * Returns the current amount of max sectors
	 **/
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

	/**
	  * Returns information about the sector with the given logical
	  * sector
	 **/
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

	/**
	  * Return information about all logical sectors
	 **/
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

	/**
	  * Clears the access count of all sectors
	 **/
	void clearAccessCount()
	{
		int ret = c::tdisk_clear_access_count(name.c_str());

		try {
			handleError(ret);
		} catch (const tDiskException &e) {
			throw tDiskException("Can't clear access count for tDisk ", name, ": ", e.message);
		}
	}

	/**
	  * Returns the amount of current internal devices
	 **/
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

	/**
	  * Returns information about the internal device with the given
	  * device id
	 **/
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

	/**
	  * The minor number of the tDisk
	 **/
	int minornumber;
	
	/**
	  * The path of the tDisk
	 **/
	std::string name;

}; //end class tDisk

/**
  * Stringifies the given f_sector_index using the given format
 **/
template <> inline std::string createResultString(const f_sector_index &index, unsigned int hierarchy, const utils::ci_string &outputFormat)
{
	if(outputFormat == "json")
		return utils::concat(
			"{\n",
				std::vector<char>(hierarchy+1, '\t'), CREATE_RESULT_STRING_MEMBER_JSON(index, disk, hierarchy+1, outputFormat), ",\n",
				std::vector<char>(hierarchy+1, '\t'), CREATE_RESULT_STRING_MEMBER_JSON(index, sector, hierarchy+1, outputFormat), ",\n",
				std::vector<char>(hierarchy+1, '\t'), CREATE_RESULT_STRING_MEMBER_JSON(index, access_count, hierarchy+1, outputFormat), "\n",
			std::vector<char>(hierarchy, '\t'), "}"
		);
	else if(outputFormat == "text")
		return utils::concat(
				CREATE_RESULT_STRING_MEMBER_TEXT(index, disk, hierarchy+1, outputFormat), "\n",
				CREATE_RESULT_STRING_MEMBER_TEXT(index, sector, hierarchy+1, outputFormat), "\n",
				CREATE_RESULT_STRING_MEMBER_TEXT(index, access_count, hierarchy+1, outputFormat), "\n"
		);
	else
		throw FormatException("Invalid output-format ", outputFormat);
}

/**
  * Stringifies the given f_sector_info using the given format
 **/
template <> inline std::string createResultString(const f_sector_info &info, unsigned int hierarchy, const utils::ci_string &outputFormat)
{
	if(outputFormat == "json")
		return utils::concat(
			"{\n",
				std::vector<char>(hierarchy+1, '\t'), CREATE_RESULT_STRING_MEMBER_JSON(info, logical_sector, hierarchy+1, outputFormat), ",\n",
				std::vector<char>(hierarchy+1, '\t'), CREATE_RESULT_STRING_MEMBER_JSON(info, access_sorted_index, hierarchy+1, outputFormat), ",\n",
				std::vector<char>(hierarchy+1, '\t'), CREATE_RESULT_STRING_MEMBER_JSON(info, physical_sector, hierarchy+1, outputFormat), "\n",
			std::vector<char>(hierarchy, '\t'), "}"
		);
	else if(outputFormat == "text")
		return utils::concat(
				CREATE_RESULT_STRING_MEMBER_TEXT(info, logical_sector, hierarchy+1, outputFormat), "\n",
				CREATE_RESULT_STRING_MEMBER_TEXT(info, access_sorted_index, hierarchy+1, outputFormat), "\n",
				createResultString(info.physical_sector, hierarchy+1, outputFormat), "\n"
		);
	else
		throw FormatException("Invalid output-format ", outputFormat);
}

/**
  * Stringifies the given f_device_performance using the given format
 **/
template <> inline std::string createResultString(const f_device_performance &perf, unsigned int hierarchy, const utils::ci_string &outputFormat)
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
		return utils::concat(
			"{\n",
				std::vector<char>(hierarchy+1, '\t'), CREATE_RESULT_STRING_NONMEMBER_JSON(avg_read_time_cycles, hierarchy+1, outputFormat), ",\n",
				std::vector<char>(hierarchy+1, '\t'), CREATE_RESULT_STRING_NONMEMBER_JSON(stdev_read_time_cycles, hierarchy+1, outputFormat), ",\n",
				std::vector<char>(hierarchy+1, '\t'), CREATE_RESULT_STRING_NONMEMBER_JSON(avg_write_time_cycles, hierarchy+1, outputFormat), ",\n",
				std::vector<char>(hierarchy+1, '\t'), CREATE_RESULT_STRING_NONMEMBER_JSON(stdev_write_time_cycles, hierarchy+1, outputFormat), "\n",
			std::vector<char>(hierarchy, '\t'), "}"
		);
	else if(outputFormat == "text")
		return utils::concat(
				CREATE_RESULT_STRING_NONMEMBER_TEXT(avg_read_time_cycles, hierarchy+1, outputFormat), "\n",
				CREATE_RESULT_STRING_NONMEMBER_TEXT(stdev_read_time_cycles, hierarchy+1, outputFormat), "\n",
				CREATE_RESULT_STRING_NONMEMBER_TEXT(avg_write_time_cycles, hierarchy+1, outputFormat), "\n",
				CREATE_RESULT_STRING_NONMEMBER_TEXT(stdev_write_time_cycles, hierarchy+1, outputFormat), "\n"
		);
	else
		throw FormatException("Invalid output-format ", outputFormat);
}

/**
  * Stringifies the given f_internal_device_info using the given format
 **/
template <> inline std::string createResultString(const enum f_internal_device_type &type, unsigned int hierarchy, const utils::ci_string &outputFormat)
{
	switch(type)
	{
	case c::f_internal_device_type_file:
		return createResultString("file", hierarchy, outputFormat);
	case c::f_internal_device_type_plugin:
		return createResultString("plugin", hierarchy, outputFormat);
	}

	throw FormatException("Undefined enum value ", type, " for f_internal_device_type");
}

/**
  * Stringifies the given f_internal_device_info using the given format
 **/
template <> inline std::string createResultString(const f_internal_device_info &info, unsigned int hierarchy, const utils::ci_string &outputFormat)
{
	if(outputFormat == "json")
		return utils::concat(
			"{\n",
				std::vector<char>(hierarchy+1, '\t'), CREATE_RESULT_STRING_MEMBER_JSON(info, disk, hierarchy+1, outputFormat), ",\n",
				std::vector<char>(hierarchy+1, '\t'), CREATE_RESULT_STRING_MEMBER_JSON(info, name, hierarchy+1, outputFormat), ",\n",
				std::vector<char>(hierarchy+1, '\t'), CREATE_RESULT_STRING_MEMBER_JSON(info, type, hierarchy+1, outputFormat), ",\n",
				std::vector<char>(hierarchy+1, '\t'), CREATE_RESULT_STRING_MEMBER_JSON(info, performance, hierarchy+1, outputFormat), "\n",
			std::vector<char>(hierarchy, '\t'), "}"
		);
	else if(outputFormat == "text")
		return utils::concat(
				CREATE_RESULT_STRING_MEMBER_TEXT(info, disk, hierarchy+1, outputFormat), "\n",
				CREATE_RESULT_STRING_MEMBER_TEXT(info, name, hierarchy+1, outputFormat), "\n",
				CREATE_RESULT_STRING_MEMBER_TEXT(info, type, hierarchy+1, outputFormat), "\n",
				createResultString(info.performance, hierarchy+1, outputFormat), "\n"
		);
	else
		throw FormatException("Invalid output-format ", outputFormat);
}

}

#endif //TDISK_HPP

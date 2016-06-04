/**
  *
  * tDisk backend
  * @author Thomas Sparber (2015-2016)
  *
 **/

#ifndef TDISK_HPP
#define TDISK_HPP

#include <algorithm>
#include <errno.h>
#include <string>
#include <string.h>
#include <vector>

#include <convert.hpp>
#include <filesystem.hpp>
#include <logger.hpp>
#include <performance.hpp>
#include <performanceimprovement.hpp>
#include <resultformatter.hpp>
#include <tdiskexception.hpp>
#include <tdiskofflineexception.hpp>
#include <utils.hpp>

namespace td
{

namespace c
{
	extern "C"
	{
		#include "tdisk.h"
	}
} //end namespace c

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
	  * Gets all currently loaded tDisks
	  * @param out The container where the disks should be stored
	 **/
	template <template<class ...T> class cont>
	static void getTDisks(cont<tDisk> &out)
	{
		std::string error;
		std::string offline;
		std::vector<char*> buffer(100);
		for(char* &c : buffer)c = new char[256];

		int devices = c::tdisk_get_devices(&buffer[0], 256, 100);
		if(devices >= 0)
		{
			for(int i = 0; i < devices && i < (int)buffer.size(); ++i)
			{
				try {
					tDisk loaded(utils::concat("/dev/", &buffer[i][0]));
					loaded.loadSize();
					out.push_back(std::move(loaded));
				} catch (const tDiskOfflineException &e) {
					offline = utils::concat("Unable to load disk ", &buffer[i][0], ": ", e.what());
					break;
				} catch (const tDiskException &e) {
					error = utils::concat("Unable to load disk ", &buffer[i][0], ": ", e.what());
					break;
				}
			}
		}

		for(char* &c : buffer)delete [] c;

		if(error != "")throw tDiskException("Unable to load all tDisks: ", error);
		if(offline != "")throw tDiskOfflineException("Unable to load all tDisks: ", offline);
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

		int number;
		if(!utils::convertTo(name.substr(7), number))
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
		int number;
		if(utils::convertTo(str, number))return tDisk(number);
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
		} catch(const tDiskOfflineException &e) {
			throw tDiskOfflineException("Unable to create new tDisk ", i_minornumber, ": ", e.what());
		} catch(const tDiskException &e) {
			throw tDiskException("Unable to create new tDisk ", i_minornumber, ": ", e.what());
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
		} catch(const tDiskOfflineException &e) {
			throw tDiskOfflineException("Unable to create new tDisk: ", e.what());
		} catch(const tDiskException &e) {
			throw tDiskException("Unable to create new tDisk: ", e.what());
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
		} catch(const tDiskOfflineException &e) {
			throw tDiskOfflineException("Unable to remove tDisk: ", e.what());
		} catch(const tDiskException &e) {
			throw tDiskException("Unable to remove tDisk: ", e.what());
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
		} catch(const tDiskOfflineException &e) {
			throw tDiskOfflineException("Unable to remove tDisk: ", e.what());
		} catch(const tDiskException &e) {
			throw tDiskException("Unable to remove tDisk: ", e.what());
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
		if(ret >= 0)return;

		switch(-ret)
		{
		case ENODEV:
			throw tDiskOfflineException("The driver is offline");
		default:
			throw tDiskException(strerror(errno));
		}
	}

public:

	/**
	  * Default constructor
	 **/
	tDisk() :
		minornumber(-1),
		name(),
		size(),
		online()
	{}

	/**
	  * Constructs a tDisk using the given minornumber
	 **/
	tDisk(int i_minornumber) :
		minornumber(i_minornumber),
		name(utils::concat("/dev/td", minornumber)),
		size(),
		online()
	{}

	/**
	  * Constructs a tDisk using the given path
	 **/
	tDisk(const std::string &str_name) :
		minornumber(getMinorNumber(str_name)),
		name(str_name),
		size(),
		online()
	{}

	/**
	  * Constructs a tDisk using the given path and minornumber
	 **/
	tDisk(int i_minornumber, const std::string &str_name) :
		minornumber(i_minornumber),
		name(str_name),
		size(),
		online()
	{}

	/**
	  * Checks if the current tDisk equals the given tDisk
	 **/
	bool operator== (const tDisk &other) const
	{
		return (minornumber == other.minornumber);
	}

	bool isValid() const
	{
		return (minornumber != -1);
	}

	/**
	  * Returns the name/path of the tDisk
	 **/
	std::string getName() const
	{
		return name;
	}

	/**
	  * Returns the path of the tDisk
	 **/
	std::string getPath() const
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
	  * Returns whether the tDisk is online or not
	 **/
	bool isOnline() const
	{
		return online;
	}

	/**
	  * Sets the online status of the tDisk
	 **/
	void setOnline(bool b_online)
	{
		this->online = b_online;
	}

	/**
	  * Checks whether the tDisk is online and
	  * returns the status
	 **/
	bool checkOnline() const
	{
		const std::string deviceName = utils::concat("td", minornumber);
		std::vector<std::string> disks;
		tDisk::getTDisks(disks);

		online = false;
		for(const std::string &disk : disks)
		{
			if(disk == deviceName)
			{
				online = true;
				break;
			}
		}

		return online;
	}

	/**
	  * Removes the tDisk from the system
	 **/
	void remove()
	{
		tDisk::remove(*this);
		online = false;
	}

	/**
	  * Loads the size of the tDisk
	 **/
	void loadSize()
	{
		this->size = getSize();
		online = true;
	}

	/**
	  * Adds the given internal disk to the tDisk
	  * @param path The disk to be added. If the file doesn't exist it
	  * is treated as a plugin name
	 **/
	void addDisk(const std::string &path, bool format)
	{
		int ret = c::tdisk_add_disk(name.c_str(), path.c_str(), format);

		try {
			handleError(ret);
		} catch (const tDiskOfflineException &e) {
			throw tDiskOfflineException("Can't add disk \"", path ,"\" to tDisk ", name, ": ", e.what());
		} catch (const tDiskException &e) {
			throw tDiskException("Can't add disk \"", path ,"\" to tDisk ", name, ": ", e.what());
		}
		online = true;
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
		} catch (const tDiskOfflineException &e) {
			throw tDiskOfflineException("Can't get max sectors for tDisk ", name, ": ", e.what());
		} catch (const tDiskException &e) {
			throw tDiskException("Can't get max sectors for tDisk ", name, ": ", e.what());
		}

		online = true;
		return maxSectors;
	}

	/**
	  * Returns the current size in bytes
	 **/
	unsigned long long getSize() const
	{
		uint64_t sizeBytes;
		int ret = c::tdisk_get_size_bytes(name.c_str(), &sizeBytes);

		try {
			handleError(ret);
		} catch (const tDiskOfflineException &e) {
			throw tDiskOfflineException("Can't get size for tDisk ", name, ": ", e.what());
		} catch (const tDiskException &e) {
			throw tDiskException("Can't get size for tDisk ", name, ": ", e.what());
		}

		online = true;
		return sizeBytes;
	}

	/**
	  * Returns the current size in bytes
	 **/
	unsigned int getBlocksize() const
	{
		uint32_t blocksize;
		int ret = c::tdisk_get_blocksize(name.c_str(), &blocksize);

		try {
			handleError(ret);
		} catch (const tDiskOfflineException &e) {
			throw tDiskOfflineException("Can't get blocksize for tDisk ", name, ": ", e.what());
		} catch (const tDiskException &e) {
			throw tDiskException("Can't get blocksize for tDisk ", name, ": ", e.what());
		}

		online = true;
		return blocksize;
	}

	/**
	  * Returns information about the sector with the given logical
	  * sector
	 **/
	f_sector_index getSectorIndex(unsigned long long logicalSector) const
	{
		f_sector_index index;
		int ret = c::tdisk_get_sector_index(name.c_str(), logicalSector, &index);

		try {
			handleError(ret);
		} catch (const tDiskOfflineException &e) {
			throw tDiskOfflineException("Can't get sector index ", logicalSector, " for tDisk ", name, ": ", e.what());
		} catch (const tDiskException &e) {
			throw tDiskException("Can't get sector index ", logicalSector, " for tDisk ", name, ": ", e.what());
		}

		online = true;
		return std::move(index);
	}

	/**
	  * Return information about all logical sectors
	 **/
	std::vector<f_sector_info> getAllSectorIndices() const
	{
		unsigned long long max_sectors = getMaxSectors();

		performance::start("getAllSectorIndices");
		std::vector<f_sector_info> indices((std::size_t)max_sectors);
		int ret = c::tdisk_get_all_sector_indices(name.c_str(), &indices[0], max_sectors);
		performance::stop("getAllSectorIndices");

		try {
			handleError(ret);
		} catch (const tDiskOfflineException &e) {
			throw tDiskOfflineException("Can't all sector indices for tDisk ", name, ": ", e.what());
		} catch (const tDiskException &e) {
			throw tDiskException("Can't all sector indices for tDisk ", name, ": ", e.what());
		}

		online = true;
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
		} catch (const tDiskOfflineException &e) {
			throw tDiskOfflineException("Can't clear access count for tDisk ", name, ": ", e.what());
		} catch (const tDiskException &e) {
			throw tDiskException("Can't clear access count for tDisk ", name, ": ", e.what());
		}

		online = true;
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
		} catch (const tDiskOfflineException &e) {
			throw tDiskOfflineException("Can't get internal devices count for tDisk ", name, ": ", e.what());
		} catch (const tDiskException &e) {
			throw tDiskException("Can't get internal devices count for tDisk ", name, ": ", e.what());
		}

		online = true;
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
		} catch (const tDiskOfflineException &e) {
			throw tDiskOfflineException("Can't get device info for device ", (int)device, " for tDisk ", name, ": ", e.what());
		} catch (const tDiskException &e) {
			throw tDiskException("Can't get device info for device ", (int)device, " for tDisk ", name, ": ", e.what());
		}

		online = true;
		return info;
	}

	/**
	  * Returns all files which are stored on the given internal device
	 **/
	std::vector<FileAssignment> getFilesOnDevice(unsigned int device, bool getPercentages, bool filesOnly)
	{
		std::vector<f_sector_info> sectorIndices = getAllSectorIndices();

		unsigned int blocksize = getBlocksize();
		std::vector<std::pair<unsigned long long,unsigned long long> > positions;
		for(const f_sector_info &info : sectorIndices)
		{
			if(info.physical_sector.disk == device)
				positions.push_back(std::make_pair(info.logical_sector*blocksize, (info.logical_sector+1)*blocksize));
		}

		std::vector<FileAssignment> files = fs::getFilesOnDisk(getPath(), positions, getPercentages, filesOnly);

		performance::start("sortFiles");
		std::sort(files.begin(), files.end(), [](const FileAssignment &a, const FileAssignment &b)
		{
			return a.percentage > b.percentage;
		});
		performance::stop("sortFiles");

		return std::move(files);
	}

	tDiskPerformanceImprovement getPerformanceImprovement(std::size_t amountFiles)
	{
		unsigned int fastestDevice = 1;
		double bestPerformance = 999999999;
		long double avgPerformance = 0;
		const unsigned int devices = getInternalDevicesCount();
		for(unsigned int device = 1; device <= devices; ++device)
		{
			f_internal_device_info info = getDeviceInfo(device);

			double avg_read_dec = (double)info.performance.mod_avg_read / (1 << c::get_measure_records_shift());
			double avg_write_dec = (double)info.performance.mod_avg_write / (1 << c::get_measure_records_shift());

			double avg_read_time_cycles = (double)info.performance.avg_read_time_cycles			+ avg_read_dec;
			double avg_write_time_cycles = (double)info.performance.avg_write_time_cycles		+ avg_write_dec;

			double performance = avg_read_time_cycles + avg_write_time_cycles;
			if(performance < bestPerformance)
			{
				bestPerformance = performance;
				fastestDevice = device;
			}

			avgPerformance += performance;
		}
		avgPerformance /= devices;

		std::vector<FileAssignment> files = getFilesOnDevice(fastestDevice, true, true);

		long double avgPercentage = 0;
		for(const FileAssignment &f : files)
		{
			avgPercentage += f.percentage;
		}
		avgPercentage /= files.size();

		long double performanceDifference = (avgPerformance - bestPerformance) / avgPerformance;

		tDiskPerformanceImprovement p((double)(performanceDifference * avgPercentage));
		for(std::size_t i = 0; i < amountFiles && i < files.size(); ++i)
		{
			const FileAssignment &f = files[i];
			p.addFile(f.filename, (double)(f.percentage * performanceDifference));
		}

		return std::move(p);
	}

	friend void createResultString<tDisk>(std::ostream &ss, const tDisk &disk, unsigned int hierarchy, const utils::ci_string &outputFormat);

private:

	/**
	  * The minor number of the tDisk
	 **/
	int minornumber;
	
	/**
	  * The path of the tDisk
	 **/
	std::string name;

	/**
	  * The size in bytes of the tDisk
	 **/
	uint64_t size;

	/**
	  * A flag whether the tDisk is currently
	  * online
	 **/
	mutable bool online;

}; //end class tDisk

/**
  * Stringifies the given f_sector_index using the given format
 **/
template <> inline void createResultString(std::ostream &ss, const f_sector_index &index, unsigned int hierarchy, const utils::ci_string &outputFormat)
{
	bool used = index.used;

	if(outputFormat == "json")
	{
		ss<<"{\n";
			insertTab(ss, hierarchy+1); CREATE_RESULT_STRING_MEMBER_JSON(ss, index, disk, hierarchy+1, outputFormat); ss<<",\n";
			insertTab(ss, hierarchy+1); CREATE_RESULT_STRING_MEMBER_JSON(ss, index, sector, hierarchy+1, outputFormat); ss<<",\n";
			insertTab(ss, hierarchy+1); CREATE_RESULT_STRING_NONMEMBER_JSON(ss, used, hierarchy+1, outputFormat); ss<<",\n";
			insertTab(ss, hierarchy+1); CREATE_RESULT_STRING_MEMBER_JSON(ss, index, access_count, hierarchy+1, outputFormat); ss<<"\n";
		insertTab(ss, hierarchy); ss<<"}";
	}
	else if(outputFormat == "text")
	{
		CREATE_RESULT_STRING_MEMBER_TEXT(ss, index, disk, hierarchy+1, outputFormat); ss<<"\n";
		CREATE_RESULT_STRING_MEMBER_TEXT(ss, index, sector, hierarchy+1, outputFormat); ss<<"\n";
		CREATE_RESULT_STRING_NONMEMBER_TEXT(ss, used, hierarchy+1, outputFormat); ss<<"\n";
		CREATE_RESULT_STRING_MEMBER_TEXT(ss, index, access_count, hierarchy+1, outputFormat); ss<<"\n";
	}
	else
		throw FormatException("Invalid output-format ", outputFormat);
}

/**
  * Stringifies the given f_sector_info using the given format
 **/
template <> inline void createResultString(std::ostream &ss, const f_sector_info &info, unsigned int hierarchy, const utils::ci_string &outputFormat)
{
	if(outputFormat == "json")
	{
		ss<<"{\n";
			insertTab(ss, hierarchy+1), CREATE_RESULT_STRING_MEMBER_JSON(ss, info, logical_sector, hierarchy+1, outputFormat); ss<<",\n";
			insertTab(ss, hierarchy+1), CREATE_RESULT_STRING_MEMBER_JSON(ss, info, access_sorted_index, hierarchy+1, outputFormat); ss<<",\n";
			insertTab(ss, hierarchy+1), CREATE_RESULT_STRING_MEMBER_JSON(ss, info, physical_sector, hierarchy+1, outputFormat); ss<<"\n";
		insertTab(ss, hierarchy); ss<<"}";
	}
	else if(outputFormat == "text")
	{
		CREATE_RESULT_STRING_MEMBER_TEXT(ss, info, logical_sector, hierarchy+1, outputFormat); ss<<"\n";
		CREATE_RESULT_STRING_MEMBER_TEXT(ss, info, access_sorted_index, hierarchy+1, outputFormat); ss<<"\n";
		createResultString(ss, info.physical_sector, hierarchy+1, outputFormat); ss<<"\n";
	}
	else
		throw FormatException("Invalid output-format ", outputFormat);
}

/**
  * Stringifies the given f_device_performance using the given format
 **/
template <> inline void createResultString(std::ostream &ss, const f_device_performance &perf, unsigned int hierarchy, const utils::ci_string &outputFormat)
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
	{
		ss<<"{\n";
			insertTab(ss, hierarchy+1); CREATE_RESULT_STRING_NONMEMBER_JSON(ss, avg_read_time_cycles, hierarchy+1, outputFormat); ss<<",\n";
			insertTab(ss, hierarchy+1); CREATE_RESULT_STRING_NONMEMBER_JSON(ss, stdev_read_time_cycles, hierarchy+1, outputFormat); ss<<",\n";
			insertTab(ss, hierarchy+1); CREATE_RESULT_STRING_NONMEMBER_JSON(ss, avg_write_time_cycles, hierarchy+1, outputFormat); ss<<",\n";
			insertTab(ss, hierarchy+1); CREATE_RESULT_STRING_NONMEMBER_JSON(ss, stdev_write_time_cycles, hierarchy+1, outputFormat); ss<<"\n";
		insertTab(ss, hierarchy); ss<<"}";
	}
	else if(outputFormat == "text")
	{
		CREATE_RESULT_STRING_NONMEMBER_TEXT(ss, avg_read_time_cycles, hierarchy+1, outputFormat); ss<<"\n";
		CREATE_RESULT_STRING_NONMEMBER_TEXT(ss, stdev_read_time_cycles, hierarchy+1, outputFormat); ss<<"\n";
		CREATE_RESULT_STRING_NONMEMBER_TEXT(ss, avg_write_time_cycles, hierarchy+1, outputFormat); ss<<"\n";
		CREATE_RESULT_STRING_NONMEMBER_TEXT(ss, stdev_write_time_cycles, hierarchy+1, outputFormat); ss<<"\n";
	}
	else
		throw FormatException("Invalid output-format ", outputFormat);
}

/**
  * Stringifies the given f_internal_device_info using the given format
 **/
template <> inline void createResultString(std::ostream &ss, const enum f_internal_device_type &type, unsigned int hierarchy, const utils::ci_string &outputFormat)
{
	switch(type)
	{
	case c::f_internal_device_type_file:
		createResultString(ss, "file", hierarchy, outputFormat);
		break;
	case c::f_internal_device_type_plugin:
		createResultString(ss, "plugin", hierarchy, outputFormat);
		break;
	default:
		throw FormatException("Undefined enum value ", type, " for f_internal_device_type");
	}
}

/**
  * Stringifies the given f_internal_device_info using the given format
 **/
template <> inline void createResultString(std::ostream &ss, const f_internal_device_info &info, unsigned int hierarchy, const utils::ci_string &outputFormat)
{
	if(outputFormat == "json")
	{
		ss<<"{\n";
			insertTab(ss, hierarchy+1); CREATE_RESULT_STRING_MEMBER_JSON(ss, info, disk, hierarchy+1, outputFormat); ss<<",\n";
			insertTab(ss, hierarchy+1); CREATE_RESULT_STRING_MEMBER_JSON(ss, info, name, hierarchy+1, outputFormat); ss<<",\n";
			insertTab(ss, hierarchy+1); CREATE_RESULT_STRING_MEMBER_JSON(ss, info, path, hierarchy+1, outputFormat); ss<<",\n";
			insertTab(ss, hierarchy+1); CREATE_RESULT_STRING_MEMBER_JSON(ss, info, size, hierarchy+1, outputFormat); ss<<",\n";
			insertTab(ss, hierarchy+1); CREATE_RESULT_STRING_MEMBER_JSON(ss, info, type, hierarchy+1, outputFormat); ss<<",\n";
			insertTab(ss, hierarchy+1); CREATE_RESULT_STRING_MEMBER_JSON(ss, info, performance, hierarchy+1, outputFormat); ss<<"\n";
		insertTab(ss, hierarchy); ss<<"}";
	}
	else if(outputFormat == "text")
	{
		CREATE_RESULT_STRING_MEMBER_TEXT(ss, info, disk, hierarchy+1, outputFormat); ss<<"\n";
		CREATE_RESULT_STRING_MEMBER_TEXT(ss, info, name, hierarchy+1, outputFormat); ss<<"\n";
		CREATE_RESULT_STRING_MEMBER_TEXT(ss, info, path, hierarchy+1, outputFormat); ss<<"\n";
		CREATE_RESULT_STRING_MEMBER_TEXT(ss, info, size, hierarchy+1, outputFormat); ss<<"\n";
		CREATE_RESULT_STRING_MEMBER_TEXT(ss, info, type, hierarchy+1, outputFormat); ss<<"\n";
		createResultString(ss, info.performance, hierarchy+1, outputFormat); ss<<"\n";
	}
	else
		throw FormatException("Invalid output-format ", outputFormat);
}

/**
  * Stringifies the given tDisk using the given format
 **/
template <> inline void createResultString(std::ostream &ss, const tDisk &disk, unsigned int hierarchy, const utils::ci_string &outputFormat)
{
	if(outputFormat == "json")
	{
		ss<<"{\n";
			insertTab(ss, hierarchy+1); CREATE_RESULT_STRING_MEMBER_JSON(ss, disk, minornumber, hierarchy+1, outputFormat); ss<<",\n";
			insertTab(ss, hierarchy+1); CREATE_RESULT_STRING_MEMBER_JSON(ss, disk, name, hierarchy+1, outputFormat); ss<<",\n";
			insertTab(ss, hierarchy+1); CREATE_RESULT_STRING_MEMBER_JSON(ss, disk, size, hierarchy+1, outputFormat); ss<<",\n";
			insertTab(ss, hierarchy+1); CREATE_RESULT_STRING_MEMBER_JSON(ss, disk, online, hierarchy+1, outputFormat); ss<<"\n";
		insertTab(ss, hierarchy); ss<<"}";
	}
	else if(outputFormat == "text")
	{
		CREATE_RESULT_STRING_MEMBER_TEXT(ss, disk, minornumber, hierarchy+1, outputFormat); ss<<"\n";
		CREATE_RESULT_STRING_MEMBER_TEXT(ss, disk, name, hierarchy+1, outputFormat); ss<<"\n";
		CREATE_RESULT_STRING_MEMBER_TEXT(ss, disk, size, hierarchy+1, outputFormat); ss<<"\n";
		CREATE_RESULT_STRING_MEMBER_TEXT(ss, disk, online, hierarchy+1, outputFormat); ss<<"\n";
	}
	else
		throw FormatException("Invalid output-format ", outputFormat);
}

} //end namespace td

#endif //TDISK_HPP

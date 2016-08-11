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
using c::f_tdisk_debug_info;
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
	static void getTDisks(std::vector<std::string> &out);

	/**
	  * Gets all currently loaded tDisks
	  * @param out The container where the disks should be stored
	 **/
	static void getTDisks(std::vector<tDisk> &out);

	/**
	  * Gets the minornumber of the tDisk with the given path
	  * @param name The path of the tDisk to get the minornumber 
	  * @returns The minro number of the tDisk
	 **/
	static int getMinorNumber(const std::string &name);

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
	static tDisk create(int i_minornumber, unsigned int blocksize);

	/**
	  * Creates a new tDisk with the given blocksize. The next available
	  * minornumber is taken
	  * @param blocksize The blocksize for the new tDisk
	  * @returns The new tDisk
	 **/
	static tDisk create(unsigned int blocksize);

	/**
	  * Removes the tDisk with the given minornumber from the system
	  * @param i_minornumber The minornumber of thetDisk to be removed
	 **/
	static void remove(int i_minornumber);

	/**
	  * Removes the tDisk with the given path from the system
	  * @param name The path of the tDisk to be removed
	 **/
	static void remove(const std::string &name);

	/**
	  * Removes the given tDisk from the system
	  * @param disk The disk to be removed 
	 **/
	static void remove(const tDisk &disk)
	{
		remove(disk.minornumber);
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
	bool checkOnline() const;

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
	void addDisk(const std::string &path, bool format);

	/**
	  * Returns the current amount of max sectors
	 **/
	unsigned long long getMaxSectors() const;

	/**
	  * Returns the current size in bytes
	 **/
	unsigned long long getSize() const;

	/**
	  * Returns the current size in bytes
	 **/
	unsigned int getBlocksize() const;

	/**
	  * Returns information about the sector with the given logical
	  * sector
	 **/
	f_sector_index getSectorIndex(unsigned long long logicalSector) const;

	/**
	  * Return information about all logical sectors
	 **/
	std::vector<f_sector_info> getAllSectorIndices() const;

	/**
	  * Clears the access count of all sectors
	 **/
	void clearAccessCount();

	/**
	  * Returns the amount of current internal devices
	 **/
	unsigned int getInternalDevicesCount() const;

	/**
	  * Returns information about the internal device with the given
	  * device id
	 **/
	f_internal_device_info getDeviceInfo(unsigned int device) const;

	/**
	  * Returns the (next) debug info for the tDisk
	 **/
	f_tdisk_debug_info getDebugInfo(uint64_t currentId=0) const;

	/**
	  * Returns all files which are stored on the given internal device
	 **/
	std::vector<FileAssignment> getFilesOnDevice(unsigned int device, bool getPercentages, bool filesOnly);

	/**
	  * Gets the performance improvement of the tDisk
	 **/
	tDiskPerformanceImprovement getPerformanceImprovement(std::size_t amountFiles);

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
template <> void createResultString(std::ostream &ss, const f_sector_index &index, unsigned int hierarchy, const utils::ci_string &outputFormat);

/**
  * Stringifies the given f_sector_info using the given format
 **/
template <> void createResultString(std::ostream &ss, const f_sector_info &info, unsigned int hierarchy, const utils::ci_string &outputFormat);

/**
  * Stringifies the given f_device_performance using the given format
 **/
template <> void createResultString(std::ostream &ss, const f_device_performance &perf, unsigned int hierarchy, const utils::ci_string &outputFormat);

/**
  * Stringifies the given f_internal_device_info using the given format
 **/
template <> void createResultString(std::ostream &ss, const enum f_internal_device_type &type, unsigned int hierarchy, const utils::ci_string &outputFormat);

/**
  * Stringifies the given f_internal_device_info using the given format
 **/
template <> void createResultString(std::ostream &ss, const f_internal_device_info &info, unsigned int hierarchy, const utils::ci_string &outputFormat);

/**
  * Stringifies the given f_tdisk_debug_info using the given format
 **/
template <> void createResultString(std::ostream &ss, const f_tdisk_debug_info &info, unsigned int hierarchy, const utils::ci_string &outputFormat);

/**
  * Stringifies the given tDisk using the given format
 **/
template <> void createResultString(std::ostream &ss, const tDisk &disk, unsigned int hierarchy, const utils::ci_string &outputFormat);

} //end namespace td

#endif //TDISK_HPP

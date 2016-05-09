/**
  *
  * tDisk backend
  * @author Thomas Sparber (2015-2016)
  *
 **/

#ifndef FILESYSTEM_HPP
#define FILESYSTEM_HPP

#include <algorithm>
#include <functional>
#include <string>
#include <vector>

#include <device.hpp>

namespace td
{

namespace fs
{

	/**
	  * Gets device information for the with the given name
	 **/
	Device getDevice(const std::string &name);

	/**
	  * Gets all available blockdevices including its partitions
	 **/
	void getDevices(std::vector<Device> &out);

	/**
	  * Returns a list of files which are stored on the given
	  * disk at the given position
	 **/
	std::vector<std::string> getFilesOnDisk(const std::string &disk, unsigned long long start, unsigned long long end);

	/**
	  * This function iterates over all files on the given filesystem
	  * and calls callback for every file. The callback takes
	  * the blocksize, filename, file index block and data blocks
	 **/
	void iterateFiles(const std::string &disk, std::function<bool(unsigned int, const std::string&, unsigned long long, const std::vector<unsigned long long>&)> callback);

} //end namespace fs

} //end namespace td

#endif //FILESYSTEM_HPP

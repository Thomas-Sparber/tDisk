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
#include <fileassignment.hpp>

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
	std::vector<FileAssignment> getFilesOnDisk(const std::string &disk, std::vector<std::pair<unsigned long long,unsigned long long> > positions, bool calculatePercentage, bool filesOnly);

	/**
	  * This function iterates over all files on the given filesystem
	  * and calls callback for every file. The callback takes
	  * the blocksize, filename, file index block and data blocks
	 **/
	void iterateFiles(const std::string &disk, bool filesOnly, std::function<bool(unsigned int, const std::string&, unsigned long long, const std::vector<unsigned long long>&)> callback);

} //end namespace fs

} //end namespace td

#endif //FILESYSTEM_HPP

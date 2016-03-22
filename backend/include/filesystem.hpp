/**
  *
  * tDisk backend
  * @author Thomas Sparber (2015-2016)
  *
 **/

#ifndef FILESYSTEM_HPP
#define FILESYSTEM_HPP

#include <algorithm>
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

} //end namespace fs

} //end namespace td

#endif //FILESYSTEM_HPP

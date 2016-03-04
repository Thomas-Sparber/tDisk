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

	Device getDevice(const std::string &name);

} //end namespace fs

} //end namespace td

#endif //FILESYSTEM_HPP

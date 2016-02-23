#ifndef DEVICE_HPP
#define DEVICE_HPP

#include <string>
#include <vector>

#include <ci_string.hpp>
#include <device_type.hpp>
#include <resultformatter.hpp>

namespace td
{

namespace fs
{

struct Device
{

	Device();

	bool operator== (const Device &other) const;

	bool isValid() const;

	Device getSplitted(uint64_t newsize) const;

	bool containsDevice(const std::string &n) const;

	bool isRedundant() const;

	std::string name;
	device_type type;
	uint64_t size;
	std::vector<Device> subdevices;

}; //end struct Device

} //end namespace fs


template <> std::string createResultString(const fs::Device &device, unsigned int hierarchy, const ci_string &outputFormat);

} //end namespace td

#endif //DEVICE_HPP

/**
  *
  * tDisk backend
  * @author Thomas Sparber (2015-2016)
  *
 **/

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

/**
  * This struct is used by the DeviceAdvisor to create
  * device advices
 **/
struct Device
{

	/**
	  * Default constructor
	 **/
	Device();

	/**
	  * Compares the current Device with the given Device
	 **/
	bool operator== (const Device &other) const;

	/**
	  * Checks if the Device is valid. This is used for
	  * raid devices if they have enough devices to work
	  * properly
	 **/
	bool isValid() const;

	/**
	  * Returns a copy of the current device which only has
	  * the given size in bytes. This allows e.g. to use
	  * one device in multiple (separate) disk arrays
	 **/
	Device getSplitted(uint64_t newsize) const;

	/**
	  * Checks if the Device contains a device with the given
	  * name (e.g. as member of a disk array)
	 **/
	bool containsDevice(const std::string &n) const;

	/**
	  * Returns whether the Device is redundant e.g. raid1
	 **/
	bool isRedundant() const;

	/**
	  * The name of the device
	 **/
	std::string name;

	/**
	  * The name of the device
	 **/
	std::string path;

	/**
	  * The device type e.g. File, Blockdevice, raid...
	 **/
	device_type type;

	/**
	  * The size of the device in bytes
	 **/
	uint64_t size;

	/**
	  * The subdevices. e.g. for raid
	 **/
	std::vector<Device> subdevices;

}; //end struct Device

} //end namespace fs


/**
  * Stringifies a td::fs::Device with the given format
 **/
template <> void createResultString(std::ostream &ss, const fs::Device &device, unsigned int hierarchy, const utils::ci_string &outputFormat);

} //end namespace td

#endif //DEVICE_HPP

/**
  *
  * tDisk backend
  * @author Thomas Sparber (2015-2016)
  *
 **/

#ifndef DEVICE_TYPE_HPP
#define DEVICE_TYPE_HPP

#include <string>

namespace td
{

namespace fs
{

/**
  * Represents the type of a device.
  * The purpose of this struct is to be used
  * as an enum.
 **/
struct device_type
{

	/**
	  * Default invalid device
	 **/
	const static device_type invalid;


	/**
	  * Device is a blockdevice
	 **/
	const static device_type blockdevice;

	/**
	  * Device is a part of a blockdevice.
	  * e.g. when it was splitted using
	  * td::fs::Device::getSplitted
	 **/
	const static device_type blockdevice_part;

	/**
	  * Defice is a file
	 **/
	const static device_type file;

	/**
	  * Device is a part of a file.
	  * e.g. when it was splitted using
	  * td::fs::Device::getSplitted
	 **/
	const static device_type file_part;

	/**
	  * Device is a raid 1 device
	 **/
	const static device_type raid1;

	/**
	  * Device is a raid 5 device
	 **/
	const static device_type raid5;

	/**
	  * Device is a raid 6 device
	 **/
	const static device_type raid6;

	/**
	  * Converts the device_type to an int
	  * to make it comparable
	 **/
	operator int() const
	{
		return value;
	}

	/**
	  * Returns the name of the type
	 **/
	const std::string& getName() const
	{
		return name;
	}

private:

	/**
	  * Default private constructor because we don't allow
	  * to create new types dynamically (like an enum)
	 **/
	device_type(const std::string &str_name, int i_value) :
		name(str_name),
		value(i_value)
	{}

	/**
	  * The name of the device type
	 **/
	std::string name;

	/**
	  * Int representation of the device type
	 **/
	int value;

}; //end struct device_type

} //end namespace td

} //end namespace td

#endif //DEVICE_TYPE_HPP

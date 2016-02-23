#ifndef DEVICE_TYPE_HPP
#define DEVICE_TYPE_HPP

#include <string>

namespace td
{

namespace fs
{

struct device_type
{
	const static device_type invalid;

	const static device_type blockdevice;
	const static device_type blockdevice_part;
	const static device_type file;
	const static device_type file_part;
	const static device_type raid1;
	const static device_type raid5;
	const static device_type raid6;

	operator int() const
	{
		return value;
	}

	const std::string& getName() const
	{
		return name;
	}

private:
	device_type(const std::string &str_name, int i_value) :
		name(str_name),
		value(i_value)
	{}

	std::string name;
	int value;
}; //end struct device_type

} //end namespace td

} //end namespace td

#endif //DEVICE_TYPE_HPP

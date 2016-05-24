/**
  *
  * tDisk backend
  * @author Thomas Sparber (2015-2016)
  *
 **/

#include <algorithm>
#include <vector>

#include <device.hpp>
#include <backendexception.hpp>

using std::string;
using std::vector;

using namespace td;
using namespace fs;

using utils::ci_string;
using utils::concat;

Device::Device() :
	name(),
	path(),
	type(device_type::invalid),
	size(),
	subdevices()
{}

bool Device::operator== (const Device &other) const
{
	if(name != other.name)return false;
	if(path != other.path)return false;
	if(type != other.type)return false;
	if(size != other.size)return false;

	if(subdevices.size() != other.subdevices.size())return false;
	for(const Device &d : other.subdevices)
		if(std::find(subdevices.begin(), subdevices.end(), d) == subdevices.cend())return false;

	return true;
}

bool Device::isValid() const
{
	if(type == device_type::raid6 && subdevices.size() < 4)return false;
	if(type == device_type::raid5 && subdevices.size() < 3)return false;
	if(type == device_type::raid1 && subdevices.size() < 2)return false;
	return true;
}

Device Device::getSplitted(uint64_t newsize) const
{
	Device d(*this);
	d.size = newsize;

	if(type == device_type::file || type == device_type::file_part)
		d.type = device_type::file_part;
	else if(type == device_type::blockdevice || type == device_type::blockdevice_part)
		d.type = device_type::blockdevice_part;
	else
		throw BackendException("Can't split a device of type ", type.getName());

	return std::move(d);
}

bool Device::containsDevice(const std::string &n) const
{
	for(const Device &d : subdevices)
	{
		if(d.path == n)return true;
		if(d.containsDevice(n))return true;
	}
	return false;
}

bool Device::isRedundant() const
{
	if(type == device_type::raid1)return true;
	if(type == device_type::raid5)return true;
	if(type == device_type::raid6)return true;
	return false;
}

template <> void td::createResultString(std::ostream &ss, const fs::Device &device, unsigned int hierarchy, const ci_string &outputFormat)
{
	const string &type = device.type.getName();

	if(outputFormat == "json")
	{
		ss<<"{\n";
			insertTab(ss, hierarchy+1); CREATE_RESULT_STRING_MEMBER_JSON(ss, device, name, hierarchy+1, outputFormat); ss<<",\n";
			insertTab(ss, hierarchy+1); CREATE_RESULT_STRING_MEMBER_JSON(ss, device, path, hierarchy+1, outputFormat); ss<<",\n";
			insertTab(ss, hierarchy+1); CREATE_RESULT_STRING_NONMEMBER_JSON(ss, type, hierarchy+1, outputFormat); ss<<",\n";
			insertTab(ss, hierarchy+1); CREATE_RESULT_STRING_MEMBER_JSON(ss, device, size, hierarchy+1, outputFormat); ss<<",\n";
			insertTab(ss, hierarchy+1); CREATE_RESULT_STRING_MEMBER_JSON(ss, device, subdevices, hierarchy+1, outputFormat); ss<<"\n";
		insertTab(ss, hierarchy); ss<<"}";
	}
	else if(outputFormat == "text")
	{
		CREATE_RESULT_STRING_MEMBER_TEXT(ss, device, name, hierarchy+1, outputFormat); ss<<"\n";
		CREATE_RESULT_STRING_MEMBER_TEXT(ss, device, path, hierarchy+1, outputFormat); ss<<"\n";
		CREATE_RESULT_STRING_NONMEMBER_TEXT(ss, type, hierarchy+1, outputFormat); ss<<"\n";
		CREATE_RESULT_STRING_MEMBER_TEXT(ss, device, size, hierarchy+1, outputFormat); ss<<"\n";
		CREATE_RESULT_STRING_MEMBER_TEXT(ss, device, subdevices, hierarchy+1, outputFormat); ss<<"\n";
	}
	else
		throw FormatException("Invalid output-format ", outputFormat);
}

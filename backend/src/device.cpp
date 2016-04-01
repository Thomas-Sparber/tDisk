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

template <> string td::createResultString(const fs::Device &device, unsigned int hierarchy, const ci_string &outputFormat)
{
	const string &type = device.type.getName();

	if(outputFormat == "json")
		return concat(
			"{\n",
				vector<char>(hierarchy+1, '\t'), CREATE_RESULT_STRING_MEMBER_JSON(device, name, hierarchy+1, outputFormat), ",\n",
				vector<char>(hierarchy+1, '\t'), CREATE_RESULT_STRING_MEMBER_JSON(device, path, hierarchy+1, outputFormat), ",\n",
				vector<char>(hierarchy+1, '\t'), CREATE_RESULT_STRING_NONMEMBER_JSON(type, hierarchy+1, outputFormat), ",\n",
				vector<char>(hierarchy+1, '\t'), CREATE_RESULT_STRING_MEMBER_JSON(device, size, hierarchy+1, outputFormat), ",\n",
				vector<char>(hierarchy+1, '\t'), CREATE_RESULT_STRING_MEMBER_JSON(device, subdevices, hierarchy+1, outputFormat), "\n",
			vector<char>(hierarchy, '\t'), "}"
		);
	else if(outputFormat == "text")
		return concat(
				CREATE_RESULT_STRING_MEMBER_TEXT(device, name, hierarchy+1, outputFormat), "\n",
				CREATE_RESULT_STRING_MEMBER_TEXT(device, path, hierarchy+1, outputFormat), "\n",
				CREATE_RESULT_STRING_NONMEMBER_TEXT(type, hierarchy+1, outputFormat), "\n",
				CREATE_RESULT_STRING_MEMBER_TEXT(device, size, hierarchy+1, outputFormat), "\n",
				CREATE_RESULT_STRING_MEMBER_TEXT(device, subdevices, hierarchy+1, outputFormat), "\n"
		);
	else
		throw FormatException("Invalid output-format ", outputFormat);
}

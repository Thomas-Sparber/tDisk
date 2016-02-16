#ifndef DEVICEADVISOR_HPP
#define DEVICEADVISOR_HPP

#include <algorithm>
#include <string>
#include <vector>

#include <frontend.hpp>
#include <resultformatter.hpp>

namespace td
{

namespace advisor
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

	std::string getName() const
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

struct device
{
	device() :
		name(),
		type(device_type::invalid),
		size(),
		recommendation_score(),
		subdevices()
	{}

	bool operator== (const device &other) const
	{
		if(name != other.name)return false;
		if(type != other.type)return false;
		if(size != other.size)return false;
		if(recommendation_score != other.recommendation_score)return false;

		if(subdevices.size() != other.subdevices.size())return false;
		for(const device &d : other.subdevices)
			if(std::find(subdevices.begin(), subdevices.end(), d) == subdevices.cend())return false;

		return true;
	}

	bool isValid() const
	{
		if(type == device_type::raid6 && subdevices.size() < 4)return false;
		if(type == device_type::raid5 && subdevices.size() < 3)return false;
		if(type == device_type::raid1 && subdevices.size() < 2)return false;
		return true;
	}

	device getSplitted(uint64_t newsize) const
	{
		device d(*this);
		d.size = newsize;

		if(type == device_type::file || type == device_type::file_part)
			d.type = device_type::file_part;
		else if(type == device_type::blockdevice || type == device_type::blockdevice_part)
			d.type = device_type::blockdevice_part;
		else
			throw FrontendException("Can't split a device of type ", type.getName());

		return std::move(d);
	}

	bool containsDevice(const std::string &n) const
	{
		for(const device &d : subdevices)
		{
			if(d.name == n)return true;
			if(d.containsDevice(n))return true;
		}
		return false;
	}

	bool isRedundant() const
	{
		if(type == device_type::raid1)return true;
		if(type == device_type::raid5)return true;
		if(type == device_type::raid6)return true;
		return false;
	}

	std::string name;
	device_type type;
	uint64_t size;
	int recommendation_score;
	std::vector<device> subdevices;
}; //end struct device

struct tdisk_advice
{
	tdisk_advice() :
		devices(),
		recommendation_score(),
		redundancy_level(),
		wasted_space()
	{}

	bool operator== (const tdisk_advice &other)
	{
		if(devices.size() != other.devices.size())return false;

		for(const device &d : other.devices)
			if(std::find(devices.begin(), devices.end(), d) == devices.cend())return false;

		return true;
	}

	bool operator< (const tdisk_advice &other) const
	{
		if(recommendation_score == other.recommendation_score)return wasted_space > other.wasted_space;
		return recommendation_score < other.recommendation_score;
	}

	std::vector<device> devices;
	int recommendation_score;
	int redundancy_level;
	uint64_t wasted_space;
}; //end struct tdisk_advice

std::vector<tdisk_advice> getTDiskAdvices(const std::vector<std::string> &files);

} //end namespace advisor

template <> inline std::string createResultString(const advisor::device &device, unsigned int hierarchy, const ci_string &outputFormat)
{
	const std::string &type = device.type.getName();

	if(outputFormat == "json")
		return concat(
			"{\n",
				std::vector<char>(hierarchy+1, '\t'), CREATE_RESULT_STRING_MEMBER_JSON(device, name, hierarchy+1, outputFormat), ",\n",
				std::vector<char>(hierarchy+1, '\t'), CREATE_RESULT_STRING_NONMEMBER_JSON(type, hierarchy+1, outputFormat), ",\n",
				std::vector<char>(hierarchy+1, '\t'), CREATE_RESULT_STRING_MEMBER_JSON(device, size, hierarchy+1, outputFormat), ",\n",
				std::vector<char>(hierarchy+1, '\t'), CREATE_RESULT_STRING_MEMBER_JSON(device, recommendation_score, hierarchy+1, outputFormat), ",\n",
				std::vector<char>(hierarchy+1, '\t'), CREATE_RESULT_STRING_MEMBER_JSON(device, subdevices, hierarchy+1, outputFormat), "\n",
			std::vector<char>(hierarchy, '\t'), "}"
		);
	else if(outputFormat == "text")
		return concat(
				CREATE_RESULT_STRING_MEMBER_TEXT(device, recommendation_score, hierarchy+1, outputFormat), "\n",
				CREATE_RESULT_STRING_NONMEMBER_TEXT(type, hierarchy+1, outputFormat), "\n",
				CREATE_RESULT_STRING_MEMBER_TEXT(device, size, hierarchy+1, outputFormat), "\n",
				CREATE_RESULT_STRING_MEMBER_TEXT(device, recommendation_score, hierarchy+1, outputFormat), "\n",
				CREATE_RESULT_STRING_MEMBER_TEXT(device, subdevices, hierarchy+1, outputFormat), "\n"
		);
	else
		throw FormatException("Invalid output-format ", outputFormat);
}

template <> inline std::string createResultString(const advisor::tdisk_advice &disk, unsigned int hierarchy, const ci_string &outputFormat)
{
	if(outputFormat == "json")
		return concat(
			"{\n",
				std::vector<char>(hierarchy+1, '\t'), CREATE_RESULT_STRING_MEMBER_JSON(disk, devices, hierarchy+1, outputFormat), ",\n",
				std::vector<char>(hierarchy+1, '\t'), CREATE_RESULT_STRING_MEMBER_JSON(disk, recommendation_score, hierarchy+1, outputFormat), ",\n",
				std::vector<char>(hierarchy+1, '\t'), CREATE_RESULT_STRING_MEMBER_JSON(disk, redundancy_level, hierarchy+1, outputFormat), ",\n",
				std::vector<char>(hierarchy+1, '\t'), CREATE_RESULT_STRING_MEMBER_JSON(disk, wasted_space, hierarchy+1, outputFormat), "\n",
			std::vector<char>(hierarchy, '\t'), "}"
		);
	else if(outputFormat == "text")
		return concat(
				createResultString(disk.devices, hierarchy+1, outputFormat), "\n",
				CREATE_RESULT_STRING_MEMBER_TEXT(disk, recommendation_score, hierarchy+1, outputFormat), "\n",
				CREATE_RESULT_STRING_MEMBER_TEXT(disk, redundancy_level, hierarchy+1, outputFormat), "\n",
				CREATE_RESULT_STRING_MEMBER_TEXT(disk, wasted_space, hierarchy+1, outputFormat), "\n"
		);
	else
		throw FormatException("Invalid output-format ", outputFormat);
}

} //end namespace td

#endif //DEVICEADVISOR_HPP

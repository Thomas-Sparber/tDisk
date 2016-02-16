#ifndef DEVICEADVISOR_HPP
#define DEVICEADVISOR_HPP

#include <algorithm>
#include <string>
#include <vector>

#include <filesystem.hpp>
#include <frontend.hpp>
#include <resultformatter.hpp>

namespace td
{

namespace advisor
{

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

		for(const fs::device &d : other.devices)
			if(std::find(devices.begin(), devices.end(), d) == devices.cend())return false;

		return true;
	}

	bool operator< (const tdisk_advice &other) const
	{
		if(recommendation_score == other.recommendation_score)return wasted_space > other.wasted_space;
		return recommendation_score < other.recommendation_score;
	}

	std::vector<fs::device> devices;
	int recommendation_score;
	int redundancy_level;
	uint64_t wasted_space;
}; //end struct tdisk_advice

std::vector<tdisk_advice> getTDiskAdvices(const std::vector<std::string> &files);

} //end namespace advisor

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

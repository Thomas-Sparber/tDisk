#ifndef DEVICEADVISOR_HPP
#define DEVICEADVISOR_HPP

#include <algorithm>
#include <string>
#include <vector>

#include <filesystem.hpp>
#include <resultformatter.hpp>
#include <tdisk_advice.hpp>

namespace td
{

namespace advisor
{

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

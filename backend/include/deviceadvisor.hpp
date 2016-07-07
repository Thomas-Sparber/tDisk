/**
  *
  * tDisk backend
  * @author Thomas Sparber (2015-2016)
  *
 **/

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

	/**
	  * Creates all possible disk combinations using the given
	  * devices and sorts them accoring to their rank
	 **/
	std::vector<tdisk_advice> getTDiskAdvices(const std::vector<std::string> &files);

} //end namespace advisor

/**
  * Stringifies a td::advisor::tdisk_advice using the given format
 **/
template <> inline void createResultString(std::ostream &ss, const advisor::tdisk_advice &disk, unsigned int hierarchy, const utils::ci_string &outputFormat)
{
	if(outputFormat == "json")
	{
		ss<<"{\n";
			insertTab(ss, hierarchy+1); CREATE_RESULT_STRING_MEMBER_JSON(ss, disk, devices, hierarchy+1, outputFormat); ss<<",\n";
			insertTab(ss, hierarchy+1); CREATE_RESULT_STRING_MEMBER_JSON(ss, disk, recommendation_score, hierarchy+1, outputFormat); ss<<",\n";
			insertTab(ss, hierarchy+1); CREATE_RESULT_STRING_MEMBER_JSON(ss, disk, redundancy_level, hierarchy+1, outputFormat); ss<<",\n";
			insertTab(ss, hierarchy+1); CREATE_RESULT_STRING_MEMBER_JSON(ss, disk, wasted_space, hierarchy+1, outputFormat); ss<<"\n";
		insertTab(ss, hierarchy); ss<<"}";
	}
	else if(outputFormat == "text")
	{
		createResultString(ss, disk.devices, hierarchy+1, outputFormat); ss<<"\n";
		CREATE_RESULT_STRING_MEMBER_TEXT(ss, disk, recommendation_score, hierarchy+1, outputFormat); ss<<"\n";
		CREATE_RESULT_STRING_MEMBER_TEXT(ss, disk, redundancy_level, hierarchy+1, outputFormat); ss<<"\n";
		CREATE_RESULT_STRING_MEMBER_TEXT(ss, disk, wasted_space, hierarchy+1, outputFormat); ss<<"\n";
	}
	else
		throw FormatException("Invalid output-format ", outputFormat);
}

} //end namespace td

#endif //DEVICEADVISOR_HPP

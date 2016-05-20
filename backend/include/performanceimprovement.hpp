/**
  *
  * tDisk backend
  * @author Thomas Sparber (2015-2016)
  *
 **/

#ifndef PERFORMANCEIMPROVEMENT_HPP
#define PERFORMANCEIMPROVEMENT_HPP

#include <string>
#include <vector>

#include <resultformatter.hpp>

namespace td
{

struct FilePerformanceImprovement
{
	FilePerformanceImprovement(const std::string &str_path, double d_performance) :
		path(str_path),
		performance(d_performance)
	{}

	std::string path;
	double performance;
}; //end struct PilePerformanceImprovement

struct tDiskPerformanceImprovement
{
	tDiskPerformanceImprovement(double d_percentage) :
		percentage(d_percentage),
		files()
	{}

	void addFile(const std::string &path, double performanceImprovement)
	{
		files.emplace_back(path, performanceImprovement);
	}

	double percentage;
	std::vector<FilePerformanceImprovement> files;
}; //end struct tDiskPerformanceImprovement

/**
  * Stringifies the given tDisk using the given format
 **/
template <> inline std::string createResultString(const FilePerformanceImprovement &file, unsigned int hierarchy, const utils::ci_string &outputFormat)
{
	if(outputFormat == "json")
		return utils::concat(
			"{\n",
				std::vector<char>(hierarchy+1, '\t'), CREATE_RESULT_STRING_MEMBER_JSON(file, path, hierarchy+1, outputFormat), ",\n",
				std::vector<char>(hierarchy+1, '\t'), CREATE_RESULT_STRING_MEMBER_JSON(file, performance, hierarchy+1, outputFormat), "\n",
			std::vector<char>(hierarchy, '\t'), "}"
		);
	else if(outputFormat == "text")
		return utils::concat(
				CREATE_RESULT_STRING_MEMBER_TEXT(file, path, hierarchy+1, outputFormat), "\n",
				CREATE_RESULT_STRING_MEMBER_TEXT(file, performance, hierarchy+1, outputFormat), "\n"
		);
	else
		throw FormatException("Invalid output-format ", outputFormat);
}

/**
  * Stringifies the given tDisk using the given format
 **/
template <> inline std::string createResultString(const tDiskPerformanceImprovement &tdisk, unsigned int hierarchy, const utils::ci_string &outputFormat)
{
	if(outputFormat == "json")
		return utils::concat(
			"{\n",
				std::vector<char>(hierarchy+1, '\t'), CREATE_RESULT_STRING_MEMBER_JSON(tdisk, percentage, hierarchy+1, outputFormat), ",\n",
				std::vector<char>(hierarchy+1, '\t'), CREATE_RESULT_STRING_MEMBER_JSON(tdisk, files, hierarchy+1, outputFormat), "\n",
			std::vector<char>(hierarchy, '\t'), "}"
		);
	else if(outputFormat == "text")
		return utils::concat(
				CREATE_RESULT_STRING_MEMBER_TEXT(tdisk, percentage, hierarchy+1, outputFormat), "\n",
				CREATE_RESULT_STRING_MEMBER_TEXT(tdisk, files, hierarchy+1, outputFormat), "\n"
		);
	else
		throw FormatException("Invalid output-format ", outputFormat);
}

}; //end namespace td

#endif //PERFORMANCEIMPROVEMENT_HPP

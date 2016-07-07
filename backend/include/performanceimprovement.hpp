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

/**
  * This struct represents the performance improvement of a file in
  * percent of using a tDisk instead of a normal disk
 **/
struct FilePerformanceImprovement
{
	/**
	  * Default constructor
	 **/
	FilePerformanceImprovement(const std::string &str_path, double d_performance) :
		path(str_path),
		performance(d_performance)
	{}

	/** The path of the file **/
	std::string path;

	/** The performance improvmenet int percentage **/
	double performance;

}; //end struct PilePerformanceImprovement

/**
  * This struct represents the performance improvement in
  * percent of using a tDisk instead of a normal disk
 **/
struct tDiskPerformanceImprovement
{
	/**
	  * Default constructor
	 **/
	tDiskPerformanceImprovement(double d_percentage) :
		percentage(d_percentage),
		files()
	{}

	/**
	  * Adds the file to the performance improvement
	 **/
	void addFile(const std::string &path, double performanceImprovement)
	{
		files.emplace_back(path, performanceImprovement);
	}

	/** The performance improvmenet int percentage **/
	double percentage;

	/**
	  * Contains all files which are involved in the performance improvement
	 **/
	std::vector<FilePerformanceImprovement> files;

}; //end struct tDiskPerformanceImprovement

/**
  * Stringifies the given tDisk using the given format
 **/
template <> inline void createResultString(std::ostream &ss, const FilePerformanceImprovement &file, unsigned int hierarchy, const utils::ci_string &outputFormat)
{
	if(outputFormat == "json")
	{
		ss<<"{\n";
			insertTab(ss, hierarchy+1), CREATE_RESULT_STRING_MEMBER_JSON(ss, file, path, hierarchy+1, outputFormat); ss<<",\n";
			insertTab(ss, hierarchy+1), CREATE_RESULT_STRING_MEMBER_JSON(ss, file, performance, hierarchy+1, outputFormat); ss<<"\n";
		insertTab(ss, hierarchy); ss<<"}";
	}
	else if(outputFormat == "text")
	{
		CREATE_RESULT_STRING_MEMBER_TEXT(ss, file, path, hierarchy+1, outputFormat); ss<<"\n";
		CREATE_RESULT_STRING_MEMBER_TEXT(ss, file, performance, hierarchy+1, outputFormat); ss<<"\n";
	}
	else
		throw FormatException("Invalid output-format ", outputFormat);
}

/**
  * Stringifies the given tDisk using the given format
 **/
template <> inline void createResultString(std::ostream &ss, const tDiskPerformanceImprovement &tdisk, unsigned int hierarchy, const utils::ci_string &outputFormat)
{
	if(outputFormat == "json")
	{
		ss<<"{\n";
			insertTab(ss, hierarchy+1); CREATE_RESULT_STRING_MEMBER_JSON(ss, tdisk, percentage, hierarchy+1, outputFormat); ss<<",\n";
			insertTab(ss, hierarchy+1); CREATE_RESULT_STRING_MEMBER_JSON(ss, tdisk, files, hierarchy+1, outputFormat); ss<<"\n";
		insertTab(ss, hierarchy); ss<<"}";
	}
	else if(outputFormat == "text")
	{
		CREATE_RESULT_STRING_MEMBER_TEXT(ss, tdisk, percentage, hierarchy+1, outputFormat); ss<<"\n";
		CREATE_RESULT_STRING_MEMBER_TEXT(ss, tdisk, files, hierarchy+1, outputFormat); ss<<"\n";
	}
	else
		throw FormatException("Invalid output-format ", outputFormat);
}

}; //end namespace td

#endif //PERFORMANCEIMPROVEMENT_HPP

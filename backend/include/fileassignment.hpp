/**
  *
  * tDisk backend
  * @author Thomas Sparber (2015-2016)
  *
 **/

#ifndef FILEASSIGNMENT_HPP
#define FILEASSIGNMENT_HPP

#include <string>

#include <resultformatter.hpp>

namespace td
{

/**
  * This struct is used to store the value
  * of how much üpercent a file is stored
  * on an internal device of a tDisk.
  * e.g. 95% of the file "/home/..." is stored
  * on internal device 2 of tDisk /dev/td1
 **/
struct FileAssignment
{

	/**
	  * Default constructor
	 **/
	FileAssignment(const std::string &str_filename, double d_percentage) :
		filename(str_filename),
		percentage(d_percentage)
	{}

	/**
	  * The path and name of the file
	 **/
	std::string filename;

	/**
	  * The percentage value how much of the data
	  * is stored on the tDisk
	 **/
	double percentage;

}; //end struct FileAssignment



/**
  * Stringifies the given FileAssignment using the given format
 **/
template <> inline std::string createResultString(const FileAssignment &fa, unsigned int hierarchy, const utils::ci_string &outputFormat)
{
	if(outputFormat == "json")
		return utils::concat(
			"{\n",
				std::vector<char>(hierarchy+1, '\t'), CREATE_RESULT_STRING_MEMBER_JSON(fa, filename, hierarchy+1, outputFormat), ",\n",
				std::vector<char>(hierarchy+1, '\t'), CREATE_RESULT_STRING_MEMBER_JSON(fa, percentage, hierarchy+1, outputFormat), "\n",
			std::vector<char>(hierarchy, '\t'), "}"
		);
	else if(outputFormat == "text")
		return utils::concat(
				CREATE_RESULT_STRING_MEMBER_TEXT(fa, filename, hierarchy+1, outputFormat), "\n",
				CREATE_RESULT_STRING_MEMBER_TEXT(fa, percentage, hierarchy+1, outputFormat), "\n"
		);
	else
		throw FormatException("Invalid output-format ", outputFormat);
}

} //end namespace td

#endif //FILEASSIGNMENT_HPP

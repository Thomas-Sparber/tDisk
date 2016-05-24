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
template <> inline void createResultString(std::ostream &ss, const FileAssignment &fa, unsigned int hierarchy, const utils::ci_string &outputFormat)
{
	if(outputFormat == "json")
	{
		ss<<"{\n";
			insertTab(ss, hierarchy+1); CREATE_RESULT_STRING_MEMBER_JSON(ss, fa, filename, hierarchy+1, outputFormat); ss<<",\n";
			insertTab(ss, hierarchy+1); CREATE_RESULT_STRING_MEMBER_JSON(ss, fa, percentage, hierarchy+1, outputFormat); ss<<"\n";
		insertTab(ss, hierarchy); ss<<"}";
	}
	else if(outputFormat == "text")
	{
		CREATE_RESULT_STRING_MEMBER_TEXT(ss, fa, filename, hierarchy+1, outputFormat); ss<<"\n";
		CREATE_RESULT_STRING_MEMBER_TEXT(ss, fa, percentage, hierarchy+1, outputFormat); ss<<"\n";
	}
	else
		throw FormatException("Invalid output-format ", outputFormat);
}

} //end namespace td

#endif //FILEASSIGNMENT_HPP

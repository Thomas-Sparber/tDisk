#ifndef CONFIGFILE_HPP
#define CONFIGFILE_HPP

#include <string>
#include <vector>

#include <ci_string.hpp>
#include <resultformatter.hpp>

namespace td
{

struct tdisk_global_option
{
	tdisk_global_option() :
		name(),
		value()
	{}

	ci_string name;
	ci_string value;
}; //end struct tdisk_global_option

struct tdisk_config
{
	tdisk_config() :
		minornumber(),
		blocksize(),
		devices()
	{}

	int minornumber;
	long blocksize;
	std::vector<std::string> devices;
}; //end struct tdisk_config

struct configuration
{
	configuration() :
		global_options(),
		tdisks()
	{}

	void merge(const configuration &config);
	void addDevice(const tdisk_config &device);
	void addOption(const tdisk_global_option &option);

	std::vector<tdisk_global_option> global_options;
	std::vector<tdisk_config> tdisks;
}; //end struct configuration

configuration loadConfiguration(const std::string &file);
void saveConfiguration(configuration &config, const std::string &file);
void process(const std::string &str, configuration &config);

template <> inline std::string createResultString(const configuration &config, unsigned int hierarchy, const ci_string &outputFormat)
{
	if(outputFormat == "json")
		return concat(
			"{\n",
				std::vector<char>(hierarchy+1, '\t'), CREATE_RESULT_STRING_MEMBER_JSON(config, global_options, hierarchy+1, outputFormat), ",\n",
				std::vector<char>(hierarchy+1, '\t'), CREATE_RESULT_STRING_MEMBER_JSON(config, tdisks, hierarchy+1, outputFormat), "\n",
			std::vector<char>(hierarchy, '\t'), "}"
		);
	else if(outputFormat == "text")
		return concat(
				createResultString(config.global_options, hierarchy+1, outputFormat), "\n",
				createResultString(config.tdisks, hierarchy+1, outputFormat), "\n"
		);
	else
		throw FormatException("Invalid output-format ", outputFormat);
}

template <template <class ...> class container>
inline std::string createResultString_array(const container<tdisk_global_option> &v, unsigned int hierarchy, const ci_string &outputFormat)
{
	std::vector<std::string> temp(v.size());
	if(outputFormat == "json")
		for(size_t i = 0; i < v.size(); ++i)
		{
			temp[i] = concat(
				(i == 0) ? "{\n" : "",
				std::vector<char>(hierarchy+1, '\t'), createResultString(v[i], hierarchy + 1, outputFormat),
				(i < v.size()-1) ? ",\n" : concat("\n", std::vector<char>(hierarchy, '\t'), "}"));
		}
	else if(outputFormat == "text")
		for(size_t i = 0; i < v.size(); ++i)
		{
			temp[i] = concat(
				createResultString(v[i], hierarchy + 1, outputFormat),
				(i < v.size()-1) ? "\n" : "");
		}
	else
		throw FormatException("Invalid output-format ", outputFormat);

	return concat(temp);
}

template <> inline std::string createResultString(const tdisk_global_option &option, unsigned int /*hierarchy*/, const ci_string &outputFormat)
{
	if(outputFormat == "json")
		return concat("\"", option.name, "\": \"", option.value, "\"");
	else if(outputFormat == "text")
		return concat(option.name, " = ", option.value);
	else
		throw FormatException("Invalid output-format ", outputFormat);
}

template <> inline std::string createResultString(const tdisk_config &config, unsigned int hierarchy, const ci_string &outputFormat)
{
	if(outputFormat == "json")
		return concat(
			"{\n",
				std::vector<char>(hierarchy+1, '\t'), CREATE_RESULT_STRING_MEMBER_JSON(config, minornumber, hierarchy+1, outputFormat), ",\n",
				std::vector<char>(hierarchy+1, '\t'), CREATE_RESULT_STRING_MEMBER_JSON(config, blocksize, hierarchy+1, outputFormat), ",\n",
				std::vector<char>(hierarchy+1, '\t'), CREATE_RESULT_STRING_MEMBER_JSON(config, devices, hierarchy+1, outputFormat), "\n",
			std::vector<char>(hierarchy, '\t'), "}"
		);
	else if(outputFormat == "text")
		return concat(
				CREATE_RESULT_STRING_MEMBER_TEXT(config, minornumber, hierarchy+1, outputFormat), "\n",
				CREATE_RESULT_STRING_MEMBER_TEXT(config, blocksize, hierarchy+1, outputFormat), "\n",
				CREATE_RESULT_STRING_MEMBER_TEXT(config, devices, hierarchy+1, outputFormat), "\n"
		);
	else
		throw FormatException("Invalid output-format ", outputFormat);
}

} //end namespace td

#endif //CONFIGFILE_HPP

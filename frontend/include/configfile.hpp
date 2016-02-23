#ifndef CONFIGFILE_HPP
#define CONFIGFILE_HPP

#include <string>
#include <vector>

#include <ci_string.hpp>
#include <options.hpp>
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

class configuration
{

public:
	configuration() :
		global_options(),
		tdisks()
	{}

	configuration(const std::string &file, const Options &options) :
		global_options(),
		tdisks()
	{
		load(file, options);
	}

	void load(const std::string &file, const Options &options);

	void save(const std::string &file) const;

	void merge(const configuration &config);
	void addDevice(const tdisk_config &device);
	void addOption(const tdisk_global_option &option);

private:
	void process(const std::string &str, const Options &options);

public:
	std::vector<tdisk_global_option> global_options;
	std::vector<tdisk_config> tdisks;
}; //end struct configuration

template <> std::string createResultString(const configuration &config, unsigned int hierarchy, const ci_string &outputFormat);

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

template <> std::string createResultString(const tdisk_global_option &option, unsigned int hierarchy, const ci_string &outputFormat);

template <> std::string createResultString(const tdisk_config &config, unsigned int hierarchy, const ci_string &outputFormat);

} //end namespace td

#endif //CONFIGFILE_HPP

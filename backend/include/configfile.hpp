/**
  *
  * tDisk backend
  * @author Thomas Sparber (2015-2016)
  *
 **/

#ifndef CONFIGFILE_HPP
#define CONFIGFILE_HPP

#include <string>
#include <vector>

#include <ci_string.hpp>
#include <options.hpp>
#include <resultformatter.hpp>

namespace td
{

class configuration
{

public:

	/**
	  * This struct represents one loadable config-file
	  * global option
	 **/
	struct tdisk_global_option
	{

		/**
		  * Default constructor
		 **/
		tdisk_global_option() :
			name(),
			value()
		{}

		/**
		  * The global option's nmae
		 **/
		utils::ci_string name;

		/**
		  * The global option's value
		 **/
		utils::ci_string value;

	}; //end struct tdisk_global_option

	/**
	  * This struct represents the configuration of
	  * one tDisk.
	 **/
	struct tdisk_config
	{

		/**
		  * Default constructor
		 **/
		tdisk_config() :
			minornumber(-1),
			blocksize(),
			devices()
		{}

		/**
		  * Compares if two disk configurations are equal
		 **/
		bool operator== (const tdisk_config &other) const
		{
			return (minornumber == other.minornumber);
		}

		/**
		  * Return whether the configuration for the
		  * configured device is valid
		 **/
		bool isValid() const;

		/**
		  * The tDisk's minornumber
		 **/
		int minornumber;

		/**
		  * The tDisk's blocksize
		 **/
		long blocksize;

		/**
		  * The internal devices of the tDisk
		 **/
		std::vector<std::string> devices;

	}; //end struct tdisk_config

	/**
	  * Default constructor
	 **/
	configuration() :
		global_options(),
		tdisks()
	{}

	/**
	  * Creates a new configuration by loading it
	  * it from the given file
	 **/
	configuration(const std::string &file, const Options &options) :
		global_options(),
		tdisks()
	{
		load(file, options);
	}

	/**
	  * Returns the tDisk config with the given minornumber
	 **/
	tdisk_config& getTDisk(int minornumber);

	/**
	  * Returns the tDisk config with the given minornumber
	 **/
	const tdisk_config& getTDisk(int minornumber) const;

	/**
	  * Loads the configuration from the given file
	 **/
	void load(const std::string &file, const Options &options);

	/**
	  * Saves the configuration to the given file
	 **/
	void save(const std::string &file) const;

	/**
	  * Merges the config entries from the given
	  * config object into the current one
	 **/
	void merge(const configuration &config);

	/**
	  * Adds a config device to the current config object
	 **/
	void addDevice(const tdisk_config &device);

	/**
	  * Adds a global option to the current config object
	 **/
	void addOption(const tdisk_global_option &option);

	/**
	  * Processes the given configuration string which
	  * is usually read from a file
	 **/
	void process(const std::string &str, const Options &options);

	/**
	  * The list of current global config options
	 **/
	std::vector<tdisk_global_option> global_options;

	/**
	  * The list of current config devices
	 **/
	std::vector<tdisk_config> tdisks;

}; //end struct configuration

/**
  * Creates the formatted result string for a td::configuration object
 **/
template <> std::string createResultString(const configuration &config, unsigned int hierarchy, const utils::ci_string &outputFormat);

/**
  * Creates the formatted result string for an
  * array of td::configuration::tdisk_global_option objects
 **/
template <template <class ...> class container>
inline std::string createResultString_array(const container<configuration::tdisk_global_option> &v, unsigned int hierarchy, const utils::ci_string &outputFormat)
{
	std::vector<std::string> temp(v.size());
	if(outputFormat == "json")
		for(size_t i = 0; i < v.size(); ++i)
		{
			temp[i] = utils::concat(
				(i == 0) ? "{\n" : "",
				std::vector<char>(hierarchy+1, '\t'), createResultString(v[i], hierarchy + 1, outputFormat),
				(i < v.size()-1) ? ",\n" : utils::concat("\n", std::vector<char>(hierarchy, '\t'), "}"));
		}
	else if(outputFormat == "text")
		for(size_t i = 0; i < v.size(); ++i)
		{
			temp[i] = utils::concat(
				createResultString(v[i], hierarchy + 1, outputFormat),
				(i < v.size()-1) ? "\n" : "");
		}
	else
		throw FormatException("Invalid output-format ", outputFormat);

	return utils::concat(temp);
}

/**
  * Creates the formatted result string for a 
  * td::configuration::tdisk_global_option object
 **/
template <> std::string createResultString(const configuration::tdisk_global_option &option, unsigned int hierarchy, const utils::ci_string &outputFormat);

/**
  * Creates the formatted result string for a 
  * td::configuration::tdisk_config object
 **/
template <> std::string createResultString(const configuration::tdisk_config &config, unsigned int hierarchy, const utils::ci_string &outputFormat);

} //end namespace td

#endif //CONFIGFILE_HPP

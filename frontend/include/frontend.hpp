#ifndef FRONTEND_HPP
#define FRONTEND_HPP

#include <string>
#include <vector>

#include <ci_string.hpp>
#include <utils.hpp>

namespace td
{

	struct FrontendException
	{
		template <class ...T>
		FrontendException(T ...t) :
			what(td::concat(t...))
		{}

		std::string what;
	}; //end class FrontendException

	struct Option
	{
		Option(const ci_string &str_name, const std::vector<ci_string> &v_values) :
			name(str_name),
			values(v_values),
			value(values.empty() ? "" : values[0])
		{}

		template <class T>
		void setValue(const T &t)
		{
			value = concat(t);
		}

		ci_string getStringValue() const
		{
			return value;
		}

		char getCharValue() const
		{
			if(value.empty())throw FrontendException("Value is empty");
			return value[0];
		}

		long getLongValue() const
		{
			char *test;
			long v = strtol(value.c_str(), &test, 10);
			if(test != value.c_str()+value.length())throw FrontendException("Invalid number ", value);
			return v;
		}

		ci_string name;
		std::vector<ci_string> values;
		ci_string value;
	}; //end struct option

	void addOption(const Option &option);
	bool optionExists(const ci_string &name);
	Option& getOption(const ci_string &name);
	ci_string getOptionValue(const ci_string &name);
	void setOptionValue(const ci_string &name, const ci_string &value);

	std::string add_tDisk(const std::vector<std::string> &args, const ci_string &outputFormat);
	std::string remove_tDisk(const std::vector<std::string> &args, const ci_string &outputFormat);
	std::string add_disk(const std::vector<std::string> &args, const ci_string &outputFormat);
	std::string get_max_sectors(const std::vector<std::string> &args, const ci_string &outputFormat);
	std::string get_sector_index(const std::vector<std::string> &args, const ci_string &outputFormat);
	std::string get_all_sector_indices(const std::vector<std::string> &args, const ci_string &outputFormat);
	std::string clear_access_count(const std::vector<std::string> &args, const ci_string &outputFormat);
	std::string get_internal_devices_count(const std::vector<std::string> &args, const ci_string &outputFormat);
	std::string get_device_info(const std::vector<std::string> &args, const ci_string &outputFormat);
	std::string load_config_file(const std::vector<std::string> &args, const ci_string &outputFormat);

} //end namespace td

#endif //FRONTEND_HPP

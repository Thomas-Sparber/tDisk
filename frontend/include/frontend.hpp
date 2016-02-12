#ifndef FRONTEND_HPP
#define FRONTEND_HPP

#include <string>
#include <vector>

#include <utils.hpp>
#include <ci_string.hpp>

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

	std::string add_tDisk(const std::vector<std::string> &args, const ci_string &outputFormat);
	std::string remove_tDisk(const std::vector<std::string> &args, const ci_string &outputFormat);
	std::string add_disk(const std::vector<std::string> &args, const ci_string &outputFormat);
	std::string get_max_sectors(const std::vector<std::string> &args, const ci_string &outputFormat);
	std::string get_sector_index(const std::vector<std::string> &args, const ci_string &outputFormat);
	std::string get_all_sector_indices(const std::vector<std::string> &args, const ci_string &outputFormat);
	std::string clear_access_count(const std::vector<std::string> &args, const ci_string &outputFormat);
	std::string get_internal_devices_count(const std::vector<std::string> &args, const ci_string &outputFormat);
	std::string get_device_info(const std::vector<std::string> &args, const ci_string &outputFormat);

} //end namespace td

#endif //FRONTEND_HPP

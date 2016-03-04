#ifndef BACKEND_HPP
#define BACKEND_HPP

#include <string>
#include <vector>

#include <ci_string.hpp>
#include <options.hpp>
#include <utils.hpp>

namespace td
{

	Options getDefaultOptions();

	std::string add_tDisk(const std::vector<std::string> &args, Options &options);
	std::string remove_tDisk(const std::vector<std::string> &args, Options &options);
	std::string add_disk(const std::vector<std::string> &args, Options &options);
	std::string get_max_sectors(const std::vector<std::string> &args, Options &options);
	std::string get_sector_index(const std::vector<std::string> &args, Options &options);
	std::string get_all_sector_indices(const std::vector<std::string> &args, Options &options);
	std::string clear_access_count(const std::vector<std::string> &args, Options &options);
	std::string get_internal_devices_count(const std::vector<std::string> &args, Options &options);
	std::string get_device_info(const std::vector<std::string> &args, Options &options);
	std::string load_config_file(const std::vector<std::string> &args, Options &options);
	std::string get_device_advice(const std::vector<std::string> &args, Options &options);

} //end namespace td

#endif //BACKEND_HPP

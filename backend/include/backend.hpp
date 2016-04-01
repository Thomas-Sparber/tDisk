/**
  *
  * tDisk backend
  * @author Thomas Sparber (2015-2016)
  *
  * This file contains the interface definitions for the
  * tDisk backend. It is possible to perform all major tDisk
  * tasks using the following functions
  *
 **/

#ifndef BACKEND_HPP
#define BACKEND_HPP

#include <string>
#include <vector>

#include <ci_string.hpp>
#include <options.hpp>
#include <utils.hpp>

namespace td
{

	/**
	  * Returns the default options for the backend functions.
	  * This includes e.g. output-format (text/json)
	 **/
	Options getDefaultOptions();

	/************************ Backend functions ***********************/

	/**
	  * Gets all currently loaded tDisks of the system
	  * @param args: not needed
	  * @param options: The command options (e.g. output-format)
	 **/
	std::string get_tDisks(const std::vector<std::string> &args, Options &options);

	/**
	  * Gets information about the given tDisk
	  * @param args:
	  *  - tDisk minor number (e.g. 0) or path (e.g. /dev/td0)
	  * @param options: The command options (e.g. output-format)
	 **/
	std::string get_tDisk(const std::vector<std::string> &args, Options &options);

	/**
	  * Gets all available devices which could be used for a tDisk
	  * @param args: not needed
	  * @param options: The command options (e.g. output-format)
	 **/
	std::string get_devices(const std::vector<std::string> &args, Options &options);

	/**
	  * Creates a new tDisk
	  * @param args:
	  *  - (Optional: The tDisk minor number, e.g. 0)
	  *  - The blocksize of the tDisk, e.g. 16384
	  *  - Internal device 1
	  *  - Internal device 2
	  *  - ...
	  *  - Internal device n
	  * @param options: The command options (e.g. output-format)
	 **/
	std::string create_tDisk(const std::vector<std::string> &args, Options &options);

	/**
	  * Adds a new tDisk to the system.
	  * @param args:
	  *  - (Optional: The tDisk minor number, e.g. 0)
	  *  - The blocksize of the tDisk, e.g. 16384
	  * @param options: The command options (e.g. output-format)
	 **/
	std::string add_tDisk(const std::vector<std::string> &args, Options &options);

	/**
	  * Removes a tDisk from the system.
	  * @param args:
	  *  - tDisk minor number (e.g. 0) or path (e.g. /dev/td0)
	  * @param options: The command options (e.g. output-format)
	 **/
	std::string remove_tDisk(const std::vector<std::string> &args, Options &options);

	/**
	  * Adds one or more nacking files to a tDisk.
	  * @param args:
	  *  - tDisk minor number (e.g. 0) or path (e.g. /dev/td0)
	  *  - Backing file to add (path to file or disk or plugin name)
	  *  - ...
	  * @param options: The command options (e.g. output-format)
	 **/
	std::string add_disk(const std::vector<std::string> &args, Options &options);

	/**
	  * Gets the current maximum amount of sectors (defined
	  * by the blocksize and header) for the given tDisk
	  * @param args:
	  * - tDisk minor number (e.g. 0) or path (e.g. /dev/td0)
	  * @param options: The command options (e.g. output-format)
	 **/
	std::string get_max_sectors(const std::vector<std::string> &args, Options &options);

	/**
	  * Gets the current size in bytes (defined
	  * by size_blocks and blocksize) for the given tDisk
	  * @param args:
	  * - tDisk minor number (e.g. 0) or path (e.g. /dev/td0)
	  * @param options: The command options (e.g. output-format)
	 **/
	std::string get_size_bytes(const std::vector<std::string> &args, Options &options);

	/**
	  * Gets the sector information for the given logical
	  * sector of the given tDisk
	  * @param args:
	  *  - tDisk minor number (e.g. 0) or path (e.g. /dev/td0)
	  *  - Logical sector number (e.g. 1234)
	  * @param options: The command options (e.g. output-format)
	  * @return A stringified version of td::c::f_sector_index
	 **/
	std::string get_sector_index(const std::vector<std::string> &args, Options &options);

	/**
	  * Gets the sector information of all sectors of the
	  * give tDisk.
	  * @param args:
	  *  - tDisk minor number (e.g. 0) or path (e.g. /dev/td0)
	  * @param options: The command options (e.g. output-format)
	  * @return A stringified version of an array of td::c::f_sector_index
	 **/
	std::string get_all_sector_indices(const std::vector<std::string> &args, Options &options);

	/**
	  * Sets the access count for all sectors of the given
	  * tDisk to 0.
	  * @param args:
	  *  - tDisk minor number (e.g. 0) or path (e.g. /dev/td0)
	  * @param options: The command options (e.g. output-format)
	 **/
	std::string clear_access_count(const std::vector<std::string> &args, Options &options);

	/**
	  * Gets the current number of internal devices of the
	  * given tDisk
	  * @param args:
	  *  - tDisk minor number (e.g. 0) or path (e.g. /dev/td0)
	  * @param options: The command options (e.g. output-format)
	 **/
	std::string get_internal_devices_count(const std::vector<std::string> &args, Options &options);

	/**
	  * Returns information about the internal device with
	  * the given index number of the given tDisk.
	  * @param args:
	  *  - tDisk minor number (e.g. 0) or path (e.g. /dev/td0)
	  *  - Device number (e.g. 1)
	  * @param options: The command options (e.g. output-format)
	 **/
	std::string get_device_info(const std::vector<std::string> &args, Options &options);

	/**
	  * Loads the given config file @see ConfigFile
	  * @param args:
	  *  - Path to the config file to be loaded
	  * @param options: The command options (e.g. output-format)
	  * @return A stringified version of td::configuration
	  * which contains all commands which have been loaded.
	  * Ideally, this result should be the same as the
	  * comfig file.
	 **/
	std::string load_config_file(const std::vector<std::string> &args, Options &options);

	/**
	  * Returns a device on how to configure a given list of
	  * devices optimally.
	  * @param args:
	  *  - File or disk or plugin
	  *  - ...
	  * @param options: The command options (e.g. output-format)
	  * @return A stringified version of an array of td::advisor::tdisk_advice
	 **/
	std::string get_device_advice(const std::vector<std::string> &args, Options &options);

} //end namespace td

#endif //BACKEND_HPP

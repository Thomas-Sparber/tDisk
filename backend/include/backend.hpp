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

#include <backendresult.hpp>
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
	BackendResult get_tDisks(const std::vector<std::string> &args, Options &options);

	/**
	  * Gets information about the given tDisk
	  * @param args:
	  *  - tDisk minor number (e.g. 0) or path (e.g. /dev/td0)
	  * @param options: The command options (e.g. output-format)
	 **/
	BackendResult get_tDisk(const std::vector<std::string> &args, Options &options);

	/**
	  * Gets all available devices which could be used for a tDisk
	  * @param args: not needed
	  * @param options: The command options (e.g. output-format)
	 **/
	BackendResult get_devices(const std::vector<std::string> &args, Options &options);

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
	BackendResult create_tDisk(const std::vector<std::string> &args, Options &options);

	/**
	  * Adds a new tDisk to the system.
	  * @param args:
	  *  - (Optional: The tDisk minor number, e.g. 0)
	  *  - The blocksize of the tDisk, e.g. 16384
	  * @param options: The command options (e.g. output-format)
	 **/
	BackendResult add_tDisk(const std::vector<std::string> &args, Options &options);

	/**
	  * Removes a tDisk from the system.
	  * @param args:
	  *  - tDisk minor number (e.g. 0) or path (e.g. /dev/td0)
	  * @param options: The command options (e.g. output-format)
	 **/
	BackendResult remove_tDisk(const std::vector<std::string> &args, Options &options);

	/**
	  * Adds one or more backing files to a tDisk.
	  * @param args:
	  *  - tDisk minor number (e.g. 0) or path (e.g. /dev/td0)
	  *  - Backing file to add (path to file or disk or plugin name)
	  *  - ...
	  * @param options: The command options (e.g. output-format)
	 **/
	BackendResult add_disk(const std::vector<std::string> &args, Options &options);

	/**
	  * Removes a backing file from a tDisk.
	  * @param args:
	  *  - tDisk minor number (e.g. 0) or path (e.g. /dev/td0)
	  *  - Backing file to remove (device id)
	  * @param options: The command options (e.g. output-format)
	 **/
	BackendResult remove_disk(const std::vector<std::string> &args, Options &options);

	/**
	  * Gets the current maximum amount of sectors (defined
	  * by the blocksize and header) for the given tDisk
	  * @param args:
	  * - tDisk minor number (e.g. 0) or path (e.g. /dev/td0)
	  * @param options: The command options (e.g. output-format)
	 **/
	BackendResult get_max_sectors(const std::vector<std::string> &args, Options &options);

	/**
	  * Gets the current size in bytes (defined
	  * by size_blocks and blocksize) for the given tDisk
	  * @param args:
	  * - tDisk minor number (e.g. 0) or path (e.g. /dev/td0)
	  * @param options: The command options (e.g. output-format)
	 **/
	BackendResult get_size_bytes(const std::vector<std::string> &args, Options &options);

	/**
	  * Gets the sector information for the given logical
	  * sector of the given tDisk
	  * @param args:
	  *  - tDisk minor number (e.g. 0) or path (e.g. /dev/td0)
	  *  - Logical sector number (e.g. 1234)
	  * @param options: The command options (e.g. output-format)
	  * @return A stringified version of td::c::f_sector_index
	 **/
	BackendResult get_sector_index(const std::vector<std::string> &args, Options &options);

	/**
	  * Gets the sector information of all sectors of the
	  * give tDisk.
	  * @param args:
	  *  - tDisk minor number (e.g. 0) or path (e.g. /dev/td0)
	  * @param options: The command options (e.g. output-format)
	  * @return A stringified version of an array of td::c::f_sector_index
	 **/
	BackendResult get_all_sector_indices(const std::vector<std::string> &args, Options &options);

	/**
	  * Sets the access count for all sectors of the given
	  * tDisk to 0.
	  * @param args:
	  *  - tDisk minor number (e.g. 0) or path (e.g. /dev/td0)
	  * @param options: The command options (e.g. output-format)
	 **/
	BackendResult clear_access_count(const std::vector<std::string> &args, Options &options);

	/**
	  * Gets the current number of internal devices of the
	  * given tDisk
	  * @param args:
	  *  - tDisk minor number (e.g. 0) or path (e.g. /dev/td0)
	  * @param options: The command options (e.g. output-format)
	 **/
	BackendResult get_internal_devices_count(const std::vector<std::string> &args, Options &options);

	/**
	  * Returns information about the internal device with
	  * the given index number of the given tDisk.
	  * @param args:
	  *  - tDisk minor number (e.g. 0) or path (e.g. /dev/td0)
	  *  - Device number (e.g. 1)
	  * @param options: The command options (e.g. output-format)
	 **/
	BackendResult get_device_info(const std::vector<std::string> &args, Options &options);

	/**
	  * Returns debug information about the tdisk. When the current
	  * debug id is given, then the next debug info is returned
	  * @param args:
	  *  - tDisk minor number (e.g. 0) or path (e.g. /dev/td0)
	  *  - (Optional: The current debug id)
	  * @param options: The command options (e.g. output-format)
	 **/
	BackendResult get_debug_info(const std::vector<std::string> &args, Options &options);

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
	BackendResult load_config_file(const std::vector<std::string> &args, Options &options);

	/**
	  * Returns a device on how to configure a given list of
	  * devices optimally.
	  * @param args:
	  *  - File or disk or plugin
	  *  - ...
	  * @param options: The command options (e.g. output-format)
	  * @return A stringified version of an array of td::advisor::tdisk_advice
	 **/
	BackendResult get_device_advice(const std::vector<std::string> &args, Options &options);

	/**
	  * Returns all files which are located on the given disk
	  * at the given position
	  * @param args:
	  *  - Disk
	  *  - Start in bytes
	  *  - End in bytes
	  * @param options: The command options (e.g. output-format)
	  * @return An array of the files
	 **/
	BackendResult get_files_at(const std::vector<std::string> &args, Options &options);

	/**
	  * Returns all files which are located on the given internal device
	  * of the given tDisk
	  * @param args:
	  *  - tDisk
	  *  - Internal device index
	  * @param options: The command options (e.g. output-format)
	  * @return An array of the files
	 **/
	BackendResult get_files_on_disk(const std::vector<std::string> &args, Options &options);

	/**
	  * Runs the tDisk post create script which creates a filesystem,
	  * mounts it, creates a data folder (to hide lost+found) and returns
	  * the mount point
	  * @param args:
	  *  - tDisk
	  * @param options: The command options (e.g. output-format)
	  * @return The mountpoint of the tDisk
	 **/
	BackendResult tdisk_post_create(const std::vector<std::string> &args, Options &options);

	/**
	  * Runs the tDisk pre remove script which just unmounts the tDisk.
	  * @param args:
	  *  - tDisk
	  * @param options: The command options (e.g. output-format)
	  * @return
	 **/
	BackendResult tdisk_pre_remove(const std::vector<std::string> &args, Options &options);

	/**
	  * Returns the performance improvement in percent of the tDisk
	  * and lists the top 20 files with the highest performance improvement
	  * @param args:
	  *  - tDisk
	  * @param options: The command options (e.g. output-format)
	  * @return The performance imrpovement of the tDisk
	 **/
	BackendResult performance_improvement(const std::vector<std::string> &args, Options &options);

	/**
	  * Returns the usage of the internal devices in percent
	  * @param args:
	  *  - tDisk
	  * @param options: The command options (e.g. output-format)
	  * @return The internal disk usages in percent
	 **/
	BackendResult get_internal_device_usage(const std::vector<std::string> &args, Options &options);

} //end namespace td

#endif //BACKEND_HPP

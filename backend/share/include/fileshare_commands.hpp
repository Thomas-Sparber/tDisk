/**
  *
  * tDisk backend share
  * @author Thomas Sparber (2015-2016)
  *
 **/

#ifndef FILESHARE_COMMANDS_HPP
#define FILESHARE_COMMANDS_HPP

#include <string>
#include <vector>

#include <backendresult.hpp>
#include <options.hpp>

namespace td
{
	
	Options getDefaultFileshareOptions();

	BackendResult list_fileshares(const std::vector<std::string> &args, Options &options);

	BackendResult list_fileshare(const std::vector<std::string> &args, Options &options);

}

#endif //FILESHARE_COMMANDS_HPP

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

}

#endif //FILESHARE_COMMANDS_HPP

#include <fileshare_commands.hpp>

using std::string;
using std::vector;

using namespace td;

Options td::getDefaultFileshareOptions()
{
	return Options({
		Option::output_format
	});
}

BackendResult td::list_fileshares(const vector<string> &args, Options &options)
{
	BackendResult result;

	return std::move(result);
}

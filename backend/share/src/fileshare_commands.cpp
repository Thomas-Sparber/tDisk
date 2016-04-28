/**
  *
  * tDisk backend share
  * @author Thomas Sparber (2015-2016)
  *
 **/

#include <fileshare.hpp>
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

BackendResult td::list_fileshares(const vector<string> &/*args*/, Options &options)
{
	BackendResult res;

	vector<string> shares;
	Fileshare::getAllFileshares(shares);

	res.result(shares, options.getOptionValue("output-format"));

	return std::move(res);
}

BackendResult td::list_fileshare(const vector<string> &args, Options &options)
{
	BackendResult res;

	if(args.empty())
	{
		res.error(BackendResultType::general, "\"list_fileshare\" needs the name of the share to print information for");
		return std::move(res);
	}

	Fileshare share = Fileshare::load(args[0]);

	res.result(share, options.getOptionValue("output-format"));

	return std::move(res);
}

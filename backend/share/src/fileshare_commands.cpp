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

BackendResult td::create(const vector<string> &args, Options &/*options*/)
{
	BackendResult res;

	if(args.size() < 2)
	{
		res.error(BackendResultType::general, "\"create\" needs at least the desired name and path");
		return std::move(res);
	}

	Fileshare share;
	if(args.size() == 2)share = Fileshare::create(args[0], args[1]);
	else share = Fileshare::create(args[0], args[1], args[2]);

	//res.result("", options.getOptionValue("output-format"));
	res.message(BackendResultType::general, utils::concat("Fileshare ",share.getName()," created successfully"));

	return std::move(res);
}

BackendResult td::remove_fileshare(const vector<string> &args, Options &/*options*/)
{
	BackendResult res;

	if(args.empty())
	{
		res.error(BackendResultType::general, "\"remove\" needs the name of the share to be deleted");
		return std::move(res);
	}

	Fileshare::remove(args[0]);

	res.message(BackendResultType::general, utils::concat("Fileshare ",args[0]," removed"));

	return std::move(res);
}

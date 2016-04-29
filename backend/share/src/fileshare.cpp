/**
  *
  * tDisk backend share
  * @author Thomas Sparber (2015-2016)
  *
 **/

#include <backendexception.hpp>
#include <fileshare.hpp>
#include <utils.hpp>

using std::string;
using std::vector;

using namespace td;

#ifdef __linux__

//sudo net usershare add my_share /media/cdrom0/ "Comment of my new share" Everyone:F guest_ok=y

void Fileshare::getAllFileshares(vector<string> &out)
{
	string result = utils::exec("net usershare list -l");
	utils::split(result, '\n', out);
}

Fileshare Fileshare::load(const std::string &name)
{
	string path;
	string description;
	vector<string> helper;
	string result = utils::exec(utils::concat("net usershare info -l \"",name,"\""));
	utils::split(result, '\n', helper);

	for(const string &line : helper)
	{
		vector<string> splits;
		utils::split(line, '=', splits);
		if(splits.size() < 2)continue;

		if(splits[0] == "path")
			path = splits[1];
		else if(splits[0] == "comment")
			description = splits[1];
	}

	if(name == "")throw BackendException("Invalid share: no name");
	if(path == "")throw BackendException("Invalid share ",name,": no path");

	return Fileshare(name, path, description);
}

Fileshare Fileshare::create(const string &name, const string &path, const string &description)
{
	string result = utils::exec(utils::concat("net usershare add \"",name,"\" \"",path,"\" \"",description,"\" Everyone:F guest_ok=y"));

	if(result != "")throw BackendException("Error when creating fileshare ",name,": ",result);

	return Fileshare(name, path, description);
}

void Fileshare::remove(const string &name)
{
	string result = utils::exec(utils::concat("net usershare delete \"",name,"\""));

	if(result != "")throw BackendException("Error when creating fileshare ",name,": ",result);
}

#else

#endif //__linux__

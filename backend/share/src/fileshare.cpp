/**
  *
  * tDisk backend share
  * @author Thomas Sparber (2015-2016)
  *
 **/

#include <iostream>

#include <backendexception.hpp>
#include <fileshare.hpp>
#include <logger.hpp>
#include <utils.hpp>

using std::cout;
using std::endl;
using std::string;
using std::vector;

using namespace td;

#ifdef __linux__

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

	if(result != "")throw BackendException("Error when removing fileshare ",name,": ",result);

	vector<string> help;
	utils::split(utils::exec("net status shares parseable"), '\n', help);
	for(const string &s : help)
	{
		vector<string> share;
		utils::split(s, '\\', share);
		if(share.size() < 2)continue;
		LOG(LogLevel::debug, "Checking open connection to share ",share[0]);
		if(share[0] == name)
		{
			LOG(LogLevel::debug, "Share ",share[0]," matches. Killing pid ",share[1]);
			utils::exec(utils::concat("kill -2 ", share[1]));
		}
	}
}

#else

void Fileshare::getAllFileshares(vector<string> &out)
{
	string result = utils::exec("net share");

	vector<string> help;
	utils::split(result, '\n', help, false);

	std::size_t i;
	for(i = 0; i < help.size()-1; ++i)
	{
		if(help[i].substr(0,3) == "---")
		{
			i++;
			break;
		}
	}

	for(; i < help.size()-1; ++i)
	{
		vector<string> splits;
		utils::split(help[i], ' ', splits, false);
		if(splits.size() < 3)continue;
		
		out.push_back(splits[0]);
	}
}

Fileshare Fileshare::load(const std::string &name)
{
	string path;
	string description;
	vector<string> help;
	string result = utils::exec(utils::concat("net share \"",name,"\""));

	utils::split(result, '\n', help, false);

	for(std::size_t i = 1; i < help.size()-1 && i < 3; ++i)
	{
		vector<string> splits;
		utils::split(help[i], ' ', splits, false);
		if(splits.size() < 2)continue;
		
		switch(i)
		{
		case 1:
			path = splits[1];
			break;
		case 2:
			description = splits[1];
			break;
		}
	}

	if(path == "")throw BackendException("Invalid share ",name,": no path");

	return Fileshare(name, path, description);
}

Fileshare Fileshare::create(const string &name, const string &path, const string &description)
{
	cout<<utils::concat("net share \"",name,"\"=\"",path,"\" /remark:\"",description,"\"")<<endl;
	string result = utils::exec(utils::concat("net share \"",name,"\"=\"",path,"\" /remark:\"",description,"\""));

	return Fileshare(name, path, description);
}

void Fileshare::remove(const string &name)
{
	utils::exec(utils::concat("net share \"",name,"\" /delete"));
}

#endif //__linux__

/**
  *
  * tDisk backend
  * @author Thomas Sparber (2015-2016)
  *
 **/

#include <fstream>
#include <iostream>
#include <sstream>
#include <string.h>
#include <strings.h>

#include <ci_string.hpp>
#include <configfile.hpp>
#include <backendexception.hpp>
#include <tdisk.hpp>

using std::cout;
using std::endl;
using std::fstream;
using std::getline;
using std::string;
using std::stringstream;
using std::vector;

using td::utils::ci_string;
using td::utils::concat;

using namespace td;

const char* loadTDisk(const char *it, const char *end, configuration::tdisk_config &out);
const char* loadOption(const ci_string &name, const char *it, const char *end, configuration::tdisk_global_option &out, const td::Options &options);

bool td::configuration::tdisk_config::isValid() const
{
	return
		minornumber != 0 &&
		blocksize != 0 &&
		blocksize % td::c::get_tdisk_blocksize_mod() == 0 &&
		!devices.empty();
}

void td::configuration::merge(const configuration &config)
{
	for(const configuration::tdisk_config &device : config.tdisks)
		addDevice(device);

	for(const configuration::tdisk_global_option &option : config.global_options)
		addOption(option);
}

void td::configuration::addDevice(const configuration::tdisk_config &device)
{
	for(const configuration::tdisk_config &orig_config : tdisks)
	{
		if(device.minornumber == orig_config.minornumber)
			throw BackendException("Minornumber ", orig_config.minornumber, " is used twice");

		for(const string new_device : device.devices)
		{
			for(const string orig_device : orig_config.devices)
			{
				if(new_device == orig_device)
					throw BackendException("Two different tDisks can't share the same device");
			}
		}
	}

	tdisks.push_back(device);
}

void td::configuration::addOption(const configuration::tdisk_global_option &option)
{
	bool found = false;

	for(configuration::tdisk_global_option &orig_option : global_options)
	{
		if(orig_option.name == option.name)
		{
			orig_option = option;
			found = true;
			break;
		}
	}

	if(!found)global_options.push_back(option);
}

void configuration::load(const string &file, const td::Options &options)
{
	string str;
	stringstream ss;
	fstream in(file, fstream::in);

	while(getline(in, str))
	{
		if(str.empty() || str[0] == '#')continue;
		ss<<str;
	}

	process(ss.str(), options);
}

void configuration::save(const string &file) const
{
	fstream out(file, fstream::out);

	out<<"# Auto generated config file by tDisk backend"<<endl<<endl;

	for(const configuration::tdisk_global_option &option : global_options)
		out<<createResultString(option, 0, "json")<<endl;

	for(const configuration::tdisk_config &tdisk : tdisks)
		out<<"tDisk: "<<createResultString(tdisk, 0, "json")<<endl;
}

const char* getName(const char *it, const char *end, ci_string &out)
{
	for(; it < end; ++it)
	{
		if(isspace(*it))continue;

		if(const char *colon = strchr(it, ':'))
		{
			const char *it2 = colon + 1;
			while(colon > it && isspace(*(colon-1)))colon--;

			if(*it == '"' || *it == '\'')
			{
				it++;
				if(*(colon-1) != '"' && *(colon-1) != '\'')throw BackendException("No trailing quote found for name");
				colon--;
			}

			out = ci_string(it, colon);
			return it2;
		}
		else throw BackendException("Didn't find a colon");
	}

	throw BackendException("No name found");
}

void configuration::process(const string &str, const td::Options &options)
{
	const char *end = str.c_str()+str.length();
	for(const char *it = str.c_str(); it < end; ++it)
	{
		ci_string name;
		it = getName(it, end, name);

		if(name == "tdisk")
		{
			configuration::tdisk_config tdc;
			it = loadTDisk(it, end, tdc);
			addDevice(std::move(tdc));
		}
		else
		{
			configuration::tdisk_global_option tgo;
			it = loadOption(name, it, end, tgo, options);
			addOption(std::move(tgo));
		}

		if(it < end && *it == ',')it++;
	}
}

const char* getClosingbracket(const char *it, const char *end, const char o, const char c)
{
	const char *it2;
	unsigned int brackets = 1;
	bool instring = false;
	bool escape = false;
	for(it2 = it+1; it2 < end; ++it2)
	{
		if(*it2 == o && !instring && !escape)brackets++;
		if(*it2 == c && !instring && !escape)brackets--;
		if((*it2 == '"' || *it2 == '\'') && !escape)instring = !instring;
		escape = *it2 == '\\';
		if(brackets == 0)break;
	}

	if(brackets == 0)return it2;
	else return nullptr;
}

const char* getStringValue(const char *it, const char *end, string &out)
{
	while(isspace(*it) && it < end)it++;

	bool escape = false;
	bool instring = false;
	const char *it2;
	for(it2 = it; it2 < end; ++it2)
	{
		if(isspace(*it2) && !instring && !escape)break;
		if((*it2 == '"' || *it2 == '\'') && !escape)
		{
			if(instring)break;
			else instring = true;
		};
		escape = *it2 == '\\';
	}

	const char *ret = end = it2;

	if(*(end-1) == ',')end--;
	if(*it == '"' || *it == '\'')
	{
		it++;
		if(*end != '"' && *end != '\'')throw BackendException("String not closed: ", *end);
	}

	out = string(it, end);
	return ret;
}

template <class T>
const char* getLongValue(const char *it, const char *end, T &out)
{
	string str;
	end = getStringValue(it, end, str);

	char *test;
	out = strtol(str.c_str(), &test, 10);
	if(test != str.c_str()+str.length())throw BackendException("Invalid number ", str);
	return end;
}

const char* getStringArrayValue(const char *it, const char *end, vector<string> &out)
{
	for(; it < end; ++it)
	{
		if(isspace(*it))continue;
		if(*it == '[')break;
		throw BackendException("Expected '['");
	}

	end = getClosingbracket(it, end, '[', ']');
	if(!end)throw BackendException("Expected ']' at the end of declaration");

	for(it = it+1; it < end; ++it)
	{
		if(isspace(*it))continue;

		string value;
		//cout<<"Before:"<<it<<endl;
		it = getStringValue(it, end, value);
		if(it+1 < end && *(it+1) == ',')it++;
		//cout<<"After:"<<it<<endl;
		out.push_back(value);
	}

	return end+1;
}

const char* loadTDisk(const char *it, const char *end, configuration::tdisk_config &out)
{
	//Set to an invalid value
	out.minornumber = -1;

	for(; it < end; ++it)
	{
		if(isspace(*it))continue;
		if(*it == '{')break;
		throw BackendException("Invalid tDisk config declaration: '{' expected");
	}

	end = getClosingbracket(it, end, '{', '}');
	if(!end)throw BackendException("Expected '}' at the end of tdisk declaration");

	for(it = it+1; it < end; ++it)
	{
		ci_string name;
		it = getName(it, end, name);

		if(name == "minornumber")it = getLongValue(it, end, out.minornumber);
		else if(name == "blocksize")it = getLongValue(it, end, out.blocksize);
		else if(name == "devices")it = getStringArrayValue(it, end, out.devices);
		else throw BackendException("Invalid tDisk option ", name);
	}

	//cout<<"New tDisk:"<<endl;
	//cout<<" - minornumber: "<<out.minornumber<<endl;
	//cout<<" - blocksize: "<<out.blocksize<<endl;
	//cout<<" - devices: "<<endl;
	//for(const string device: out.devices)
	//	cout<<"   - "<<device<<endl;

	if(out.minornumber == -1)
		throw BackendException("No minornumber specified");
	if(out.blocksize == 0 || out.blocksize % td::c::get_tdisk_blocksize_mod())
		throw BackendException("blocksize must be a multiple of ", td::c::get_tdisk_blocksize_mod());
	//if(out.devices.empty())
	//	throw BackendException("Please specify at least one device");
	if(out.devices.size() > td::c::get_tdisk_max_physical_disks())
		throw BackendException("Exceeded maximum number (", td::c::get_tdisk_max_physical_disks(),") of internal devices: ", out.devices.size());

	return end+1;
}

const char* loadOption(const ci_string &name, const char *it, const char *end, configuration::tdisk_global_option &out, const td::Options &options)
{
	if(!options.optionExists(name))throw BackendException("Option ", name, " does not exist!");

	string value;
	out.name = name;
	end = getStringValue(it, end, value);
	out.value = value.c_str();
	return end;
}

template <> std::string td::createResultString(const configuration &config, unsigned int hierarchy, const ci_string &outputFormat)
{
	if(outputFormat == "json")
		return concat(
			"{\n",
				std::vector<char>(hierarchy+1, '\t'), CREATE_RESULT_STRING_MEMBER_JSON(config, global_options, hierarchy+1, outputFormat), ",\n",
				std::vector<char>(hierarchy+1, '\t'), CREATE_RESULT_STRING_MEMBER_JSON(config, tdisks, hierarchy+1, outputFormat), "\n",
			std::vector<char>(hierarchy, '\t'), "}"
		);
	else if(outputFormat == "text")
		return concat(
				createResultString(config.global_options, hierarchy+1, outputFormat), "\n",
				createResultString(config.tdisks, hierarchy+1, outputFormat), "\n"
		);
	else
		throw FormatException("Invalid output-format ", outputFormat);
}

template <> std::string td::createResultString(const configuration::tdisk_global_option &option, unsigned int /*hierarchy*/, const ci_string &outputFormat)
{
	if(outputFormat == "json")
		return concat("\"", option.name, "\": \"", option.value, "\"");
	else if(outputFormat == "text")
		return concat(option.name, " = ", option.value);
	else
		throw FormatException("Invalid output-format ", outputFormat);
}

template <> std::string td::createResultString(const configuration::tdisk_config &config, unsigned int hierarchy, const ci_string &outputFormat)
{
	if(outputFormat == "json")
		return concat(
			"{\n",
				std::vector<char>(hierarchy+1, '\t'), CREATE_RESULT_STRING_MEMBER_JSON(config, minornumber, hierarchy+1, outputFormat), ",\n",
				std::vector<char>(hierarchy+1, '\t'), CREATE_RESULT_STRING_MEMBER_JSON(config, blocksize, hierarchy+1, outputFormat), ",\n",
				std::vector<char>(hierarchy+1, '\t'), CREATE_RESULT_STRING_MEMBER_JSON(config, devices, hierarchy+1, outputFormat), "\n",
			std::vector<char>(hierarchy, '\t'), "}"
		);
	else if(outputFormat == "text")
		return concat(
				CREATE_RESULT_STRING_MEMBER_TEXT(config, minornumber, hierarchy+1, outputFormat), "\n",
				CREATE_RESULT_STRING_MEMBER_TEXT(config, blocksize, hierarchy+1, outputFormat), "\n",
				CREATE_RESULT_STRING_MEMBER_TEXT(config, devices, hierarchy+1, outputFormat), "\n"
		);
	else
		throw FormatException("Invalid output-format ", outputFormat);
}

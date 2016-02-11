#include <functional>
#include <iostream>
#include <string>
#include <vector>
#include <map>

#include <ci_string.hpp>
#include <tdisk.hpp>
#include <frontend.hpp>

using std::cerr;
using std::cout;
using std::endl;
using std::find;
using std::function;
using std::map;
using std::pair;
using std::string;
using std::vector;

using namespace td;

struct Command
{
	Command(const ci_string &str_name, function<string(const vector<string>&,const ci_string&)> fn_func) : name(str_name), func(fn_func) {}
	ci_string name;
	function<string(const vector<string>&,const ci_string&)> func;
}; //end struct Command

struct Option
{
	Option(const ci_string &str_name, const vector<ci_string> &v_values) : name(str_name), values(v_values), value(values.empty() ? "" : values[0]) {}
	ci_string name;
	vector<ci_string> values;
	ci_string value;
}; //end struct option

std::string handleCommand(int argc, char **args);
void printHelp(const string &progName);

vector<Command> commands {
	Command("add", add_tDisk),
	Command("remove", remove_tDisk),
	Command("add_disk", add_disk),
	Command("get_max_sectors", get_max_sectors),
	Command("get_sector_index", get_sector_index),
	Command("get_all_sector_indices", get_all_sector_indices),
	Command("clear_access_count", clear_access_count),
	Command("get_internal_devices_count", get_internal_devices_count),
	Command("get_device_info", get_device_info)
};

vector<Option> options {
	Option("output-format", { "text", "json" })
};

ci_string getOptionValue(const ci_string &name)
{
	for(const Option &option : options)
	{
		if(option.name == name)
			return option.value;
	}

	throw FrontendException("Option \"", name ,"\" is not set");
}

int main(int argc, char *args[])
{
	string result;
	string error;

	try {
		result = handleCommand(argc, args);
	} catch(const FrontendException &e) {
		error = e.what;
	} catch(const tDiskException &e) {
		error = e.message;
	}

	if(error != "")cerr<<error<<endl;
	else cout<<result<<endl;

	return 0;
}

string handleCommand(int argc, char **args)
{
	string programName = args[0];

	ci_string option;
	while(argc > 1 && (option = args[1]).substr(0, 2) == "--")
	{
		argc--; args++;
		bool found = false;
		size_t pos = option.find("=");
		if(pos == option.npos)throw FrontendException("Invalid option format ", option);
		ci_string name = option.substr(2, pos-2);
		ci_string value = option.substr(pos+1);

		for(Option &o : options)
		{
			if(o.name == name)
			{
				found = true;
				if(!o.values.empty())
				{
					bool valueFound = false;
					for(const ci_string &v : o.values)
					{
						if(v == value)
						{
							valueFound = true;
							break;
						}
					}

					if(!valueFound)
						throw FrontendException("Invalid value \"", value ,"\" for option ", name);
				}
				o.value = value;
				break;
			}
		}

		if(!found)throw FrontendException("Invalid option \"", option ,"\"");
	}

	if(argc <= 1)
	{
		printHelp(programName);
		throw FrontendException("Please provide a command");
	}

	ci_string cmd = args[1];

	for(const Command &command : commands)
	{
		if(command.name == cmd)
		{
			vector<string> v_args(args+2, args+argc);
			return command.func(v_args, getOptionValue("output-format"));
		}
	}

	printHelp(programName);
	throw FrontendException("Unknown command ", cmd);
}

void printHelp(const string &progName)
{
	cout<<"Usage:"<<endl;
	cout<<endl;
	cout<<progName<<" [command] [command-options]"<<endl;
}

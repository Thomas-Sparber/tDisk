#include <functional>
#include <iostream>
#include <string>
#include <vector>
#include <map>

#include <ci_string.hpp>
#include <tdisk.hpp>
#include <backend.hpp>
#include <backendexception.hpp>

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

using utils::ci_string;

struct Command
{
	Command(const ci_string &str_name, function<string(const vector<string>&,Options&)> fn_func) : name(str_name), func(fn_func) {}
	ci_string name;
	function<string(const vector<string>&,Options&)> func;
}; //end struct Command

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
	Command("get_device_info", get_device_info),
	Command("load_config_file", load_config_file),
	Command("get_device_advice", get_device_advice)
};

int main(int argc, char *args[])
{
	string result;
	string error;

	try {
		result = handleCommand(argc, args);
	} catch(const BackendException &e) {
		error = e.what;
	} catch(const tDiskException &e) {
		error = e.message;
	} catch(const FormatException &e) {
		error = e.what;
	}

	if(error != "")cerr<<error<<endl;
	else cout<<result<<endl;

	return 0;
}

string handleCommand(int argc, char **args)
{
	string programName = args[0];
	td::Options options = getDefaultOptions();

	ci_string option;
	while(argc > 1 && (option = args[1]).substr(0, 2) == "--")
	{
		argc--; args++;
		size_t pos = option.find("=");
		if(pos == option.npos)throw BackendException("Invalid option format ", option);
		ci_string name = option.substr(2, pos-2);
		ci_string value = option.substr(pos+1);

		options.setOptionValue(name, value);
	}

	if(argc <= 1)
	{
		printHelp(programName);
		throw BackendException("Please provide a command");
	}

	ci_string cmd = args[1];

	for(const Command &command : commands)
	{
		if(command.name == cmd)
		{
			vector<string> v_args(args+2, args+argc);
			return command.func(v_args, options);
		}
	}

	printHelp(programName);
	throw BackendException("Unknown command ", cmd);
}

void printHelp(const string &progName)
{
	cout<<"Usage:"<<endl;
	cout<<endl;
	cout<<progName<<" [command] [command-options]"<<endl;
}

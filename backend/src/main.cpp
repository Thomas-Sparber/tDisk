/**
  *
  * tDisk backend
  * @author Thomas Sparber (2015-2016)
  *
 **/

#include <functional>
#include <iostream>
#include <string>
#include <vector>
#include <map>

#include <ci_string.hpp>
#include <tdisk.hpp>
#include <backend.hpp>
#include <backendexception.hpp>
#include <utils.hpp>

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

/**
  * This struct defines one backend command.
  * it has a name, a corresponding functio
  * and a description
 **/
struct Command
{
	Command(const ci_string &str_name, function<BackendResult(const vector<string>&,Options&)> fn_func, const string &str_description) :
		name(str_name),
		func(fn_func),
		description(str_description)
	{}
	ci_string name;
	function<BackendResult(const vector<string>&,Options&)> func;
	string description;
}; //end struct Command

std::string handleCommand(int argc, char **args, const string &defaultConfigFile);
void printHelp(const string &progName);

vector<Command> commands {
	Command("get_tdisks", get_tDisks,
		"Prints a list of all available tDisks"),

	Command("get_tdisk", get_tDisk,
		"Prints details about the given tDisk. It needs the minornumber or\n"
		"path as argument"),
		
	Command("get_devices", get_devices,
		"Prints all available devices which could be used for a tDisk."),

	Command("create", create_tDisk,
		"Creates a new tDisk. It needs the blocksize (e.g. 16386) and all\n"
		"the desired devices (in this order) as arguments. It is also possible\n"
		"possible to specify the minornumber: [minornumber] [blocksize] ..."),

	Command("add", add_tDisk,
		"Adds a new tDisk to the system. It needs the blocksize (e.g. 16386)\n"
		"as argument. The next free minornumber is taken. It is also possible\n"
		"possible to specify the minornumber: [minornumber] [blocksize]"),
	
	Command("remove", remove_tDisk,
		"Removes the given tDisk from the system. It needs the minornumber or\n"
		"path as argument"),
	
	Command("add_disk", add_disk,
		"Adds a new internal device to the tDisk. If it is a file path it is\n"
		"added as file. If not it is added as a plugin. It needs the tDisk\n"
		"minornumber/path and filename/pluginname as argument"),
	
	Command("get_max_sectors", get_max_sectors,
		"Gets the current maximum amount of sectors for the given tDisk. It\n"
		"needs the tDisk minornumber/path as argument"),
	
	Command("get_size_bytes", get_size_bytes,
		"Gets the size in bytes for the given tDisk. It needs the tDisk\n"
		"minornumber/path as argument"),
	
	Command("get_sector_index", get_sector_index,
		"Gets information about the given sector index. It needs the tDisk\n"
		"minornumber/path and logical sector (0-max_sectors) as argument"),
	
	Command("get_all_sector_indices", get_all_sector_indices,
		"Returns infomration about all sector indices. It needs the tDisk\n"
		"minornumber/path as argument"),
	
	Command("clear_access_count", clear_access_count,
		"Resets the access count of all sectors. It needs the tDisk\n"
		"minornumber/path as argument"),
	
	Command("get_internal_devices_count", get_internal_devices_count,
		"Gets the amount of internal devices for the given tDisk. It needs\n"
		"the tDisk minornumber/path as argument"),
	
	Command("get_device_info", get_device_info,
		"Gets device information of the device with the given id. It needs\n"
		"the tDisk minornumber/path and device id as argument"),
	
	Command("load_config_file", load_config_file,
		"Loads the given config file. It needs the path to the config file as\n"
		"argument"),
	
	Command("get_device_advice", get_device_advice,
		"Returns a device on how to configure a given list of devices\n"
		"optimally. It needs all desired devices as agrument")
};

vector<string> configfiles = {
	"/etc/tdisk/tdisk.config",
	"./tdisk.config",
	"tdisk.config",
	"./example.config",
	"example.config"
};

int main(int argc, char *args[])
{
	string result;
	string error;

	string configFile;

	for(string &f : configfiles)
	{
		const string &path = utils::dirnameOf(f, args[0]);
		const string &file = utils::filenameOf(f);

		f = utils::concatPath(path, file);
		if(utils::fileExists(f))
		{
			configFile = f;
			break;
		}
	}

	try {
		result = handleCommand(argc, args, configFile);
	} catch(const BackendException &e) {
		error = e.what();
	} catch(const FormatException &e) {
		error = e.what();
	} catch(const tDiskException &e) {
		error = e.what();
	}

	if(error != "")cerr<<error<<endl;
	else cout<<result<<endl;

	return 0;
}

string handleCommand(int argc, char **args, const string &defaultConfigFile)
{
	string programName = args[0];
	td::Options options = getDefaultOptions();

	Option configfile(
		"configfile",
		"The file where all configuration data should be read/written."
	);
	configfile.setValue(defaultConfigFile);
	options.addOption(configfile);

	Option configonly(
		"config-only",
		"A flag whether the operations should be writte to config file only.\n"
		"If this flag is set to yes, nothing is done on the actual system",
		{ "yes", "no" }
	);
	configonly.setValue("no");
	options.addOption(configonly);

	Option temporary(
		"temporary",
		"A flag whether the operations should only be done temporary. This\n"
		"means the actions are performed, but not stored to any config file.",
		{ "yes", "no" }
	);
	temporary.setValue("no");
	options.addOption(temporary);

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

	if(options.getOptionBoolValue("config-only") && options.getOptionBoolValue("temporary"))
		throw new BackendException("config-only and temporary cannot be set at the same time.");

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
			BackendResult r = command.func(v_args, options);
			//TODO error handling
			return r.result();
		}
	}

	printHelp(programName);
	throw BackendException("Unknown command ", cmd);
}

void printHelp(const string &progName)
{
	cout<<"Usage: "<<progName<<" ([options]) [command] [command-arguments...]"<<endl;
	cout<<endl;
	cout<<"The following options are available:"<<endl;
	const td::Options allOptions = getDefaultOptions();
	for(const Option &o : allOptions.getAllOptions())
	{
		std::size_t pos = 0;
		string description = o.getDescription();
		while((pos=description.find("\n", pos)) != string::npos)
		{
			description = description.replace(pos, 1, "\n\t   ");
			pos++;
		}

		cout<<"\t --"<<o.getName()<<endl;
		cout<<"\t   "<<description<<endl;
	}
	cout<<endl;

	cout<<"The following commands are available:"<<endl;
	for(const Command &c : commands)
	{
		std::size_t pos = 0;
		string description = c.description;
		while((pos=description.find("\n", pos)) != string::npos)
		{
			description = description.replace(pos, 1, "\n\t   ");
			pos++;
		}
		
		cout<<"\t - "<<c.name<<endl;
		cout<<"\t   "<<description<<endl;
	}
	cout<<endl;
}

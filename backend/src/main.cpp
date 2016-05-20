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
#include <extinodescan.hpp>
#include <inodescanfake.hpp>
#include <tdisk.hpp>
#include <backend.hpp>
#include <backendexception.hpp>
#include <shell.hpp>
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

BackendResult handleCommand(int argc, char **args);
void printHelp();

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
		"optimally. It needs all desired devices as agrument"),
	
	Command("get_files_at", get_files_at,
		"Returns all files which are located on the given disk at the given\n"
		"position. It needs the device, start and end bytes as argument"),
	
	Command("get_files_on_disk", get_files_on_disk,
		"Returns all files which are located on the given internal device of\n"
		"the given tDisk. It needs the tDisk, and device index as argument"),
	
	Command("tdisk_post_create", tdisk_post_create,
		"Runs the tDisk post create script which creates a filesystem,\n"
		"mounts it, creates a data folder (to hide lost+found) and prints\n"
		"the mount point. It needs the desired tDisk as argument"),
	
	Command("performance_improvement", performance_improvement,
		"Returns the performance improvement in percent of the tDisk and\n"
		"lists the top 20 files with the highest performance improvement.\n"
		"It needs the desired tDisk as argument")
};

vector<string> configfiles = {
	"/etc/tdisk/tdisk.config",
	"./tdisk.config",
	"tdisk.config",
	"./example.config",
	"example.config"
};

string programName;
Options options;

//The ext filesystem is only available on linux
#ifdef __linux__
bool extRegistered = InodeScan::registerInodeScan<ExtInodeScan>();
#else
bool extRegistered = InodeScan::registerInodeScan<InodeScanFake>();
#endif //__linux__

int main(int argc, char *args[])
{
	shell::initShell(args[0]);
	programName = args[0];
	options = getDefaultOptions();

	options.addOption(Option(
		"configfile",
		"The file where all configuration data should be read/written."
	));

	options.addOption(Option(
		"config-only",
		"A flag whether the operations should be writte to config file only.\n"
		"If this flag is set to yes, nothing is done on the actual system",
		{ "no", "yes" }
	));

	options.addOption(Option(
		"temporary",
		"A flag whether the operations should only be done temporary. This\n"
		"means the actions are performed, but not stored to any config file.",
		{ "no", "yes" }
	));

	BackendResult result = handleCommand(argc, args);

	const string &errors = result.errors();
	const string &warnings = result.warnings();
	const string &messages = result.messages();

	if(options.getOptionValue("output-format") == "json")
	{
		BackendErrorCode errorCode = result.errorCode();

		cout<<"{"<<endl;
		cout<<"\t"<<utils::concat(CREATE_RESULT_STRING_NONMEMBER_JSON(errorCode, 1, "json"))<<","<<endl;
		cout<<"\t"<<utils::concat(CREATE_RESULT_STRING_NONMEMBER_JSON(errors, 1, "json"))<<","<<endl;
		cout<<"\t"<<utils::concat(CREATE_RESULT_STRING_NONMEMBER_JSON(warnings, 1, "json"))<<","<<endl;
		cout<<"\t"<<utils::concat(CREATE_RESULT_STRING_NONMEMBER_JSON(messages, 1, "json"))<<","<<endl;

		for(BackendResultType type : result.getResultTypes())
		{
			cout<<"\t"<<createResultString(type, 1, "json")<<": "<<createResultString(result.getIndividualResult(type), 1, "json")<<","<<endl;
		}

		cout<<"\t\"result\": "<<utils::replaceAll(result.result().empty() ? "null" : result.result(), "\n", "\n\t")<<endl;
		cout<<"}"<<endl;
	}
	else if(options.getOptionValue("output-format") == "text")
	{
		if(!result.result().empty())cout<<result.result()<<endl;

		if(!errors.empty())cerr<<"Errors:"<<endl<<errors<<endl;
		if(!warnings.empty())cerr<<"Warnings:"<<endl<<warnings<<endl;
		if(!messages.empty())cerr<<"Messages:"<<endl<<messages<<endl;
	}

	switch(result.errorCode())
	{
		case BackendErrorCode::Success: return 0;
		case BackendErrorCode::Error: return 1;
		case BackendErrorCode::Warning: return 2;
	}
}

BackendResult handleCommand(int argc, char **args)
{
	for(string &f : configfiles)
	{
		const string &path = utils::dirnameOf(f, args[0]);
		const string &file = utils::filenameOf(f);

		f = utils::concatPath(path, file);
		if(utils::fileExists(f))
		{
			options.setOptionValue("configfile", f.c_str());
			break;
		}
	}	

	BackendResult r;
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
	{
		r.error(BackendResultType::general, "config-only and temporary cannot be set at the same time.");
		return std::move(r);
	}

	if(argc <= 1)
	{
		printHelp();
		r.error(BackendResultType::general, "Please provide a command");
		return std::move(r);
	}

	ci_string cmd = args[1];

	for(const Command &command : commands)
	{
		if(command.name == cmd)
		{
			vector<string> v_args(args+2, args+argc);
			r = command.func(v_args, options);
			return std::move(r);
		}
	}

	printHelp();
	r.error(BackendResultType::general, utils::concat("Unknown command ", cmd));
	return std::move(r);
}

void printHelp()
{
	cout<<"Usage: "<<programName<<" ([options]) [command] [command-arguments...]"<<endl;
	cout<<endl;
	cout<<"The following options are available:"<<endl;
	for(const Option &o : options.getAllOptions())
	{
		string description = utils::replaceAll(o.getDescription(), "\n", "\n\t   ");

		cout<<"\t --"<<o.getName()<<"=";
		if(o.hasPredefinedValues())
		{
			cout<<"[";
			bool first = true;
			for(const utils::ci_string value : o.getPredefinedValues())
			{
				if(first)first = false;
				else cout<<",";
				cout<<value;
			}
			cout<<"]"<<endl;
		}
		else
		{
			cout<<"..."<<endl;
		}

		cout<<"\t   "<<description<<endl;
	}
	cout<<endl;

	cout<<"The following commands are available:"<<endl;
	for(const Command &c : commands)
	{
		string description = utils::replaceAll(c.description, "\n", "\n\t   ");
		
		cout<<"\t - "<<c.name<<endl;
		cout<<"\t   "<<description<<endl;
	}
	cout<<endl;
}

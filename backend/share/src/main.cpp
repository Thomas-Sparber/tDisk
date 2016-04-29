/**
  *
  * tDisk backend share
  * @author Thomas Sparber (2015-2016)
  *
 **/

#include <functional>
#include <iostream>
#include <string>
#include <vector>
#include <map>

#include <ci_string.hpp>
#include <backendexception.hpp>
#include <fileshare_commands.hpp>
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
	Command("list_fileshares", list_fileshares,
		"Prints a list of all available fileshares"),

	Command("list_fileshare", list_fileshare,
		"Prints information about the fileshare with the given name"),

	Command("create", create,
		"Creates a new fileshare using the given name, path and optional\n"
		"description"),

	Command("remove", remove_fileshare,
		"Removes the fileshare with the given name"),
};

vector<string> configfiles = {
	"/etc/tdisk/tdisk-shares.config",
	"./tdisk-shares.config",
	"tdisk-shares.config"
};

string programName;
Options options;

int main(int argc, char *args[])
{
	programName = args[0];
	options = getDefaultFileshareOptions();

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

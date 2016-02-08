#include <functional>
#include <iostream>
#include <string>
#include <vector>
#include <map>

#include <ci_string.hpp>
#include <tdisk.hpp>

using std::cerr;
using std::cout;
using std::endl;
using std::find;
using std::function;
using std::map;
using std::pair;
using std::string;
using std::vector;

using td::tDisk;
using td::tDiskException;

struct FrontendException
{
	template <class ...T> FrontendException(T ...t) : what(td::concat(t...)) {}
	string what;
}; //end class FrontendException

struct Command
{
	Command(const ci_string &str_name, function<string(int,char**)> fn_func) : name(str_name), func(fn_func) {}
	ci_string name;
	function<string(int,char**)> func;
}; //end struct Command

struct Option
{
	Option(const ci_string &str_name, const vector<ci_string> &v_values) : name(str_name), values(v_values), value(values.empty() ? "" : values[0]) {}
	ci_string name;
	vector<ci_string> values;
	ci_string value;
}; //end struct option

string handleCommand(int argc, char **args);
string add_tDisk(int argc, char *args[]);
string remove_tDisk(int argc, char *args[]);
string add_disk(int argc, char *args[]);
string get_max_sectors(int argc, char *args[]);
string get_sector_index(int argc, char *args[]);
string get_all_sector_indices(int argc, char *args[]);
string clear_access_count(int argc, char *args[]);
string get_internal_devices_count(int argc, char *args[]);
string get_device_info(int argc, char *args[]);
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

template <class T>
string createResultString(const T &t, unsigned int hierarchy)
{
	throw FrontendException("Can't create result string of type ", typeid(t).name());
}

template <class T>
string createResultString_string(const T &s)
{
	const ci_string &outputFormat = getOptionValue("output-format");
	if(outputFormat == "json")
		return td::concat('"', s, '"');
	else if(outputFormat == "text")
		return td::concat(s);
	else
		throw FrontendException("Invalid output-format ", outputFormat);
}

template <class T>
string createResultString_number(const T &i)
{
	const ci_string &outputFormat = getOptionValue("output-format");
	if(outputFormat == "json")
		return td::concat(i);
	else if(outputFormat == "text")
		return td::concat(i);
	else
		throw FrontendException("Invalid output-format ", outputFormat);
}

template <class T, template <class ...> class container>
string createResultString_array(const container<T> &v, unsigned int hierarchy)
{
	vector<string> temp(v.size());
	const ci_string &outputFormat = getOptionValue("output-format");
	if(outputFormat == "json")
		for(size_t i = 0; i < v.size(); ++i)
		{
			temp[i] = td::concat(
				(i == 0) ? "[\n" : "",
				vector<char>(hierarchy+1, '\t'), createResultString(v[i], hierarchy + 1),
				(i < v.size()-1) ? ",\n" : td::concat("\n", vector<char>(hierarchy, '\t'), "]"));
		}
	else if(outputFormat == "text")
		for(size_t i = 0; i < v.size(); ++i)
		{
			temp[i] = td::concat(
				createResultString(v[i], hierarchy + 1),
				(i < v.size()-1) ? "\n" : "");
		}
	else
		throw FrontendException("Invalid output-format ", outputFormat);

	return td::concat(temp);
}

template <class S, class T>
string createResultString_map(const map<S,T> &m, unsigned int hierarchy)
{
	size_t i = 0;
	vector<string> temp(m.size());
	const ci_string &outputFormat = getOptionValue("output-format");
	if(outputFormat == "json")
		for(const pair<S,T> &p : m)
		{
			temp[i] = td::concat(
				(i == 0) ? "{\n" : "",
				vector<char>(hierarchy+1, '\t'), createResultString(p.first, hierarchy + 1), ": ", createResultString(p.second, hierarchy + 1),
				(i < m.size()-1) ? ",\n" : td::concat("\n", vector<char>(hierarchy, '\t'), "}"));
			i++;
		}
	else if(outputFormat == "text")
		for(const pair<S,T> &p : m)
		{
			temp[i] = td::concat(
				createResultString(p.first, hierarchy + 1), " = ", createResultString(p.second, hierarchy + 1),
				(i < m.size()-1) ? "\n" : "");
			i++;
		}
	else
		throw FrontendException("Invalid output-format ", outputFormat);

	return td::concat(temp);
}

template <> string createResultString(const string &s, unsigned int /*hierarchy*/) { return createResultString_string(s); }
template <> string createResultString(const char &s, unsigned int /*hierarchy*/) { return createResultString_string(s); }
//template <> string createResultString(const char* &s, unsigned int /*hierarchy*/) { return createResultString_string(s); }
template <> string createResultString(const uint8_t &i, unsigned int /*hierarchy*/) { return createResultString_number((unsigned int)i); }
template <> string createResultString(const uint16_t &i, unsigned int /*hierarchy*/) { return createResultString_number(i); }
template <> string createResultString(const int &i, unsigned int /*hierarchy*/) { return createResultString_number(i); }
template <> string createResultString(const unsigned int &i, unsigned int /*hierarchy*/) { return createResultString_number(i); }
template <> string createResultString(const long &i, unsigned int /*hierarchy*/) { return createResultString_number(i); }
template <> string createResultString(const unsigned long &i, unsigned int /*hierarchy*/) { return createResultString_number(i); }
template <> string createResultString(const long long &i, unsigned int /*hierarchy*/) { return createResultString_number(i); }
template <> string createResultString(const unsigned long long &i, unsigned int /*hierarchy*/) { return createResultString_number(i); }
template <> string createResultString(const float &i, unsigned int /*hierarchy*/) { return createResultString_number(i); }
template <> string createResultString(const double &i, unsigned int /*hierarchy*/) { return createResultString_number(i); }
template <> string createResultString(const long double &i, unsigned int /*hierarchy*/) { return createResultString_number(i); }

template <class T> string createResultString(const vector<T> &v, unsigned int hierarchy) { return createResultString_array(v, hierarchy); }

#define CREATE_RESULT_STRING_MEMBER_JSON(object, member, hierarchy) #member, ": ", createResultString(object.member, hierarchy)
#define CREATE_RESULT_STRING_MEMBER_TEXT(object, member, hierarchy) #member, " = ", createResultString(object.member, hierarchy)

#define CREATE_RESULT_STRING_NONMEMBER_JSON(var, hierarchy) #var, ": ", createResultString(var, hierarchy)
#define CREATE_RESULT_STRING_NONMEMBER_TEXT(var, hierarchy) #var, " = ", createResultString(var, hierarchy)

template <> string createResultString(const td::sector_index &index, unsigned int hierarchy)
{
	const ci_string &outputFormat = getOptionValue("output-format");
	if(outputFormat == "json")
		return td::concat(
			"{\n",
				vector<char>(hierarchy+1, '\t'), CREATE_RESULT_STRING_MEMBER_JSON(index, disk, hierarchy+1), ",\n",
				vector<char>(hierarchy+1, '\t'), CREATE_RESULT_STRING_MEMBER_JSON(index, sector, hierarchy+1), ",\n",
				vector<char>(hierarchy+1, '\t'), CREATE_RESULT_STRING_MEMBER_JSON(index, access_count, hierarchy+1), "\n",
			vector<char>(hierarchy, '\t'), "}"
		);
	else if(outputFormat == "text")
		return td::concat(
				CREATE_RESULT_STRING_MEMBER_TEXT(index, disk, hierarchy+1), "\n",
				CREATE_RESULT_STRING_MEMBER_TEXT(index, sector, hierarchy+1), "\n",
				CREATE_RESULT_STRING_MEMBER_TEXT(index, access_count, hierarchy+1), "\n"
		);
	else
		throw FrontendException("Invalid output-format ", outputFormat);
}

template <> string createResultString(const td::sector_info &info, unsigned int hierarchy)
{
	const ci_string &outputFormat = getOptionValue("output-format");
	if(outputFormat == "json")
		return td::concat(
			"{\n",
				vector<char>(hierarchy+1, '\t'), CREATE_RESULT_STRING_MEMBER_JSON(info, logical_sector, hierarchy+1), ",\n",
				vector<char>(hierarchy+1, '\t'), CREATE_RESULT_STRING_MEMBER_JSON(info, access_sorted_index, hierarchy+1), ",\n",
				vector<char>(hierarchy+1, '\t'), CREATE_RESULT_STRING_MEMBER_JSON(info, physical_sector, hierarchy+1), "\n",
			vector<char>(hierarchy, '\t'), "}"
		);
	else if(outputFormat == "text")
		return td::concat(
				CREATE_RESULT_STRING_MEMBER_TEXT(info, logical_sector, hierarchy+1), "\n",
				CREATE_RESULT_STRING_MEMBER_TEXT(info, access_sorted_index, hierarchy+1), "\n",
				createResultString(info.physical_sector, hierarchy+1), "\n"
		);
	else
		throw FrontendException("Invalid output-format ", outputFormat);
}

template <> string createResultString(const td::device_performance &perf, unsigned int hierarchy)
{
	double avg_read_dec		= (double)perf.mod_avg_read / (1 << MEASUE_RECORDS_SHIFT);
	double stdev_read_dec	= (double)perf.mod_stdev_write / (1 << MEASUE_RECORDS_SHIFT);
	double avg_write_dec	= (double)perf.mod_avg_write / (1 << MEASUE_RECORDS_SHIFT);
	double stdev_write_dec	= (double)perf.mod_stdev_write / (1 << MEASUE_RECORDS_SHIFT);

	double avg_read_time_cycles = (double)perf.avg_read_time_cycles			+ avg_read_dec;
	double stdev_read_time_cycles = (double)perf.stdev_read_time_cycles		+ stdev_read_dec;
	double avg_write_time_cycles = (double)perf.avg_write_time_cycles		+ avg_write_dec;
	double stdev_write_time_cycles = (double)perf.stdev_write_time_cycles	+ stdev_write_dec;

	const ci_string &outputFormat = getOptionValue("output-format");
	if(outputFormat == "json")
		return td::concat(
			"{\n",
				vector<char>(hierarchy+1, '\t'), CREATE_RESULT_STRING_NONMEMBER_JSON(avg_read_time_cycles, hierarchy+1), ",\n",
				vector<char>(hierarchy+1, '\t'), CREATE_RESULT_STRING_NONMEMBER_JSON(stdev_read_time_cycles, hierarchy+1), ",\n",
				vector<char>(hierarchy+1, '\t'), CREATE_RESULT_STRING_NONMEMBER_JSON(avg_write_time_cycles, hierarchy+1), ",\n",
				vector<char>(hierarchy+1, '\t'), CREATE_RESULT_STRING_NONMEMBER_JSON(stdev_write_time_cycles, hierarchy+1), "\n",
			vector<char>(hierarchy, '\t'), "}"
		);
	else if(outputFormat == "text")
		return td::concat(
				CREATE_RESULT_STRING_NONMEMBER_TEXT(avg_read_time_cycles, hierarchy+1), "\n",
				CREATE_RESULT_STRING_NONMEMBER_TEXT(stdev_read_time_cycles, hierarchy+1), "\n",
				CREATE_RESULT_STRING_NONMEMBER_TEXT(avg_write_time_cycles, hierarchy+1), "\n",
				CREATE_RESULT_STRING_NONMEMBER_TEXT(stdev_write_time_cycles, hierarchy+1), "\n"
		);
	else
		throw FrontendException("Invalid output-format ", outputFormat);
}

template <> string createResultString(const td::internal_device_info &info, unsigned int hierarchy)
{
	const ci_string &outputFormat = getOptionValue("output-format");
	if(outputFormat == "json")
		return td::concat(
			"{\n",
				vector<char>(hierarchy+1, '\t'), CREATE_RESULT_STRING_MEMBER_JSON(info, disk, hierarchy+1), ",\n",
				vector<char>(hierarchy+1, '\t'), CREATE_RESULT_STRING_MEMBER_JSON(info, performance, hierarchy+1), "\n",
			vector<char>(hierarchy, '\t'), "}"
		);
	else if(outputFormat == "text")
		return td::concat(
				CREATE_RESULT_STRING_MEMBER_TEXT(info, disk, hierarchy+1), "\n",
				createResultString(info.performance, hierarchy+1), "\n"
		);
	else
		throw FrontendException("Invalid output-format ", outputFormat);
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
			return command.func(argc, args);
	}

	printHelp(programName);
	throw FrontendException("Unknown command ", cmd);
}

string add_tDisk(int argc, char *args[])
{
	tDisk disk;
	if(argc > 2)disk = tDisk::create(atoi(args[2]));
	else disk = tDisk::create();
	return createResultString(td::concat("Device ", disk.getName(), " opened"), 0);
}

string remove_tDisk(int argc, char *args[])
{
	if(argc <= 2)throw FrontendException("\"remove\" needs the device to be removed");

	tDisk d = tDisk::get(args[2]);
	d.remove();
	return createResultString(td::concat("tDisk ", d.getName(), " removed"), 0);
}

string add_disk(int argc, char *args[])
{
	if(argc <= 3)throw FrontendException("\"add_disk\" needs the tDisk and at least one file to add");

	vector<string> results;
	tDisk d = tDisk::get(args[2]);
	for(int i = 3; i < argc; ++i)
	{
		d.addDisk(args[i]);
		results.push_back(td::concat("Successfully added disk ", args[i]));
	}
	return createResultString(results, 0);
}

string get_max_sectors(int argc, char *args[])
{
	if(argc <= 2)throw FrontendException("\"get_max_sectors\" needs the td device");

	tDisk d = tDisk::get(args[2]);
	unsigned long long maxSectors = d.getMaxSectors();
	return createResultString(maxSectors, 0);
}

string get_sector_index(int argc, char *args[])
{
	if(argc <= 3)throw FrontendException("\"get_sector_index\" needs the td device and the sector index to retrieve");

	char *test;
	unsigned long long logicalSector = strtoull(args[3], &test, 10);
	if(test != args[3] + strlen(args[3]))throw FrontendException(args[3], " is not a valid number");

	tDisk d = tDisk::get(args[2]);
	td::sector_index index = d.getSectorIndex(logicalSector);
	return createResultString(index, 0);
}

string get_all_sector_indices(int argc, char *args[])
{
	if(argc <= 2)throw FrontendException("\"get_all_sector_indices\" needs the td device");

	tDisk d = tDisk::get(args[2]);
	vector<td::sector_info> sectorIndices = d.getAllSectorIndices();
	return createResultString(sectorIndices, 0);
}

string clear_access_count(int argc, char *args[])
{
	if(argc <= 2)throw FrontendException("\"clear_access_count\" needs the td device\n");

	tDisk d = tDisk::get(args[2]);
	d.clearAccessCount();
	return createResultString(td::concat("Access count for tDisk ", d.getName(), " cleared"), 0);
}

string get_internal_devices_count(int argc, char *args[])
{
	if(argc <= 2)throw FrontendException("\"get_internal_device_count\" needs the td device\n");

	tDisk d = tDisk::get(args[2]);
	td::tdisk_index devices = d.getInternalDevicesCount();
	return createResultString(devices, 0);
}

string get_device_info(int argc, char *args[])
{
	if(argc <= 3)throw FrontendException("\"get_device_info\" needs the td device and the device to retrieve infos for\n");

	char *test;
	long device = strtol(args[3], &test, 10);
	if(test != args[3] + strlen(args[3]))
		throw FrontendException("Invalid number ", args[3], " for device index");

	if(device != (td::tdisk_index)device)
		throw FrontendException("Device index ", device, " exceeds limit of devices");

	tDisk d = tDisk::get(args[2]);
	td::internal_device_info info = d.getDeviceInfo((td::tdisk_index)device);
	return createResultString(info, 0);
}

void printHelp(const string &progName)
{
	cout<<"Usage:"<<endl;
	cout<<endl;
	cout<<progName<<" [command] [command-options]"<<endl;
}

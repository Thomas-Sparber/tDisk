/**
  *
  * tDisk backend
  * @author Thomas Sparber (2015-2016)
  *
 **/

#include <shell.hpp>
#include <utils.hpp>

using std::string;
using std::unique_ptr;
using std::vector;

using namespace td;
using namespace shell;

bool initialized = false;
string executable;

#ifdef __linux__

/**
  * Builds the command path on linux
 **/
string getCommandPath(const string &command)
{
	if(!initialized)
	{
		LOG(LogLevel::warn, "Shell is not initialized");
		return utils::concatPath("scripts",(command+".sh"));		
	}

	//LOG(LogLevel::debug, "concat: ",executable," & scripts & ",command," & .sh");
	return utils::concatPath(executable,"scripts",(command+".sh"));
}

#else

/**
  * Builds the command path on windows
 **/
string getCommandPath(const string &command)
{
	if(!initialized)
	{
		LOG(LogLevel::warn, "Shell is not initialized");
		return utils::concatPath("scripts",(command+".bat"));		
	}

	return utils::concatPath(executable,"scripts",(command+".bat"));
}

#endif //__linux__

void shell::initShell(const char *c_executable)
{
	initialized = true;
	executable = utils::dirnameOf(c_executable, utils::filenameOf(c_executable).c_str());
	LOG(LogLevel::debug, "Initializing shell with executable ",c_executable,": ",executable);
}

vector<unique_ptr<ShellObjectBase> > shell::execute_internal(const ShellCommand &command, const string &args)
{
	const string commandPath = getCommandPath(command.command);
	const string cmd = utils::concat(commandPath," ",args);
	LOG(LogLevel::debug, "Executing shell command: ",cmd);

	const string &stringResult = utils::exec(cmd);

	vector<string> results;
	utils::split(stringResult, "\n\n", results, false);

	vector<unique_ptr<ShellObjectBase> > ret;
	for(const string &result : results)
	{
		unique_ptr<ShellObjectBase> o(command.ret->clone());
		o->parse(result);
		ret.push_back(std::move(o));
	}

	return std::move(ret);
}

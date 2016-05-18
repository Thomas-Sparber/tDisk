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

#ifdef __linux__

string getCommandPath(const string &command)
{
	return utils::concat("scripts/",command,".sh");
}

#endif //__linux__

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

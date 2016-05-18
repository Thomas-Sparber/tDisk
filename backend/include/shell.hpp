/**
  *
  * tDisk backend
  * @author Thomas Sparber (2015-2016)
  *
 **/

#ifndef SHELL_HPP
#define SHELL_HPP

#include <iostream>
#include <memory>
#include <vector>

#include <logger.hpp>
#include <shellobject.hpp>

namespace td
{

namespace shell
{

	struct ShellCommand
	{
		ShellCommand(const std::string &str_command, const ShellObjectBase &so_ret) :
			command(str_command),
			ret(so_ret.clone())
		{}

		std::string command;
		std::unique_ptr<ShellObjectBase> ret;
	}; //end struct ShellCommand

	std::vector<std::unique_ptr<ShellObjectBase> > execute_internal(const ShellCommand &command, const std::string &args);

	template <class ...Args>
	std::vector<std::unique_ptr<ShellObjectBase> > execute(const ShellCommand &command, Args ...args)
	{
		return  execute_internal(command, utils::concatQuoted(args...));
	}

	const ShellCommand TestCommand("test", TestResult());

	inline bool runTestCommand()
	{
		std::vector<std::unique_ptr<ShellObjectBase> > result = execute(TestCommand, 5, 7, 2);

		if(result.size() == 5)
		{
			bool passed = true;
			std::string testName;
			int testNumber;
			bool testSuccess;
			std::size_t i;
			for(i = 0; i < result.size(); ++i)
			{
				passed = passed && result[i]->get<name>(testName);
				passed = passed && result[i]->get<number>(testNumber);
				passed = passed && result[i]->get<success>(testSuccess);
				passed = passed && testSuccess;

				if(!passed)break;
			}

			if(passed)LOG(LogLevel::debug, result.size()," tests passed");
			else LOG(LogLevel::error, "Error at test ",i,"/",result.size(),": ",result[i]->get<name>()," ",result[i]->get<number,int>(),": ",result[i]->get<success,bool>());

			return passed;
		}
		else
		{
			if(!result.empty())LOG(LogLevel::error, "Test command threw an error: ",result[0]->getMessage());
			else LOG(LogLevel::error, "Test command didn't return any message");
			return false;
		}
	}

} //end namespace shell

} //end namespace td

#endif //SHELL_HPP

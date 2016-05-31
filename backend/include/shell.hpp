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

/**
  * The shell namespace contains functions to execute scripts
  * on the filesystem and parse their result
 **/
namespace shell
{

	/**
	  * A Shell command is identified by a command string
	  * and a result object
	 **/
	struct ShellCommand
	{
		/**
		  * Constructs a ShellCommand using a string identifier
		  * and a return object
		 **/
		ShellCommand(const std::string &str_command, const ShellObjectBase &so_ret) :
			command(str_command),
			ret(so_ret.clone())
		{}

		/**
		  * The string identifier of the command
		 **/
		std::string command;

		/**
		  * The return object of the command
		 **/
		std::unique_ptr<ShellObjectBase> ret;

	}; //end struct ShellCommand

	/**
	  * This function must be called before executing any Shell commands.
	  * The argument executable must be the name of the current program
	 **/
	void initShell(const char *executable);

	/**
	  * This function is called internally to execute
	  * the geven command with the given arguments
	 **/
	std::vector<std::unique_ptr<ShellObjectBase> > execute_internal(const ShellCommand &command, const std::string &args);

	/**
	  * Executes the given command with an arbitrary
	  * amount of arguments. The result is the parsed
	  * return value
	 **/
	template <class ...Args>
	std::vector<std::unique_ptr<ShellObjectBase> > execute(const ShellCommand &command, const Args& ...args)
	{
		return  execute_internal(command, utils::concatQuoted(args...));
	}

	/**
	  * The test command which can be used to test the
	  * functionality of the shell
	 **/
	const ShellCommand TestCommand("test", TestResult());

	/**
	  * The command which executes the tasks which are needed
	  * after creating a tDisk
	 **/
	const ShellCommand tDiskPostCreateCommand("tdisk-post-create", tDiskPostCreateResult());

	/**
	  * The command which returns the mount point of the given device
	 **/
	const ShellCommand GetMountPointCommand("get-mount-point", GetMountPointResult());

	/**
	  * The command which returns the free space of the given device
	 **/
	const ShellCommand DiskFreeSpaceCommand("disk-free-space", DiskFreeSpaceResult());

	/**
	  * This function calls the test command and checks its
	  * return type to see if the shell is working properly
	 **/
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

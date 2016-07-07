/**
  *
  * tDisk backend
  * @author Thomas Sparber (2015-2016)
  *
 **/

#ifndef SHELLRETVALUE_HPP
#define SHELLRETVALUE_HPP

#include <string>

namespace td
{

namespace shell
{

	/**
	  * A ShellRetValueBase is the Base of a RetValue.
	  * It represents one single parameter of the return
	  * value of a shell command
	 **/
	struct ShellRetValueBase
	{
		/**
		  * Default virtual destructor
		 **/
		virtual ~ShellRetValueBase() {}

		/**
		  * Gets the current value of the parameter
		 **/
		virtual const std::string& getValue() const = 0;

		/**
		  * Sets the value of the parameter
		 **/
		virtual void setValue(const std::string &value) = 0;

	}; //end struct ShellRetValueBase

	/**
	  * The ShellRetValue is the class that implements the functions
	  * defined by the ShellRetValueBase. Like the ShellRetValueBase, the
	  * ShellRetValue represents one parameter of the return value of
	  * a shell command with the difference that ShellRetValue is identified
	  * by a name, lina number (within the return value) and value type
	 **/
	template <class Type, int line, const char *name>
	struct ShellRetValue : ShellRetValueBase
	{
		/**
		  * Default constructor
		 **/
		ShellRetValue() :
			ShellRetValueBase(),
			value()
		{}

		/**
		  * Constructs a ShellRetValue using a predefined value
		 **/
		ShellRetValue(const std::string &v) :
			value(v)
		{}

		/**
		  * Gets the name of the ShellRetValue
		 **/
		const char* getName() const
		{
			return name;
		}

		/**
		  * Gets the line of the ShellRetValue
		 **/
		int getLine() const
		{
			return line;
		}

		/**
		  * Gets the string value
		 **/
		virtual const std::string& getValue() const
		{
			return value;
		}

		/**
		  * Sets the string value
		 **/
		virtual void setValue(const std::string &v)
		{
			this->value = v;
		}

		/**
		  * The current value of the ShellRetValue
		 **/
		std::string value;

	}; //end class ShellRetValue

} //end namespace shell

} //end namespace td

#endif //SHELLRETVALUE_HPP

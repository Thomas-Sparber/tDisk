/**
  *
  * tDisk backend
  * @author Thomas Sparber (2015-2016)
  *
 **/

#ifndef OPTION_HPP
#define OPTION_HPP

#include <string>
#include <vector>

#include <ci_string.hpp>
#include <utils.hpp>

namespace td
{

/**
  * An option can be used to set specific settings such as
  * output-format. An option needs to have a name and a description.
  * It can also have a predefined set of predefined values.
 **/
class Option
{

public:

	/**
	  * Constructs an option using a name, a description and an optional
	  * vector of predefined values.
	 **/
	Option(const utils::ci_string &name, const std::string &description, const std::vector<utils::ci_string> &values=std::vector<utils::ci_string>());

	/**
	  * Gets the name of the option
	 **/
	const utils::ci_string& getName() const
	{
		return name;
	}

	/**
	  * Gets the description of the option
	 **/
	const std::string& getDescription() const
	{
		return description;
	}

	/**
	  * Sets the current option value using any data type
	 **/
	template <class T>
	void setValue(const T &t)
	{
		setValue(utils::ci_string(utils::concat(t).c_str()));
	}

	/**
	  * Sets the current option value
	 **/
	void setValue(const utils::ci_string &value);

	/**
	  * Gets the value of the option in the form of a string
	 **/
	const utils::ci_string& getStringValue() const
	{
		return value;
	}

	/**
	  * Returns whether the option has predefined values set
	 **/
	bool hasPredefinedValues() const
	{
		return !values.empty();
	}

	/**
	  * Returns the value of the option as char
	 **/
	char getCharValue() const;

	/**
	  * Returns the value of the option as a long
	 **/
	long getLongValue() const;

public:

	/**
	  * Default option output_format. This can either be text or json
	 **/
	const static Option output_format;

private:

	/**
	  * The name of the option
	 **/
	utils::ci_string name;
	
	/**
	  * The description of the option
	 **/
	std::string description;
	
	/**
	  * Predefined values
	 **/
	std::vector<utils::ci_string> values;
	
	/**
	  * The current value of the option
	 **/
	utils::ci_string value;

}; //end class Option

} //end namespace td

#endif //OPTION_HPP

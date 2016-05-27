/**
  *
  * tDisk backend
  * @author Thomas Sparber (2015-2016)
  *
 **/

#ifndef OPTIONS_HPP
#define OPTIONS_HPP

#include <string>
#include <vector>

#include <option.hpp>

namespace td
{

/**
  * The class Options manages a givien set of individual options
  * td::Option.
 **/
class Options
{

public:

	/**
	  * Default constructor
	 **/
	Options() :
		options()
	{}

	/**
	  * Constructs an object using a given predefied set of options
	  * @param v_options The predefined set of options
	 **/
	Options(const std::vector<Option> &v_options);

	/**
	  * Adds the given options to be managed
	 **/
	void addOptions(const Option &o)
	{
		addOption(o);
	}

	/**
	  * Adds the given options to be managed
	 **/
	template <class ...O>
	void addOptions(const Option &o, const O& ...others)
	{
		addOption(o);
		addOptions(others...);
	}

	/**
	  * Adds the given option
	  * @param option The option to be managed
	 **/
	void addOption(const Option &option);
	
	/**
	  * Checks whether the option exists
	  * @param name The name of the option to check
	  * @returns true if it exists
	 **/
	bool optionExists(const utils::ci_string &name) const;

	/**
	  * Gets the option with the given name. Throws a BackendException
	  * if the option does not exist.
	  * @param name The name of the option to get
	  * @returns The option that matches the given name
	 **/
	Option& getOption(const utils::ci_string &name);

	/**
	  * Gets the option with the given name. Throws a BackendException
	  * if the option does not exist.
	  * @param name The name of the option to get
	  * @returns The option that matches the given name
	 **/
	const Option& getOption(const utils::ci_string &name) const;

	/**
	  * Returns the current value of the option with the given name
	  * @param name The name of the option
	  * @returns The current value of the option
	 **/
	utils::ci_string getOptionValue(const utils::ci_string &name) const
	{
		return getOption(name).getStringValue();
	}

	/**
	  * Returns the current string value of the option with the given name
	  * @param name The name of the option
	  * @returns The current stringvalue of the option
	 **/
	std::string getStringOptionValue(const utils::ci_string &name) const
	{
		return getOption(name).getStringValue().c_str();
	}

	/**
	  * Returns the current bool value of the option with the given name
	  * @param name The name of the option
	  * @returns The current bool value of the option
	 **/
	bool getOptionBoolValue(const utils::ci_string &name) const
	{
		return getOption(name).getBoolValue();
	}

	/**
	  * Returns the current long value of the option with the given name
	  * @param name The name of the option
	  * @returns The current long value of the option
	 **/
	bool getOptionLongValue(const utils::ci_string &name) const
	{
		return getOption(name).getLongValue();
	}

	/**
	  * Returns the current char value of the option with the given name
	  * @param name The name of the option
	  * @returns The current char value of the option
	 **/
	char getOptionCharValue(const utils::ci_string &name) const
	{
		return getOption(name).getCharValue();
	}

	/**
	  * Sets the value of the option with the given name
	  * @param name The name of the option
	  * @param value The value to set
	 **/
	void setOptionValue(const utils::ci_string &name, const utils::ci_string &value)
	{
		getOption(name).setValue(value);
	}

	/**
	  * Gets all options
	 **/
	const std::vector<Option>& getAllOptions() const
	{
		return options;
	}

private:

	/**
	  * The internal vector where all options are stored
	 **/
	 std::vector<Option> options;

}; //end class Options

} //end namespace td

#endif //OPTIONS_HPP

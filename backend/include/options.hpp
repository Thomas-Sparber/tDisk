#ifndef OPTIONS_HPP
#define OPTIONS_HPP

#include <string>
#include <vector>

#include <option.hpp>

namespace td
{

class Options
{

public:
	Options() :
		options()
	{}

	Options(const std::vector<Option> &v_options);

	void addOptions(const Option &o)
	{
		addOption(o);
	}

	template <class ...O>
	void addOptions(const Option &o, O ...others)
	{
		addOption(o);
		addOptions(others...);
	}

	void addOption(const Option &option);
	
	bool optionExists(const utils::ci_string &name) const;

	Option& getOption(const utils::ci_string &name);

	const Option& getOption(const utils::ci_string &name) const;

	utils::ci_string getOptionValue(const utils::ci_string &name) const
	{
		return getOption(name).getStringValue();
	}

	void setOptionValue(const utils::ci_string &name, const utils::ci_string &value)
	{
		getOption(name).setValue(value);
	}

private:
	std::vector<Option> options;

}; //end class Options

} //end namespace td

#endif //OPTIONS_HPP

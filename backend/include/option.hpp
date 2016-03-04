#ifndef OPTION_HPP
#define OPTION_HPP

#include <string>
#include <vector>

#include <ci_string.hpp>
#include <utils.hpp>

namespace td
{

class Option
{

public:
	Option(const utils::ci_string &name, const std::string &description, const std::vector<utils::ci_string> &values);

	const utils::ci_string& getName() const
	{
		return name;
	}

	const std::string& getDescription() const
	{
		return description;
	}

	template <class T>
	void setValue(const T &t)
	{
		setValue(utils::concat(t));
	}

	void setValue(const utils::ci_string &value);

	const utils::ci_string& getStringValue() const
	{
		return value;
	}

	bool hasPredefinedValues() const
	{
		return !values.empty();
	}

	char getCharValue() const;

	long getLongValue() const;

public:
	const static Option output_format;

private:
	utils::ci_string name;
	std::string description;
	std::vector<utils::ci_string> values;
	utils::ci_string value;

}; //end class Option

} //end namespace td

#endif //OPTION_HPP

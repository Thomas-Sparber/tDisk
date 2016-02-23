#ifndef OPTION_HPP
#define OPTION_HPP

#include <string>
#include <vector>

#include <ci_string.hpp>

namespace td
{

class Option
{

public:
	Option(const ci_string &name, const std::string &description, const std::vector<ci_string> &values);

	const ci_string& getName() const
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
		setValue(concat(t));
	}

	void setValue(const ci_string &value);

	const ci_string& getStringValue() const
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
	ci_string name;
	std::string description;
	std::vector<ci_string> values;
	ci_string value;

}; //end class Option

} //end namespace td

#endif //OPTION_HPP

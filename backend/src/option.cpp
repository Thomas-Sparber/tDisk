#include <backendexception.hpp>
#include <option.hpp>

using namespace td;

const Option Option::output_format(
	"output-format",
	"Defnes the output format. This can either be \"text\" or \"json\"",
	{ "text", "json" }
);

Option::Option(const ci_string &str_name, const std::string &str_description, const std::vector<ci_string> &v_values) :
	name(str_name),
	description(str_description),
	values(v_values),
	value(values.empty() ? "" : values[0])
{}

char Option::getCharValue() const
{
	if(value.empty())throw BackendException("Value is empty");
	return value[0];
}

long Option::getLongValue() const
{
	char *test;
	long v = strtol(value.c_str(), &test, 10);
	if(test != value.c_str()+value.length())throw BackendException("Invalid number ", value);
	return v;
}

void Option::setValue(const ci_string &str_value)
{
	if(hasPredefinedValues())
	{
		bool valueFound = false;
		for(const ci_string &v : values)
		{
			if(v == str_value)
			{
				valueFound = true;
				break;
			}
		}

		if(!valueFound)
			throw BackendException("Invalid value \"", str_value ,"\" for option ", name);
	}
	value = str_value;
}

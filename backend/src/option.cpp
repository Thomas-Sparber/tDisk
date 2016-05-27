/**
  *
  * tDisk backend
  * @author Thomas Sparber (2015-2016)
  *
 **/

#include <backendexception.hpp>
#include <convert.hpp>
#include <option.hpp>

using namespace td;

using utils::ci_string;

const Option Option::output_format(
	"output-format",
	"Defines the output format. This can either be \"text\" or \"json\"",
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
	uint64_t v;
	if(!utils::convertTo(value, v))throw BackendException("Invalid number ", value);
	return (long)v;
}

bool Option::getBoolValue() const
{
	if(value == "yes")return true;
	if(value == "no")return false;
	if(value == "true")return true;
	if(value == "false")return false;

	try {
		return (getLongValue() != 0);
	} catch(const BackendException &e) {
		throw BackendException("Invalid bool ",value);
	}
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

#include <backendexception.hpp>
#include <options.hpp>

using namespace td;

Options::Options(const std::vector<Option> &v_options) :
	options()
{
	for(const Option &o : v_options)
		addOption(o);
}

void Options::addOption(const Option &option)
{
	if(optionExists(option.getName()))
		getOption(option.getName()) = option;
	else
		options.push_back(option);
}

bool Options::optionExists(const ci_string &name) const
{
	for(const Option &option : options)
	{
		if(option.getName() == name)
			return true;
	}

	return false;
}

Option& Options::getOption(const ci_string &name)
{
	for(Option &option : options)
	{
		if(option.getName() == name)
			return option;
	}

	throw BackendException("Option \"", name ,"\" is not set");
}

const Option& Options::getOption(const ci_string &name) const
{
	for(const Option &option : options)
	{
		if(option.getName() == name)
			return option;
	}

	throw BackendException("Option \"", name ,"\" is not set");
}

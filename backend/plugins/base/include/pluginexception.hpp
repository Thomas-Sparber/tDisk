#ifndef PLUGINEXCEPTION_HPP
#define PLUGINEXCEPTION_HPP

#include <string>

#include <utils.hpp>

namespace td
{

struct PluginException
{

	template <class ...T>
	PluginException(T ...t) :
		what(utils::concat(t...))
	{}

	std::string what;

};; //end struct PluginException

} //end namespace td

#endif //PLUGINEXCEPTION_HPP

#ifndef PLUGIN_HPP
#define PLUGIN_HPP

#include <string>

struct nl_sock;

namespace td
{

class Plugin
{

public:
	Plugin(const std::string &name, bool registerInKernel);

	Plugin(Plugin &&other) = default;

	Plugin(const Plugin &other) = delete;

	virtual ~Plugin();

	Plugin& operator= (Plugin &&other) = default;

	Plugin& operator= (const Plugin &other) = delete;

	void registerInKernel();

	void unregister();

	virtual bool read(unsigned long long offset, char *data, std::size_t length) const = 0;

	virtual bool write(unsigned long long offset, char *data, std::size_t length) = 0;

	void listen();

private:
	std::string name;
	mutable struct nl_sock *socket;
	int familyId;

}; //end class Plugin

} //end namespace td

#endif //PLUGIN_HPP

#ifndef PLUGIN_HPP
#define PLUGIN_HPP

#include <string>
#include <vector>

struct nl_sock;
struct nl_msg;

namespace td
{

enum class PluginOperation : char
{
	read = 'r',
	write = 'w',
	size = 's'
}; //end class PluginOperation

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

	virtual bool read(unsigned long long offset, std::vector<char> &data, std::size_t length) const = 0;

	virtual bool write(unsigned long long offset, const std::vector<char> &data, std::size_t length) = 0;

	virtual unsigned long long getSize() const = 0;

	void listen();

	bool poll(unsigned int time);

	int receive();

	bool isRunning() const
	{
		return running;
	}

	void stop()
	{
		running = false;
	}

	static void stopAllPlugins();

	friend int td::worker_function(nl_msg *msg, void *arg);

protected:
	bool messageReceived(uint32_t sequenceNumber, PluginOperation operation, unsigned long long offset, std::vector<char> &data, int length);

	bool sendFinishedMessage(uint32_t sequenceNumber, std::vector<char> &data, int length);

private:
	std::string name;
	mutable struct nl_sock *socket;
	int familyId;
	bool running;

}; //end class Plugin

} //end namespace td

#endif //PLUGIN_HPP

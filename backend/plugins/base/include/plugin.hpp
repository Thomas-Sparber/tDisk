#ifndef PLUGIN_HPP
#define PLUGIN_HPP

#include <string>
#include <vector>

#include <historybuffer.hpp>
#include <writebuffer.hpp>

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
	Plugin(const std::string &name="", unsigned long long maxWriteBytesJoin=0, unsigned int writeQueues=1, unsigned int maxHistoryBuffer=0, bool registerInKernel=false);

	Plugin(Plugin &&other) = default;

	Plugin(const Plugin &other) = delete;

	virtual ~Plugin();

	Plugin& operator= (Plugin &&other) = default;

	Plugin& operator= (const Plugin &other) = delete;

	void registerInKernel();

	bool unregister();

	virtual bool read(unsigned long long offset, char *data, std::size_t length) const = 0;

	virtual bool write(unsigned long long offset, const char *data, std::size_t length) = 0;

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

	bool writeBufferedData(WriteBuffer &writeBuffer);

	void setName(const std::string &str_name)
	{
		bool wasRegistered = unregister();
		name = str_name;
		if(wasRegistered)registerInKernel();
	}

private:
	std::string name;
	mutable struct nl_sock *socket;
	int familyId;
	bool running;
	std::vector<WriteBuffer> writeBuffers;
	HistoryBuffer history;

}; //end class Plugin

} //end namespace td

#endif //PLUGIN_HPP

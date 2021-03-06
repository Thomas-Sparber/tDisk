#include <netlink/netlink.h>
#include <netlink/socket.h>
#include <netlink/types.h>
#include <netlink/genl/genl.h>
#include <netlink/genl/ctrl.h>
#include <netlink/genl/mngt.h>
#include <iostream>
#include <set>
#include <signal.h>
#include <thread>
#include <unistd.h>
#include <vector>

namespace td {
	namespace c {
		extern "C" {
			#include <tdisk/interface.h>	//Kernel module interface
		}
	}
}

#include <communication_helper.hpp>
#include <plugin.hpp>

using std::cerr;
using std::cout;
using std::endl;
using std::string;
using std::thread;
using std::vector;
using std::set;

using namespace td;
using namespace c;

set<Plugin*> currentPlugins;

namespace td
{

void Plugin::stopAllPlugins()
{
	for(Plugin *p : currentPlugins)
	{
		p->stop();
	}
}

void signalHandler(int)
{
	cout<<"Received signal. Stopping all plugins."<<endl;
	Plugin::stopAllPlugins();
}

int worker_function(nl_msg *msg, void *arg)
{
	Plugin *currentPlugin = (Plugin*)arg;

	genlmsghdr *header = genlmsg_hdr(nlmsg_hdr(msg));
	nlattr *attrs = genlmsg_attrdata(header, NLTD_HEADER_SIZE);
	int attributes = genlmsg_attrlen(header, NLTD_HEADER_SIZE);

	if(!currentPlugin)
	{
		cerr<<"Plugin can't be null!"<<endl;
		return -1;
	}

	nlattr *requestNumber = nla_find(attrs, attributes, NLTD_REQ_NUMBER);
	nlattr *offset = nla_find(attrs, attributes, NLTD_REQ_OFFSET);
	nlattr *length = nla_find(attrs, attributes, NLTD_REQ_LENGTH);
	nlattr *buffer = nla_find(attrs, attributes, NLTD_REQ_BUFFER);

	if(!requestNumber)
	{
		cerr<<"Received a message without request number!"<<endl;
		return -1;
	}

	if(!offset || !length)
	{
		cerr<<"Received a message without an offset or length!"<<endl;
		return -1;
	}

	PluginOperation operation;
	if(header->cmd == NLTD_CMD_READ)operation = PluginOperation::read;
	else if(header->cmd ==  NLTD_CMD_WRITE)operation = PluginOperation::write;
	else if(header->cmd ==  NLTD_CMD_SIZE)operation = PluginOperation::size;
	else
	{
		cerr<<"Invalid message type "<<header->cmd<<endl;
		return -1;
	}

	std::size_t l = (std::size_t)nla_get_u32(length);

	vector<char> b;
	if(buffer)b = vector<char>((char*)nla_data(buffer), (char*)nla_data(buffer) + l);

	bool success = currentPlugin->messageReceived(nla_get_u32(requestNumber), operation, nla_get_u64(offset), b, l);

	if(success)return 0;
	else return -1;
}

} //end namespace td

Plugin::Plugin(const string &str_name, unsigned long long maxWriteBytesJoin, unsigned int writeQueues, unsigned int maxHistoryBuffer, bool b_registerInKernel) :
	name(str_name),
	socket(nullptr),
	familyId(),
	running(false),
	writeBuffers(writeQueues, maxWriteBytesJoin),
	history(maxHistoryBuffer)
{
	if(b_registerInKernel)
	{
		registerInKernel();
	}

	currentPlugins.insert(this);
}

Plugin::~Plugin()
{
	unregister();
	currentPlugins.erase(this);
}

void Plugin::registerInKernel()
{
	unregister();

	cout<<"Registering plugin \""<<name<<"\""<<endl;

	socket = nl_socket_alloc();
	nl_socket_disable_seq_check(socket);
	nl_socket_modify_cb(socket, NL_CB_VALID, NL_CB_CUSTOM, worker_function, this);
	nl_socket_modify_err_cb(socket, NL_CB_DEBUG, nullptr, nullptr);
	nl_socket_set_buffer_size(socket, 2097152, 2097152);

	genl_connect(socket);
	familyId = genl_ctrl_resolve(socket, NLTD_NAME);

	signal(SIGINT, signalHandler);

	sendNlMessage(socket, NLTD_CMD_REGISTER, 0, familyId,
		createArg<NLTD_PLUGIN_NAME>(name)
	);
}

bool Plugin::unregister()
{
	stop();

	if(socket)
	{
		sendNlMessage(socket, NLTD_CMD_UNREGISTER, 0, familyId,
			createArg<NLTD_PLUGIN_NAME>(name)
		);

		nl_socket_free(socket);
		socket = nullptr;
		return true;
	}

	return false;
}

void Plugin::listen()
{
	running = true;

	while(running)
	{
		if(!poll(1000))
		{
			//Flushing buffers when there is nothing to do
			for(WriteBuffer &writeBuffer : writeBuffers)
			{
				if(writeBuffer.age() > 5 && !writeBuffer.empty())
				{
					//cout<<"Empting write buffer because timed out"<<endl;
					writeBufferedData(writeBuffer);
				}
			}

			continue;
		}

		int ret = receive();
		if(ret != 0)cout<<"Code: "<<ret<<": "<<nl_geterror(ret)<<endl;
	}

	for(WriteBuffer &writeBuffer : writeBuffers)
		writeBufferedData(writeBuffer);
}

bool Plugin::poll(unsigned int time)
{
	struct pollfd fd;
	fd.fd = nl_socket_get_fd(socket);;
	fd.events = POLLIN;
	const int result = ::poll(&fd, 1, time);
	return (result > 0);
}

int Plugin::receive()
{
	return nl_recvmsgs_default(socket);
}

bool Plugin::writeBufferedData(WriteBuffer &writeBuffer)
{
	unsigned long long oldPos;
	vector<char> oldData = writeBuffer.getAndPop(oldPos);
	if(oldData.empty())return true;
	return write(oldPos, &oldData[0], oldData.size());
}

bool Plugin::messageReceived(uint32_t sequenceNumber, PluginOperation operation, unsigned long long offset, vector<char> &data, std::size_t length)
{
	bool success = false;
	if(operation == PluginOperation::read)
	{
		if(data.size() != length)data = vector<char>(length);

		//Check read history
		if(history.get(offset, &data[0], length))success = true;

		if(!success)
		{
			//Check write buffers
			for(WriteBuffer &writeBuffer : writeBuffers)
			{
				if(writeBuffer.read(offset, &data[0], length))
				{
					success = true;
					break;
				}
			}
		}

		//Finally, call actual read function
		if(!success)
		{
			success = read(offset, &data[0], length);
			if(success)history.set(offset, &data[0], length);
		}
	}
	else if(operation == PluginOperation::write)
	{
		if(data.size() != length)
		{
			cerr<<"Invalid data size given: "<<data.size()<<"/"<<length<<endl;
			success = false;
		}
		else
		{
			//Find suitable write buffer
			WriteBuffer *suitableWriteBuffer = nullptr;
			for(WriteBuffer &writeBuffer : writeBuffers)
			{
				if(writeBuffer.canBeCombined(offset, &data[0], length))
				{
					suitableWriteBuffer = &writeBuffer;
					break;
				}
			}

			//Empty oldest WriteBuffer if nothing suitable was found
			if(!suitableWriteBuffer)
			{
				//cout<<"Need to empty write buffer. Not matching data"<<endl;

				//Use oldest WriteBuffer
				for(WriteBuffer &writeBuffer : writeBuffers)
				{
					if(!suitableWriteBuffer ||
						writeBuffer.age() > suitableWriteBuffer->age() ||
						(writeBuffer.age() == suitableWriteBuffer->age() && writeBuffer.size() > suitableWriteBuffer->size()))
					{
						suitableWriteBuffer = &writeBuffer;
					}
				}

				writeBufferedData(*suitableWriteBuffer);	//TODO check return value
			}

			//Save data in WriteBuffer
			if(suitableWriteBuffer->append(offset, &data[0], length))
			{
				//Save data to actual plugin
				success = writeBufferedData(*suitableWriteBuffer);
			}
			else success = true;

			if(success)history.set(offset, &data[0], length);
			data.clear();
		}
	}
	else if(operation == PluginOperation::size)
	{
		data = vector<char>(length);
		snprintf(&data[0], length, "%llu", getSize());
		success = true;
	}
	else
	{
		cerr<<"Invalid message type "<<(char)operation<<endl;
		success = false;
	}

	/*if(data.size() != (std::size_t)length)
	{
		cerr<<"Data size mismach: "<<data.size()<<"/"<<length<<endl;
		success = false;
	}*/

	return sendFinishedMessage(sequenceNumber, data, success);
}

bool Plugin::sendFinishedMessage(uint32_t sequenceNumber, vector<char> &data, bool success)
{
	int ret = success ? 0 : -1;
	std::size_t length = data.size();

	return sendNlMessage(socket, NLTD_CMD_FINISHED, 0, familyId,
		createArg<NLTD_REQ_NUMBER>(sequenceNumber),
		createArg<NLTD_REQ_BUFFER>(data),
		createArg<NLTD_REQ_LENGTH>(length),
		createArg<NLTD_REQ_RET>(ret)
	);
}

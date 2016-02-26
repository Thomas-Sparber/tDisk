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
	else
	{
		cerr<<"Invalid message type "<<header->cmd<<endl;
		return -1;
	}

	int l = nla_get_s32(length);
	if(l < 0)
	{
		cerr<<"Length can't be < 0: "<<l<<endl;
		return -1;
	}

	vector<char> b;
	if(buffer)b = vector<char>((char*)nla_data(buffer), (char*)nla_data(buffer) + l);

	bool success = currentPlugin->messageReceived(nla_get_u32(requestNumber), operation, nla_get_u64(offset), b, l);

	if(success)return 0;
	else return -1;
}

} //end namespace td

Plugin::Plugin(const string &str_name, bool b_registerInKernel) :
	name(str_name),
	socket(nullptr),
	familyId(),
	running(false)
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

	socket = nl_socket_alloc();
	nl_socket_disable_seq_check(socket);
	nl_socket_modify_cb(socket, NL_CB_VALID, NL_CB_CUSTOM, worker_function, this);
	nl_socket_modify_err_cb(socket, NL_CB_DEBUG, nullptr, nullptr);
	nl_socket_set_buffer_size(socket, 65536, 65536);

	genl_connect(socket);
	familyId = genl_ctrl_resolve(socket, NLTD_NAME);

	signal(SIGINT, signalHandler);

	sendNlMessage(socket, NLTD_CMD_REGISTER, 0, familyId,
		createArg<NLTD_PLUGIN_NAME>(name)
	);
}

void Plugin::unregister()
{
	stop();

	if(socket)
	{
		sendNlMessage(socket, NLTD_CMD_UNREGISTER, 0, familyId,
			createArg<NLTD_PLUGIN_NAME>(name)
		);

		nl_socket_free(socket);
		socket = nullptr;
	}
}

void Plugin::listen()
{
	running = true;

	while(running)
	{
		if(!poll(1000))continue;

		int ret = receive();
		if(ret != 0)cout<<"Code: "<<ret<<": "<<nl_geterror(ret)<<endl;
	}
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

bool Plugin::messageReceived(uint32_t sequenceNumber, PluginOperation operation, unsigned long long offset, vector<char> &data, int length)
{
	bool success;
	if(operation == PluginOperation::read)
	{
		data = vector<char>(length);
		success = read(offset, data, length);
	}
	else if(operation == PluginOperation::write)
	{
		success = write(offset, data, length);
	}
	else
	{
		cerr<<"Invalid message type "<<(char)operation<<endl;
		success = false;
	}

	if(data.size() != (std::size_t)length)success = false;

	return sendFinishedMessage(sequenceNumber, data, success ? length : -1);
}

bool Plugin::sendFinishedMessage(uint32_t sequenceNumber, vector<char> &data, int length)
{
	return sendNlMessage(socket, NLTD_CMD_FINISHED, 0, familyId,
		createArg<NLTD_REQ_NUMBER>(sequenceNumber),
		createArg<NLTD_REQ_BUFFER>(data),
		createArg<NLTD_REQ_LENGTH>(length)
	);
}

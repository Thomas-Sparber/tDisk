#include <netlink/netlink.h>
#include <netlink/socket.h>
#include <netlink/genl/genl.h>
#include <netlink/genl/ctrl.h>
#include <iostream>

namespace td {
	namespace c {
		extern "C" {
			#include <tdisk/interface.h>	//Kernel module interface
		}
	}
}
#include <plugin.hpp>

using std::cerr;
using std::endl;
using std::string;

using namespace td;
using namespace c;

static int worker_function(struct nl_msg *msg, void *arg)
{
	Plugin *currentPlugin = static_cast<Plugin*>(arg);
	//nl_send_auto(sk, msg)
	//int nl_send_simple(struct nl_sock *sk, int type, int flags, void *buf, size_t size);

	//int genl_send_simple  ( struct nl_sock *  sk,  int  family,  int  cmd,  int  version,  int  flags  )

	/*nl_msg *response = nlmsg_alloc();
	genlmsg_put(response, 0, 0, currentPlugin->familyId, 0, flags, cmd, 0);
	NLA_PUT_U32(response, NL80211_ATTR_IFINDEX, ifIndex);
	ret = nl_send_auto(currentPlugin->socket, response);
	nl_complete_msg(currentPlugin->socket, response);
	nlmsg_free(response);*/
	//nl_recvmsgs_default(currentPlugin->socket);

	return 0;
}

Plugin::Plugin(const string &str_name, bool b_registerInKernel) :
	name(str_name),
	socket(nullptr),
	familyId()
{
	if(b_registerInKernel)
	{
		registerInKernel();
	}
}

Plugin::~Plugin()
{
	unregister();
}

void Plugin::registerInKernel()
{
	unregister();

	socket = nl_socket_alloc();
	genl_connect(socket);
	familyId = genl_ctrl_resolve(socket, NLTD_NAME);

	nl_socket_modify_cb(socket, NL_CB_VALID, NL_CB_CUSTOM, worker_function, this);

	int ret;
	char tempName[NLTD_MAX_NAME];
	strncpy(tempName, name.c_str(), NLTD_MAX_NAME);

	nl_msg *msg = nlmsg_alloc();
	genlmsg_put(msg, 0, NL_AUTO_SEQ, familyId, NLTD_HEADER_SIZE, 0, NLTD_CMD_REGISTER, NLTD_VERSION);
	NLA_PUT(msg, NLTD_PLUGIN_NAME, NLTD_MAX_NAME, tempName);
	ret = nl_send_auto(socket, msg);
	nl_complete_msg(socket, msg);
	nlmsg_free(msg);

	return;

 nla_put_failure:
	cerr<<"Can't add attribute"<<endl;
	nlmsg_free(msg);

	//uint32_t nl_socket_get_local_port(const struct nl_sock *sk);
	//int nl_socket_set_nonblocking(const struct nl_sock *sk);

	//Message buffer size. Default 32 KiB
	//int nl_socket_set_buffer_size(struct nl_sock *sk, int rx, int tx);
}

void Plugin::unregister()
{
	if(socket)
	{
		int ret;
		char tempName[NLTD_MAX_NAME];
		strncpy(tempName, name.c_str(), NLTD_MAX_NAME);

		nl_msg *msg = nlmsg_alloc();
		genlmsg_put(msg, 0, NL_AUTO_SEQ, familyId, NLTD_HEADER_SIZE, 0, NLTD_CMD_UNREGISTER, NLTD_VERSION);
		NLA_PUT(msg, NLTD_PLUGIN_NAME, NLTD_MAX_NAME, tempName);
		ret = nl_send_auto(socket, msg);
		nl_complete_msg(socket, msg);
		nlmsg_free(msg);

		nl_socket_free(socket);
		socket = nullptr;
		return;

 nla_put_failure:
		cerr<<"Can't add attribute"<<endl;
		nlmsg_free(msg);
		nl_socket_free(socket);
		socket = nullptr;
	}
}

void Plugin::listen()
{
	nl_recvmsgs_default(socket);
}

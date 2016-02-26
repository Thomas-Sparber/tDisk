#ifndef COMMUNICATION_HERLPER_HPP
#define COMMUNICATION_HERLPER_HPP

#include <errno.h>
#include <netlink/netlink.h>
#include <netlink/types.h>
#include <netlink/genl/genl.h>
#include <string>
#include <vector>

#include <pluginexception.hpp>

namespace td
{

namespace c
{
	extern "C" {
	 	#include <tdisk/interface.h>
	}
} //end namespoace c

template <c::nl_tdisk_attr type, class T>
struct Arg
{
	Arg(const T &t_t) :
		t(t_t)
	{}
	const T &t;
}; //end struct Args

template <c::nl_tdisk_attr type, class T>
inline Arg<type,T> createArg(const T &t)
{
	return Arg<type, T>(t);
}

template <int type, typename = typename std::enable_if<type == c::NLTD_PLUGIN_NAME>::type>
inline void addNlArg(nl_msg *msg, const std::string &name)
{
	char tempName[NLTD_MAX_NAME];
	strncpy(tempName, name.c_str(), NLTD_MAX_NAME);
	NLA_PUT(msg, c::NLTD_PLUGIN_NAME, NLTD_MAX_NAME, tempName);
	return;

 nla_put_failure:
	throw PluginException("Can't add argument name to message: ", nl_geterror(errno));
}

template <int type, typename = typename std::enable_if<type == c::NLTD_REQ_NUMBER>::type>
inline void addNlArg(nl_msg *msg, const uint32_t &number)
{
	NLA_PUT_U32(msg, c::NLTD_REQ_NUMBER, number);
	return;

 nla_put_failure:
	throw PluginException("Can't add argument request number to message", nl_geterror(errno));
}

template <int type, typename = typename std::enable_if<type == c::NLTD_REQ_OFFSET>::type>
inline void addNlArg(nl_msg *msg, const unsigned long long &offset)
{
	NLA_PUT_U64(msg, c::NLTD_REQ_OFFSET, offset);
	return;

 nla_put_failure:
	throw PluginException("Can't add argument offset to message", nl_geterror(errno));
}

template <int type, typename = typename std::enable_if<type == c::NLTD_REQ_LENGTH>::type>
inline void addNlArg(nl_msg *msg, const int &length)
{
	NLA_PUT_S32(msg, c::NLTD_REQ_LENGTH, length);
	return;

 nla_put_failure:
	throw PluginException("Can't add argument offset to message", nl_geterror(errno));
}

template <int type, typename = typename std::enable_if<type == c::NLTD_REQ_BUFFER>::type>
inline void addNlArg(nl_msg *msg, const std::vector<char> &data)
{
	NLA_PUT(msg, c::NLTD_REQ_LENGTH, data.size(), &data[0]);
	return;

 nla_put_failure:
	throw PluginException("Can't add argument offset to message", nl_geterror(errno));
}

inline void addNlArgs(nl_msg*) {}

template <c::nl_tdisk_attr type, class T>
inline void addNlArgs(nl_msg *msg, const Arg<type, T> &a)
{
	addNlArg<type>(msg, a.t);
}

template <c::nl_tdisk_attr type, class T, class ...Args>
inline void addNlArgs(nl_msg *msg, const Arg<type, T> &a, Args ...args)
{
	addNlArgs(msg, a);
	addNlArgs(msg, args...);
}

template <class ...Args>
inline bool sendNlMessage(nl_sock *socket, uint8_t type, int port, int familyId, Args ...args)
{
	nl_msg *msg = nlmsg_alloc();
	genlmsg_put(msg, port, NL_AUTO_SEQ, familyId, NLTD_HEADER_SIZE, 0, type, NLTD_VERSION);

	try {
		addNlArgs(msg, args...);
	} catch(const PluginException &e) {
		std::cerr<<"Error sending message "<<(int)type<<": "<<e.what<<std::endl;
		nlmsg_free(msg);
		return false;
	}

	int ret = nl_send_auto(socket, msg);
	if(ret < 0)std::cerr<<"Error sending message: "<<ret<<std::endl;

	nlmsg_free(msg);
	return (ret >= 0);
}

} //end namespace td

#endif //COMMUNICATION_HERLPER_HPP

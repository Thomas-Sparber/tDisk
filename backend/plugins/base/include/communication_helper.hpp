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

template <int type, typename std::enable_if<type == c::NLTD_PLUGIN_NAME, bool>::type* = nullptr>
inline void addNlArg(nl_msg *msg, const std::string &name)
{
	char tempName[NLTD_MAX_NAME];
	strncpy(tempName, name.c_str(), NLTD_MAX_NAME);
	int ret = nla_put(msg, c::NLTD_PLUGIN_NAME, NLTD_MAX_NAME, tempName);
	if(ret)throw PluginException("Can't add argument name to message: ", nl_geterror(ret));
}

template <int type, typename std::enable_if<type == c::NLTD_REQ_NUMBER, bool>::type* = nullptr>
inline void addNlArg(nl_msg *msg, const uint32_t &number)
{
	int ret = nla_put_u32(msg, c::NLTD_REQ_NUMBER, number);
	if(ret)throw PluginException("Can't add argument request number to message: ", nl_geterror(ret));
}

template <int type, typename std::enable_if<type == c::NLTD_REQ_OFFSET, bool>::type* = nullptr>
inline void addNlArg(nl_msg *msg, const unsigned long long &offset)
{
	int ret = nla_put_u64(msg, c::NLTD_REQ_OFFSET, offset);
	if(ret)throw PluginException("Can't add argument offset to message: ", nl_geterror(ret));
}

template <int type, typename std::enable_if<type == c::NLTD_REQ_LENGTH, bool>::type* = nullptr>
inline void addNlArg(nl_msg *msg, const std::size_t &length)
{
	int ret = nla_put_u32(msg, c::NLTD_REQ_LENGTH, (uint32_t)length);
	if(ret)throw PluginException("Can't add argument length to message: ", nl_geterror(ret));
}

template <int type, typename std::enable_if<type == c::NLTD_REQ_RET, bool>::type* = nullptr>
inline void addNlArg(nl_msg *msg, const int &r)
{
	int ret = nla_put_s32(msg, c::NLTD_REQ_RET, r);
	if(ret)throw PluginException("Can't add argument length to message: ", nl_geterror(ret));
}

template <int type, typename std::enable_if<type == c::NLTD_REQ_BUFFER, bool>::type* = nullptr>
inline void addNlArg(nl_msg *msg, const std::vector<char> &data)
{
	int ret = nla_put(msg, c::NLTD_REQ_BUFFER, (int)data.size(), &data[0]);
	if(ret)throw PluginException("Can't add argument buffer to message: ", nl_geterror(ret));
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

template <int type, typename std::enable_if<type == c::NLTD_PLUGIN_NAME, bool>::type* = nullptr>
inline std::size_t getNlArgumentSize(const std::string&) { return NLTD_MAX_NAME; }

template <int type, typename std::enable_if<type == c::NLTD_REQ_NUMBER, bool>::type* = nullptr>
inline std::size_t getNlArgumentSize(const uint32_t&) { return 4; }

template <int type, typename std::enable_if<type == c::NLTD_REQ_OFFSET, bool>::type* = nullptr>
inline std::size_t getNlArgumentSize(const unsigned long long&) { return 8; }

template <int type, typename std::enable_if<type == c::NLTD_REQ_LENGTH, bool>::type* = nullptr>
inline std::size_t getNlArgumentSize(const std::size_t&) { return 4; }

template <int type, typename std::enable_if<type == c::NLTD_REQ_RET, bool>::type* = nullptr>
inline std::size_t getNlArgumentSize(const int&) { return 4; }

template <int type, typename std::enable_if<type == c::NLTD_REQ_BUFFER, bool>::type* = nullptr>
inline std::size_t getNlArgumentSize(const std::vector<char> &data) { return data.size(); }

inline std::size_t getNlArgumentsSize() { return 0; }

template <c::nl_tdisk_attr type, class T>
inline std::size_t getNlArgumentsSize(const Arg<type, T> &a)
{
	return getNlArgumentSize<type>(a.t);
}

template <c::nl_tdisk_attr type, class T, class ...Args>
inline std::size_t getNlArgumentsSize(const Arg<type, T> &a, Args ...args)
{
	return getNlArgumentsSize(a) +
		getNlArgumentsSize(args...);
}

template <class ...Args>
inline bool sendNlMessage(nl_sock *socket, uint8_t type, int port, int familyId, Args ...args)
{
	std::size_t size = getNlArgumentsSize(args...) + 1024;
	nl_msg *msg = nlmsg_alloc_size(size);
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

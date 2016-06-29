/**
  *
  * tDisk backend serial
  * @author Thomas Sparber (2015-2016)
  *
 **/

#ifndef SERIALPORT_HPP
#define SERIALPORT_HPP

#include <iostream>
#include <istream>
#include <errno.h>
#include <ostream>
#include <poll.h>
#include <string>
#include <string.h>
#include <vector>

#include <utils.hpp>

namespace td
{

static constexpr const char endSequence[] = "<[[[[end-of-serial-request]]]]>\n";

inline static std::size_t sequenceLength()
{
	return utils::arrayLength(endSequence) - 1;		//exclude \0
}

class Serialport
{

public:
	static bool listSerialports(std::vector<Serialport> &out);

public:

	Serialport(const std::string &str_name, int i_timeout=10000) :
		name(str_name),
		friendlyName(),
		connection(nullptr),
		timeout(i_timeout)
	{}

	Serialport(const Serialport &other) = delete;

	Serialport(Serialport &&other) = default;

	~Serialport()
	{
		closeConnection();
	}

	Serialport& operator=(const Serialport &other) = delete;

	Serialport& operator=(Serialport &&other) = default;

	void setName(const std::string &str_name)
	{
		name = str_name;
	}

	std::string getName() const
	{
		return name;
	}

	void setFriendlyName(const std::string &str_friendlyName)
	{
		friendlyName = str_friendlyName;
	}

	std::string getFriendlyName() const
	{
		return friendlyName;
	}

	bool openConnection();

	bool closeConnection();

	bool read(char *current_byte, std::size_t length);

	bool write(const char *current_byte, std::size_t length);

	bool request(std::istream &in, std::ostream &out);
	
	void* getConnection()
	{
		return connection;
	}
	
	const void* getConnection() const
	{
		return connection;
	}

	void setTimeout(int i_timeout)
	{
		this->timeout = i_timeout;
	}

	int getTimeout() const
	{
		return timeout;
	}

private:
	std::string name;
	
	std::string friendlyName;
	
	void *connection;

	int timeout;

}; //end class Serialport

} //end namespace td

#endif //SERIALPORT_HPP

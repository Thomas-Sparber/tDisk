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
#include <string>
#include <string.h>
#include <vector>

#include <utils.hpp>

namespace td
{

static constexpr const char endSequence[] = "<[[[[end-of-serial-request]]]]>\n";

static constexpr const char hashSequence[] = "<[[[[serial-request-hash]]]]>\n";

static constexpr const char hashMatchesSequence[] = "<[[[[serial-request-hash-matches]]]]>\n";

inline static std::size_t endSequenceLength()
{
	return utils::arrayLength(endSequence) - 1;		//exclude \0
}

inline static std::size_t hashSequenceLength()
{
	return utils::arrayLength(hashSequence) - 1;		//exclude \0
}

inline static std::size_t hashSequenceMatchesLength()
{
	return utils::arrayLength(hashSequence) - 1;		//exclude \0
}

class Serialport
{

public:
	enum Baud
	{
		Baud_110,
		Baud_300,
		Baud_600,
		Baud_1200,
		Baud_2400,
		Baud_4800,
		Baud_9600,
		//Baud_14400,
		Baud_19200,
		Baud_38400,
		Baud_57600,
		Baud_115200,
		//Baud_128000,
		//Baud_256000
	}; //end enum Baud

	enum Parity
	{
		EvenParity,
		MarkParity,
		NoParity,
		OddParity,
		SpaceParity
	}; //end enum Parity

	static bool listSerialports(std::vector<Serialport> &out);

public:

	Serialport(const std::string &str_name, int i_timeout=10000) :
		name(str_name),
		friendlyName(),
		connection(nullptr),
		timeout(i_timeout),
		baud(Baud_1200),
		parity(NoParity)
	{}

	Serialport(const Serialport &other) = delete;

	Serialport(Serialport &&other) :
		name(other.name),
		friendlyName(other.friendlyName),
		connection(other.connection),
		timeout(other.timeout),
		baud(other.baud),
		parity(other.parity)
	{
		other.connection = nullptr;
	}

	~Serialport()
	{
		closeConnection();
	}

	Serialport& operator=(const Serialport &other) = delete;

	Serialport& operator=(Serialport &&other)
	{
		closeConnection();

		name = other.name;
		friendlyName = other.friendlyName;
		connection = other.connection;
		timeout = other.timeout;

		return (*this);
	}

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

	Baud getBaud() const
	{
		return baud;
	}

	void setBaud(Baud b_baud)
	{
		this->baud = b_baud;
	}

	Parity getParity() const
	{
		return parity;
	}

	void setParity(Parity p_parity)
	{
		this->parity = p_parity;
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

	Baud baud;

	Parity parity;

}; //end class Serialport

} //end namespace td

#endif //SERIALPORT_HPP

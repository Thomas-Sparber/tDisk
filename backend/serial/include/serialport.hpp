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

/**
  * The char secuence which is used to mark the
  * end of a serial port communication
 **/
static constexpr const char endSequence[] = "<[[[[end-of-serial-request]]]]>\n";

/**
  * This char sequence is used as a delimiter to signal
  * the end of the hash sequence and the beginning of
  * the request
 **/
static constexpr const char hashSequence[] = "<[[[[serial-request-hash]]]]>\n";

/**
  * This char sequence is used to signal that the hashes match
 **/
static constexpr const char hashMatchesSequence[] = "<[[[[serial-request-hash-matches]]]]>\n";

/**
  * Returns the length of the end sequence
 **/
inline static std::size_t endSequenceLength()
{
	return utils::arrayLength(endSequence) - 1;		//exclude \0
}

/**
  * Returns the length of the hash sequence
 **/
inline static std::size_t hashSequenceLength()
{
	return utils::arrayLength(hashSequence) - 1;		//exclude \0
}

/**
  * Returns the length of the hash matches sequence
 **/
inline static std::size_t hashMatchesSequenceLength()
{
	return utils::arrayLength(hashMatchesSequence) - 1;		//exclude \0
}

/**
  * This class represents a serialport and provides
  * the functionality to lista all available serialports
  * and to send and receive requests.
 **/
class Serialport
{

public:

	/**
	  * The baud rate in bits per second
	  * which is used for communication
	 **/
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

	/**
	  * The parity which is used for communication
	 **/
	enum Parity
	{
		EvenParity,
		MarkParity,
		NoParity,
		OddParity,
		SpaceParity
	}; //end enum Parity

	/**
	  * Gets a list of all serialports currently installed
	  * on the system
	 **/
	static bool listSerialports(std::vector<Serialport> &out);

public:

	/**
	  * The constructor needs the name of the serialport
	  * and an optional timeout in milliseconds
	 **/
	Serialport(const std::string &str_name, int i_timeout=10000) :
		name(str_name),
		friendlyName(),
		connection(nullptr),
		timeout(i_timeout),
		baud(Baud_1200),
		parity(NoParity)
	{}

	/**
	  * Serialports are not copyable
	 **/
	Serialport(const Serialport &other) = delete;

	/**
	  * Move constructor
	 **/
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

	/**
	  * The destructor closes the connection to the serialport
	 **/
	~Serialport()
	{
		closeConnection();
	}

	/**
	  * Serialports are not assignable
	 **/
	Serialport& operator=(const Serialport &other) = delete;

	/**
	  * Move operator
	 **/
	Serialport& operator=(Serialport &&other)
	{
		closeConnection();

		name = other.name;
		friendlyName = other.friendlyName;
		connection = other.connection;
		timeout = other.timeout;

		return (*this);
	}

	/**
	  * Sets the name of the serial port
	 **/
	void setName(const std::string &str_name)
	{
		name = str_name;
	}

	/**
	  * Returns the name of the serial port
	 **/
	std::string getName() const
	{
		return name;
	}

	/**
	  * Sets the human readable name of the serial port
	 **/
	void setFriendlyName(const std::string &str_friendlyName)
	{
		friendlyName = str_friendlyName;
	}

	/**
	  * Returns the human readable name of the serial port
	 **/
	std::string getFriendlyName() const
	{
		return friendlyName;
	}

	/**
	  * Returns the baud rate used for communcation
	 **/
	Baud getBaud() const
	{
		return baud;
	}

	/**
	  * Sets the baud rate used for communication
	 **/
	void setBaud(Baud b_baud)
	{
		this->baud = b_baud;
	}

	/**
	  * Returns the parity used for communication
	 **/
	Parity getParity() const
	{
		return parity;
	}

	/**
	  * Sets the baud rate used for communication
	 **/
	void setParity(Parity p_parity)
	{
		this->parity = p_parity;
	}

	/**
	  * Opens the connection
	 **/
	bool openConnection();

	/**
	  * Closes the connection
	 **/
	bool closeConnection();

	/**
	  * Sends and receives the given request to the serial port
	 **/
	bool request(std::istream &in, std::ostream &out);

	/**
	  * Returns the internal representation of the connection.
	  * This is different on different operating systems
	 **/
	void* getConnection()
	{
		return connection;
	}

	/**
	  * Returns the internal representation of the connection.
	  * This is different on different operating systems
	 **/
	const void* getConnection() const
	{
		return connection;
	}

	/**
	  * Sets the timeout used to read from serial port
	 **/
	void setTimeout(int i_timeout)
	{
		this->timeout = i_timeout;
	}

	/**
	  * Returns the timeout used to read from serial port
	 **/
	int getTimeout() const
	{
		return timeout;
	}

protected:

	/**
	  * Reads the given amount of bytes from the serial port
	 **/
	bool read(char *current_byte, std::size_t length);

	/**
	  * Writes the given amount of bytes to the serial port
	 **/
	bool write(const char *current_byte, std::size_t length);

private:

	/**
	  * The name of the serial port
	 **/
	std::string name;

	/**
	  * The friendly name of the serial port
	 **/
	std::string friendlyName;

	/**
	  * The internal representation of the communication
	 **/
	void *connection;

	/**
	  * The timeout in milliseconds
	 **/
	int timeout;

	/**
	  * The baud rate
	 **/
	Baud baud;

	/**
	  * The parity
	 **/
	Parity parity;

}; //end class Serialport

} //end namespace td

#endif //SERIALPORT_HPP

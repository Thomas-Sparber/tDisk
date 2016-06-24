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
#include <ostream>
#include <string>
#include <vector>

#include <utils.hpp>

namespace td
{

static constexpr const char endSequence[] = "<[[[[end-of-serial-request]]]]>\n";

inline static std::size_t sequenceLength()
{
	return utils::arrayLength(endSequence);
}

class Serialport
{

public:
	static bool listSerialports(std::vector<Serialport> &out);

public:
	
	Serialport(const std::string &str_name) :
		name(str_name),
		friendlyName(),
		connection(nullptr)
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

	bool request(std::istream &in, std::ostream &out)
	{
		if(!openConnection())return false;

		char current_byte;
		std::size_t currentSequencePosition = 0;

		//Reading from input stream and writing to serial port
		while(in.read(&current_byte, 1))
		{
			if(!write(&current_byte, 1))
			{
				std::cerr<<"Error writing to Serial port"<<std::endl;
				closeConnection();
				return false;
			}

			if(current_byte == endSequence[currentSequencePosition])
			{
				currentSequencePosition++;
				if(currentSequencePosition == sequenceLength())
				{
					//All data written
					break;
				}
			}
			else currentSequencePosition = 0;
		}

		if(currentSequencePosition != sequenceLength())
		{
			if(!write(endSequence, sequenceLength()))
			{
				std::cerr<<"Error writing end sequence to Serial port"<<std::endl;
				closeConnection();
				return false;
			}
		}

		//Now reading from serial port and writing to output stream
		currentSequencePosition = 0;

		while(read(&current_byte, 1))
		{
			if(current_byte == endSequence[currentSequencePosition])
			{
				currentSequencePosition++;
				if(currentSequencePosition == sequenceLength())
				{
					//All data written
					break;
				}

				//Not writing end sequence to out stream
				continue;
			}
			else
			{
				out.write(endSequence, currentSequencePosition);
			}

			out.write(&current_byte, 1);

			if(!out)
			{
				std::cerr<<"Error writing to output stream"<<std::endl;
				closeConnection();
				return false;
			}
		}

		return (currentSequencePosition == sequenceLength());
	}
	
	void* getConnection()
	{
		return connection;
	}
	
	const void* getConnection() const
	{
		return connection;
	}

private:
	std::string name;
	
	std::string friendlyName;
	
	void *connection;

}; //end class Serialport

} //end namespace td

#endif //SERIALPORT_HPP

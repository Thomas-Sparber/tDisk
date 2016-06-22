/**
  *
  * tDisk backend serial
  * @author Thomas Sparber (2015-2016)
  *
 **/

#ifndef SERIALPORT_HPP
#define SERIALPORT_HPP

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
	Serialport(const std::string &name);
	
	Serialport(const Serialport &other) = delete;
	
	Serialport(Serialport &&other) = default;
	
	~Serialport();
	
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
	
	bool request(std::istream &in, std::ostream &out);
	
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

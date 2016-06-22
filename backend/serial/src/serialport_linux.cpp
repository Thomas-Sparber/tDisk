/**
  *
  * tDisk backend serial
  * @author Thomas Sparber (2015-2016)
  *
 **/

#ifdef __linux__

#include <serialport.hpp>

using std::string;
using std::vector;

using namespace td;

bool Serialport::listSerialports(vector<Serialport> &out)
{
	
}

Serialport::Serialport(const string &str_name) :
	name(str_name),
	friendlyName(),
	connection(nullptr)
{
	
}

Serialport::~Serialport()
{
	closeConnection();
}

bool Serialport::closeConnection()
{
	
}

bool Serialport::openConnection()
{
	
}

bool Serialport::request(std::istream &in, std::ostream &out)
{
	
}

#endif //__linux__


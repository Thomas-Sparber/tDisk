/**
  *
  * tDisk backend serial
  * @author Thomas Sparber (2015-2016)
  *
 **/

#ifdef __linux__

#include <dirent.h>
#include <fcntl.h>
#include <linux/ioctl.h>
#include <linux/serial.h>
#include <poll.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <serialport.hpp>

using std::cerr;
using std::endl;
using std::string;
using std::vector;

using namespace td;

bool Serialport::listSerialports(vector<Serialport> &out)
{
	if(DIR *dir = opendir("/dev"))
	{
		while(dirent *ent = readdir(dir))
		{
			if(strncmp(ent->d_name, "ttyS", 4) != 0)continue;

			const string path = string("/dev/") + ent->d_name;
			int fd = open(path.c_str(), O_RDWR | O_NONBLOCK | O_NOCTTY);
			if(fd <= 0)
			{
				if(errno == EACCES)
					cerr<<"Access denied to open serial port /dev/"<<ent->d_name<<endl;
				continue;
			}
			

			bool isSerialPort = true;
#ifdef TIOCGSERIAL
			serial_struct serinfo;
			isSerialPort = (ioctl(fd, TIOCGSERIAL, &serinfo) >= 0);
#endif
			close(fd);

			if(!isSerialPort)continue;

			Serialport port(ent->d_name);
			port.friendlyName = ent->d_name;
			out.push_back(std::move(port));
		}

		closedir(dir);
		return true;
	}
	else
	{
		cerr<<"Can't list all serial ports: unable to read /dev"<<endl;
		return false;
	}
}

bool Serialport::closeConnection()
{
	bool success = true;

	if(connection)
	{
		if(close(*((int*)connection)) < 0)
		{
			cerr<<"Error closing serial connection"<<endl;
			success = false;
		}

		delete (int*)connection;
	}

	connection = nullptr;
	return success;
}

bool Serialport::openConnection()
{
	if(connection)return true;

	connection = new int();
	const string path = string("/dev/") + name;
	*((int*)connection) = open(path.c_str(), O_RDWR | O_NONBLOCK | O_NOCTTY);

	if(*((int*)connection) <= 0)
	{
		cerr<<"Can't open connection: "<<path<<": "<<strerror(errno)<<endl;
		delete (int*)connection;
		connection = nullptr;
		return false;
	}

	return true;
}

bool Serialport::read(char *current_byte, std::size_t length)
{
	struct pollfd pollfd;
	pollfd.fd = *((int*)connection);

	return (::read(*((int*)connection), current_byte, length) > 0);
}

bool Serialport::write(const char *current_byte, std::size_t length)
{
	return (::write(*((int*)connection), current_byte, length) > 0);
}

#endif //__linux__


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
#include <termios.h>

#include <serialport.hpp>

using std::cerr;
using std::endl;
using std::string;
using std::vector;

using namespace td;

int set_interface_attribs(int fd, int speed, int parity)
{
	struct termios tty;
	memset (&tty, 0, sizeof tty);
	if (tcgetattr (fd, &tty) != 0)
	{
		cerr<<"error from tcgetattr:"<<strerror(errno)<<endl;
		return -1;
	}

	cfsetospeed (&tty, speed);
	cfsetispeed (&tty, speed);

	tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8;     // 8-bit chars
	// disable IGNBRK for mismatched speed tests; otherwise receive break
	// as \000 chars
	tty.c_iflag &= ~IGNBRK;         // disable break processing
	tty.c_lflag = 0;                // no signaling chars, no echo,
	// no canonical processing
	tty.c_oflag = 0;                // no remapping, no delays
	tty.c_cc[VMIN]  = 0;            // read doesn't block
	tty.c_cc[VTIME] = 5;            // 0.5 seconds read timeout

	tty.c_iflag &= ~(IXON | IXOFF | IXANY); // shut off xon/xoff ctrl

	tty.c_cflag |= (CLOCAL | CREAD);// ignore modem controls,
	// enable reading
	tty.c_cflag &= ~(PARENB | PARODD);      // shut off parity
	tty.c_cflag |= parity;
	tty.c_cflag &= ~CSTOPB;
	tty.c_cflag &= ~CRTSCTS;

	if (tcsetattr (fd, TCSANOW, &tty) != 0)
	{
		cerr<<"error from tcsetattr"<<strerror(errno)<<endl;
		return -1;
	}
	return 0;
}

void set_blocking (int fd, int should_block)
{
	struct termios tty;
	memset (&tty, 0, sizeof tty);
	if (tcgetattr (fd, &tty) != 0)
	{
		cerr<<"error from tggetattr"<<strerror(errno)<<endl;
		return;
	}

	tty.c_cc[VMIN]  = should_block ? 1 : 0;
	tty.c_cc[VTIME] = 5;            // 0.5 seconds read timeout

	if (tcsetattr (fd, TCSANOW, &tty) != 0)
		cerr<<"error setting term attributes"<<strerror(errno)<<endl;
}

inline int getLinuxBaud(Serialport::Baud baud)
{
	switch(baud)
	{
		case Serialport::Baud_110: return B110;
		case Serialport::Baud_300: return B300;
		case Serialport::Baud_600: return B600;
		case Serialport::Baud_1200: return B1200;
		case Serialport::Baud_2400: return B2400;
		case Serialport::Baud_4800: return B4800;
		case Serialport::Baud_9600: return B9600;
		//case Serialport::Baud_14400: return B14400;
		case Serialport::Baud_19200: return B19200;
		case Serialport::Baud_38400: return B38400;
		case Serialport::Baud_57600: return B57600;
		case Serialport::Baud_115200: return B115200;
		//case Serialport::Baud_128000: return B128000;
		//case Serialport::Baud_256000: return B256000;
		default:
			cerr<<"Warning invalid baud value: "<<baud<<endl;
			return baud;
	}
}

inline int getLinuxParity(Serialport::Parity parity)
{
	switch(parity)
	{
		case Serialport::EvenParity: return PARENB;
		case Serialport::Markparity: return PARENB|PARODD|CMSPAR;
		case Serialport::NoParity: return 0;
		case Serialport::OddParity: return PARENB|PARODD;
		case Serialport::SpaceParity: return PARENB|CMSPAR;
		default:
			cerr<<"Warning invalid parity "<<parity<<endl;
			return 0;
	}
}

bool Serialport::listSerialports(vector<Serialport> &out)
{
	if(DIR *dir = opendir("/dev"))
	{
		while(dirent *ent = readdir(dir))
		{
			if(strncmp(ent->d_name, "ttyS", 4) != 0)continue;

			const string path = string("/dev/") + ent->d_name;
			int fd = open(path.c_str(), O_RDWR | O_NONBLOCK | O_SYNC);
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
	*((int*)connection) = open(path.c_str(), O_RDWR | O_SYNC | O_NOCTTY);

	if(*((int*)connection) <= 0)
	{
		cerr<<"Can't open connection: "<<path<<": "<<strerror(errno)<<endl;
		delete (int*)connection;
		connection = nullptr;
		return false;
	}

	set_interface_attribs (*((int*)connection), getLinuxBaud(baud)/*B1200 B230400*/, getLinuxParity(parity));
	set_blocking (*((int*)connection), 0);

	return true;
}

bool Serialport::read(char *current_byte, std::size_t length)
{
	struct pollfd pollfd;
	pollfd.fd = *((int*)connection);
	pollfd.events = POLLIN;

	if(!poll(&pollfd, 1, timeout))
	{
		cerr<<"Read timeout of "<<timeout<<" reached"<<endl;
		return false;
	}
	

	return (::read(*((int*)connection), current_byte, length) > 0);
}

bool Serialport::write(const char *current_byte, std::size_t length)
{
	//cerr<<"Writing "<<string(current_byte, length)<<endl;
	return (::write(*((int*)connection), current_byte, length) > 0);
}

#endif //__linux__


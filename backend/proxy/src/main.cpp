/**
  *
  * tDisk backend share
  * @author Thomas Sparber (2015-2016)
  *
 **/

#include <errno.h>
#include <error.h>
#include <fcntl.h>
#include <iostream>
#include <poll.h>
#include <sstream>
#include <string>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <termios.h>
#include <unistd.h>

#include <base64.h>
#include <curldefinitions.hpp>
#include <md5.hpp>
#include <serial_config.hpp>
#include <serialport.hpp>

using std::cerr;
using std::cout;
using std::endl;
using std::min;
using std::string;
using std::stringstream;

using namespace td;

//const string ttyFile("/dev/ttyGS0");
const string ttyFile("/dev/ttyS1");
const string host("http://localhost:8080");

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

void set_blocking(int fd, int should_block)
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
		case Serialport::MarkParity: return PARENB|PARODD|CMSPAR;
		case Serialport::NoParity: return 0;
		case Serialport::OddParity: return PARENB|PARODD;
		case Serialport::SpaceParity: return PARENB|CMSPAR;
		default:
			cerr<<"Warning invalid parity "<<parity<<endl;
			return 0;
	}
}

int main(int argc, char *args[])
{
	initCurl();

	int fd = open(ttyFile.c_str(), O_RDWR | O_SYNC /*| O_NONBLOCK */| O_NOCTTY);
	if(fd <= 0)
	{
		cerr<<"Can't open tty "<<ttyFile<<": "<<strerror(errno)<<endl;
		return 1;
	}

	set_interface_attribs (fd, getLinuxBaud(default_baud)/*B1200 B230400*/, getLinuxParity(default_parity));
	set_blocking (fd, 0);

	bool echo = false;
	if(argc > 1 && string(args[1]) == "-e")
	{
		echo = true;
	}

//	struct pollfd pollfd;
//	pollfd.fd = fd;
	char c;
	stringstream ss;
	while(true)
	{
//		while(!poll(&pollfd, 1, 1000));
		int r = read(fd, &c, 1);
		if(r <= 0)continue;

		ss.write(&c, 1);

		if(ss.str().length() >= td::endSequenceLength() && ss.str().compare(ss.str().length() - td::endSequenceLength(), td::endSequenceLength(), td::endSequence) == 0)
		{
			const string str = ss.str().substr(0, ss.str().length() - td::endSequenceLength());
			ss.str(string());

			string hash;
			string request;

			std::size_t pos = str.find(td::hashSequence);
			if(pos == string::npos)
			{
				request = str;
			}
			else
			{
				hash = str.substr(0, pos);
				request = str.substr(pos + td::hashSequenceLength());
			}
			cerr<<"New request: "<<request;
			if(hash != "")cerr<<" ("<<hash<<")";
			cerr<<endl;

			string result;
			if(echo)
			{
				cerr<<"Echo"<<endl;
				result = request;
			}
			else
			{
				long code;
				string contentType;
				result = getSite(host+request, code, contentType);
			}

			result = base64_encode((const unsigned char*)result.c_str(), result.length());
			const string md5Hash = md5::getMD5(result);
			string answer;
			if(md5Hash == hash)
			{
				cerr<<"Hashes match"<<endl;
				answer = string(td::hashMatchesSequence) + td::endSequence;
			}
			else
			{
				cerr<<"Transferring"<<endl;
				answer = result + td::endSequence;
			}
			
			int res = write(fd, answer.c_str(), answer.length());
			if(res <= 0)cerr<<"Error writing to tty "<<ttyFile<<": "<<strerror(errno)<<endl;
		}
	}
}

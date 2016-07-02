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
#include <serialport.hpp>

using std::cerr;
using std::cout;
using std::endl;
using std::min;
using std::string;
using std::stringstream;

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

int main(int argc, char *args[])
{
	initCurl();

	int fd = open(ttyFile.c_str(), O_RDWR | O_SYNC /*| O_NONBLOCK */| O_NOCTTY);
	if(fd <= 0)
	{
		cerr<<"Can't open tty "<<ttyFile<<": "<<strerror(errno)<<endl;
		return 1;
	}

	set_interface_attribs (fd, B1200/*B230400*/, 0);
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
			cerr<<"New request: "<<request<<endl;

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
			if((md5Hash == hash))
			{
				cerr<<"Hashes match"<<endl;
				answer = string(td::hashMatchesSequence) + td::endSequence;
			}
			else
			{
				answer = result + td::endSequence;
			}
			
			int res = write(fd, answer.c_str(), answer.length());
			if(res <= 0)cerr<<"Error writing to tty "<<ttyFile<<": "<<strerror(errno)<<endl;
		}
	}
}

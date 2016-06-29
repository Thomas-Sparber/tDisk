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
#include <sstream>
#include <string>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <curldefinitions.hpp>

using std::cout;
using std::endl;
using std::string;
using std::stringstream;

const string endString("<[[[[end-of-serial-request]]]]>\n");
//const string tty("/dev/ttyGS0");
const string tty("/dev/ttyS0");
const string host("http://localhost:8080");

int main(int argc, char *args[])
{
	initCurl();

	int fd = open(tty.c_str(), O_RDWR | /*O_NONBLOCK | */O_NOCTTY);
	if(fd <= 0)
	{
		cout<<"Can't open tty "<<tty<<": "<<strerror(errno)<<endl;
		return 1;
	}

	bool echo = false;
	if(argc > 1 && string(args[1]) == "-e")
	{
		echo = true;
	}

	char c;
	stringstream ss;
	while(true)
	{
		read(fd, &c, 1);
		//if(!fs)cout<<"Error reading from tty "<<tty<<endl;

		ss.write(&c, 1);

		if(ss.str().length() >= endString.length() && ss.str().compare(ss.str().length() - endString.length(), endString.length(), endString) == 0)
		{
			const string request = ss.str().substr(0, ss.str().length() - endString.length());
			cout<<"New request: "<<request<<endl;
			ss.str(string());

			string result;
			if(echo)
			{
				cout<<"Echo"<<endl;
				result = request;
			}
			else
			{
				long code;
				string contentType;
				result = getSite(host+request, code, contentType);
			}

			const string answer = result + endString;
			cout<<"Result: "<<answer<<endl;

			int res = write(fd, answer.c_str(), answer.length());
			if(res <= 0)cout<<"Error writing to tty "<<tty<<": "<<strerror(errno)<<endl;
		}
	}
}

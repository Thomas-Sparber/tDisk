/**
  *
  * tDisk backend share
  * @author Thomas Sparber (2015-2016)
  *
 **/

#include <errno.h>
#include <error.h>
#include <iostream>
#include <sstream>
#include <string>
#include <unistd.h>

#include <curldefinitions.hpp>

using std::cout;
using std::endl;
using std::string;
using std::stringstream;

const string endString("<[[[[end-of-serial-request]]]]>\n");
const string tty("/dev/ttyGS0");
const string host("http://localhost:8080");

int main(int argc, char *args[])
{
	initCurl();

	FILE *fd = fopen(tty.c_str(), "r+");
	if(!fd)
	{
		cout<<"Can't open tty "<<tty<<endl;
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
		fread(&c, 1, 1, fd);
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

			int res = fwrite(answer.c_str(), 1, answer.length(), fd);
			if(res <= 0)cout<<"Error writing to tty "<<tty<<": "<<strerror(errno)<<endl;
		}
	}
}

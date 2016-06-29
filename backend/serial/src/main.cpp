/**
  *
  * tDisk backend serial
  * @author Thomas Sparber (2015-2016)
  *
 **/

#include <iostream>
#include <string>
#include <vector>

#include <ci_string.hpp>
#include <serialport.hpp>
#include <utils.hpp>

using std::cerr;
using std::cin;
using std::cout;
using std::endl;
using std::find;
using std::string;
using std::vector;

using namespace td;

using utils::ci_string;

void printUsage(const char *progName)
{
	cerr<<"Usage: "<<progName<<" command [parameters]"<<endl;
	cerr<<endl;
	cerr<<"Valid commands are:"<<endl;
	cerr<<"\t- list_serialports: Lists all serial ports"<<endl;
	cerr<<"\t- request: Sends a request to the given serial port"<<endl;
}

void listSerialPorts()
{
	vector<Serialport> ports;
	if(!Serialport::listSerialports(ports))
	{
		cerr<<"Error getting serial ports"<<endl;
	}

	for(const Serialport &port : ports)
	{
		cout<<port.getName()<<endl;
		cout<<port.getFriendlyName()<<endl;
		cout<<endl;
	}
}

void request(const string &p)
{
	Serialport port(p);

	port.request(cin, cout);
	cout.flush();
}

int main(int argc, char *args[])
{
	if(argc < 2)
	{
		printUsage(args[0]);
		return 1;
	}

	ci_string command(args[1]);
	if(command == "list_serialports")
	{
		listSerialPorts();
	}
	else if(command == "request")
	{
		if(argc < 3)
		{
			cerr<<"Please specify the desired serial port"<<endl;
			printUsage(args[0]);
			return 1;
		}
		
		request(args[2]);
	}
	else
	{
		cerr<<"Invalid command "<<command<<endl;
		printUsage(args[0]);
		return 1;
	}
	
	return 0;
}

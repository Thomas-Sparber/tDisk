/**
  *
  * tDisk backend serial
  * @author Thomas Sparber (2015-2016)
  *
 **/

#ifndef __linux__

#include <stdio.h>
#include <comdef.h>
#include <iostream>
#include <WbemCli.h>
#include <windows.h>

#include <serialport.hpp>
#include <wmi.hpp>
#include <wmiclasses.hpp>

using std::cerr;
using std::endl;
using std::string;
using std::vector;
using std::wstring;
using Wmi::Win32_SerialPort;
using Wmi::WmiException;

using namespace td;

struct Connection
{
	HANDLE hSerial;
	DCB dcbSerialParams;
	COMMTIMEOUTS timeouts;
}; //end struct connection

inline Connection& conn(Serialport *ref)
{
	return *(Connection*)ref->getConnection();
}

inline const Connection& conn(const Serialport *ref)
{
	return *(const Connection*)ref->getConnection();
}

bool Serialport::listSerialports(vector<Serialport> &out)
{
	try {
		for(const Win32_SerialPort &port : Wmi::retrieveAllWmi<Win32_SerialPort>())
		{
			Serialport temp(port.Name);
			temp.friendlyName = port.Description;
			out.push_back(std::move(temp));
		}
	} catch (const WmiException &ex) {
		cerr<<"Error listing serial ports: "<<ex.errorMessage<<" ("<<ex.hexErrorCode()<<")"<<endl;
		return false;
	}

	return true;
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
	bool success = true;
	
	if(connection)
	{
		if(CloseHandle(conn(this).hSerial) == 0)
		{
			cerr<<"Error closing serial port"<<endl;
			success = false;
		}

		delete (Connection*)connection;
	}

	connection = nullptr;
	return success;
}

bool Serialport::openConnection()
{
	if(connection)return true;

	connection = new Connection();
	const string path = utils::concat("\\\\.\\",name);
	conn(this).hSerial = CreateFile(path.c_str(), GENERIC_READ|GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
	if(conn(this).hSerial == INVALID_HANDLE_VALUE)
	{
		cerr<<"Error opening serial port "<<conn(this).hSerial<<endl;
		return false;
    }
	
	//Set device parameters (38400 baud, 1 start bit,
	//1 stop bit, no parity)
	conn(this).dcbSerialParams.DCBlength = sizeof(conn(this).dcbSerialParams);
	if (GetCommState(conn(this).hSerial, &conn(this).dcbSerialParams) == 0)
	{
		cerr<<"Error getting device state"<<endl;
		closeConnection();
		return false;
	}
     
	conn(this).dcbSerialParams.BaudRate = CBR_38400;
	conn(this).dcbSerialParams.ByteSize = 8;
	conn(this).dcbSerialParams.StopBits = ONESTOPBIT;
	conn(this).dcbSerialParams.Parity = NOPARITY;
	if(SetCommState(conn(this).hSerial, &conn(this).dcbSerialParams) == 0)
	{
		cerr<<"Error setting device parameters"<<endl;
		closeConnection();
		return false;
    }

	//Set COM port timeout settings
	conn(this).timeouts.ReadIntervalTimeout = 50;
	conn(this).timeouts.ReadTotalTimeoutConstant = 50;
	conn(this).timeouts.ReadTotalTimeoutMultiplier = 10;
	conn(this).timeouts.WriteTotalTimeoutConstant = 50;
	conn(this).timeouts.WriteTotalTimeoutMultiplier = 10;
	if(SetCommTimeouts(conn(this).hSerial, &conn(this).timeouts) == 0)
	{
		cerr<<"Error setting timeouts"<<endl;
		closeConnection();
		return false;
	}
	
	return true;
}

bool Serialport::request(std::istream &in, std::ostream &out)
{
	if(!openConnection())return false;
	
	char current_byte;
	DWORD bytes_written;
	std::size_t currentSequencePosition = 0;
	
	//Reading from input stream and writing to serial port
	while(in.read(&current_byte, 1))
	{
		if(!WriteFile(conn(this).hSerial, &current_byte, 1, &bytes_written, nullptr))
		{
			cerr<<"Error writing to Serial port"<<endl;
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
		if(!WriteFile(conn(this).hSerial, &endSequence, (DWORD)sequenceLength(), &bytes_written, nullptr))
		{
			cerr<<"Error writing end sequence to Serial port"<<endl;
			closeConnection();
			return false;
		}
	}
	
	//Now reading from serial port and writing to output stream
	currentSequencePosition = 0;
	
	while(ReadFile(conn(this).hSerial, &current_byte, 1, &bytes_written, nullptr))
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
			cerr<<"Error writing to output stream"<<endl;
			closeConnection();
			return false;
		}
	}

	return (currentSequencePosition == sequenceLength());
}

#endif //__linux__

/**
  *
  * tDisk backend serial
  * @author Thomas Sparber (2015-2016)
  *
 **/

#ifndef __linux__

#include <stdio.h>
#include <comdef.h>
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

inline DWORD getWindowsBaud(Serialport::Baud baud)
{
	switch(baud)
	{
		case Serialport::Baud_110: return CBR_110;
		case Serialport::Baud_300: return CBR_300;
		case Serialport::Baud_600: return CBR_600;
		case Serialport::Baud_1200: return CBR_1200;
		case Serialport::Baud_2400: return CBR_2400;
		case Serialport::Baud_4800: return CBR_4800;
		case Serialport::Baud_9600: return CBR_9600;
		//case Serialport::Baud_14400: return CBR_14400;
		case Serialport::Baud_19200: return CBR_19200;
		case Serialport::Baud_38400: return CBR_38400;
		case Serialport::Baud_57600: return CBR_57600;
		case Serialport::Baud_115200: return CBR_115200;
		//case Serialport::Baud_128000: return CBR_128000;
		//case Serialport::Baud_256000: return CBR_256000;
		default:
			cerr<<"Warning invalid baud value: "<<baud<<endl;
			return baud;
	}
}

inline BYTE getWindowsParity(Serialport::Parity parity)
{
	switch(parity)
	{
		case Serialport::EvenParity: return EVENPARITY;
		case Serialport::MarkParity: return MARKPARITY;
		case Serialport::NoParity: return NOPARITY;
		case Serialport::OddParity: return ODDPARITY;
		case Serialport::SpaceParity: return SPACEPARITY;
		default:
			cerr<<"Warning invalid parity "<<parity<<endl;
			return 0;
	}
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
		delete (Connection*)connection;
		connection = nullptr;
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
     
	conn(this).dcbSerialParams.BaudRate = getWindowsBaud(baud);
	conn(this).dcbSerialParams.ByteSize = 8;
	conn(this).dcbSerialParams.StopBits = ONESTOPBIT;
	conn(this).dcbSerialParams.Parity = getWindowsParity(parity);
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

bool Serialport::read(char *current_byte, std::size_t length)
{
	DWORD bytes_written;
	return ReadFile(conn(this).hSerial, &current_byte, (DWORD)length, &bytes_written, nullptr);
}

bool Serialport::write(const char *current_byte, std::size_t length)
{
	DWORD bytes_written;
	return WriteFile(conn(this).hSerial, &current_byte, (DWORD)length, &bytes_written, nullptr);
}

#endif //__linux__

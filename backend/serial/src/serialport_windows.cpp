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

using std::cerr;
using std::endl;
using std::string;
using std::vector;
using std::wstring;

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
	CoInitialize(nullptr);

	//Create the WBEM locator
	IWbemLocator *pLocator = NULL;
	HRESULT hr = CoCreateInstance(CLSID_WbemLocator, NULL, CLSCTX_INPROC_SERVER, IID_IWbemLocator, (void**)&pLocator);
	if(FAILED(hr))
	{
		switch(hr)
		{
			case REGDB_E_CLASSNOTREG:
			cerr<<"Error initializing IWbemLocator: REGDB_E_CLASSNOTREG ("<<hr<<")"<<endl;
			break;
		case CLASS_E_NOAGGREGATION:
			cerr<<"Error initializing IWbemLocator: CLASS_E_NOAGGREGATION ("<<hr<<")"<<endl;
			break;
		case E_NOINTERFACE:
			cerr<<"Error initializing IWbemLocator: E_NOINTERFACE ("<<hr<<")"<<endl;
			break;
		case E_POINTER:
			cerr<<"Error initializing IWbemLocator: E_POINTER ("<<hr<<")"<<endl;
			break;
		default:
			cerr<<"Error initializing IWbemLocator: Unknown Error ("<<hr<<")"<<endl;
			break;
		}
		CoUninitialize();
		return false;
	}

	//Open connection to local computer
	IWbemServices *pServices = NULL;
	hr = pLocator->ConnectServer(_bstr_t("\\\\.\\root\\cimv2"), NULL, NULL, NULL, 0, NULL, NULL, &pServices);
	if(FAILED(hr))
	{
		switch(hr)
		{
		case WBEM_E_ACCESS_DENIED:
			cerr<<"Error initializing IWbemServices: WBEM_E_ACCESS_DENIED ("<<hr<<")"<<endl;
			break;
		case WBEM_E_FAILED:
			cerr<<"Error initializing IWbemServices: WBEM_E_FAILED ("<<hr<<")"<<endl;
			break;
		case WBEM_E_INVALID_NAMESPACE:
			cerr<<"Error initializing IWbemServices: WBEM_E_INVALID_NAMESPACE ("<<hr<<")"<<endl;
			break;
		case WBEM_E_INVALID_PARAMETER:
			cerr<<"Error initializing IWbemServices: WBEM_E_INVALID_PARAMETER ("<<hr<<")"<<endl;
			break;
		case WBEM_E_OUT_OF_MEMORY:
			cerr<<"Error initializing IWbemServices: WBEM_E_OUT_OF_MEMORY ("<<hr<<")"<<endl;
			break;
		case WBEM_E_TRANSPORT_FAILURE:
			cerr<<"Error initializing IWbemServices: WBEM_E_TRANSPORT_FAILURE ("<<hr<<")"<<endl;
			break;
		case WBEM_E_LOCAL_CREDENTIALS:
			cerr<<"Error initializing IWbemServices: WBEM_E_LOCAL_CREDENTIALS ("<<hr<<")"<<endl;
			break;
		default:
			cerr<<"Error initializing IWbemServices: Unknown Error ("<<hr<<")"<<endl;
			break;
		}
		pLocator->Release(); 
		CoUninitialize();
		return false;
	}

	//Set authentication proxy
	hr = CoSetProxyBlanket(pServices, 
		RPC_C_AUTHN_DEFAULT, 
		RPC_C_AUTHZ_NONE, 
		COLE_DEFAULT_PRINCIPAL, 
		RPC_C_AUTHN_LEVEL_DEFAULT, 
		RPC_C_IMP_LEVEL_IMPERSONATE, 
		nullptr, 
		EOAC_NONE 
	);
	
	if(FAILED(hr))
	{
		cerr<<"Coult not set proxy blanket. Error code: "<<hr<<endl;
		pServices->Release();
		pLocator->Release();
		CoUninitialize();
		return false;
	}

	//Execute the query
	IEnumWbemClassObject *pClassObject = NULL;
	hr = pServices->ExecQuery(_bstr_t("WQL"), _bstr_t("Select * From Win32_ParallelPort"), WBEM_FLAG_RETURN_IMMEDIATELY|WBEM_FLAG_FORWARD_ONLY, NULL, &pClassObject);
	if(FAILED(hr))
	{
		switch(hr)
		{
			case WBEM_E_ACCESS_DENIED:
			cerr<<"Error executing query: WBEM_E_ACCESS_DENIED ("<<hr<<")"<<endl;
			break;
		case WBEM_E_FAILED:
			cerr<<"Error executing query: WBEM_E_FAILED ("<<hr<<")"<<endl;
			break;
		case WBEM_E_INVALID_CLASS:
			cerr<<"Error executing query: WBEM_E_INVALID_CLASS ("<<hr<<")"<<endl;
			break;
		case WBEM_E_INVALID_PARAMETER:
			cerr<<"Error executing query: WBEM_E_INVALID_PARAMETER ("<<hr<<")"<<endl;
			break;
		case WBEM_E_OUT_OF_MEMORY:
			cerr<<"Error executing query: WBEM_E_OUT_OF_MEMORY ("<<hr<<")"<<endl;
			break;
		case WBEM_E_SHUTTING_DOWN:
			cerr<<"Error executing query: WBEM_E_SHUTTING_DOWN ("<<hr<<")"<<endl;
			break;
		case WBEM_E_TRANSPORT_FAILURE:
			cerr<<"Error executing query: WBEM_E_TRANSPORT_FAILURE ("<<hr<<")"<<endl;
			break;
		default:
			cerr<<"Error executing query: Unknown Error ("<<hr<<")"<<endl;
			break;
		}
		pServices->Release();
		pLocator->Release(); 
		CoUninitialize();
		return false;
	}

	//Now enumerate all the ports
	hr = WBEM_S_NO_ERROR;

	//The final Next will return WBEM_S_FALSE
	while(hr == WBEM_S_NO_ERROR)
	{
		ULONG uReturned = 0;
		IWbemClassObject *apObj[10];
		hr = pClassObject->Next(WBEM_INFINITE, 10, apObj, &uReturned);
		if(SUCCEEDED(hr))
		{
			for (ULONG n = 0; n < uReturned; ++n)
			{
				VARIANT varProperty1;
				HRESULT hrGet = apObj[n]->Get(L"DeviceID", 0, &varProperty1, NULL, NULL);
				if(SUCCEEDED(hrGet) && varProperty1.vt == VT_BSTR && wcslen(varProperty1.bstrVal) > 3)
				{
					//if(_wcsnicmp(varProperty1.bstrVal, L"COM", 3) == 0 && isdigit(varProperty1.bstrVal[3]))
					//{
						wstring name(varProperty1.bstrVal);
						Serialport port(string(name.cbegin(), name.cend()));

						//Also get the friendly name of the port
						VARIANT varProperty2;
						if(SUCCEEDED(apObj[n]->Get(L"Name", 0, &varProperty2, NULL, NULL)) && varProperty2.vt == VT_BSTR)
						{
							wstring friendlyName(varProperty2.bstrVal);
							port.setFriendlyName(string(friendlyName.cbegin(), friendlyName.cend()));
						}

						out.push_back(std::move(port));
					//}
				}
			}
		}
		else
		{
			switch(hr)
			{
			case WBEM_E_INVALID_PARAMETER:
				cerr<<"Error getting next element: WBEM_E_INVALID_PARAMETER ("<<hr<<")"<<endl;
				break;
			case WBEM_E_OUT_OF_MEMORY:
				cerr<<"Error getting next element: WBEM_E_OUT_OF_MEMORY ("<<hr<<")"<<endl;
				break;
			case WBEM_E_UNEXPECTED:
				cerr<<"Error getting next element: WBEM_E_UNEXPECTED ("<<hr<<")"<<endl;
				break;
			case WBEM_E_TRANSPORT_FAILURE:
				cerr<<"Error getting next element: WBEM_E_TRANSPORT_FAILURE ("<<hr<<")"<<endl;
				break;
			case WBEM_S_FALSE:
				cerr<<"Error getting next element: WBEM_S_FALSE ("<<hr<<")"<<endl;
				break;
			case WBEM_S_TIMEDOUT:
				cerr<<"Error getting next element: WBEM_S_TIMEDOUT ("<<hr<<")"<<endl;
				break;
			default:
				cerr<<"Error getting next element: Unknown Error ("<<hr<<")"<<endl;
				break;
			}
		}
	}

	pServices->Release();
	pLocator->Release(); 
	CoUninitialize();

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

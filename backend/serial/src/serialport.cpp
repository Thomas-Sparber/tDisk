/**
  *
  * tDisk backend serial
  * @author Thomas Sparber (2015-2016)
  *
 **/

#include <sstream>
#include <string.h>
#include <vector>

//#include <base64.h>
#include <compress.hpp>
#include <database.hpp>
#include <md5.hpp>
#include <serialport.hpp>

using std::cerr;
using std::endl;
using std::istream;
using std::ostream;
using std::string;
using std::stringstream;
using std::vector;

using namespace td;

const vector<Table> dbTables = {
	Table("request", { "request", "hash", "data" })
};

bool Serialport::request(istream &in, ostream &out)
{
	if(!openConnection())return false;

	char current_byte;
	std::size_t currentSequencePosition = 0;

	Database db;
	if(!db.open("requests.db"))
		cerr<<db.getError()<<endl;

	if(!db.createTables(dbTables))
		cerr<<db.getError()<<endl;

	stringstream req;

	//Reading from input stream and writing to serial port
	while(in.read(&current_byte, 1))
	{
		/*if(!write(&current_byte, 1))
		{
			cerr<<"Error writing to Serial port: "<<strerror(errno)<<endl;
			closeConnection();
			return false;
		}*/

		if(current_byte == endSequence[currentSequencePosition])
		{
			currentSequencePosition++;
			if(currentSequencePosition == endSequenceLength())
			{
				//All data written
				break;
			}

			continue;
		}
		else
		{
			req.write(endSequence, currentSequencePosition);
			currentSequencePosition = 0;
		}

		req.write(&current_byte, 1);
	}

	//Search and write hash
	const string requestStr = req.str();
	QueryResult queryResult = db.select("request", { {"request", requestStr} });
	if(queryResult)
	{
		const string hash = queryResult.get<string>(1);
		if(!write(hash.c_str(), hash.length()))
		{
			cerr<<"Error writing to Serial port: "<<strerror(errno)<<endl;
			closeConnection();
			return false;
		}

		if(!write(hashSequence, hashSequenceLength()))
		{
			cerr<<"Error writing to Serial port: "<<strerror(errno)<<endl;
			closeConnection();
			return false;
		}
	}

	//Write request
	if(!write(requestStr.c_str(), requestStr.length()))
	{
		cerr<<"Error writing to Serial port: "<<strerror(errno)<<endl;
		closeConnection();
		return false;
	}

	if(!write(endSequence, endSequenceLength()))
	{
		cerr<<"Error writing end sequence to Serial port"<<endl;
		closeConnection();
		return false;
	}

	//Now reading from serial port and writing to output stream
	currentSequencePosition = 0;

	stringstream result;
	while(read(&current_byte, 1))
	{
		if(current_byte == endSequence[currentSequencePosition])
		{
			currentSequencePosition++;
			if(currentSequencePosition == endSequenceLength())
			{
				//All data written
				break;
			}

			//Not writing end sequence to out stream
			continue;
		}
		else
		{
			result.write(endSequence, currentSequencePosition);
			currentSequencePosition = 0;
		}

		result.write(&current_byte, 1);
	}

	string resultStr = result.str();
	if(resultStr == hashMatchesSequence)
	{
		resultStr = queryResult.get<string>(2);
	}
	else
	{
		resultStr = decompress(resultStr).data();

		bool success = db.remove("request", { { "request", requestStr} } );
		if(!success)
		{
			cerr<<db.getError()<<endl;
		}

		success = db.save("request", { { "request", requestStr }, { "hash", md5::getMD5(resultStr) }, { "data", resultStr } });
		if(!success)
		{
			cerr<<db.getError()<<endl;
		}
	}

	//resultStr = base64_decode(resultStr);
	out.write(resultStr.c_str(), resultStr.length());
	if(!out)
	{
		cerr<<"Error writing to output stream"<<endl;
		closeConnection();
		return false;
	}

	return (currentSequencePosition == endSequenceLength());
}

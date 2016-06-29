/**
  *
  * tDisk backend serial
  * @author Thomas Sparber (2015-2016)
  *
 **/

#include <string.h>

#include <md5.hpp>
#include <serialport.hpp>

using std::cerr;
using std::endl;
using std::istream;
using std::ostream;

using namespace td;

bool Serialport::request(istream &in, ostream &out)
{
	if(!openConnection())return false;

	char current_byte;
	std::size_t currentSequencePosition = 0;

	//Reading from input stream and writing to serial port
	while(in.read(&current_byte, 1))
	{
		if(!write(&current_byte, 1))
		{
			cerr<<"Error writing to Serial port: "<<strerror(errno)<<endl;
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
			//cerr<<currentSequencePosition<<"/"<<sequenceLength()<<": "<<current_byte<<endl;
		}
		else currentSequencePosition = 0;
	}

	//cerr<<"EOF"<<endl;

	if(currentSequencePosition != sequenceLength())
	{
		if(!write(endSequence, sequenceLength()))
		{
			cerr<<"Error writing end sequence to Serial port"<<endl;
			closeConnection();
			return false;
		}
	}

	//Now reading from serial port and writing to output stream
	currentSequencePosition = 0;

	while(read(&current_byte, 1))
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
			currentSequencePosition = 0;
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

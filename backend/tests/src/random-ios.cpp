#include <fstream>
#include <iostream>
#include <string>
#include <time.h>
#include <vector>

#include <input_processor.hpp>
#include <tdisk.hpp>
#include <test_utils.hpp>

using std::cerr;
using std::cin;
using std::cout;
using std::endl;
using std::fstream;
using std::move;
using std::string;
using std::vector;

using namespace td;

void doTests();

vector<InputDefinition> inputs {
	InputDefinition("disk",		"Which tDisk do you want to test?", getTDisks()),
	InputDefinition("operation","Read or write operation?", vector<string> { "read", "write" }, "read"),
	InputDefinition("start",	"Start position on the disk (in byte)?", "0"),
	InputDefinition("end",		"End position on the disk (in byte, 0 = EOF)?", "0"),
	InputDefinition("blocksize","Blocksize to use (in byte)?", "512"),
	InputDefinition("duration",	"How long (in seconds) should I test?", "10"),
	InputDefinition("command",	"Is there a command I should execute between each iteration?")
};

int main(int argc, char *args[])
{
	cout<<"This program generates random read/write operations on a disk."<<endl;
	cout<<endl;

	try {
		setInputQuestions(inputs, argc, args);
		askRemainingInputQuestions(inputs);

		doTests();
	} catch(const InputException &e) {
		cerr<<"Error: "<<e.message<<endl;
		return 1;
	}

	return 0;
}

void doTests()
{
	string disk = getValue<string>(inputs, "disk");
	bool read = getValue<string>(inputs, "operation") == "read";
	unsigned long long start = getValue<unsigned long>(inputs, "start");
	unsigned long long end = getValue<unsigned long>(inputs, "end");
	unsigned int blocksize = getValue<unsigned int>(inputs, "blocksize");
	unsigned int duration = getValue<unsigned int>(inputs, "duration");
	string command = getValue<string>(inputs, "command");

	time_t startTime;
	time_t endTime;
	time(&startTime);
	vector<char> data(blocksize);
	unsigned int iterations = 0;

	fstream file(string("/dev/") + disk, std::ios_base::binary | (read ? std::ios_base::in : std::ios_base::out));

	if(!file)throw InputException("Error opening file /dev/", disk);

	if(end == 0)
	{
		file.seekg(0, file.end);
		end = file.tellg();
	}

	start /= blocksize;
	end /= blocksize;

	if(end-start < blocksize)throw InputException("Can't do test because blocksize (", blocksize, ") is greater than end (", end, ") - start (", start, ")");
	srandom((unsigned int)time(nullptr));

	do
	{
		unsigned long long pos = (random() % (end-start) + start) * blocksize;

		if(read)
		{
			file.seekg(pos);
			file.read(&data[0], data.size());
		}
		else
		{
			utils::generateRandomData(data);
			file.seekp(pos);
			file.write(&data[0], data.size());
		}

		if(!file)throw InputException("Error in file /dev/", disk, " during operation");

		if(command != "")system(command.c_str());

		iterations++;
		time(&endTime);
	}
	while(startTime+(long)duration > endTime);

	string unit;
	long double performance = (long double)(iterations*blocksize) / (endTime-startTime);
	utils::getSizeValue(performance, unit);
	cout<<"Test finished. AVG "<<(read ? "read" : "write")<<" performance: "<<performance<<" "<<unit<<"/sec"<<endl;
}

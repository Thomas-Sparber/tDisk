#include <fstream>
#include <iostream>
#include <string>
#include <unistd.h>
#include <vector>

#include <input_processor.hpp>
#include <tdisk.hpp>
#include <test_utils.hpp>

using std::cerr;
using std::cin;
using std::cout;
using std::endl;
using std::fstream;
using std::string;
using std::vector;

using namespace td;

void doTests();

vector<InputDefinition> inputs {
	InputDefinition("disk",		"Which tDisk do you want to test?", getTDisks()),
	InputDefinition("start",	"Start position on the disk (in byte)?", "0"),
	InputDefinition("end",		"End position on the disk (in byte, 0 = EOF)?", "0"),
	InputDefinition("blocksize","Blocksize to use (in byte)?", "1048576"),
	InputDefinition("minsleep",	"Whats the minimum sleep time between each iteration (in us)?", "4500000"),
	InputDefinition("maxsleep",	"Whats the maximum sleep time between each iteration (in us)?", "5500000"),
	InputDefinition("iterations","How often should I repeat it?", "20"),
	InputDefinition("command",	"Is there a command I should execute between each iteration?")	//echo 3 > /proc/sys/vm/drop_caches
};

int main(int argc, char *args[])
{
	cout<<"This program writes random data to a dists, waits a given delay and checks if the data is still the same."<<endl;
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
	unsigned long long start = getValue<unsigned long>(inputs, "start");
	unsigned long long end = getValue<unsigned long>(inputs, "end");
	unsigned int blocksize = getValue<unsigned int>(inputs, "blocksize");
	unsigned long minsleep = getValue<unsigned long>(inputs, "minsleep");
	unsigned long maxsleep = getValue<unsigned long>(inputs, "maxsleep");
	unsigned int iterations = getValue<unsigned int>(inputs, "iterations");
	string command = getValue<string>(inputs, "command");

	vector<char> data(blocksize);
	vector<char> data2(blocksize);

	fstream file(string("/dev/") + disk, std::ios_base::binary | std::ios_base::in | std::ios_base::out);

	if(!file)throw InputException("Error opening file /dev/", disk);

	if(end == 0)
	{
		file.seekg(0, file.end);
		end = file.tellg();
	}

	if(end-start < blocksize)throw InputException("Can't do test because blocksize (", blocksize, ") is greater than end (", end, ") - start (", start, ")");
	srandom((unsigned int)time(nullptr));

	for(unsigned long i = 0; i < iterations; ++i)
	{
		unsigned long long pos = random() % (end-start-blocksize) + start;

		utils::generateRandomData(data);

		file.seekp(pos);
		file.write(&data[0], data.size());

		if(!file)throw InputException("Unable to write to file");

		usleep((__useconds_t)(minsleep + (random() % (maxsleep-minsleep))));

		file.seekg(pos);
		file.read(&data2[0], data2.size());

		if(!file)throw InputException("Unable to read file");

		for(size_t j = 0; j < data.size(); ++j)
		{
			if(data[j] != data2[j])
				throw InputException("Data is not identical at byte ", pos+j);
		}

		if(command != "")system(command.c_str());
	}
}

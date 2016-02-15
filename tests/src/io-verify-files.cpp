#include <fstream>
#include <iostream>
#include <string>
#include <unistd.h>
#include <vector>

#include <input_processor.hpp>
#include <tdisk.hpp>
#include <test_utils.hpp>
#include <utils.hpp>

using std::cerr;
using std::cin;
using std::cout;
using std::endl;
using std::fstream;
using std::string;
using std::vector;

void doTests();

vector<InputDefinition> inputs {
	InputDefinition("folder",	"Which folder should be used (where the disk is mounted)?"),
	InputDefinition("filesize",	"Which file size should I use (in byte)?", "1048576"),
	InputDefinition("sleep",	"How long should I sleep between each iteration (in us)?", "5500000"),
	InputDefinition("iterations","How often should I repeat it?", "20"),
	InputDefinition("command",	"Is there a command I should execute between each iteration?")
};

int main(int argc, char *args[])
{
	cout<<"This program writes random files to a folder, waits a given delay and checks if the files have changed or not."<<endl;
	cout<<"Please make sure that the desired disk is mounted before executing the test"<<endl;
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

bool writeFile(const string &folder, unsigned long counter, const vector<char> &data)
{
	string filename = td::concat(folder, "/testfile", counter);

	fstream file(filename, std::ios_base::binary | std::ios_base::out);
	file.write(&data[0], data.size());

	return file;
}

bool readFile(const string &folder, unsigned long counter, vector<char> &data)
{
	string filename = td::concat(folder, "/testfile", counter);

	fstream file(filename, std::ios_base::binary | std::ios_base::in);
	file.read(&data[0], data.size());

	return file;
}

void clearFiles(const string &folder, unsigned long counter)
{
	for(unsigned long i = 0; i <= counter; ++i)
	{
		string filename = td::concat(folder, "/testfile", i);
		unlink(filename.c_str());
	}
}

void doTests()
{
	string folder = getValue<string>(inputs, "folder");
	unsigned long fileSize = getValue<unsigned long>(inputs, "filesize");
	unsigned long sleepTime = getValue<unsigned long>(inputs, "sleep");
	unsigned int iterations = getValue<unsigned int>(inputs, "iterations");
	string command = getValue<string>(inputs, "command");

	unsigned long counter = 0;
	vector<char> data(fileSize);
	vector<char> data2(fileSize);

	for(unsigned long i = 0; i < iterations; ++i)
	{
		generateRandomData(data);

		if(!writeFile(folder, counter, data))
		{
			clearFiles(folder, counter);
			counter = 0;
			i--;
			continue;
		}

		usleep(sleepTime);

		if(!readFile(folder, counter, data2))
			throw InputException("Unable to read file ", counter);

		for(size_t j = 0; j < data.size(); ++j)
		{
			if(data[j] != data2[j])
				throw InputException("Data is not identical");
		}

		if(command != "")system(command.c_str());

		counter++;
	}
}

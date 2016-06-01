#include <iostream>
#include <string>

#include <blackholetdisk.hpp>

using std::cerr;
using std::cout;
using std::endl;
using std::string;
using std::vector;

using namespace td;

int main(int argc, char *args[])
{
	BlackholeTDisk api(1024LLU * 1024 * 1024 * 1024);
	api.registerInKernel();
	api.listen();

	return 0;
}

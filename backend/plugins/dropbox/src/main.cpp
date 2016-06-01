#include <iostream>
#include <string>

#include <curldefinitions.hpp>
#include <dropboxtdisk.hpp>
#include <dropboxexception.hpp>

using std::cerr;
using std::cout;
using std::endl;
using std::string;
using std::vector;

using namespace td;

int main(int argc, char *args[])
{
	void initCurl();

	try{
		DropboxTDisk api("oauth_token_secret=d7z7oc0i6ueomsi&oauth_token=yhqzektts3cithe6&uid=24443477", "tdisk1/", 163840, /*3ULL*1024*1024*1024*/249280325320400);
		//api.reserveSpace();
		api.registerInKernel();
		api.listen();
		//api.isEnoughSpaceAvailable();
		//vector<char> buffer(32768-152, 'a');
		//api.write(152, &buffer[0], buffer.size());
		//buffer = vector<char>(50, 'b');
		//api.write(4234596, &buffer[0], buffer.size());
		//buffer = vector<char>(60);
		//api.read(4234566, &buffer[0], buffer.size());
		//api.listen();
	} catch(const DropboxException &e) {
		cerr<<"Error: "<<e.what<<endl;
	}

	cleanupCurl();
	return 0;
}

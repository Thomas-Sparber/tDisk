#include <iostream>
#include <string>

#include <curldefinitions.hpp>
#include <dropbox.hpp>
#include <dropboxexception.hpp>

using std::cerr;
using std::cout;
using std::endl;

int main(int argc, char *args[])
{
	void initCurl();

	try{
		Dropbox api("oauth_token_secret=d7z7oc0i6ueomsi&oauth_token=yhqzektts3cithe6&uid=24443477");

		//cout<<api.getAccountInfo()<<endl;
		//cout<<api.downloadFile("App/2DO hispotme.txt")<<endl;
		api.uploadFile("test.txt", "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa");
		api.uploadFile("test.txt", "bbbbb", 50);
		//cout<<api.downloadFile("test.txt")<<endl;
	} catch(const DropboxException &e) {
		cerr<<"Error: "<<e.what<<endl;
	}

	cleanupCurl();
	return 0;
}

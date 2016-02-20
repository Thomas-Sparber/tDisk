#include <iostream>
#include <string>

#include <curldefinitions.hpp>
#include <dropbox.hpp>

using std::cerr;
using std::cout;
using std::endl;

int main(int argc, char *args[])
{
	void initCurl();

	Dropbox api("oauth_token_secret=d7z7oc0i6ueomsi&oauth_token=yhqzektts3cithe6&uid=24443477");

	//cout<<api.getAccountInfo()<<endl;
	cout<<api.downloadFile("App/2DO hispotme.txt")<<endl;

	cleanupCurl();
	return 0;
}

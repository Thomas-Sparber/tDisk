#ifndef DROPBOX_HPP
#define DROPBOX_HPP

#include <string>
#include <sstream>

#include <accountinfo.hpp>
#include <liboauthcpp/liboauthcpp.h>

class Dropbox
{

public:
	Dropbox(const std::string &accessToken="");


	bool isAuthenticated() const
	{
		return !accessToken.empty();
	}

	AccountInfo getAccountInfo() const;

	std::string downloadFile(const std::string &path, long long start=0, std::size_t length=0) const;

private:
	std::string loadOAuthToken();

	std::string getDropboxSite(const std::string &site, long long start=0, std::size_t length=0) const;

private:
	OAuth::Consumer consumer;
	OAuth::Token token;
	mutable OAuth::Client oauth;
	std::string accessToken;

}; //end class Dropbox

#endif //DROPBOX_HPP

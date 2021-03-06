#ifndef DROPBOX_HPP
#define DROPBOX_HPP

#include <string>
#include <sstream>

#include <accountinfo.hpp>
#include <filemetadata.hpp>
#include <liboauthcpp/liboauthcpp.h>
#include <linkmetadata.hpp>

namespace td
{

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

	FileMetadata uploadFile(const std::string &path, const std::string &content);

	FileMetadata uploadFile(const std::string &path, const std::string &content, long long offset);

	FileMetadata getMetadata(const std::string &path, bool list=true, bool mediaInfo=false, unsigned int fileLimit=10000) const;

	//LinkMetadata getLink(const std::string &link, bool list=true, bool mediaInfo=false, unsigned int fileLimit=10000) const;

private:
	std::string loadOAuthToken();

	std::string getDropboxSite(const std::string &site, const std::string &getData="", long long start=0, std::size_t length=0) const;

	std::string postDropboxSite(const std::string &site, const std::string &data) const;

	std::string putDropboxSite(const std::string &site, const std::string &queryParams, const std::string &data) const;

	void handleError(long code, const std::string &response) const;

private:
	OAuth::Consumer consumer;
	OAuth::Token token;
	mutable OAuth::Client oauth;
	std::string accessToken;

}; //end class Dropbox

} //end namespace td

#endif //DROPBOX_HPP

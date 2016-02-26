#include <iostream>
#include <json/json.h>
#include <vector>

#include <chunkedupload.hpp>
#include <curldefinitions.hpp>
#include <dropbox.hpp>
#include <dropboxexception.hpp>

using std::cerr;
using std::cin;
using std::cout;
using std::endl;
using std::string;
using std::vector;

using namespace td;

const string consumerKey("6k27q1pie9lcdm0");
const string consumerSecret("3kbyrif1dpppoo7");

const string oauth_url("https://api.dropboxapi.com/1/oauth/request_token");
const string authorize_url("https://www.dropbox.com/1/oauth/authorize");
const string access_token_url("https://api.dropboxapi.com/1/oauth/access_token");

Dropbox::Dropbox(const std::string &str_accessToken) :
	Plugin("dropbox", false),
	consumer(consumerKey, consumerSecret),
	token("", ""),
	oauth(nullptr),
	accessToken(str_accessToken)
{
	oauth = OAuth::Client(&consumer);
	//OAuth::SetLogLevel(OAuth::LogLevelDebug);

	if(accessToken == "")
	{
		string oauthToken = loadOAuthToken();
		token = OAuth::Token::extract(oauthToken);

		string authorize = authorize_url + "?oauth_token=" + token.key();
		cout<<authorize<<endl;
		cin>>authorize;

		oauth = OAuth::Client(&consumer, &token);

		string oAuthQueryString = oauth.getURLQueryString(OAuth::Http::Get, access_token_url, "", true);
		cout<<access_token_url<<"?"<<oAuthQueryString<<endl; 
		cin>>accessToken;

		token = OAuth::Token::extract(accessToken);
		oauth = OAuth::Client(&consumer, &token);
	}
	else
	{
		token = OAuth::Token::extract(accessToken);
		oauth = OAuth::Client(&consumer, &token);
	}
}

string Dropbox::loadOAuthToken()
{
	std::string oAuthQueryString = oauth.getURLQueryString(OAuth::Http::Post, oauth_url); 

	long code;
	string contentType;
	string oauthToken = getSite(oauth_url, oAuthQueryString, code, contentType);
	handleError(code, oauthToken);

	return oauthToken;
}

string Dropbox::getDropboxSite(const string &site, long long start, std::size_t length) const
{
	string oAuthQueryString = oauth.getURLQueryString(OAuth::Http::Get, site);

	vector<string> header;
	if(start != 0 && length != 0)
	{
		header.push_back(concat("Range: bytes=",start,"-",start+length));
	}

	long code;
	string contentType;
	string result = getSite(site+"?"+oAuthQueryString, header, code, contentType);
	handleError(code, result);
	return result;
}

string Dropbox::postDropboxSite(const string &site, const string &postData) const
{
	string q = site + (postData.empty() ? "" : "?"+postData);
	string oAuthQueryString = oauth.getURLQueryString(OAuth::Http::Post, q);

	cout<<oAuthQueryString<<endl;

	long code;
	string contentType;
	string result = getSite(site, oAuthQueryString, code, contentType);
	handleError(code, result);
	return result;
}

string Dropbox::putDropboxSite(const string &site, const std::string &getParams, const string &data) const
{
	string q = site + (getParams.empty() ? "" : "?"+getParams);
	string oAuthQueryString = oauth.getURLQueryString(OAuth::Http::Put, site);

	long code;
	string contentType;
	string result = uploadSite(site+"?"+oAuthQueryString, vector<string>(), "", data, code, contentType);
	handleError(code, result);
	return result;
}

void Dropbox::handleError(long code, const string &response) const
{
	switch(code)
	{
	case 200: return;	//Everything is fine...
	case 400: throw DropboxException("Bad input parameter: ", response);
	case 401: throw DropboxException("Bad or expired token: ", response);
	case 403: throw DropboxException("Bad OAuth request: ", response);
	case 404: throw DropboxException("File or folder not found at the specified path: ", response);
	case 405: throw DropboxException("Request method not expected: ", response);
	case 406: throw DropboxException("Too many file entries to return: ", response);
	case 409: throw DropboxException("Conflict occurred: ", response);
	case 411: throw DropboxException("MIssing content length header: ", response);
	case 429: throw DropboxException("tDisk is making too many requests and is now rate limited: ", response);
	case 503: throw DropboxException("tDisk is rate limited or transient server error: ", response);
	case 507: throw DropboxException("User is over storage quota: ", response);
	default: throw DropboxException("Unknown dropbox error (",code,"): ", response);
	}
}

AccountInfo Dropbox::getAccountInfo() const
{
	Json::Value root;
	Json::Reader reader;
	string response = getDropboxSite("https://api.dropboxapi.com/1/account/info");

	if(!reader.parse(response, root) || root.type() != Json::objectValue)
		throw DropboxException("Invalid json object for retrieving account info");

	AccountInfo info(root);
	return std::move(info);
}

string Dropbox::downloadFile(const string &path, long long start, std::size_t length) const
{
	return getDropboxSite(string("https://content.dropboxapi.com/1/files/auto/")+encodeURI(path, false), start, length);
}

FileMetadata Dropbox::uploadFile(const string &path, const string &content)
{
	Json::Value root;
	Json::Reader reader;
	string response = putDropboxSite(string("https://content.dropboxapi.com/1/files_put/auto/")+encodeURI(path, false), "", content);

	if(!reader.parse(response, root) || root.type() != Json::objectValue)
		throw DropboxException("Invalid json object for a file metadata");

	FileMetadata file(root);
	return std::move(file);
}

FileMetadata Dropbox::uploadFile(const string &path, const string &content, long long offset)
{
	Json::Value root;
	Json::Reader reader;
	string response = putDropboxSite(string("https://content.dropboxapi.com/1/chunked_upload"), concat("offset=", offset), content);

	if(!reader.parse(response, root) || root.type() != Json::objectValue)
		throw DropboxException("Invalid json object for a chunked upload");

	ChunkedUpload upload(root);
	root = Json::Value();

	response = postDropboxSite(string("https://content.dropboxapi.com/1/commit_chunked_upload/auto/")+encodeURI(path, false), concat("upload_id=", upload.upload_id));

	if(!reader.parse(response, root) || root.type() != Json::objectValue)
		throw DropboxException("Invalid json object for a file metadata");

	FileMetadata file(root);
	return std::move(file);
}

FileMetadata Dropbox::getMetadata(const std::string &path) const
{
	Json::Value root;
	Json::Reader reader;
	string response = getDropboxSite(string("https://api.dropboxapi.com/1/metadata/auto/")+encodeURI(path, false));

	if(!reader.parse(response, root) || root.type() != Json::objectValue)
		throw DropboxException("Invalid json object for a file metadata");

	FileMetadata file(root);
	return std::move(file);
}

/*LinkMetadata Dropbox::getLink(const std::string &link, bool list, bool mediaInfo, unsigned int fileLimit) const
{
	Json::Value root;
	Json::Reader reader;
	string response = postDropboxSite(string("https://api.dropbox.com/1/metadata/link"),
		concat("link=",link,
				"&list=",(list?"true":"false"),
				"&include_media_info=",(mediaInfo?"true":"false"),
				"&file_limit=",fileLimit));

	if(!reader.parse(response, root) || root.type() != Json::objectValue)
		throw DropboxException("Invalid json object for a link metadata");

	LinkMetadata l(root);
	return std::move(l);
}*/

bool Dropbox::read(unsigned long long offset, vector<char> &data, std::size_t length) const
{

}

bool Dropbox::write(unsigned long long offset, const vector<char> &data, std::size_t length)
{

}

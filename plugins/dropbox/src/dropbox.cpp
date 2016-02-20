#include <iostream>
#include <json/json.h>
#include <vector>

#include <curldefinitions.hpp>
#include <dropbox.hpp>
#include <dropboxexception.hpp>

using std::cerr;
using std::cin;
using std::cout;
using std::endl;
using std::string;
using std::vector;

const string consumerKey("6k27q1pie9lcdm0");
const string consumerSecret("3kbyrif1dpppoo7");

const string oauth_url("https://api.dropboxapi.com/1/oauth/request_token");
const string authorize_url("https://www.dropbox.com/1/oauth/authorize");
const string access_token_url("https://api.dropboxapi.com/1/oauth/access_token");

Dropbox::Dropbox(const std::string &str_accessToken) :
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

string Dropbox::postDropboxSite(const string &site, const string &data, long long offset) const
{
	string oAuthQueryString = oauth.getURLQueryString(OAuth::Http::Post, site);

	vector<string> header;
	if(offset != 0)
	{
		header.push_back(concat("Range: bytes=",offset,"-"));
	}

	long code;
	string contentType;
	string result = getSite(site+"?"+oAuthQueryString, header, data, code, contentType);
	handleError(code, result);
	return result;
}

string Dropbox::putDropboxSite(const string &site, const std::string &queryParams, const string &data) const
{
	string q = site + (queryParams.empty() ? "" : "?"+queryParams);
	string oAuthQueryString = oauth.getURLQueryString(OAuth::Http::Put, site);

	cout<<site<<"?"<<oAuthQueryString<<endl;

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

void Dropbox::uploadFile(const string &path, const string &content, long long offset)
{
	//Json::Value root;
	//Json::Reader reader;
	string response = putDropboxSite(string("https://content.dropboxapi.com/1/chunked_upload"), concat("offset=", offset), content);

	//if(!reader.parse(response, root) || root.type() != Json::objectValue)
	//	throw DropboxException("Invalid json object for a file metadata");

	cout<<response<<endl;
	//return FileMetadata;
}

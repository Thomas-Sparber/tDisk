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

	if(code != 200)
	{
		cerr<<"Invalid response!"<<endl;
		cerr<<"Content: "<<oauthToken<<endl;
		cerr<<"Code: "<<code<<endl;
		cerr<<"ContentType: "<<contentType<<endl;
	}

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
	return result;
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

string Dropbox::downloadFile(const std::string &path, long long start, std::size_t length) const
{
	return getDropboxSite(string("https://content.dropboxapi.com/1/files/auto/")+encodeURI(path, false), start, length);
}

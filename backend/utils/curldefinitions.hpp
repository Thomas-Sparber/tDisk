#ifndef CURLDEFINITIONS_HPP
#define CURLDEFINITIONS_HPP

#include <curl/curl.h>
#include <string>
#include <string.h>
#include <vector>

/**
  * This function should be called by the main
  * thread to initialize cURL.
 **/
inline void initCurl()
{
	curl_global_init(CURL_GLOBAL_ALL);
}

/**
  * This function should be called by the main thread
  * to clean up cURL
 **/
inline void cleanupCurl()
{
	curl_global_cleanup();
}

inline std::string encodeURI(const std::string &path, bool replaceSlash=true)
{
	static CURL *curl_handle = curl_easy_init();
	char *str = curl_easy_escape(curl_handle, path.c_str(), (int)path.length());
	std::string ret(str);
	curl_free(str);

	if(!replaceSlash)
	{
		std::size_t pos;
		while((pos=ret.find("%2F")) != ret.npos)
		{
			ret = ret.replace(pos, 3, "/");
		}
	}
	//curl_easy_cleanup(curl_handle);
	return std::move(ret);
}

/**
  * A cURL callback to write the data to the momory
 **/
inline size_t writeMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
	std::string &str = (*(std::string*)userp);
	const char *chars = static_cast<char*>(contents);
	str.append(chars, size*nmemb);
	return size*nmemb;
}

/**
  * A cURL callback to upload data
 **/
inline size_t readMemoryCallback(void *buffer, size_t size, size_t nmemb, void *userp)
{
	std::string &str = (*(std::string*)userp);
	std::size_t currentSize = std::min(size*nmemb, str.size());
	memcpy(buffer, str.c_str(), currentSize);
	str = str.substr(currentSize);
	return currentSize;
}

/**
  * The main function to download websites.
  * It takes the website as string, a long reference to store the result code,
  * a string reference to store the content type, headers,
  * an optional httppost argument and an optional flag wether to use
  * the tor network
 **/
inline void getSite(const std::string &site,
			long &code,
			std::string &contentType,
			const std::vector<std::string> &header,
			const std::string &formpost,
			size_t (*downloadCallback)(void*, size_t, size_t, void*),
			void *downloadCallbackObject,
			size_t (*uploadCallback)(void*, size_t, size_t, void*)=nullptr,
			void *uploadCallbackObject=nullptr,
			unsigned long long upload_size=0)
{
	CURL *curl_handle = curl_easy_init();

	//Prepare  and addheaders
	struct curl_slist *headers = nullptr;
	for(unsigned int i = 0; i < header.size(); ++i)
		headers = curl_slist_append(headers, header[i].c_str());
	if(headers)curl_easy_setopt(curl_handle, CURLOPT_HTTPHEADER, headers);

	//Add post form
	if(!formpost.empty())curl_easy_setopt(curl_handle, CURLOPT_POSTFIELDS, formpost.c_str());
	else curl_easy_setopt(curl_handle, CURLOPT_HTTPGET, 1);	//This forces to be a GET request

	//URL
	curl_easy_setopt(curl_handle, CURLOPT_URL, site.c_str());

	//Write memory function
	curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, downloadCallback);
	curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, downloadCallbackObject);

	//Upload memory function
	if(uploadCallback && uploadCallbackObject)
	{
		curl_easy_setopt(curl_handle, CURLOPT_READFUNCTION, uploadCallback);
		curl_easy_setopt(curl_handle, CURLOPT_READDATA, uploadCallbackObject);
		curl_easy_setopt(curl_handle, CURLOPT_UPLOAD, 1L);
		curl_easy_setopt(curl_handle, CURLOPT_INFILESIZE_LARGE, (curl_off_t)upload_size);
	}

	//User agent
	curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, "tDisk");

	curl_easy_setopt(curl_handle, CURLOPT_FOLLOWLOCATION, true);		//Redirections
	curl_easy_setopt(curl_handle, CURLOPT_MAXREDIRS, 10);				//Avoid looping redirections
	curl_easy_setopt(curl_handle, CURLOPT_NOSIGNAL, 1);

	//If the speed is below 100 byte/sec for more than 60 seconds
	//then the connection is closed
	curl_easy_setopt(curl_handle, CURLOPT_LOW_SPEED_LIMIT, 100);
	curl_easy_setopt(curl_handle, CURLOPT_LOW_SPEED_TIME, 20);

	//This actually downloads the site
	curl_easy_perform(curl_handle);

	//Response code and error checking
	curl_easy_getinfo(curl_handle, CURLINFO_RESPONSE_CODE, &code);

	//Get content type
	char *ct = nullptr;
	curl_easy_getinfo(curl_handle, CURLINFO_CONTENT_TYPE, &ct);
	if(ct != nullptr)contentType = ct;

	if(headers)curl_slist_free_all(headers);
	curl_easy_cleanup(curl_handle);
}

/**
  * Downloads the given site using acustom
  * callback function. All the data is then sent
  * to this function
 **/
inline void getSite(const std::string &site,
			long &code,
			std::string &contentType,
			const std::vector<std::string> &header,
			size_t (*callback)(void*, size_t, size_t, void*),
			void *callbackObject)
{
	//Default empty values
	const std::string emptyPost;

	getSite(site, code, contentType, header, emptyPost, callback, callbackObject);
}

/**
  * The standard download function
 **/
inline std::string getSite(const std::string &site,
			long &code,
			std::string &contentType)
{
	//The dataBuffer is sent to the callback function
	//which fills it with the downloaded data.
	std::string dataBuffer;

	//Empty default values
	const std::vector<std::string> emptyHeader;
	const std::string emptyPost;

	getSite(site, code, contentType, emptyHeader, emptyPost, &writeMemoryCallback, (void*)&dataBuffer);

	return dataBuffer;
}

/**
  * The standard download function
 **/
inline std::string getSite(const std::string &site,
			const std::vector<std::string> &header,
			long &code,
			std::string &contentType)
{
	//The dataBuffer is sent to the callback function
	//which fills it with the downloaded data.
	std::string dataBuffer;

	//Empty default values
	const std::string emptyPost;

	getSite(site, code, contentType, header, emptyPost, &writeMemoryCallback, (void*)&dataBuffer);

	return dataBuffer;
}

/**
  * The standard download function
 **/
inline std::string getSite(const std::string &site,
			const std::string &post,
			long &code,
			std::string &contentType)
{
	//The dataBuffer is sent to the callback function
	//which fills it with the downloaded data.
	std::string dataBuffer;

	//Empty default values
	const std::vector<std::string> emptyHeader;

	getSite(site, code, contentType, emptyHeader, post, &writeMemoryCallback, (void*)&dataBuffer);

	return dataBuffer;
}

/**
  * The standard download function
 **/
inline std::string getSite(const std::string &site,
			const std::vector<std::string> &header,
			const std::string &post,
			long &code,
			std::string &contentType)
{
	//The dataBuffer is sent to the callback function
	//which fills it with the downloaded data.
	std::string dataBuffer;

	getSite(site, code, contentType, header, post, &writeMemoryCallback, (void*)&dataBuffer);

	return dataBuffer;
}

/**
  * The standard download function
 **/
inline std::string uploadSite(const std::string &site,
			const std::vector<std::string> &header,
			const std::string &post,
			const std::string &content,
			long &code,
			std::string &contentType)
{
	//The dataBuffer is sent to the callback function
	//which fills it with the downloaded data.
	std::string dataBuffer;


	std::string toUpload(content);

	getSite(site, code, contentType, header, post, &writeMemoryCallback, (void*)&dataBuffer, &readMemoryCallback, (void*)&toUpload, toUpload.size());

	return dataBuffer;
}

#endif // CURLDEFINITIONS_HPP

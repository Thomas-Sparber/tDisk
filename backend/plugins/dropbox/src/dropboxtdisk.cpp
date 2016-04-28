#include <iostream>
#include <list>
#include <string>
#include <string.h>

#include <dropboxtdisk.hpp>

using std::cerr;
using std::cout;
using std::endl;
using std::list;
using std::min;
using std::string;
using std::vector;

using namespace td;

inline string getFileName(const string &tDiskPath, unsigned int i)
{
	return utils::concat("/",tDiskPath,"/",i,".td");
}

DropboxTDisk::DropboxTDisk(const string &str_tDiskPath, unsigned int ui_blocksize, unsigned long long llu_size, unsigned int maxHistoryBuffer) :
	Plugin(),
	Dropbox(),
	user(),
	tDiskPath(str_tDiskPath),
	blocksize(ui_blocksize),
	size(llu_size),
	dropboxHistory(maxHistoryBuffer)
{
	if(!tDiskPath.empty() && tDiskPath[0] == '/')tDiskPath = tDiskPath.substr(1);
	if(!tDiskPath.empty() && tDiskPath[tDiskPath.length()-1] == '/')tDiskPath = tDiskPath.substr(0, tDiskPath.length()-1);
}

DropboxTDisk::DropboxTDisk(const string &str_accessToken, const string &str_tDiskPath, unsigned int ui_blocksize, unsigned long long llu_size, unsigned int maxHistoryBuffer) :
	Plugin("", ui_blocksize, 5),
	Dropbox(str_accessToken),
	user(),
	tDiskPath(str_tDiskPath),
	blocksize(ui_blocksize),
	size(llu_size),
	dropboxHistory(maxHistoryBuffer)
{
	if(!tDiskPath.empty() && tDiskPath[0] == '/')tDiskPath = tDiskPath.substr(1);
	if(!tDiskPath.empty() && tDiskPath[tDiskPath.length()-1] == '/')tDiskPath = tDiskPath.substr(0, tDiskPath.length()-1);
	user = loadDropboxUser();
	Plugin::setName(utils::concat("dropbox_",user,"_",tDiskPath));
}

string DropboxTDisk::loadDropboxUser() const
{
	const AccountInfo info = Dropbox::getAccountInfo();
	return std::move(info.email);
}

bool DropboxTDisk::internalRead(unsigned long long offset, char *data, std::size_t length, bool saveHistory) const
{
	if(dropboxHistory.get(offset, data, length))return 2;

	while(length > 0)
	{
		unsigned long long file = offset / blocksize;
		std::size_t fileOffset = std::size_t(offset % blocksize);
		std::size_t currentLength = min(length, blocksize-fileOffset);

		try {
			if(!dropboxHistory.get(offset, data, currentLength))
			{
				cout<<"Read request: "<<offset<<" - "<<offset+length<<endl;

				//string result;
				const string fileName = getFileName(tDiskPath, unsigned(file));
				//if(fileOffset == 0 && currentLength == blocksize)
				const string result = downloadFile(fileName);
				//else
				//	result = downloadFile(fileName, fileOffset, currentLength);

				if(result.length() != blocksize)
				{
					cerr<<"Warning: downloaded file length is not as expected: "<<result.length()<<"/"<<blocksize<<endl;
					return false;
				}
				if(saveHistory)dropboxHistory.set(file*blocksize, result.c_str(), blocksize);
				memcpy(data, result.c_str()+fileOffset, min(result.length()-fileOffset, currentLength));
			}
		} catch(const DropboxException &e) {
			//cerr<<"Error reading data "<<offset<<" - "<<offset+length<<": "<<e.what<<endl;
			//return false;

			if(saveHistory)
			{
				//Save "fake" history
				vector<char> fakeData(blocksize, 0);
				dropboxHistory.set(file*blocksize, &fakeData[0], blocksize);
			}

			for(std::size_t i = 0; i < currentLength; ++i)
				data[i] = 0;
		}

		length -= currentLength;
		data += currentLength;
		offset += currentLength;
	}
	return true;
}

bool DropboxTDisk::read(unsigned long long offset, char *data, std::size_t length) const
{
	return internalRead(offset, data, length, true);
}

bool DropboxTDisk::internalWrite(unsigned long long offset, const char *data, std::size_t length)
{
	try {
		vector<char> buffer(blocksize);

		while(length > 0)
		{
			unsigned long long file = offset / blocksize;
			std::size_t fileOffset = std::size_t(offset % blocksize);
			std::size_t currentLength = min(length, blocksize-fileOffset);

			const string fileName = getFileName(tDiskPath, unsigned(file));
			if(fileOffset == 0 && currentLength == blocksize)
			{
				uploadFile(fileName, string(data, currentLength));
			}
			else
			{
				internalRead(file*blocksize, &buffer[0], blocksize, false);
				string result(&buffer[0], buffer.size());// = downloadFile(fileName);
				if(result.length() != blocksize)
				{
					cerr<<"Warning: downloaded file length is not as expected: "<<result.length()<<"/"<<blocksize<<endl;
					return false;
				}
				result.replace(fileOffset, currentLength, data, currentLength);
				dropboxHistory.set(file*blocksize, result.c_str(), blocksize);
				uploadFile(fileName, result);
			}

			length -= currentLength;
			data += currentLength;
			offset += currentLength;
		}
		return true;
	} catch(const DropboxException &e) {
		cerr<<"Error reading data "<<offset<<" - "<<offset+length<<": "<<e.what<<endl;
		return false;
	}
}

bool DropboxTDisk::write(unsigned long long offset, const char *data, std::size_t length)
{
	cout<<"Write request: "<<offset<<" - "<<offset+length<<endl;

	return internalWrite(offset, data, length);
}

bool DropboxTDisk::isEnoughSpaceAvailable() const
{
	const AccountInfo info = Dropbox::getAccountInfo();

	unsigned long long requiredSize = size;
	unsigned long long available = info.quota_info.quota - info.quota_info.normal;

	try {
		const FileMetadata data = getMetadata(tDiskPath);
		for(const FileMetadata &file : data.contents)
		{
			if(file.bytes > requiredSize)
			{
				//We have more files allocated than we need...
				requiredSize = 0;
				break;
			}
			requiredSize -= file.bytes;
		}
	} catch(const DropboxException &) {
		//Folder does not exist yet. Ignoring it...
	}

	cout<<"Available: "<<available<<", Required: "<<requiredSize<<"/"<<size<<endl;
	return available >= requiredSize;
}

void DropboxTDisk::reserveSpace()
{
	string buffer(blocksize, 0);
	list<FileMetadata> existing;

	try {
		const FileMetadata data = getMetadata(tDiskPath);
		existing.insert(existing.end(), data.contents.begin(), data.contents.end());
	} catch(const DropboxException &e) {
		//Folder does not exist yet. Ignoring it...
	}

	for(unsigned int i = 0; (unsigned long long)i*blocksize < size; ++i)
	{
		const string path = getFileName(tDiskPath, i);

		bool exists = false;
		for(auto it = existing.begin(); it != existing.end(); ++it)
		{
			if(it->path == path && it->bytes == blocksize)
			{
				existing.erase(it);
				exists = true;
				break;
			}
		}
		if(exists)continue;

		cout<<"Creating file "<<path<<" ("<<buffer.size()<<")"<<endl;
		uploadFile(path, buffer);
	}
}

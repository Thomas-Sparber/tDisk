#ifndef DROPBOXTDISK_HPP
#define DROPBOXTDISK_HPP

#include <iostream>

#include <dropbox.hpp>
#include <historybuffer.hpp>
#include <plugin.hpp>

namespace td
{

class DropboxTDisk : public Plugin, protected Dropbox
{

public:
	DropboxTDisk(const std::string &tDiskPath, unsigned int blocksize, unsigned long long size, unsigned int maxHistoryBuffer=10);

	DropboxTDisk(const std::string &accessToken, const std::string &tDiskPath, unsigned int blocksize, unsigned long long size, unsigned int maxHistoryBuffer=10);

	virtual bool read(unsigned long long offset, char *data, std::size_t length) const;

	virtual bool write(unsigned long long offset, const char *data, std::size_t length);

	virtual unsigned long long getSize() const
	{
		std::cout<<"Size request: "<<size<<std::endl;
		return size;
	}

	bool isEnoughSpaceAvailable() const;

	void reserveSpace();

protected:
	std::string loadDropboxUser() const;

	bool internalRead(unsigned long long offset, char *data, std::size_t length, bool saveHistory) const;

	bool internalWrite(unsigned long long offset, const char *data, std::size_t length);

private:
	std::string user;
	std::string tDiskPath;
	unsigned int blocksize;
	unsigned long long size;
	mutable HistoryBuffer dropboxHistory;

}; //end class DropboxTDisk	

} //end namespace td

#endif //DROPBOXTDISK_HPP

#ifndef DROPBOXTDISK_HPP
#define DROPBOXTDISK_HPP

#include <dropbox.hpp>
#include <plugin.hpp>

namespace td
{

class DropboxTDisk : public Plugin, protected Dropbox
{

public:
	DropboxTDisk(const std::string &tDiskPath, unsigned int blocksize, unsigned long long size);

	DropboxTDisk(const std::string &accessToken, const std::string &tDiskPath, unsigned int blocksize, unsigned long long size);

	virtual bool read(unsigned long long offset, char *data, std::size_t length) const;

	virtual bool write(unsigned long long offset, const char *data, std::size_t length);

	virtual unsigned long long getSize() const
	{
		return size;
	}

	bool isEnoughSpaceAvailable() const;

	void reserveSpace();

private:
	std::string loadDropboxUser() const;

private:
	std::string user;
	std::string tDiskPath;
	unsigned int blocksize;
	unsigned long long size;

}; //end class DropboxTDisk	

} //end namespace td

#endif //DROPBOXTDISK_HPP

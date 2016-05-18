/**
  *
  * tDisk backend
  * @author Thomas Sparber (2015-2016)
  *
 **/
 
#ifdef __linux__

#ifndef EXTINODESCAN_HPP
#define EXTINODESCAN_HPP

#include <ext2fs/ext2_fs.h>
#include <ext2fs/ext2fs.h>

#include <extinode.hpp>
#include <inodescan.hpp>

namespace td
{

class ExtInodeScan : public InodeScan
{

public:
	ExtInodeScan(const std::string &path);

	ExtInodeScan(const ExtInodeScan &other) = delete;

	ExtInodeScan(ExtInodeScan &&other) :
		InodeScan(other),
		fs(other.fs),
		scan(other.scan)
	{
		other.fs = nullptr;
		other.scan = nullptr;
	}

	ExtInodeScan& operator=(const ExtInodeScan &other) = delete;

	ExtInodeScan& operator=(ExtInodeScan &&other);

	virtual ~ExtInodeScan();

	virtual bool valid() const;

	virtual Inode* getInitialInode() const
	{
		return new ExtInode(fs);
	}

	virtual bool nextInode(Inode *iterator) const;

	virtual unsigned int getBlocksize() const;

private:
	ext2_filsys fs;
	ext2_inode_scan scan;

}; //end class ExtInodeScan

} //end namespace td

#endif //EXTINODESCAN_HPP

#endif //__linux__
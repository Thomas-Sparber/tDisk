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

/**
  * This class is used to iterate over all inodes
  * of a ext-family filesystem
 **/
class ExtInodeScan : public InodeScan
{

public:

	/**
	  * Opens a filesystem located on the device/file
	  * with the given path
	 **/
	ExtInodeScan(const std::string &path);

	/**
	  * Copying an InodeScan is illegal
	 **/
	ExtInodeScan(const ExtInodeScan &other) = delete;

	/**
	  * Move constructor
	 **/
	ExtInodeScan(ExtInodeScan &&other) :
		InodeScan(other),
		fs(other.fs),
		scan(other.scan)
	{
		other.fs = nullptr;
		other.scan = nullptr;
	}

	/**
	  * Copy-assigning is illegal
	 **/
	ExtInodeScan& operator=(const ExtInodeScan &other) = delete;

	/**
	  * Move assignment operator
	 **/
	ExtInodeScan& operator=(ExtInodeScan &&other);

	/**
	  * Closes the ext filesystem
	 **/
	virtual ~ExtInodeScan();

	/**
	  * Checks whether the inodescan was opened successfully
	 **/
	virtual bool valid() const;

	/**
	  * Returns the first inode on the filesystem
	 **/
	virtual Inode* getInitialInode() const
	{
		return new ExtInode(fs);
	}

	/**
	  * Iterates forwards
	 **/
	virtual bool nextInode(Inode *iterator) const;

	/**
	  * Gets the blocksize of the filesystem
	 **/
	virtual unsigned int getBlocksize() const;

	virtual void reset();

private:

	/**
	  * The handle to the ext filesystem
	 **/
	ext2_filsys fs;

	/**
	  * The ext scan object
	 **/
	ext2_inode_scan scan;

}; //end class ExtInodeScan

} //end namespace td

#endif //EXTINODESCAN_HPP

#endif //__linux__

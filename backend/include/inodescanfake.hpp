/**
  *
  * tDisk backend
  * @author Thomas Sparber (2015-2016)
  *
 **/
 
#ifndef __linux__

#ifndef INODESCANFAKE_HPP
#define INODESCANFAKE_HPP

#include <time.h>

#include <inodefake.hpp>
#include <inodescan.hpp>

namespace td
{

/**
  * This class is used to iterate over all inodes
  * of a fake filesystem (random data)
 **/
class InodeScanFake : public InodeScan
{

public:

	/**
	  * Opens a filesystem located on the device/file
	  * with the given path
	 **/
	InodeScanFake(const std::string &str_path) :
		InodeScan(str_path)
	{
		srand((unsigned)time(nullptr));
	}

	/**
	  * Copying an InodeScan is illegal
	 **/
	InodeScanFake(const InodeScanFake &other) = delete;

	/**
	  * Move constructor
	 **/
	InodeScanFake(InodeScanFake &&other) = default;

	/**
	  * Default destructor
	 **/
	virtual ~InodeScanFake()
	{}

	/**
	  * Copy-assigning is illegal
	 **/
	InodeScanFake& operator=(const InodeScanFake &other) = delete;

	/**
	  * Move assignment operator
	 **/
	InodeScanFake& operator=(InodeScanFake &&other) = default;

	/**
	  * Checks whether the inodescan was opened successfully
	 **/
	virtual bool valid() const
	{
		return true;
	}

	/**
	  * Returns the first inode on the filesystem
	 **/
	virtual Inode* getInitialInode() const
	{
		return new InodeFake();
	}

	/**
	  * Iterates forwards
	 **/
	virtual bool nextInode(Inode */*iterator*/) const
	{
		return ((rand() % 1000) != 0);
	}

	/**
	  * Gets the blocksize of the filesystem
	 **/
	virtual unsigned int getBlocksize() const
	{
		return 512;
	}

	virtual void reset()
	{}

}; //end class InodeScanFake

} //end namespace td

#endif //INODESCANFAKE_HPP

#endif //__linux__

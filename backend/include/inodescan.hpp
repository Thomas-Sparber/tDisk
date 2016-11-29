/**
  *
  * tDisk backend
  * @author Thomas Sparber (2015-2016)
  *
 **/

#ifndef INODESCAN_HPP
#define INODESCAN_HPP

#include <functional>
#include <string>
#include <vector>

#include <inode.hpp>

namespace td
{

/**
  * This class is used to iterate over allinodes
  * of an abstract filesystem
 **/
class InodeScan
{

public:

	/**
	  * Opens a filesystem located on the device/file
	  * with the given path
	 **/
	InodeScan(const std::string &str_path) :
		path(str_path)
	{}

	/**
	  * Default destructor
	 **/
	virtual ~InodeScan() {}

	/**
	  * Checks whether the inodescan was opened successfully
	 **/
	virtual bool valid() const = 0;

	/**
	  * Returns the first inode on the filesystem
	 **/
	virtual Inode* getInitialInode() const = 0;

	/**
	  * Iterates forwards
	 **/
	virtual bool nextInode(Inode *iterator) const = 0;

	/**
	  * Gets the blocksize of the filesystem
	 **/
	virtual unsigned int getBlocksize() const = 0;

	virtual void reset() = 0;

	/**
	  * Returns the path of the filesystem
	 **/
	const std::string& getPath() const
	{
		return path;
	}

	/**
	  * This static function is used for all filesystem
	  * classes to register themselves. This makes it much
	  * easier to select the right InodeScan for a given
	  * filesystem.
	 **/
	template <class T>
	static bool registerInodeScan()
	{
		try {
			registeredInodeScans().push_back([] (const std::string &path)
			{
				return new T(path);
			});
		} catch (...) {
			return false;
		}

		return true;
	}

	/**
	  * This function gets an InodeScan for the filesystem of
	  * the given path
	 **/
	static InodeScan* getInodeScan(const std::string &path)
	{
		for(auto createFunction : registeredInodeScans())
		{
			InodeScan *scan = createFunction(path);
			if(scan->valid())return scan;
			delete scan;
		}

		return nullptr;
	}

private:

	/**
	  * Stores all the create functions which are used
	  * to create the individual InodeScans
	 **/
	static std::vector<std::function<InodeScan*(const std::string&)> >& registeredInodeScans()
	{
		static std::vector<std::function<InodeScan*(const std::string&)> > reg;
		return reg;
	}

	/**
	  * The path of the filesystem
	 **/
	std::string path;

}; //end class InodeScan

} //end namespace td

#endif //INODESCAN_HPP

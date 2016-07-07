/**
  *
  * tDisk backend
  * @author Thomas Sparber (2015-2016)
  *
 **/

#ifndef INODE_HPP
#define INODE_HPP

#include <string>
#include <memory>
#include <vector>

namespace td
{

/**
  * This class represents an inode of an abstract filesystem
 **/
class Inode
{

public:

	/**
	  * Default constructor
	 **/
	Inode() {}

	/**
	  * Default destructor
	 **/
	virtual ~Inode() {}

	/**
	  * Creates a copy of the inode
	 **/
	virtual Inode* clone() = 0;

	/**
	  * Returns the physical block where the inode data is stored
	  * on disk
	 **/
	virtual unsigned long long getInodeBlock() const = 0;

	/**
	  * Returns the physical blocks where the data of the inode
	  * are stored
	 **/
	virtual void getDataBlocks(std::vector<unsigned long long> &out) const = 0;

	/**
	  * Returns the physical blocks where the data of the inode
	  * are stored
	 **/
	std::vector<unsigned long long> getDataBlocks() const
	{
		std::vector<unsigned long long> ret;
		getDataBlocks(ret);
		return std::move(ret);
	}

	/**
	  * Returns the foll path of the inode.
	  * The parent inode is needed to calculate the path
	 **/
	virtual std::string getPath(const Inode *parent) const = 0;

	/**
	  * Returns whether the inode is a directory or not
	 **/
	virtual bool isDirectory() const = 0;

	/**
	  * Checks whether the inode (if directory) contains
	  * the given inode
	 **/
	virtual bool contains(const Inode *inode) const = 0;

	/**
	  * Returns the inode content (if inode is directory)
	 **/
	std::vector<std::unique_ptr<Inode> > getContent() const
	{
		std::vector<std::unique_ptr<Inode> > ret;
		getContent(ret);
		return std::move(ret);
	}

	/**
	  * Returns the inode content (if inode is directory)
	 **/
	virtual void getContent(std::vector<std::unique_ptr<Inode> > &out) const = 0;

}; //end class Inode

} //end namespace td

#endif //INODE_HPP

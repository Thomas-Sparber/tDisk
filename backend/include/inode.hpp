/**
  *
  * tDisk backend
  * @author Thomas Sparber (2015-2016)
  *
 **/

#ifndef INODE_HPP
#define INODE_HPP

#include <string>
#include <vector>

namespace td
{

class Inode
{

public:
	Inode() {}

	virtual ~Inode() {}

	virtual Inode* clone() = 0;

	virtual unsigned long long getInodeBlock() const = 0;

	virtual void getDataBlocks(std::vector<unsigned long long> &out) const = 0;

	std::vector<unsigned long long> getDataBlocks() const
	{
		std::vector<unsigned long long> ret;
		getDataBlocks(ret);
		return std::move(ret);
	}

	virtual std::string getPath(const Inode *parent) const = 0;

	virtual bool isDirectory() const = 0;

	virtual bool contains(const Inode *inode) const = 0;

}; //end class Inode

} //end namespace td

#endif //INODE_HPP

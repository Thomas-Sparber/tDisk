/**
  *
  * tDisk backend
  * @author Thomas Sparber (2015-2016)
  *
 **/

#ifndef __linux__

#ifndef INODEFAKE_HPP
#define INODEFAKE_HPP

#include <inode.hpp>
#include <utils.hpp>

namespace td
{

class InodeFake : public Inode
{

public:
	InodeFake() :
		Inode()
	{}

	virtual ~InodeFake() {}

	virtual Inode* clone()
	{
		return new InodeFake(*this);
	}

	virtual unsigned long long getInodeBlock() const
	{
		return rand() % 1490944;
	}

	virtual void getDataBlocks(std::vector<unsigned long long> &out) const
	{
		unsigned long long start = rand() % 1490944;

		for(int i = 0; i < rand() % 1234; ++i)
			out.push_back(start++);
	}

	virtual std::string getPath(const Inode */*p*/) const
	{
		std::string path;

		for(int i = 0; i < rand() % 5; ++i)
			path = utils::concat(path,"fakepath",rand()%3,"/");

		return utils::concat(path,"fakefile",rand()%3);
	}

	virtual bool isDirectory() const
	{
		return ((rand() % 10) == 0);
	}

	virtual bool contains(const Inode */*i*/) const
	{
		return ((rand() % 10) == 0);
	}

	virtual void getContent(std::vector<std::unique_ptr<Inode> > &out) const
	{
		if(!isDirectory())return;

		for(int i = 0; i < rand() % 15; ++i)
		{
			out.emplace_back(new InodeFake());
		}
	}

}; //end class InodeFake

} //end namespace td

#endif //INODEFAKE_HPP

#endif //__linux__
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

class InodeScanFake : public InodeScan
{

public:
	InodeScanFake(const std::string &str_path) :
		InodeScan(str_path)
	{
		srand((unsigned)time(nullptr));
	}

	InodeScanFake(const InodeScanFake &other) = delete;

	InodeScanFake(InodeScanFake &&other) = default;

	virtual ~InodeScanFake()
	{}

	InodeScanFake& operator=(const InodeScanFake &other) = delete;

	InodeScanFake& operator=(InodeScanFake &&other) = default;

	virtual bool valid() const
	{
		return true;
	}

	virtual Inode* getInitialInode() const
	{
		return new InodeFake();
	}

	virtual bool nextInode(Inode */*iterator*/) const
	{
		return ((rand() % 1000) != 0);
	}

	virtual unsigned int getBlocksize() const
	{
		return 512;
	}

}; //end class InodeScanFake

} //end namespace td

#endif //INODESCANFAKE_HPP

#endif //__linux__
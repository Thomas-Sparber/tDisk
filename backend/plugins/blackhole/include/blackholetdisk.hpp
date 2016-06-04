#ifndef BLACKHOLETDISK_HPP
#define BLACKHOLETDISK_HPP

#include <iostream>

#include <plugin.hpp>

namespace td
{

class BlackholeTDisk : public Plugin
{

public:
	BlackholeTDisk(unsigned long long llu_size) :
		Plugin("blackhole"),
		size(llu_size)
	{}

	virtual bool read(unsigned long long /*offset*/, char */*data*/, std::size_t /*length*/) const
	{
		return true;
	}

	virtual bool write(unsigned long long /*offset*/, const char */*data*/, std::size_t /*length*/)
	{
		return true;
	}

	virtual unsigned long long getSize() const
	{
		std::cout<<"Size request: "<<size<<std::endl;
		return size;
	}

private:
	unsigned long long size;

}; //end class BlackholeTDisk	

} //end namespace td

#endif //BLACKHOLETDISK_HPP

#ifndef TEST_UTILS_HPP
#define TEST_UTILS_HPP

#include <tdisk.hpp>

inline std::vector<std::string> getTDisks()
{
	std::vector<std::string> disks;
	td::tDisk::getTDisks(disks);
	return std::move(disks);
}

inline void generateRandomData(std::vector<char> &data)
{
	for(char &c : data)
		c = (char) (random() % 256);
}

inline void getSizeValue(long double &size, std::string &unit)
{
	if(size > 1024LL*1024LL*1024LL*1024LL)
	{
		unit = "TiB";
		size /= 1024LL*1024LL*1024LL*1024LL;
	}
	else if(size > 1024*1024*1024)
	{
		unit = "GiB";
		size /= 1024*1024*1024;
	}
	else if(size > 1024*1024)
	{
		unit = "MiB";
		size /= 1024*1024;
	}
	else if(size > 1024)
	{
		unit = "KiB";
		size /= 1024;
	}
	else unit = "Byte";
}

#endif //TEST_UTILS_HPP

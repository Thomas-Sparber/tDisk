#ifndef TEST_UTILS_HPP
#define TEST_UTILS_HPP

#include <tdisk.hpp>

inline std::vector<std::string> getTDisks()
{
	std::vector<std::string> disks;
	td::tDisk::getTDisks(disks);
	return std::move(disks);
}

#endif //TEST_UTILS_HPP

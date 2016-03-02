#include <algorithm>

#include <tdisk_advice.hpp>

using namespace td;
using namespace advisor;

tdisk_advice::tdisk_advice() :
	devices(),
	recommendation_score(),
	redundancy_level(),
	wasted_space()
{}

bool tdisk_advice::operator== (const tdisk_advice &other)
{
	if(devices.size() != other.devices.size())return false;

	for(const fs::Device &d : other.devices)
		if(std::find(devices.begin(), devices.end(), d) == devices.cend())return false;

	return true;
}

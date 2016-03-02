#ifndef TDISK_ADVICE_HPP
#define TDISK_ADVICE_HPP

#include <vector>

#include <device.hpp>

namespace td
{

namespace advisor
{

struct tdisk_advice
{
	tdisk_advice();

	bool operator== (const tdisk_advice &other);

	bool operator< (const tdisk_advice &other) const
	{
		if(recommendation_score == other.recommendation_score)return wasted_space > other.wasted_space;
		return recommendation_score < other.recommendation_score;
	}

	std::vector<fs::Device> devices;
	int recommendation_score;
	int redundancy_level;
	uint64_t wasted_space;
}; //end struct tdisk_advice

} //end namespace advisor

} //end namespace td

#endif //DEVICE_ADVICE_HPP

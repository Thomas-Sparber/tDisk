/**
  *
  * tDisk Driver
  * @author Thomas Sparber (2015)
  *
 **/

#ifndef TDISK_ADVICE_HPP
#define TDISK_ADVICE_HPP

#include <vector>

#include <device.hpp>

namespace td
{

namespace advisor
{

/**
  * A tDisk_adviceis created by the DeviceAdvisor and represents a
  * list of devices which can be combined into a tDisk
 **/
struct tdisk_advice
{

	/**
	  * Default constructor
	 **/
	tdisk_advice();

	/**
	  * Compares if a tdisk_advice equals an other
	 **/
	bool operator== (const tdisk_advice &other);

	/**
	  * Tests if the recommendation score of one tdisk_advice is less
	  * than an other
	 **/
	bool operator< (const tdisk_advice &other) const
	{
		if(recommendation_score == other.recommendation_score)return wasted_space > other.wasted_space;
		return recommendation_score < other.recommendation_score;
	}

	/**
	  * The list of devices which can be combined into a tDisk
	 **/
	std::vector<fs::Device> devices;

	/**
	  * The remmendation score of this device combination
	 **/
	int recommendation_score;

	/**
	  * The amount of disks which are allowed to fail at the same time
	 **/
	int redundancy_level;

	/**
	  * The amount of wsted space in bytes
	 **/
	uint64_t wasted_space;

}; //end struct tdisk_advice

} //end namespace advisor

} //end namespace td

#endif //DEVICE_ADVICE_HPP

/**
  *
  * tDisk backend
  * @author Thomas Sparber (2015-2016)
  *
 **/

#ifndef PERFORMANCE_HPP
#define PERFORMANCE_HPP

#include <chrono>
#include <logger.hpp>
#include <map>
#include <string>
#include <thread>

namespace td
{

/**
  * This namespace contains data structures and functions
  * to measure the performance of some parts of the code
 **/
namespace performance
{

	/**
	  * The PerformanceIndex is identified by a section name and
	  * the thread id and is used to match performance measure points
	 **/
	struct PerformanceIndex
	{
		PerformanceIndex(const std::string &str_what, std::thread::id t_id)  :
			what(str_what),
			id(t_id)
		{}

		/**
		  * Compares two PerformanceIndices. Is used by td::map
		  * to be used as index
		 **/
		bool operator< (const PerformanceIndex &other) const
		{
			if(id != other.id)return (id < other.id);
			return (what < other.what);
		}

		/**
		  * The performance section name
		 **/
		std::string what;

		/**
		  * The thread id of the thread to be measured
		 **/
		std::thread::id id;

	}; //end struct performanceIndex

	/**
	  * Checks whether performance measures are enabled
	  * or not
	 **/
	inline bool& measurePerformance()
	{
		static bool measure = true;
		return measure;
	}

	/**
	  * Enables of disables performance measurement
	 **/
	inline void setMeasurePerformance(bool measure)
	{
		measurePerformance() = measure;
	}

	/**
	  * Retrieves a map of all currently running measures
	 **/
	inline std::map<PerformanceIndex,std::chrono::high_resolution_clock::time_point>& currentMeasures()
	{
		static std::map<PerformanceIndex,std::chrono::high_resolution_clock::time_point> measures;
		return measures;
	}

	/**
	  * Startes a named performance measure
	 **/
	inline void start(const std::string &what)
	{
		if(!measurePerformance())return;

		currentMeasures().insert(
			std::make_pair(
				PerformanceIndex(what, std::this_thread::get_id()),
				std::chrono::high_resolution_clock::now()
			)
		);
	}

	/**
	  * Stops the performance measure with the given name.
	  * The time in seconds is logged
	 **/
	inline double stop(const std::string &what)
	{
		if(!measurePerformance())return 0;

		auto found = currentMeasures().find(PerformanceIndex(what, std::this_thread::get_id()));

		if(found == currentMeasures().cend())
		{
			LOG(LogLevel::error, "Error: measure ",what," was stopped before it was started!");
			return 0;
		}

		auto elapsed = std::chrono::high_resolution_clock::now() - found->second;
		double seconds = (double)std::chrono::duration_cast<std::chrono::microseconds>(elapsed).count() / 1000000;
		currentMeasures().erase(found);

		LOG(LogLevel::info, "Measure ",what,": ",seconds," s");
		return seconds;
	}

} //end namespace performance

} //end namespace td

#endif //PERFORMANCE_HPP

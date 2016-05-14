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

namespace performance
{

	struct PerformanceIndex
	{
		PerformanceIndex(const std::string &str_what, std::thread::id t_id)  :
			what(str_what),
			id(t_id)
		{}

		bool operator< (const PerformanceIndex &other) const
		{
			if(id != other.id)return (id < other.id);
			return (what < other.what);
		}

		std::string what;
		std::thread::id id;
	}; //end class performanceIndex

	inline bool& measurePerformance()
	{
		static bool measure = true;
		return measure;
	}

	inline void setMeasurePerformance(bool measure)
	{
		measurePerformance() = measure;
	}

	inline std::map<PerformanceIndex,std::chrono::high_resolution_clock::time_point>& currentMeasures()
	{
		static std::map<PerformanceIndex,std::chrono::high_resolution_clock::time_point> measures;
		return measures;
	}

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

	inline double stop(const std::string &what)
	{
		if(!measurePerformance)return 0;

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

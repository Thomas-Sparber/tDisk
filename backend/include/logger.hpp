/**
  *
  * tDisk backend
  * @author Thomas Sparber (2015-2016)
  *
 **/

#ifndef LOGGER_HPP
#define LOGGER_HPP

#include <iostream>

#include <utils.hpp>

namespace td
{

	#define LOG(level, ...) doLog(__FILE__, __LINE__, level, __VA_ARGS__)

	enum class LogLevel : int
	{
		debug = 1,
		info = 2,
		warn = 3,
		error = 4
	}; // end enum class LogLevel

	inline LogLevel& logLevel()
	{
		static LogLevel level = LogLevel::debug;
		return level;
	}

	inline void setLogLevel(LogLevel level)
	{
		logLevel() = level;
	}

	inline const char* logLevelString(LogLevel level)
	{
		switch(level)
		{
			case LogLevel::debug: return "DEBUG";
			case LogLevel::info: return "INFO";
			case LogLevel::warn: return "WARN";
			case LogLevel::error: return "ERROR";
		}

		return "";
	}

	template <class ...T>
	inline void doLog(const char *file, int line, LogLevel level, const T& ...t)
	{
		if(level < logLevel())return;

		std::cerr<<file<<"("<<line<<") "<<logLevelString(level)<<": "<<utils::concat(t...)<<std::endl;
	}

} //end namespace td

#endif //LOGGER_HPP

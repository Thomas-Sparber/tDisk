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

	/**
	  * This macro shluld be the main function for logging.
	  * it calls doLog with the correct filename and line number
	 **/
	#define LOG(level, ...) doLog(__FILE__, __LINE__, level, __VA_ARGS__)

	/**
	  * This enum defines the different log levels
	 **/
	enum class LogLevel : int
	{
		debug = 1,
		info = 2,
		warn = 3,
		error = 4
	}; // end enum class LogLevel

	/**
	  * Gets the current log level
	 **/
	inline LogLevel& logLevel()
	{
		static LogLevel level = LogLevel::debug;
		return level;
	}

	/**
	  * Sets the current log level
	 **/
	inline void setLogLevel(LogLevel level)
	{
		logLevel() = level;
	}

	/**
	  * Gets the string representation of the given loglevel
	 **/
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

	/**
	  * This variadic function does the actual logging to std::cerr
	 **/
	template <class ...T>
	inline void doLog(const char *file, int line, LogLevel level, const T& ...t)
	{
		if(level < logLevel())return;

		std::cerr<<file<<"("<<line<<") "<<logLevelString(level)<<": "<<utils::concat(t...)<<std::endl;
	}

} //end namespace td

#endif //LOGGER_HPP

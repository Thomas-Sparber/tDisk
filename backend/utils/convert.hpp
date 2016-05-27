/**
  *
  * tDisk backend
  * @author Thomas Sparber (2015-2016)
  *
 **/

#ifndef CONVERT_HPP
#define CONVERT_HPP

#include <string>

#include <ci_string.hpp>

namespace td
{

namespace utils
{

	template <class string>
	inline bool convertTo(const string &value, uint64_t &out)
	{
		char *test;
		out = strtoull(value.c_str(), &test, 0);
		return (test == value.c_str() + value.length());
	}

	template <class string>
	inline bool convertTo(const string &value, uint32_t &out)
	{
		char *test;
		out = (uint32_t)strtoul(value.c_str(), &test, 0);
		return (test == value.c_str() + value.length());
	}

	template <class string>
	inline bool convertTo(const string &value, uint16_t &out)
	{
		char *test;
		out = (uint16_t)strtoul(value.c_str(), &test, 0);
		return (test == value.c_str() + value.length());
	}

	template <class string>
	inline bool convertTo(const string &value, int64_t &out)
	{
		char *test;
		out = strtoll(value.c_str(), &test, 0);
		return (test == value.c_str() + value.length());
	}

	template <class string>
	inline bool convertTo(const string &value, int32_t &out)
	{
		char *test;
		out = (int32_t)strtol(value.c_str(), &test, 0);
		return (test == value.c_str() + value.length());
	}

	template <class string>
	inline bool convertTo(const string &value, int16_t &out)
	{
		char *test;
		out = (int16_t)strtol(value.c_str(), &test, 0);
		return (test == value.c_str() + value.length());
	}

	template <class string>
	inline bool convertTo(const string &value, int8_t &out)
	{
		char *test;
		out = (int8_t)strtol(value.c_str(), &test, 0);
		return (test == value.c_str() + value.length());
	}

	template <class string>
	inline bool convertTo(const string &value, char &out)
	{
		if(value.empty())return false;
		out = value[0];
		return (value.length() == 1);
	}

	template <class string>
	inline bool convertTo(const string &value, unsigned char &out)
	{
		if(value.empty())return false;
		out = value[0];
		return (value.length() == 1);
	}

	template <class string>
	inline bool convertTo(const string &value, float &out)
	{
		char *test;
		out = strtof(value.c_str(), &test);
		return (test == value.c_str() + value.length());
	}

	template <class string>
	inline bool convertTo(const string &value, double &out)
	{
		char *test;
		out = strtod(value.c_str(), &test);
		return (test == value.c_str() + value.length());
	}

	template <class string>
	inline bool convertTo(const string &value, long double &out)
	{
		char *test;
		out = strtold(value.c_str(), &test);
		return (test == value.c_str() + value.length());
	}

	template <class string>
	inline bool convertTo(const string &value, std::string &out)
	{
		out = value;
		return true;
	}

	template <class string>
	inline bool convertTo(const string &value, ci_string &out)
	{
		out = value;
		return true;
	}

	template <class string>
	inline bool convertTo(const string &value, bool &out)
	{
		if(value == "1")out = true;
		else if(ci_string(value.cbegin(), value.cend()) == "true")out = true;
		else out = false;

		return true;
	}

} //end namespace utils

} //end namespace td

#endif //CONVERT_HPP

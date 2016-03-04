#ifndef UTILS_HPP
#define UTILS_HPP

#include <sstream>
#include <vector>

namespace td
{

	namespace utils
	{

		/**
		  * Helper function to concat variables
		 **/
		template <class S>
		inline void concat(std::stringstream &ss, const S &s)
		{
			ss<<s;
		}

		/**
		  * Helper function to concat variables
		 **/
		template <class T>
		inline void concat(std::stringstream &ss, const std::vector<T> &v)
		{
			for(const T &t : v)
				concat(ss, t);
		}

		/**
		  * Helper function to concat variables
		 **/
		template <class S, class ...T>
		inline void concat(std::stringstream &ss, const S &s, T ...t)
		{
			concat(ss, s);
			concat(ss, t...);
		}

		/**
		  * Cancats any kind of data into a single string
		 **/
		template <class ...T>
		inline std::string concat(T ...t)
		{
			std::stringstream ss;
			concat(ss, t...);
			return ss.str();
		}

		/**
		  * Fills the given vector with random chars
		 **/
		inline void generateRandomData(std::vector<char> &data)
		{
			for(char &c : data)
				c = (char) (random() % 256);
		}

		/**
		  *	Converts the given size value from bytes to a
		  * "better human readable" value and stores the unit in unit.
		  * The output unit ranges from Byte to TiB.
		 **/
		template <class size_type>
		inline void getSizeValue(size_type &size, std::string &unit)
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

	} //end namespace utils

} //end namespace td

#endif //UTILS_HPP

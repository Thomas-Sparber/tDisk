#ifndef UTILS_HPP
#define UTILS_HPP

#include <sstream>
#include <vector>

namespace td
{

	template <class S>
	inline void concat(std::stringstream &ss, const S &s)
	{
		ss<<s;
	}

	template <class T>
	inline void concat(std::stringstream &ss, const std::vector<T> &v)
	{
		for(const T &t : v)
			concat(ss, t);
	}

	template <class S, class ...T>
	inline void concat(std::stringstream &ss, const S &s, T ...t)
	{
		concat(ss, s);
		concat(ss, t...);
	}

	template <class ...T>
	inline std::string concat(T ...t)
	{
		std::stringstream ss;
		concat(ss, t...);
		return ss.str();
	}

} //end namespace td

#endif //UTILS_HPP

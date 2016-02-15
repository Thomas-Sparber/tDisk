#ifndef RESULTFORMATTER_HPP
#define RESULTFORMATTER_HPP

#include <string>
#include <vector>
#include <map>

#include <ci_string.hpp>
#include <utils.hpp>

namespace td
{

	#define CREATE_RESULT_STRING_MEMBER_JSON(object, member, hierarchy, outputFormat) "\"", #member, "\": ", createResultString(object.member, hierarchy, outputFormat)
	#define CREATE_RESULT_STRING_MEMBER_TEXT(object, member, hierarchy, outputFormat) #member, " = ", createResultString(object.member, hierarchy, outputFormat)

	#define CREATE_RESULT_STRING_NONMEMBER_JSON(var, hierarchy, outputFormat) "\"", #var, "\": ", createResultString(var, hierarchy, outputFormat)
	#define CREATE_RESULT_STRING_NONMEMBER_TEXT(var, hierarchy, outputFormat) #var, " = ", createResultString(var, hierarchy, outputFormat)

	struct FormatException
	{
		template <class ...T>
		FormatException(T ...t) :
			what(td::concat(t...))
		{}

		std::string what;
	}; //end class FormatException

	template <class T>
	inline std::string createResultString(const T &t, unsigned int hierarchy, const ci_string &outputFormat)
	{
		throw FormatException("Can't create result string of type ", typeid(t).name());
	}

	template <class T>
	inline std::string createResultString_string(const T &s, const ci_string &outputFormat)
	{
		if(outputFormat == "json")
			return concat('"', s, '"');
		else if(outputFormat == "text")
			return concat(s);
		else
			throw FormatException("Invalid output-format ", outputFormat);
	}

	template <class T>
	inline std::string createResultString_number(const T &i, const ci_string &outputFormat)
	{
		if(outputFormat == "json")
			return concat(i);
		else if(outputFormat == "text")
			return concat(i);
		else
			throw FormatException("Invalid output-format ", outputFormat);
	}

	template <class T, template <class ...> class container>
	inline std::string createResultString_array(const container<T> &v, unsigned int hierarchy, const ci_string &outputFormat)
	{
		std::vector<std::string> temp(v.size());
		if(outputFormat == "json")
			for(size_t i = 0; i < v.size(); ++i)
			{
				temp[i] = concat(
					(i == 0) ? "[\n" : "",
					std::vector<char>(hierarchy+1, '\t'), createResultString(v[i], hierarchy + 1, outputFormat),
					(i < v.size()-1) ? ",\n" : concat("\n", std::vector<char>(hierarchy, '\t'), "]"));
			}
		else if(outputFormat == "text")
			for(size_t i = 0; i < v.size(); ++i)
			{
				temp[i] = concat(
					createResultString(v[i], hierarchy + 1, outputFormat),
					(i < v.size()-1) ? "\n" : "");
			}
		else
			throw FormatException("Invalid output-format ", outputFormat);

		return concat(temp);
	}

	template <class S, class T>
	inline std::string createResultString_map(const std::map<S,T> &m, unsigned int hierarchy, const ci_string &outputFormat)
	{
		size_t i = 0;
		std::vector<std::string> temp(m.size());
		if(outputFormat == "json")
			for(const std::pair<S,T> &p : m)
			{
				temp[i] = concat(
					(i == 0) ? "{\n" : "",
					std::vector<char>(hierarchy+1, '\t'), createResultString(p.first, hierarchy + 1, outputFormat), ": ", createResultString(p.second, hierarchy + 1, outputFormat),
					(i < m.size()-1) ? ",\n" : concat("\n", std::vector<char>(hierarchy, '\t'), "}"));
				i++;
			}
		else if(outputFormat == "text")
			for(const std::pair<S,T> &p : m)
			{
				temp[i] = concat(
					createResultString(p.first, hierarchy + 1, outputFormat), " = ", createResultString(p.second, hierarchy + 1, outputFormat),
					(i < m.size()-1) ? "\n" : "");
				i++;
			}
		else
			throw FormatException("Invalid output-format ", outputFormat);

		return concat(temp);
	}

	template <> inline std::string createResultString(const std::string &s, unsigned int /*hierarchy*/, const ci_string &outputFormat)
	{
		return createResultString_string(s, outputFormat);
	}

	template <> inline std::string createResultString(const ci_string &s, unsigned int /*hierarchy*/, const ci_string &outputFormat)
	{
		return createResultString_string(s, outputFormat);
	}

	template <> inline std::string createResultString(const char &s, unsigned int /*hierarchy*/, const ci_string &outputFormat)
	{
		return createResultString_string(s, outputFormat);
	}

	//template <> inline std::string createResultString((const char*) &s, unsigned int /*hierarchy*/, const ci_string &outputFormat)
	//{
	//	return createResultString_string(std::string(s), outputFormat);
	//}

	template <> inline std::string createResultString(const uint8_t &i, unsigned int /*hierarchy*/, const ci_string &outputFormat)
	{
		return createResultString_number((unsigned int)i, outputFormat);
	}

	template <> inline std::string createResultString(const uint16_t &i, unsigned int /*hierarchy*/, const ci_string &outputFormat)
	{
		return createResultString_number(i, outputFormat);
	}

	template <> inline std::string createResultString(const int &i, unsigned int /*hierarchy*/, const ci_string &outputFormat)
	{
		return createResultString_number(i, outputFormat);
	}

	template <> inline std::string createResultString(const unsigned int &i, unsigned int /*hierarchy*/, const ci_string &outputFormat)
	{
		return createResultString_number(i, outputFormat);
	}

	template <> inline std::string createResultString(const long &i, unsigned int /*hierarchy*/, const ci_string &outputFormat)
	{
		return createResultString_number(i, outputFormat);
	}

	template <> inline std::string createResultString(const unsigned long &i, unsigned int /*hierarchy*/, const ci_string &outputFormat)
	{
		return createResultString_number(i, outputFormat);
	}

	template <> inline std::string createResultString(const long long &i, unsigned int /*hierarchy*/, const ci_string &outputFormat)
	{
		return createResultString_number(i, outputFormat);
	}

	template <> inline std::string createResultString(const unsigned long long &i, unsigned int /*hierarchy*/, const ci_string &outputFormat)
	{
		return createResultString_number(i, outputFormat);
	}

	template <> inline std::string createResultString(const float &i, unsigned int /*hierarchy*/, const ci_string &outputFormat)
	{
		return createResultString_number(i, outputFormat);
	}

	template <> inline std::string createResultString(const double &i, unsigned int /*hierarchy*/, const ci_string &outputFormat)
	{
		return createResultString_number(i, outputFormat);
	}

	template <> inline std::string createResultString(const long double &i, unsigned int /*hierarchy*/, const ci_string &outputFormat)
	{
		return createResultString_number(i, outputFormat);
	}

	template <class T> inline std::string createResultString(const std::vector<T> &v, unsigned int hierarchy, const ci_string &outputFormat)
	{
		return createResultString_array(v, hierarchy, outputFormat);
	}

} //end namespace td

#endif //RESULTMORMATTER_HPP

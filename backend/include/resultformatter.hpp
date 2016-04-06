/**
  *
  * tDisk backend
  * @author Thomas Sparber (2015-2016)
  *
 **/

#ifndef RESULTFORMATTER_HPP
#define RESULTFORMATTER_HPP

#include <map>
#include <string>
#include <typeinfo>
#include <vector>

#include <ci_string.hpp>
#include <formatexception.hpp>
#include <utils.hpp>

namespace td
{

	/**
	  * Creates a Json result string for an object member
	 **/
	#define CREATE_RESULT_STRING_MEMBER_JSON(object, member, hierarchy, outputFormat) "\"", #member, "\": ", createResultString(object.member, hierarchy, outputFormat)

	/**
	  * Creates a text result string for an object member
	 **/
	#define CREATE_RESULT_STRING_MEMBER_TEXT(object, member, hierarchy, outputFormat) #member, " = ", createResultString(object.member, hierarchy, outputFormat)

	/**
	  * Creates a Json result string for a non object member
	 **/
	#define CREATE_RESULT_STRING_NONMEMBER_JSON(var, hierarchy, outputFormat) "\"", #var, "\": ", createResultString(var, hierarchy, outputFormat)
	
	/**
	  * Creates a text result string for a non object member
	 **/
	#define CREATE_RESULT_STRING_NONMEMBER_TEXT(var, hierarchy, outputFormat) #var, " = ", createResultString(var, hierarchy, outputFormat)

	/**
	  * Stringifies the given object using the given format and data
	  * hierarchy
	  * @param t The object to be converted to a string
	  * @param hierarchy The current data hierarchy (how many \\t's)
	  * @param outputFormat The format to convert the object to string.
	  * Currently, json and text are supported
	  * @returns The object converted to string using the given format
	 **/
	template <class T>
	inline std::string createResultString(const T &t, unsigned int hierarchy, const utils::ci_string &outputFormat)
	{
		throw FormatException("Can't create result string of type ", typeid(t).name());
	}

	/**
	  * Stringifies the given object using the given format and data
	  * hierarchy. The given object is treaed as string.
	  * @param s The object to be converted to a string
	  * @param outputFormat The format to convert the object to string.
	  * Currently, json and text are supported
	  * @returns The object converted to string using the given format
	 **/
	template <class T>
	inline std::string createResultString_bool(const T &s, const utils::ci_string &outputFormat)
	{
		if(outputFormat == "json")
			return (s) ? "true" : "false";
		else if(outputFormat == "text")
			return (s) ? "yes" : "no";
		else
			throw FormatException("Invalid output-format ", outputFormat);
	}

	/**
	  * Stringifies the given object using the given format and data
	  * hierarchy. The given object is treaed as string.
	  * @param s The object to be converted to a string
	  * @param outputFormat The format to convert the object to string.
	  * Currently, json and text are supported
	  * @returns The object converted to string using the given format
	 **/
	template <class T>
	inline std::string createResultString_string(const T &s, const utils::ci_string &outputFormat)
	{
		if(outputFormat == "json")
			return utils::concat('"', s, '"');
		else if(outputFormat == "text")
			return utils::concat(s);
		else
			throw FormatException("Invalid output-format ", outputFormat);
	}

	/**
	  * Stringifies the given object using the given format and data
	  * hierarchy. The given object is treated as number
	  * @param i The object to be converted to a string
	  * @param outputFormat The format to convert the object to string.
	  * Currently, json and text are supported
	  * @returns The object converted to string using the given format
	 **/
	template <class T>
	inline std::string createResultString_number(const T &i, const utils::ci_string &outputFormat)
	{
		if(outputFormat == "json")
			return utils::concat(i);
		else if(outputFormat == "text")
			return utils::concat(i);
		else
			throw FormatException("Invalid output-format ", outputFormat);
	}

	/**
	  * Stringifies the given array object using the given format and
	  * data hierarchy
	  * @param v The object to be converted to a string
	  * @param hierarchy The current data hierarchy (how many \\t's)
	  * @param outputFormat The format to convert the object to string.
	  * Currently, json and text are supported
	  * @returns The object converted to string using the given format
	 **/
	template <class T, template <class ...> class container>
	inline std::string createResultString_array(const container<T> &v, unsigned int hierarchy, const utils::ci_string &outputFormat)
	{
		std::vector<std::string> temp(v.size());
		if(outputFormat == "json")
		{
			if(v.empty())temp = { "[]" };

			for(size_t i = 0; i < v.size(); ++i)
			{
				temp[i] = utils::concat(
					(i == 0) ? "[\n" : "",
					std::vector<char>(hierarchy+1, '\t'), createResultString(v[i], hierarchy + 1, outputFormat),
					(i < v.size()-1) ? ",\n" : utils::concat("\n", std::vector<char>(hierarchy, '\t'), "]"));
			}
		}
		else if(outputFormat == "text")
			for(size_t i = 0; i < v.size(); ++i)
			{
				temp[i] = utils::concat(
					createResultString(v[i], hierarchy + 1, outputFormat),
					(i < v.size()-1) ? "\n" : "");
			}
		else
			throw FormatException("Invalid output-format ", outputFormat);

		return utils::concat(temp);
	}

	/**
	  * Stringifies the given map using the given format and data
	  * hierarchy
	  * @param m The object to be converted to a string
	  * @param hierarchy The current data hierarchy (how many \\t's)
	  * @param outputFormat The format to convert the object to string.
	  * Currently, json and text are supported
	  * @returns The object converted to string using the given format
	 **/
	template <class S, class T>
	inline std::string createResultString_map(const std::map<S,T> &m, unsigned int hierarchy, const utils::ci_string &outputFormat)
	{
		size_t i = 0;
		std::vector<std::string> temp(m.size());
		if(outputFormat == "json")
			for(const std::pair<S,T> &p : m)
			{
				temp[i] = utils::concat(
					(i == 0) ? "{\n" : "",
					std::vector<char>(hierarchy+1, '\t'), createResultString(p.first, hierarchy + 1, outputFormat), ": ", createResultString(p.second, hierarchy + 1, outputFormat),
					(i < m.size()-1) ? ",\n" : utils::concat("\n", std::vector<char>(hierarchy, '\t'), "}"));
				i++;
			}
		else if(outputFormat == "text")
			for(const std::pair<S,T> &p : m)
			{
				temp[i] = utils::concat(
					createResultString(p.first, hierarchy + 1, outputFormat), " = ", createResultString(p.second, hierarchy + 1, outputFormat),
					(i < m.size()-1) ? "\n" : "");
				i++;
			}
		else
			throw FormatException("Invalid output-format ", outputFormat);

		return utils::concat(temp);
	}

	/**
	  * Stringifies the given bool using the given format and data
	  * hierarchy
	  * @param b The bool to be converted
	  * @param int
	  * @param outputFormat The format to convert the bool.
	  * Currently, json and text are supported
	  * @returns The object converted to string using the given format
	 **/
	template <> inline std::string createResultString(const bool &b, unsigned int /*hierarchy*/, const utils::ci_string &outputFormat)
	{
		return createResultString_bool(b, outputFormat);
	}

	/**
	  * Stringifies the given string using the given format and data
	  * hierarchy
	  * @param s The string to be converted
	  * @param int
	  * @param outputFormat The format to convert the string.
	  * Currently, json and text are supported
	  * @returns The object converted to string using the given format
	 **/
	template <> inline std::string createResultString(const std::string &s, unsigned int /*hierarchy*/, const utils::ci_string &outputFormat)
	{
		return createResultString_string(s, outputFormat);
	}

	/**
	  * Stringifies the given string using the given format and data
	  * hierarchy
	  * @param s The string to be converted
	  * @param int
	  * @param outputFormat The format to convert the string.
	  * Currently, json and text are supported
	  * @returns The object converted to string using the given format
	 **/
	template <> inline std::string createResultString(const utils::ci_string &s, unsigned int /*hierarchy*/, const utils::ci_string &outputFormat)
	{
		return createResultString_string(s, outputFormat);
	}

	/**
	  * Stringifies the given char using the given format and data
	  * hierarchy
	  * @param s The char to be converted
	  * @param int
	  * @param outputFormat The format to convert the char.
	  * Currently, json and text are supported
	  * @returns The object converted to string using the given format
	 **/
	template <> inline std::string createResultString(const char &s, unsigned int /*hierarchy*/, const utils::ci_string &outputFormat)
	{
		return createResultString_string(s, outputFormat);
	}

	/**
	  * Stringifies the given char array using the given format and data
	  * hierarchy
	  * @param s The char array to be converted
	  * @param int
	  * @param outputFormat The format to convert the char array.
	  * Currently, json and text are supported
	  * @returns The object converted to string using the given format
	 **/
	template <> inline std::string createResultString(const char* const &s, unsigned int /*hierarchy*/, const utils::ci_string &outputFormat)
	{
		return createResultString_string(std::string(s), outputFormat);
	}

	/**
	  * Stringifies the given char array using the given format and data
	  * hierarchy
	  * @param s The char array to be converted
	  * @param int
	  * @param outputFormat The format to convert the char array.
	  * Currently, json and text are supported
	  * @returns The object converted to string using the given format
	 **/
	template <std::size_t i> inline std::string createResultString(const char (&s)[i], unsigned int /*hierarchy*/, const utils::ci_string &outputFormat)
	{
		return createResultString_string(std::string(s), outputFormat);
	}

	/**
	  * Stringifies the given uint8_t using the given format and data
	  * hierarchy
	  * @param i The uint8_t to be converted
	  * @param int
	  * @param outputFormat The format to convert the uint8_t.
	  * Currently, json and text are supported
	  * @returns The object converted to string using the given format
	 **/
	template <> inline std::string createResultString(const uint8_t &i, unsigned int /*hierarchy*/, const utils::ci_string &outputFormat)
	{
		return createResultString_number((unsigned int)i, outputFormat);
	}

	/**
	  * Stringifies the given uint16_t using the given format and data
	  * hierarchy
	  * @param i The uint16_t to be converted
	  * @param int
	  * @param outputFormat The format to convert the uint16_t.
	  * Currently, json and text are supported
	  * @returns The object converted to string using the given format
	 **/
	template <> inline std::string createResultString(const uint16_t &i, unsigned int /*hierarchy*/, const utils::ci_string &outputFormat)
	{
		return createResultString_number(i, outputFormat);
	}

	/**
	  * Stringifies the given int using the given format and data
	  * hierarchy
	  * @param i The int to be converted
	  * @param int
	  * @param outputFormat The format to convert the int.
	  * Currently, json and text are supported
	  * @returns The object converted to string using the given format
	 **/
	template <> inline std::string createResultString(const int &i, unsigned int /*hierarchy*/, const utils::ci_string &outputFormat)
	{
		return createResultString_number(i, outputFormat);
	}

	/**
	  * Stringifies the given unsigned int using the given format and data
	  * hierarchy
	  * @param i The unsigned int to be converted
	  * @param int
	  * @param outputFormat The format to convert the unsigned int.
	  * Currently, json and text are supported
	  * @returns The object converted to string using the given format
	 **/
	template <> inline std::string createResultString(const unsigned int &i, unsigned int /*hierarchy*/, const utils::ci_string &outputFormat)
	{
		return createResultString_number(i, outputFormat);
	}

	/**
	  * Stringifies the given long using the given format and data
	  * hierarchy
	  * @param i The long to be converted
	  * @param int
	  * @param outputFormat The format to convert the long.
	  * Currently, json and text are supported
	  * @returns The object converted to string using the given format
	 **/
	template <> inline std::string createResultString(const long &i, unsigned int /*hierarchy*/, const utils::ci_string &outputFormat)
	{
		return createResultString_number(i, outputFormat);
	}

	/**
	  * Stringifies the given unsigned long using the given format and data
	  * hierarchy
	  * @param i The unsigned long to be converted
	  * @param int
	  * @param outputFormat The format to convert the unsigned long.
	  * Currently, json and text are supported
	  * @returns The object converted to string using the given format
	 **/
	template <> inline std::string createResultString(const unsigned long &i, unsigned int /*hierarchy*/, const utils::ci_string &outputFormat)
	{
		return createResultString_number(i, outputFormat);
	}

	/**
	  * Stringifies the given long long using the given format and data
	  * hierarchy
	  * @param i The long long to be converted
	  * @param int
	  * @param outputFormat The format to convert the long long.
	  * Currently, json and text are supported
	  * @returns The object converted to string using the given format
	 **/
	template <> inline std::string createResultString(const long long &i, unsigned int /*hierarchy*/, const utils::ci_string &outputFormat)
	{
		return createResultString_number(i, outputFormat);
	}

	/**
	  * Stringifies the given unsigned long long using the given format and data
	  * hierarchy
	  * @param i The unsigned long long to be converted
	  * @param int
	  * @param outputFormat The format to convert the unsigned long long.
	  * Currently, json and text are supported
	  * @returns The object converted to string using the given format
	 **/
	template <> inline std::string createResultString(const unsigned long long &i, unsigned int /*hierarchy*/, const utils::ci_string &outputFormat)
	{
		return createResultString_number(i, outputFormat);
	}

	/**
	  * Stringifies the given float using the given format and data
	  * hierarchy
	  * @param i The float to be converted
	  * @param int
	  * @param outputFormat The format to convert the float.
	  * Currently, json and text are supported
	  * @returns The object converted to string using the given format
	 **/
	template <> inline std::string createResultString(const float &i, unsigned int /*hierarchy*/, const utils::ci_string &outputFormat)
	{
		return createResultString_number(i, outputFormat);
	}

	/**
	  * Stringifies the given double using the given format and data
	  * hierarchy
	  * @param i The double to be converted
	  * @param int
	  * @param outputFormat The format to convert the double.
	  * Currently, json and text are supported
	  * @returns The object converted to string using the given format
	 **/
	template <> inline std::string createResultString(const double &i, unsigned int /*hierarchy*/, const utils::ci_string &outputFormat)
	{
		return createResultString_number(i, outputFormat);
	}

	/**
	  * Stringifies the given long double using the given format and data
	  * hierarchy
	  * @param i The long double to be converted
	  * @param int
	  * @param outputFormat The format to convert the long double.
	  * Currently, json and text are supported
	  * @returns The object converted to string using the given format
	 **/
	template <> inline std::string createResultString(const long double &i, unsigned int /*hierarchy*/, const utils::ci_string &outputFormat)
	{
		return createResultString_number(i, outputFormat);
	}

	/**
	  * Stringifies the given vector using the given format and data
	  * hierarchy
	  * @param v The vector to be converted
	  * @param hierarchy The current data hierarchy (how many \\t's)
	  * @param outputFormat The format to convert the vector.
	  * Currently, json and text are supported
	  * @returns The object converted to string using the given format
	 **/
	template <class T> inline std::string createResultString(const std::vector<T> &v, unsigned int hierarchy, const utils::ci_string &outputFormat)
	{
		return createResultString_array(v, hierarchy, outputFormat);
	}

} //end namespace td

#endif //RESULTMORMATTER_HPP

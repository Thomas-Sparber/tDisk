/**
  *
  * tDisk backend
  * @author Thomas Sparber (2015-2016)
  *
 **/

#ifndef RESULTFORMATTER_HPP
#define RESULTFORMATTER_HPP

#include <map>
#include <sstream>
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
	#define CREATE_RESULT_STRING_MEMBER_JSON(ss, object, member, hierarchy, outputFormat) ss<<"\""<<#member<<"\": "; createResultString(ss, object.member, hierarchy, outputFormat)

	/**
	  * Creates a text result string for an object member
	 **/
	#define CREATE_RESULT_STRING_MEMBER_TEXT(ss, object, member, hierarchy, outputFormat) ss<<#member<<" = "; createResultString(ss, object.member, hierarchy, outputFormat)

	/**
	  * Creates a Json result string for a non object member
	 **/
	#define CREATE_RESULT_STRING_NONMEMBER_JSON(ss, var, hierarchy, outputFormat) ss<<"\""<<#var<<"\": "; createResultString(ss, var, hierarchy, outputFormat)
	
	/**
	  * Creates a text result string for a non object member
	 **/
	#define CREATE_RESULT_STRING_NONMEMBER_TEXT(ss, var, hierarchy, outputFormat) ss<<#var<<" = "; createResultString(ss, var, hierarchy, outputFormat)

	inline void insertTab(std::ostream &ss, unsigned int count)
	{
		for(unsigned int i = 0; i < count; ++i)
			ss<<'\t';
	}

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
	inline void createResultString(std::ostream &ss, const T &t, unsigned int hierarchy, const utils::ci_string &outputFormat)
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
	inline void createResultString_bool(std::ostream &ss, const T &s, const utils::ci_string &outputFormat)
	{
		if(outputFormat == "json")
			ss<<((s) ? "true" : "false");
		else if(outputFormat == "text")
			ss<<((s) ? "yes" : "no");
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
	inline void createResultString_string(std::ostream &ss, const T &s, const utils::ci_string &outputFormat)
	{
		if(outputFormat == "json")
			ss<<'"'<<s<<'"';
		else if(outputFormat == "text")
			ss<<s;
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
	inline void createResultString_number(std::ostream &ss, const T &i, const utils::ci_string &outputFormat)
	{
		if(outputFormat == "json")
			ss<<i;
		else if(outputFormat == "text")
			ss<<i;
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
	inline void createResultString_array(std::ostream &ss, const container<T> &v, unsigned int hierarchy, const utils::ci_string &outputFormat)
	{
		if(outputFormat == "json")
		{
			if(v.empty())ss<< "[]";

			for(size_t i = 0; i < v.size(); ++i)
			{
				if(i == 0)ss<<"[\n";
				insertTab(ss, hierarchy+1);
				createResultString(ss, v[i], hierarchy + 1, outputFormat);
				if(i < v.size()-1)
					ss<<",\n";
				else
				{
					ss<<'\n';
					insertTab(ss, hierarchy);
					ss<<"]";
				}
				
			}
		}
		else if(outputFormat == "text")
		{
			for(size_t i = 0; i < v.size(); ++i)
			{
				createResultString(ss, v[i], hierarchy + 1, outputFormat);
				if(i < v.size()-1)ss<<"\n";
			}
		}
		else
			throw FormatException("Invalid output-format ", outputFormat);
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
	inline void createResultString_map(std::ostream &ss, const std::map<S,T> &m, unsigned int hierarchy, const utils::ci_string &outputFormat)
	{
		std::size_t i = 0;
		if(outputFormat == "json")
			for(const std::pair<S,T> &p : m)
			{
				if(i == 0)ss<<"{\n";
				insertTab(ss, hierarchy+1);
				createResultString(ss, p.first, hierarchy + 1, outputFormat);
				ss<<": ";
				createResultString(ss, p.second, hierarchy + 1, outputFormat);
				if(i < m.size()-1)
					ss<<",\n";
				else
				{
					ss<<"\n";
					insertTab(ss, hierarchy);
					ss<<"}";
				}
				i++;
			}
		else if(outputFormat == "text")
			for(const std::pair<S,T> &p : m)
			{
				createResultString(ss, p.first, hierarchy + 1, outputFormat), " = ", createResultString(p.second, hierarchy + 1, outputFormat);
				if(i < m.size()-1)ss<<"\n";
				i++;
			}
		else
			throw FormatException("Invalid output-format ", outputFormat);
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
	template <> inline void createResultString(std::ostream &ss, const bool &b, unsigned int /*hierarchy*/, const utils::ci_string &outputFormat)
	{
		createResultString_bool(ss, b, outputFormat);
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
	template <> inline void createResultString(std::ostream &ss, const std::string &s, unsigned int /*hierarchy*/, const utils::ci_string &outputFormat)
	{
		createResultString_string(ss, s, outputFormat);
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
	template <> inline void createResultString(std::ostream &ss, const utils::ci_string &s, unsigned int /*hierarchy*/, const utils::ci_string &outputFormat)
	{
		createResultString_string(ss, s, outputFormat);
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
	template <> inline void createResultString(std::ostream &ss, const char &s, unsigned int /*hierarchy*/, const utils::ci_string &outputFormat)
	{
		createResultString_string(ss, s, outputFormat);
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
	template <> inline void createResultString(std::ostream &ss, const char* const &s, unsigned int /*hierarchy*/, const utils::ci_string &outputFormat)
	{
		createResultString_string(ss, std::string(s), outputFormat);
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
	template <std::size_t i> inline void createResultString(std::ostream &ss, const char (&s)[i], unsigned int /*hierarchy*/, const utils::ci_string &outputFormat)
	{
		createResultString_string(ss, std::string(s), outputFormat);
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
	template <> inline void createResultString(std::ostream &ss, const uint8_t &i, unsigned int /*hierarchy*/, const utils::ci_string &outputFormat)
	{
		createResultString_number(ss, (unsigned int)i, outputFormat);
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
	template <> inline void createResultString(std::ostream &ss, const uint16_t &i, unsigned int /*hierarchy*/, const utils::ci_string &outputFormat)
	{
		createResultString_number(ss, i, outputFormat);
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
	template <> inline void createResultString(std::ostream &ss, const int &i, unsigned int /*hierarchy*/, const utils::ci_string &outputFormat)
	{
		createResultString_number(ss, i, outputFormat);
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
	template <> inline void createResultString(std::ostream &ss, const unsigned int &i, unsigned int /*hierarchy*/, const utils::ci_string &outputFormat)
	{
		createResultString_number(ss, i, outputFormat);
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
	template <> inline void createResultString(std::ostream &ss, const long &i, unsigned int /*hierarchy*/, const utils::ci_string &outputFormat)
	{
		createResultString_number(ss, i, outputFormat);
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
	template <> inline void createResultString(std::ostream &ss, const unsigned long &i, unsigned int /*hierarchy*/, const utils::ci_string &outputFormat)
	{
		createResultString_number(ss, i, outputFormat);
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
	template <> inline void createResultString(std::ostream &ss, const long long &i, unsigned int /*hierarchy*/, const utils::ci_string &outputFormat)
	{
		createResultString_number(ss, i, outputFormat);
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
	template <> inline void createResultString(std::ostream &ss, const unsigned long long &i, unsigned int /*hierarchy*/, const utils::ci_string &outputFormat)
	{
		createResultString_number(ss, i, outputFormat);
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
	template <> inline void createResultString(std::ostream &ss, const float &i, unsigned int /*hierarchy*/, const utils::ci_string &outputFormat)
	{
		createResultString_number(ss, i, outputFormat);
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
	template <> inline void createResultString(std::ostream &ss, const double &i, unsigned int /*hierarchy*/, const utils::ci_string &outputFormat)
	{
		createResultString_number(ss, i, outputFormat);
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
	template <> inline void createResultString(std::ostream &ss, const long double &i, unsigned int /*hierarchy*/, const utils::ci_string &outputFormat)
	{
		createResultString_number(ss, i, outputFormat);
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
	template <class T> inline void createResultString(std::ostream &ss, const std::vector<T> &v, unsigned int hierarchy, const utils::ci_string &outputFormat)
	{
		createResultString_array(ss, v, hierarchy, outputFormat);
	}

} //end namespace td

#endif //RESULTMORMATTER_HPP

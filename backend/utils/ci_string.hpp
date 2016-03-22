/**
  *
  * tDisk backend
  * @author Thomas Sparber (2015-2016)
  *
 **/

#ifndef CI_STRING_HPP
#define CI_STRING_HPP

#include <string>

namespace td
{

	namespace utils
	{

		/**
		  * The char traits struct which is used
		  * to build case insensitive strings.
		 **/
		struct ci_char_traits : public std::char_traits<char>
		{

			/**
			  * Returns whether two given characters are equal
			 **/
			static bool eq(char c1, char c2)
			{
				return toupper(c1) == toupper(c2);
			}

			/**
			  * Returns whether two given characters are not equal
			 **/
			static bool ne(char c1, char c2)
			{
				return toupper(c1) != toupper(c2);
			}

			/**
			  * Returns whether the first character is less than the second
			 **/
			static bool lt(char c1, char c2)
			{
				return toupper(c1) <  toupper(c2);
			}

			/**
			  * Compares two given strings.
			  * @return 0 when they are equal, -1 when the first string is
			  * lesser, -1 when the second string is lesser
			 **/
			static int compare(const char* s1, const char* s2, size_t n) {
				while(n-- != 0)
				{
					if(toupper(*s1) < toupper(*s2))return -1;
					if(toupper(*s1) > toupper(*s2))return 1;
					++s1; ++s2;
				}
				return 0;
			}

			/**
			  * Finds the given character in the given string
			 **/
			static const char* find(const char* s, int n, char a)
			{
				while(n-- > 0 && toupper(*s) != toupper(a))
				{
				    ++s;
				}
				return s;
			}
		};

		/**
		  * A case insensitive string
		 **/
		typedef std::basic_string<char, ci_char_traits> ci_string;

		/**
		  * Writes a ci_string to an std::ostream
		 **/
		inline std::ostream& operator<<(std::ostream &out, const ci_string &str)
		{
			out<<str.c_str();
			return out;
		}

	} //end namespace utils

} //end namespace td

#endif //CI_STRING_HPP

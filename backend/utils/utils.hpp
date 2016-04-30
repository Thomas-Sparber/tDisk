/**
  *
  * tDisk backend
  * @author Thomas Sparber (2015-2016)
  *
 **/

#ifndef UTILS_HPP
#define UTILS_HPP

#include <algorithm>
#include <fstream>
#include <sstream>
#include <sys/stat.h>
#include <vector>

#ifdef WINDOWS
#include <direct.h>
#else
#include <unistd.h>
#endif //WINDOWS

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
				c = (char) (rand() % 256);
		}

		/**
		  * Concats a path using the path separator / or \
		 **/
		inline std::string concatPath()
		{
			return "";
		}

		/**
		  * Concats a path using the path separator / or \
		 **/
		inline std::string concatPath(const std::string &a, const std::string &b)
		{
			if(a.empty() || b.empty() ||
				a.back() == '/' || a.back() == '\\' ||
				b.front() == '/' || b.front() == '\\')return concat(a, b);

			std::size_t slashes = std::count(a.cbegin(), a.cend(), '/') + std::count(b.cbegin(), b.cend(), '/');
			std::size_t backslashes = std::count(a.cbegin(), a.cend(), '\\') + std::count(b.cbegin(), b.cend(), '\\');

			char pathSeparator = (slashes >= backslashes) ? '/' : '\\';

			return concat(a, pathSeparator, b);
		}

		/**
		  * Concats a path using the path separator / or \
		 **/
		template<class ...T>
		inline std::string concatPath(const std::string &a, const std::string &b, T ...t)
		{
			return concatPath(concatPath(a, b), t...);
		}

		/**
		  * Returns the directory name of the given file/folder.
		  * If the file/folder is a relative path, the current
		  * working directory is returned. If the file/folder is
		  * a relative path using the format "./..." or "../..."
		  * the relative path to the executable is taken.
		 **/
		inline std::string dirnameOf(const std::string& fname, const char *executable)
		{
			std::size_t pos = fname.find_last_of("\\/");
			const std::string dir = (std::string::npos == pos) ? "" : fname.substr(0, pos);

			if(dir.substr(0,2) == "..")
			{
				if(executable)
					return dirnameOf(dirnameOf(executable, nullptr), nullptr);
				else
					return concatPath(dirnameOf(dirnameOf("", nullptr), nullptr), dir.substr(2));
			}
			else if(dir.substr(0,1) == ".")
			{
				if(executable)
					return dirnameOf(executable, nullptr);
				else
					return concatPath(dirnameOf("", nullptr), dir.substr(1));
			}

			if(dir.empty())
			{
				char currentPath[1024];
#ifdef WINDOWS
				_getcwd(currentPath, sizeof(currentPath));
#else
				getcwd(currentPath, sizeof(currentPath));
#endif //WINDOWS
				return currentPath;
			}

			return dir;
		}

		/**
		  * Returns the filename/dirname of the given path
		 **/
		inline std::string filenameOf(const std::string& fname)
		{
			std::size_t pos = fname.find_last_of("\\/");
			return (std::string::npos == pos) ? fname : fname.substr(pos+1);
		}

		/**
		  * Checks whether the given file exists
		 **/
		inline bool fileExists(const std::string& name)
		{
			std::ifstream f(name.c_str());
			return f.good();
		}

		/**
		  * Checks if the given folder exists on the system
		 **/
		inline bool folderExists(const std::string &name)
		{
			struct stat info;

			if(stat(name.c_str(), &info) != 0)return false;
			return (info.st_mode & S_IFDIR);
		}

		/**
		  * Checks if the given file or folder exists on the system
		 **/
		inline bool fileOrFolderExists(const std::string &name)
		{
			struct stat info;
			return (stat(name.c_str(), &info) == 0);
		}

		/**
		  * Executes the given command and returns the output
		 **/
		inline std::string exec(const std::string &cmd)
		{
#ifdef __linux__
			FILE *pipe = popen(cmd.c_str(), "r");
#else
			FILE *pipe = _popen(cmd.c_str(), "r");
#endif

			if(!pipe)return "ERROR";

			char buffer[128];
			std::string result;
			while(!feof(pipe))
			{
				if(fgets(buffer, 128, pipe))
					result += buffer;
			}

#ifdef __linux__
			pclose(pipe);
#else
			_pclose(pipe);
#endif //__linux__

			return result;
		}

		/**
		  * Replaces all occurrences of search with replace in str
		 **/
		inline std::string replaceAll(const std::string &str, const std::string &search, const std::string &replace)
		{
			std::string result;
			std::size_t last = 0;
			std::size_t pos = 0;

			while((pos=str.find(search, pos)) != std::string::npos)
			{
				result += str.substr(last, pos-last);
				result += replace;
				pos += search.length();
				last = pos;
			}
			result += str.substr(last);

			return std::move(result);
		}

		/**
		  * Splits the string using the given delimiter
		  * and stores the result in out
		 **/
		template <template<class ...T> class cont>
		void split(const std::string &toSplit, char delimiter, cont<std::string> &out, bool useEmpty=true)
		{
			std::stringstream ss(toSplit);
			std::string item;

			while(std::getline(ss, item, delimiter))
				if(useEmpty || !item.empty())out.push_back(item);
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

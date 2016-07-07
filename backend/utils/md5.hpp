/**
  *
  * tDisk backend
  * @author Thomas Sparber (2015-2016)
  *
 **/

#ifndef MD5_HPP
#define MD5_HPP

#include <fstream>
#include <sstream>
#include <string>

namespace md5
{

	extern "C" {
		#include "md5.h"
	}

	/**
	  * Calculates the md5 hash of the given string
	 **/
	std::string getMD5(const std::string &str)
	{
		struct MD5Context md5c;
		unsigned char signature[16];

		MD5Init(&md5c);
		MD5Update(&md5c, str.c_str(), (int)str.length());
		MD5Final(signature, &md5c);

		std::stringstream ss;
		for(int i = 0; i < 16; ++i)
			ss<<std::hex<<(int)signature[i];

		return ss.str();
	}

	/**
	  * Calculates the md5 hash of the given file
	 **/
	std::string getFileMD5(const std::string &file)
	{
		char buffer[1024];
		struct MD5Context md5c;
		unsigned char signature[16];
		std::fstream in(file, std::fstream::binary | std::fstream::in);

		MD5Init(&md5c);
		while(in.read(buffer, sizeof(buffer)))
		{
			MD5Update(&md5c, buffer, (int)in.gcount());
		}
		MD5Final(signature, &md5c);

		std::stringstream ss;
		for(int i = 0; i < 16; ++i)
			ss<<std::hex<<(int)signature[i];

		return ss.str();
	}

}

#endif //MD5_HPP

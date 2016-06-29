/**
  *
  * tDisk backend
  * @author Thomas Sparber (2015-2016)
  *
 **/

#ifndef MD5_HPP
#define MD5_HPP

#include <fstream>
#include <string>

namespace md5
{

	extern "C" {
		#include "md5.h"
	}

	std::string getMD5(const std::string &str)
	{
		struct MD5Context md5c;
		unsigned char signature[16];

		MD5Init(&md5c);
		MD5Update(&md5c, str.c_str(), str.length());
		MD5Final(signature, &md5c);

		return std::string((const char*)signature, 16);
	}

	std::string getFileMD5(const std::string &file)
	{
		char buffer[1024];
		struct MD5Context md5c;
		unsigned char signature[16];
		std::fstream in(file, std::fstream::binary | std::fstream::in);

		MD5Init(&md5c);
		while(in.read(buffer, sizeof(buffer)))
		{
			MD5Update(&md5c, buffer, in.gcount());
		}
		MD5Final(signature, &md5c);

		return std::string((const char*)signature, 16);
	}

}

#endif //MD5_HPP

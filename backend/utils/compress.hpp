/**
  *
  * tDisk backend serial
  * @author Thomas Sparber (2015-2016)
  *
 **/

#ifndef COMPRESS_HPP
#define COMPRESS_HPP

#include <string>
#include <vector>
#include <zlib.h>

namespace td
{

	/**
	  * Compresses the given string using the zlib algorithm
	 **/
	inline bool compress(const char *in, std::size_t length, std::vector<char> &out, int level=Z_BEST_COMPRESSION, std::size_t resize=1024)
	{
		out = std::vector<char>(0);

		int ret;
		z_stream strm;

		/* allocate deflate state */
		strm.zalloc = Z_NULL;
		strm.zfree = Z_NULL;
		strm.opaque = Z_NULL;
		ret = deflateInit(&strm, level);
		if(ret != Z_OK)return false;

		strm.avail_in = (uInt)length;
		strm.next_in = (Bytef*)const_cast<char*>(in);

		/* run deflate() on input until output buffer not full, finish
		compression if all of source has been read in */
		do
		{
			out.resize(out.size() + resize);

			strm.avail_out = (uInt)resize;
			strm.next_out = (Bytef*)&out[out.size() - resize];

			ret = deflate(&strm, Z_FINISH);    /* no bad return value */
			if(ret == Z_STREAM_ERROR)return false;  /* state not clobbered */
		}
		while(strm.avail_out == 0);


		/* clean up and return */
		deflateEnd(&strm);
		return true;
	}

	/**
	  * Compresses the given string using the zlib algorithm
	 **/
	inline bool compress(const std::string &in, std::vector<char> &out, int level=Z_BEST_COMPRESSION, std::size_t resize=1024)
	{
		return compress(in.c_str(), in.length(), out, level, resize);
	}

	/**
	  * Compresses the given string using the zlib algorithm
	 **/
	inline bool compress(const std::vector<char> &in, std::vector<char> &out, int level=Z_BEST_COMPRESSION, std::size_t resize=1024)
	{
		return compress(&in[0], in.size(), out, level, resize);
	}

	/**
	  * Compresses the given string using the zlib algorithm
	 **/
	inline std::vector<char> compress(const char *in, std::size_t length, int level=Z_BEST_COMPRESSION, std::size_t resize=1024)
	{
		std::vector<char> ret;
		compress(in, length, ret, level, resize);
		return std::move(ret);
	}

	/**
	  * Compresses the given string using the zlib algorithm
	 **/
	inline std::vector<char> compress(const std::string &in, int level=Z_BEST_COMPRESSION, std::size_t resize=1024)
	{
		std::vector<char> ret;
		compress(in.c_str(), in.length(), ret, level, resize);
		return std::move(ret);
	}

	/**
	  * Compresses the given string using the zlib algorithm
	 **/
	inline std::vector<char> compress(const std::vector<char> &in, int level=Z_BEST_COMPRESSION, std::size_t resize=1024)
	{
		std::vector<char> ret;
		compress(&in[0], in.size(), ret, level, resize);
		return std::move(ret);
	}

	/**
	  * Decompresses the given string using the zlib algorithm
	 **/
	inline bool decompress(const char *in, std::size_t length, std::vector<char> &out, std::size_t resize=1024)
	{
		out = std::vector<char>(0);

		int ret;
		z_stream strm;

		/* allocate inflate state */
		strm.zalloc = Z_NULL;
		strm.zfree = Z_NULL;
		strm.opaque = Z_NULL;
		strm.avail_in = 0;
		strm.next_in = Z_NULL;
		ret = inflateInit(&strm);
		if(ret != Z_OK)return false;

		strm.avail_in = (uInt)length;
		strm.next_in = (Bytef*)const_cast<char*>(in);

		/* run inflate() on input until output buffer not full */
		do
		{
			out.resize(out.size() + resize);

			strm.avail_out = (uInt)resize;
			strm.next_out = (Bytef*)&out[out.size() - resize];

			ret = inflate(&strm, Z_NO_FLUSH);
			if(ret == Z_STREAM_ERROR)return false;  /* state not clobbered */

			switch(ret)
			{
			case Z_NEED_DICT:
			ret = Z_DATA_ERROR;     /* and fall through */
			case Z_DATA_ERROR:
			case Z_MEM_ERROR:
				(void)inflateEnd(&strm);
				return ret;
			}
		}
		while (strm.avail_out == 0);

		/* clean up and return */
		inflateEnd(&strm);
		return (ret == Z_STREAM_END);
	}

	/**
	  * Decompresses the given string using the zlib algorithm
	 **/
	inline std::vector<char> decompress(const char *in, std::size_t length, std::size_t resize=1024)
	{
		std::vector<char> ret;
		decompress(in, length, ret, resize);
		return std::move(ret);
	}

	/**
	  * Decompresses the given string using the zlib algorithm
	 **/
	inline std::vector<char> decompress(const std::string &in, std::size_t resize=1024)
	{
		std::vector<char> ret;
		decompress(in.c_str(), in.length(), ret, resize);
		return std::move(ret);
	}

	/**
	  * Decompresses the given string using the zlib algorithm
	 **/
	inline std::vector<char> decompress(const std::vector<char> &in, std::size_t resize=1024)
	{
		std::vector<char> ret;
		decompress(&in[0], in.size(), ret, resize);
		return std::move(ret);
	}

}

#endif //COMPRESS_HPP

#ifndef CHUNKEDUPLOAD_HPP
#define CHUNKEDUPLOAD_HPP

#include <json/json.h>
#include <ostream>
#include <string>

#include <jsonutils.hpp>

namespace td
{

struct ChunkedUpload
{

	ChunkedUpload(const Json::Value &root) :
		expires(),
		upload_id(),
		offset()
	{
		extractJson(expires, root, "expires");
		extractJson(upload_id, root, "upload_id");
		extractJson(offset, root, "offset");
	}

	std::string expires;
	std::string upload_id;
	uint64_t offset;

}; //end class ChunkedUpload

} //end namespace td

#endif //CHUNKEDUPLOAD_HPP

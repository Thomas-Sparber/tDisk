#ifndef LINKMETADATA_HPP
#define LINKMETADATA_HPP

#include <json/json.h>
#include <ostream>
#include <string>
#include <vector>

#include <filemetadata.hpp>
#include <jsonutils.hpp>

struct LinkMetadata
{

	LinkMetadata(const Json::Value &r) :
		size(),
		rev(),
		hash(),
		bytes(),
		thumb_exists(),
		modified(),
		path(),
		link(),
		is_dir(),
		in_dropbox(),
		icon(),
		root(),
		client_mtime(),
		mime_type(),
		visibility(),
		photo_info({
			{ 0, 0 },
			""
		}),
		video_info ({
			{ 0, 0 },
			"",
			""
		}),
		contents(),
		revision()
	{
		extractJson(size, r, "size");
		extractJson(rev, r, "rev");
		extractJsonOptional(hash, r, "hash");
		extractJson(bytes, r, "bytes");
		extractJson(thumb_exists, r, "thumb_exists");
		extractJson(modified, r, "modified");
		extractJson(path, r, "path");
		extractJson(link, r, "link");
		extractJson(is_dir, r, "is_dir");
		extractJson(in_dropbox, r, "in_dropbox");
		extractJson(icon, r, "icon");
		extractJson(root, r, "root");
		extractJson(client_mtime, r, "client_mtime");
		extractJson(mime_type, r, "mime_type");
		extractJson(visibility, r, "visibility");

		extractJsonOptional(photo_info.lat_long, r, "photo_info", "lat_long");
		extractJsonOptional(photo_info.time_taken, r, "photo_info", "time_taken");

		extractJsonOptional(video_info.lat_long, r, "video_info", "lat_long");
		extractJsonOptional(video_info.time_taken, r, "video_info", "time_taken");
		extractJsonOptional(video_info.duration, r, "video_info", "duration");

		if(r["contents"].type() == Json::arrayValue)
			for(const Json::Value &content : r["contents"])
				contents.emplace_back(content);

		extractJsonOptional(revision, r, "revision");
	}

	//A human-readable description of the file size
	std::string size;

	//A unique identifier for the current revision of a file. This field is the same rev as elsewhere in the API and can be used to detect changes and avoid conflicts.
	std::string rev;

	//A folder's hash is useful for indicating changes to the folder's contents in later calls to /metadata. This is roughly the folder equivalent to a file's rev.
	std::string hash;

	//The file size in bytes.
	unsigned long long bytes;

	//True if the file is an image that can be converted to a thumbnail via the /thumbnails call.
	bool thumb_exists;

	//The last time the file was modified on Dropbox, in the standard date format (not included for the root folder). 
	std::string modified;

	//Returns the path to the file or folder relative to the shared link's root. If the in_dropbox value is true and the shared link points to a file or folder within the authenticated user's Dropbox, it returns the same path that would be returned by the /metadata endpoint. If the request is made without an authenticated user or the shared link is not in the authenticated user's Dropbox, this will always return null.
	std::string path;

	//Shared link of the requested file or subfolder.
	std::string link;

	//Whether the given entry is a folder or not.
	bool is_dir;

	//Returns true if the file or folder is in the authenticated user's Dropbox. If no access token is passed, then this will always return false.
	bool in_dropbox;

	//The name of the icon used to illustrate the file type in Dropbox's icon library.
	std::string icon;

	//The root or top-level folder depending on your access level. All paths returned are relative to this root level. Permitted values are either dropbox or app_folder.
	std::string root;

	//For files, this is the modification time set by the desktop client when the file was added to Dropbox, in the standard date format. Since this time is not verified (the Dropbox server stores whatever the desktop client sends up), this should only be used for display purposes (such as sorting) and not, for example, to determine if a file has changed or not.
	std::string client_mtime;
	std::string mime_type;

	//Visibility of the shared link. Possible values are PUBLIC, TEAM_ONLY, PASSWORD, TEAM_AND_PASSWORD, or SHARED_FOLDER_ONLY, though additional values may be added in the future.
	std::string visibility;

	//Only returned when the include_media_info parameter is true and the file is an image.
	struct
	{
		//GPS coordinates (lat_long).
		std::array<double,2> lat_long;

		//Creation time (time_taken)
		std::string time_taken;

    } photo_info;

	//Only returned when the include_media_info parameter is true and the file is a video.
	struct
	{
		//Creation time (time_taken)
		std::array<double,2> lat_long;

		//GPS coordinates (lat_long)
		std::string time_taken;

		//The length of the video in milliseconds (duration).
		std::string duration;

    } video_info;

	std::vector<FileMetadata> contents;

	//A deprecated field that semi-uniquely identifies a file. Use rev instead.
	unsigned int revision;

}; //end struct LinkMetadata

inline std::ostream& operator<< (std::ostream &out, const LinkMetadata &file)
{
	out<<"{"<<std::endl;
	out<<"\t\"size\": \""<<file.size<<"\","<<std::endl;
	out<<"\t\"rev\": \""<<file.rev<<"\","<<std::endl;
	out<<"\t\"hash\": \""<<file.hash<<"\","<<std::endl;
	out<<"\t\"bytes\": "<<file.bytes<<","<<std::endl;
	out<<"\t\"thumb_exists\": "<<file.thumb_exists<<","<<std::endl;
	out<<"\t\"modified\": \""<<file.modified<<"\","<<std::endl;
	out<<"\t\"path\": \""<<file.path<<"\","<<std::endl;
	out<<"\t\"link\": \""<<file.link<<"\","<<std::endl;
	out<<"\t\"is_dir\": "<<file.is_dir<<","<<std::endl;
	out<<"\t\"in_dropbox\": "<<file.in_dropbox<<","<<std::endl;
	out<<"\t\"icon\": \""<<file.icon<<"\","<<std::endl;
	out<<"\t\"root\": \""<<file.root<<"\","<<std::endl;
	out<<"\t\"client_mtime\": \""<<file.client_mtime<<"\","<<std::endl;
	out<<"\t\"mime_type\": \""<<file.mime_type<<"\","<<std::endl;
	out<<"\t\"visibility\": \""<<file.visibility<<"\","<<std::endl;

	if(!file.photo_info.time_taken.empty())
	{
		out<<"\t\"photo_info\": {"<<std::endl;
		out<<"\t\t\"lat_long\": ["<<std::endl;
		out<<"\t\t\t"<<file.photo_info.lat_long[0]<<","<<std::endl;
		out<<"\t\t\t"<<file.photo_info.lat_long[1]<<""<<std::endl;
		out<<"\t\t],"<<std::endl;
		out<<"\t\t\"time_taken\": \""<<file.photo_info.time_taken<<"\""<<std::endl;
		out<<"\t},"<<std::endl;
	}

	if(!file.video_info.time_taken.empty())
	{
		out<<"\t\"video_info\": {"<<std::endl;
		out<<"\t\t\"lat_long\": ["<<std::endl;
		out<<"\t\t\t"<<file.video_info.lat_long[0]<<","<<std::endl;
		out<<"\t\t\t"<<file.video_info.lat_long[1]<<""<<std::endl;
		out<<"\t\t],"<<std::endl;
		out<<"\t\t\"time_taken\": \""<<file.video_info.time_taken<<"\","<<std::endl;
		out<<"\t\t\"duration\": \""<<file.video_info.duration<<"\""<<std::endl;
		out<<"\t},"<<std::endl;
	}

	if(!file.contents.empty())
	{
		out<<"\t\"contents\": ["<<std::endl;
		for(std::size_t i = 0; i < file.contents.size(); ++i)
		{
			out<<file.contents[i];
			if(i != file.contents.size()-1)out<<",";
			out<<std::endl;
		}
		out<<"\t],"<<std::endl;
	}

	out<<"\t\"revision\": "<<file.revision<<""<<std::endl;
	out<<"}";

	return out;
}

#endif //LINKMETADATA_HPP

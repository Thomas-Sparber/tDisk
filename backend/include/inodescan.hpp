/**
  *
  * tDisk backend
  * @author Thomas Sparber (2015-2016)
  *
 **/

#ifndef INODESCAN_HPP
#define INODESCAN_HPP

#include <functional>
#include <string>
#include <vector>

#include <inode.hpp>

namespace td
{

class InodeScan
{

public:
	InodeScan(const std::string &str_path) :
		path(str_path)
	{}

	virtual ~InodeScan() {}

	virtual bool valid() const = 0;

	virtual Inode* getInitialInode() const = 0;

	virtual bool nextInode(Inode *iterator) const = 0;

	virtual unsigned int getBlocksize() const = 0;

	const std::string& getPath() const
	{
		return path;
	}

	template <class T>
	static bool registerInodeScan()
	{
		try {
			registeredInodeScans().push_back([] (const std::string &path)
			{
				return new T(path);
			});
		} catch (...) {
			return false;
		}

		return true;
	}

	static InodeScan* getInodeScan(const std::string &path)
	{
		for(auto createFunction : registeredInodeScans())
		{
			InodeScan *scan = createFunction(path);
			if(scan->valid())return scan;
			delete scan;
		}

		return nullptr;
	}

private:
	static std::vector<std::function<InodeScan*(const std::string&)> >& registeredInodeScans()
	{
		static std::vector<std::function<InodeScan*(const std::string&)> > reg;
		return reg;
	}

	std::string path;

}; //end class InodeScan

} //end namespace td

#endif //INODESCAN_HPP

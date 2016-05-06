/**
  *
  * tDisk backend
  * @author Thomas Sparber (2015-2016)
  *
 **/

#include <extinodescan.hpp>

using std::string;

using namespace td;

ExtInodeScan::ExtInodeScan(const string &str_path) :
	InodeScan(str_path),
	fs(nullptr),
	scan(nullptr)
{
	//Open fs
	io_manager io_ptr = unix_io_manager;
	int retval = ext2fs_open(getPath().c_str(), 0, 0, 0, io_ptr, &fs);
	if(retval)
	{
		fs = nullptr;
		return;
	}

	//Open ext2 index scan
	retval = ext2fs_open_inode_scan(fs, 0, &scan);
	if(retval)
	{
		scan = nullptr;
		ext2fs_close_free(&fs);
		fs = nullptr;
		return;
	}

	retval = ext2fs_read_inode_bitmap(fs);
	if(retval)
	{
		ext2fs_close_inode_scan(scan);
		scan = nullptr;
		ext2fs_close_free(&fs);
		fs = nullptr;
		return;
	}

	retval = ext2fs_read_block_bitmap(fs);
	if(retval)
	{
		ext2fs_close_inode_scan(scan);
		scan = nullptr;
		ext2fs_close_free(&fs);
		fs = nullptr;
		return;
	}
}

ExtInodeScan& ExtInodeScan::operator=(ExtInodeScan &&other)
{
	if(scan)ext2fs_close_inode_scan(scan);
	if(fs)ext2fs_close_free(&fs);

	fs = other.fs;
	scan = other.scan;

	other.fs = nullptr;
	other.scan = nullptr;

	return (*this);
}

ExtInodeScan::~ExtInodeScan()
{
	if(scan)ext2fs_close_inode_scan(scan);
	if(fs)ext2fs_close_free(&fs);
}

bool ExtInodeScan::valid() const
{
	return ((fs != nullptr) && (scan != nullptr));
}

bool ExtInodeScan::nextInode(Inode *iterator) const
{
	int retval;
	ExtInode *inode = reinterpret_cast<ExtInode*>(iterator);

	do
	{
		retval = ext2fs_get_next_inode(scan, &inode->ino, &inode->inode);
	}
	while(retval == EXT2_ET_BAD_BLOCK_IN_INODE_TABLE);

	return ((!retval) && (inode->ino));
}

unsigned int ExtInodeScan::getBlocksize() const
{
	return fs->blocksize;
}

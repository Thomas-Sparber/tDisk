/**
  *
  * tDisk backend
  * @author Thomas Sparber (2015-2016)
  *
 **/

#ifndef EXTINODE_HPP
#define EXTINODE_HPP

#include <ext2fs/ext2_fs.h>
#include <ext2fs/ext2fs.h>

#include <inode.hpp>

namespace td
{
	
static inline int ext_helper_save_blocks(ext2_filsys /*fs*/, blk64_t *block_nr, e2_blkcnt_t /*blockcnt*/, blk64_t /*ref_block*/, int /*ref_offset*/, void *data)
{
	std::vector<unsigned long long> &out = *((std::vector<unsigned long long>*)data);
	out.push_back(*block_nr);
	return 0;
}

struct ext_helper_lookup_struct
{
	ext_helper_lookup_struct(ext2_ino_t e_inode) :
		inode(e_inode),
		found(false)
	{}
	ext2_ino_t inode;
	bool found;
}; // end struct ext_helper_lookup_struct

static inline int ext_helper_lookup(struct ext2_dir_entry *dirent, int /*offset*/, int /*blocksize*/, char */*buf*/, void *data)
{
	ext_helper_lookup_struct *helper = (ext_helper_lookup_struct*)data;

	if(helper->inode == dirent->inode)
	{
		helper->found = true;
		return DIRENT_ABORT;
	}

	return 0;
}

class ExtInode : public Inode
{

public:
	ExtInode(ext2_filsys e_fs) :
		Inode(),
		fs(e_fs),
		ino(0),
		inode()
	{}

	ExtInode(const ExtInode &other) = default;

	ExtInode(ExtInode &&other) = default;

	virtual ~ExtInode() {}

	ExtInode& operator=(const ExtInode &other) = default;

	ExtInode& operator=(ExtInode &&other) = default;

	virtual Inode* clone()
	{
		return new ExtInode(*this);
	}

	virtual unsigned long long getInodeBlock() const
	{
		//if (!inode.i_links_count)
		//	goto next;

		blk64_t blk = ext2fs_file_acl_block(fs, &inode);
		//if(blk)ext2fs_file_acl_block_set(current_fs, &inode, blk);
		return blk;
	}

	virtual void getDataBlocks(std::vector<unsigned long long> &out) const
	{
		if(!ext2fs_inode_has_valid_blocks2(fs, const_cast<ext2_inode*>(&inode)))return;

		//if(inode.i_dtime)return;

		std::vector<char> block_buf(fs->blocksize * 3);
		ext2fs_block_iterate3(fs, ino, BLOCK_FLAG_READ_ONLY, &block_buf[0], ext_helper_save_blocks, &out);
	}

	virtual std::string getPath(const Inode *p) const
	{
		char *name;
		const ExtInode *parent = reinterpret_cast<const ExtInode*>(p);

		if(parent)
			ext2fs_get_pathname(fs, parent->ino, ino, &name);
		else
			ext2fs_get_pathname(fs, ino, 0, &name);

		return name;
	}

	virtual bool isDirectory() const
	{
		return LINUX_S_ISDIR(inode.i_mode);
	}

	virtual bool contains(const Inode *i) const
	{
		const ExtInode *converted = reinterpret_cast<const ExtInode*>(i);
		if(!isDirectory())return false;

		ext_helper_lookup_struct helper(converted->ino);
		ext2fs_dir_iterate(fs, ino, 0, nullptr, ext_helper_lookup, &helper);

		return helper.found;
	}

public:
	ext2_filsys fs;
	ext2_ino_t ino;
	struct ext2_inode inode;

}; //end class ExtInode

} //end namespace td

#endif //EXTINODE_HPP

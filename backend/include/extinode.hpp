/**
  *
  * tDisk backend
  * @author Thomas Sparber (2015-2016)
  *
 **/

#ifdef __linux__

#ifndef EXTINODE_HPP
#define EXTINODE_HPP

#include <algorithm>
#include <ext2fs/ext2_fs.h>
#include <ext2fs/ext2fs.h>

#include <inode.hpp>

namespace td
{

/**
  * This helper function is used to save the
  * data blocks of an inode to a vector
 **/
static inline int ext_helper_save_blocks(ext2_filsys /*fs*/, blk64_t *block_nr, e2_blkcnt_t /*blockcnt*/, blk64_t /*ref_block*/, int /*ref_offset*/, void *data)
{
	std::vector<unsigned long long> &out = *((std::vector<unsigned long long>*)data);
	out.push_back(*block_nr);
	return 0;
}

/**
  * This helper struct is used to find an inode
  * within another inode (directory)
 **/
struct ext_helper_lookup_struct
{
	/**
	  * The constructor takes the desired inode
	  * number to search for
	 **/
	ext_helper_lookup_struct(ext2_ino_t e_inode) :
		inode(e_inode),
		found(false)
	{}

	/** The inode number to search for **/
	ext2_ino_t inode;

	/** A flag whether the inode has been found **/
	bool found;

}; // end struct ext_helper_lookup_struct

/**
  * This helper function is used to search for an inode
  * within another inode (directory)
 **/
static inline int ext_helper_lookup(struct ext2_dir_entry *dirent, int /*offset*/, int /*blocksize*/, char */*buf*/, void *data)
{
	ext_helper_lookup_struct *helper = (ext_helper_lookup_struct*)data;

	if(helper->inode == dirent->inode)
	{
		//abort if inode was found
		helper->found = true;
		return DIRENT_ABORT;
	}

	return 0;
}

/**
  * This helper function is used the retrieve the full content
  * of a (directory) inode
 **/
static inline int ext_helper_get_content(struct ext2_dir_entry *dirent, int /*offset*/, int /*blocksize*/, char */*buf*/, void *data)
{
	std::vector<ext2_ino_t> &out = *(std::vector<ext2_ino_t>*)data;
	out.push_back(dirent->inode);
	return 0;
}

/**
  * This class represents an inode of the ext filesystem family
 **/
class ExtInode : public Inode
{

public:

	/**
	  * The constructor takes the filesystem where
	  * the inode is located as argument
	 **/
	ExtInode(ext2_filsys e_fs) :
		Inode(),
		fs(e_fs),
		ino(0),
		inode(),
		contentRetrieved(false),
		content()
	{}

	/**
	  * Default copy constructor
	 **/
	ExtInode(const ExtInode &other) = default;

	/**
	  * Default move constructor
	 **/
	ExtInode(ExtInode &&other) = default;

	/**
	  * Default virtual constructor
	 **/
	virtual ~ExtInode() {}

	/**
	  * Default assignment operator
	 **/
	ExtInode& operator=(const ExtInode &other) = default;

	/**
	  * Default move operator
	 **/
	ExtInode& operator=(ExtInode &&other) = default;

	/**
	  * Overrides the funtion from Inode to create
	  * a copy of it
	 **/
	virtual Inode* clone()
	{
		return new ExtInode(*this);
	}

	/**
	  * Returns the physical block where the inode data is stored
	  * on disk
	 **/
	virtual unsigned long long getInodeBlock() const
	{
		return ext2fs_file_acl_block(fs, &inode);
	}

	/**
	  * Returns the physical blocks where the data of the inode
	  * are stored
	 **/
	virtual void getDataBlocks(std::vector<unsigned long long> &out) const
	{
		if(!ext2fs_inode_has_valid_blocks2(fs, const_cast<ext2_inode*>(&inode)))return;

		std::vector<char> block_buf(fs->blocksize * 3);

		//Call helper function which iterates over all data blocks
		ext2fs_block_iterate3(fs, ino, BLOCK_FLAG_READ_ONLY, &block_buf[0], ext_helper_save_blocks, &out);
	}

	/**
	  * Returns the foll path of the inode.
	  * The parent inode is used to calculate the path
	 **/
	virtual std::string getPath(const Inode *p) const
	{
		char *name;
		const ExtInode *parent = reinterpret_cast<const ExtInode*>(p);

		if(parent)ext2fs_get_pathname(fs, parent->ino, ino, &name);
		else ext2fs_get_pathname(fs, ino, 0, &name);

		return name;
	}

	/**
	  * Returns whether the inode is a directory or not
	 **/
	virtual bool isDirectory() const
	{
		return LINUX_S_ISDIR(inode.i_mode);
	}

	/**
	  * If the inode is a directory, this function
	  * retrieves all files within this directory
	 **/
	void retrieveContent() const
	{
		ext2fs_dir_iterate(fs, ino, 0, nullptr, ext_helper_get_content, &content);
		contentRetrieved = true;
	}

	/**
	  * Checks whether the inode (if directory) contains
	  * the given inode
	 **/
	virtual bool contains(const Inode *i) const
	{
		const ExtInode *converted = reinterpret_cast<const ExtInode*>(i);
		if(!isDirectory())return false;
		if(!contentRetrieved)retrieveContent();

		return (std::find(content.cbegin(), content.cend(), converted->ino) != content.cend());
	}

	/**
	  * Returns the inode content (if inode is directory)
	 **/
	virtual void getContent(std::vector<std::unique_ptr<Inode> > &out) const
	{
		if(!isDirectory())return;
		if(!contentRetrieved)retrieveContent();

		for(ext2_ino_t i: content)
		{
			ExtInode *c = new ExtInode(fs);
			c->ino = i;
			ext2fs_read_inode(fs, i, &c->inode);
			out.emplace_back(c);
		}
	}

public:

	/**
	  * The filesystem where the inode is stored
	 **/
	ext2_filsys fs;

	/**
	  * The actual inode (number)
	 **/
	ext2_ino_t ino;

	/**
	  * The inode data
	 **/
	struct ext2_inode inode;

	/**
	  * Only for internal use. A flag whether the
	  * directory content was already received
	 **/
	mutable bool contentRetrieved;

	/**
	  * Only for internal use. The directory content
	 **/
	mutable std::vector<ext2_ino_t> content;

}; //end class ExtInode

} //end namespace td

#endif //EXTINODE_HPP

#endif //__linux__
